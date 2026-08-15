#include "fall_detector.h"
#include "connectivity/alert_dispatcher.h"
#include "connectivity/ble_service.h"
#include <math.h>

// Every threshold lives in app_config.h. None of them is calibrated; the
// comments there say so and say what to measure.

static unsigned long lastCountdownTick = 0;
static bool freeFallDetected = false;
static unsigned long freeFallTimestamp = 0;

// Slow-moving estimate of which way gravity points, used to tell a genuine
// posture change from a bump that leaves the wearer upright.
static float gravX = 0.0f, gravY = 0.0f, gravZ = 1.0f;
static float preFallX = 0.0f, preFallY = 0.0f, preFallZ = 1.0f;

// Delay line holding where gravity pointed roughly FALL_PREFALL_DELAY_MS ago.
//
// Path B has no free-fall phase, so there is no earlier moment at which to
// snapshot the pre-fall posture -- by the time the impact is seen, the fall has
// already happened. It used to snapshot the live gravity estimate on the impact
// sample, which is a posture already partway down.
//
// The size of that error is smaller than it first appears. This function runs
// on the 20 ms sensor tick, not at the 235 Hz the IMU is sampled at, so the
// 0.98/0.02 low-pass above has a time constant of 1/(50*0.02) = 1.0 s. A
// 400-800 ms fall moves it by about a third to a half -- enough to bias the
// phase-4 angle toward zero, not enough to erase it.
//
// The ring answers the question the snapshot was only approximating: where was
// up, before this started.
static float preRingX[FALL_PREFALL_RING_LEN];
static float preRingY[FALL_PREFALL_RING_LEN];
static float preRingZ[FALL_PREFALL_RING_LEN];
static unsigned long preRingMs[FALL_PREFALL_RING_LEN];
static uint16_t preRingPos = 0;
static bool preRingFilled = false;

// Walks back from the newest entry until one is at least
// FALL_PREFALL_DELAY_MS old, and returns it.
//
// Indexing by timestamp rather than by a fixed number of entries matters here
// because the tick is not a fixed length: rendering shares this thread, and a
// measured capture showed the sensor tick settling at ~48 ms with a frame in
// flight against its nominal 20 ms. Counting entries would make the lookback
// however long the last N ticks happened to take -- 1.3 s or 3.1 s for the same
// 64 entries -- which is not a delay line so much as a guess. Searching by age
// gives FALL_PREFALL_DELAY_MS whatever the tick rate does, and makes that
// constant load-bearing instead of decorative.
//
// Returns false when no entry is old enough yet, which is the first
// FALL_PREFALL_DELAY_MS after boot or after an IMU outage.
static bool readPreFallPosture(unsigned long now, float* px, float* py, float* pz) {
    uint16_t n = preRingFilled ? FALL_PREFALL_RING_LEN : preRingPos;
    for (uint16_t back = 1; back <= n; back++) {
        uint16_t idx = (preRingPos + FALL_PREFALL_RING_LEN - back) % FALL_PREFALL_RING_LEN;
        if (now - preRingMs[idx] >= FALL_PREFALL_DELAY_MS) {
            *px = preRingX[idx];
            *py = preRingY[idx];
            *pz = preRingZ[idx];
            return true;
        }
    }
    return false;
}

// Confirmation-window accumulators. stillSamples counts every sample judged
// after the settle delay; calmSamples counts the subset that stayed near 1g.
// The ratio between them is the stillness verdict; confirmMaxDev survives only
// so the old worst-sample rule can still be printed for comparison.
static unsigned long confirmStart = 0;
static float confirmMaxDev = 0.0f;
static uint16_t stillSamples = 0;
static uint16_t calmSamples = 0;

// Which of the two entry paths brought us into FALL_STATE_SUSPECTED, and how
// hard the impact was. Reported on the decision line so a dismissed event can
// be traced back to the threshold that dismissed it.
static const char* entryPathName = "";
static float peakImpactG = 0.0f;

// Rate limiter for the near-miss line, so a shaken wrist cannot flood the log
// that the same session is using to collect training data.
static unsigned long lastNearMissLog = 0;

void initFallDetector() {
    g_watchState.fallMonitoringActive = true;
    g_watchState.fallDetected = false;
    g_watchState.fallState = FALL_STATE_NORMAL;
}

static void enterAlert(const char* reason, uint8_t eventType) {
    g_watchState.fallDetected = true;
    g_watchState.fallState = FALL_STATE_ALERT;
    g_watchState.countdownSec = 15;
    g_watchState.fallTimestamp = millis();
    g_watchState.currentScreen = SCREEN_FALL_ALERT;
    // Start the countdown from a clean tick, otherwise the first second can be
    // consumed by however long ago the previous tick happened to fire.
    lastCountdownTick = millis();
    // Tell the phone straight away, during the countdown, so the app knows
    // even if Wi-Fi is down and the Telegram path never gets a chance.
    bleNotifyFallEvent(eventType);
    Serial.printf(" -> WARNING: %s. 15s countdown started.\n", reason);
}

void triggerSimulatedFall() {
    // Manual SOS from the quick menu: the wearer asked for help directly, so
    // skip the confirmation phases entirely.
    if (g_watchState.fallState == FALL_STATE_NORMAL) {
        enterAlert("MANUAL SOS TRIGGERED", 2);
    }
}

void cancelFallAlert() {
    g_watchState.fallDetected = false;
    g_watchState.fallState = FALL_STATE_NORMAL;
    g_watchState.countdownSec = 15;
    g_watchState.currentScreen = SCREEN_HOME;
    freeFallDetected = false;
    confirmStart = 0;
    stillSamples = 0;
    calmSamples = 0;
    // Clear the dispatcher too, or a half-finished send leaves state behind
    // that would corrupt the next alert.
    resetAlertDispatcher();
    Serial.println(" -> USER CANCELLED FALL ALERT. Safe.");
}

void updateFallDetector() {
    if (!g_watchState.fallMonitoringActive) return;

    // Without a working IMU there is nothing to detect. Bail out rather than
    // evaluating stale accelerometer values; the UI reports the outage.
    if (!g_watchState.imuOk) {
        freeFallDetected = false;
        confirmStart = 0;
        stillSamples = 0;
        calmSamples = 0;
        // Entries from before the outage describe a posture separated from the
        // present by however long the IMU was down. The age search would still
        // accept them -- they are old enough, which is exactly the problem, and
        // the older they are the more readily they pass. Discarding sends the
        // next impact down the live-estimate fallback until FALL_PREFALL_DELAY_MS
        // of post-recovery samples exist again.
        preRingFilled = false;
        preRingPos = 0;
        return;
    }

    float ax = g_watchState.accX / FALL_ACCEL_LSB_PER_G;
    float ay = g_watchState.accY / FALL_ACCEL_LSB_PER_G;
    float az = g_watchState.accZ / FALL_ACCEL_LSB_PER_G;
    float totalG = sqrtf(ax * ax + ay * ay + az * az);

    // Low-pass the acceleration to track the gravity direction.
    gravX = gravX * 0.98f + ax * 0.02f;
    gravY = gravY * 0.98f + ay * 0.02f;
    gravZ = gravZ * 0.98f + az * 0.02f;

    unsigned long now = millis();

    // Record the estimate with its timestamp, before anything reads it.
    preRingX[preRingPos] = gravX;
    preRingY[preRingPos] = gravY;
    preRingZ[preRingPos] = gravZ;
    preRingMs[preRingPos] = now;
    preRingPos = (preRingPos + 1) % FALL_PREFALL_RING_LEN;
    if (preRingPos == 0) preRingFilled = true;

#if FALL_LOG_RAW_SAMPLES
    // Every sample, every tick, whatever the state -- this is the training set.
    //
    // Logging only during confirmation would capture falls and nothing else,
    // and a classifier trained on that learns to answer "fall" to everything it
    // is ever shown. What teaches it to say no is the ordinary hours: walking,
    // eating, clapping, putting a mug down hard. Those are the negatives, and
    // they only exist in the log if the log never stops.
    //
    // The state column is what makes the file labellable afterwards: it says
    // which samples the 4-phase algorithm was reacting to, so a run can be cut
    // into events without guessing from the timestamps.
    Serial.printf("FALLCSV,%lu,%d,%d,%d,%d,%d,%d,%.3f,%d\n",
                  now,
                  g_watchState.accX,  g_watchState.accY,  g_watchState.accZ,
                  g_watchState.gyroX, g_watchState.gyroY, g_watchState.gyroZ,
                  totalG,
                  (int)g_watchState.fallState);
#endif

    if (g_watchState.fallState == FALL_STATE_NORMAL) {
        // Two ways into the confirmation phase. Path A is the classic
        // free-fall-then-impact signature. Path B was written for the falls
        // where the wrist never becomes unsupported -- a slump, a slide down a
        // wall, fainting from a chair -- and compensates with a higher impact
        // bar.
        //
        // Path B does not actually reach those yet. All 13 recorded falls dip
        // into free fall (min_g <= 0.82g, most below 0.4g), so they enter by
        // path A and path B has never been the deciding path for a measured
        // fall. A genuinely slow collapse produces no impact large enough for
        // its 3.5g bar either. See FALL_IMPACT_STANDALONE_G for why the gap is
        // left open rather than closed with invented thresholds.
        const char* entryPath = nullptr;

        // Phase 1: free fall.
        //
        // This snapshot stays on the live estimate rather than the delay line.
        // Path A has a real earlier event to snapshot on -- the onset of free
        // fall, before the impact -- so the estimate here has not yet followed
        // the body down the way path B's had. It is not perfect: balance is
        // lost before the wrist goes weightless. But the ring would change the
        // behaviour of the path that the 13 recorded falls were measured
        // through, and the fix it would buy is much smaller than path B's.
        // Left alone deliberately, to be judged against the same capture.
        if (totalG < FALL_FREEFALL_G && !freeFallDetected) {
            freeFallDetected = true;
            freeFallTimestamp = now;
            preFallX = gravX; preFallY = gravY; preFallZ = gravZ;
        }

        if (freeFallDetected) {
            if (now - freeFallTimestamp > FALL_IMPACT_WINDOW_MS) {
                freeFallDetected = false;              // no impact followed
            } else if (totalG > FALL_IMPACT_G) {
                // Phase 2, path A: impact. Not an alarm yet -- start confirming.
                freeFallDetected = false;
                entryPath = "A free-fall+impact";
            }
        }

        // Phase 2, path B: a hard impact on its own.
        if (entryPath == nullptr && totalG > FALL_IMPACT_STANDALONE_G) {
            // No free fall means no earlier event to snapshot on, so read the
            // posture out of the delay line: an estimate from
            // FALL_PREFALL_DELAY_MS ago, before the fall began.
            //
            // When nothing is old enough yet -- the first 600 ms after boot or
            // after an IMU outage -- fall back to the live estimate, which is
            // what the old code always used. Left as a documented fallback
            // rather than suppressed: refusing to enter confirmation for the
            // first moments after boot would trade an approximate tilt angle
            // for a missed fall, and the tilt check is disabled anyway.
            if (!readPreFallPosture(now, &preFallX, &preFallY, &preFallZ)) {
                preFallX = gravX; preFallY = gravY; preFallZ = gravZ;
            }
            entryPath = "B standalone impact";
        }

        if (entryPath != nullptr) {
            g_watchState.fallState = FALL_STATE_SUSPECTED;
            confirmStart = now;
            confirmMaxDev = 0.0f;
            stillSamples = 0;
            calmSamples = 0;
            peakImpactG = totalG;
            entryPathName = entryPath;
            Serial.printf(" -> [FALL] Impact %.2fg via %s. Confirming for %lu ms...\n",
                          totalG, entryPath, (unsigned long)FALL_CONFIRM_WINDOW_MS);
            return;
        }

        // Anything strong enough to be worth looking at, but not strong enough
        // to enter confirmation, gets one line saying so.
        //
        // Without this the serial output is identical whether the wearer never
        // fell or the thresholds are set too high to catch anything -- and
        // during calibration those are exactly the two cases that need telling
        // apart. Printing the near-miss and the bar it failed to clear turns a
        // silent session into a measurement.
        if (totalG > FALL_NEARMISS_LOG_G && now - lastNearMissLog >= FALL_NEARMISS_LOG_MS) {
            lastNearMissLog = now;
            Serial.printf(" -> [FALL] near miss: %.2fg (needs %.2fg standalone, "
                          "or %.2fg within %lu ms of free fall)\n",
                          totalG, (float)FALL_IMPACT_STANDALONE_G,
                          (float)FALL_IMPACT_G, (unsigned long)FALL_IMPACT_WINDOW_MS);
        }
        return;
    }

    if (g_watchState.fallState == FALL_STATE_SUSPECTED) {
        unsigned long elapsed = now - confirmStart;

        if (totalG > peakImpactG) peakImpactG = totalG;

        // Phase 3: the wearer should be lying still -- but "still" has to mean
        // mostly still, not perfectly still.
        //
        // Nothing in the first FALL_SETTLE_MS counts either way. The impact,
        // the wrist rebounding off the floor and the arm coming to rest all
        // land inside that window and would otherwise be read as movement.
        //
        // After that, count how many samples are calm rather than tracking the
        // single worst one. The worst-sample test asked the wearer to hold
        // still for 2.6 seconds and threw the alert away on one twitch -- and
        // someone who has just fallen and is still conscious does not hold
        // still. They push up on an elbow, roll, reach for something to grab.
        // The wrist is where all of that shows up. Judging on the peak meant
        // the more the wearer tried to help themselves, the more certainly the
        // watch said nothing, which is the wrong way round for the person this
        // is built for.
        //
        // confirmMaxDev is still tracked, but only to be printed: it is what
        // the old rule would have decided on, so a calibration session can be
        // read both ways from one log.
        if (elapsed >= FALL_SETTLE_MS) {
            float dev = fabsf(totalG - 1.0f);
            if (dev > confirmMaxDev) confirmMaxDev = dev;
            stillSamples++;
            if (dev < FALL_STILLNESS_MAX_DEV_G) calmSamples++;
        }

        if (elapsed < FALL_CONFIRM_WINDOW_MS) return;

        // A window with no samples in it cannot testify either way; treat it as
        // not still rather than dividing by zero.
        float calmRatio = (stillSamples > 0)
                              ? (float)calmSamples / (float)stillSamples
                              : 0.0f;
        bool still = calmRatio >= FALL_STILLNESS_MIN_CALM_RATIO;

        // Phase 4: posture must actually have changed. Angle between the
        // gravity vector before the event and the one after it.
        float preMag = sqrtf(preFallX * preFallX + preFallY * preFallY + preFallZ * preFallZ);
        float nowMag = sqrtf(gravX * gravX + gravY * gravY + gravZ * gravZ);
        float angleDeg = 0.0f;
        if (preMag > 0.1f && nowMag > 0.1f) {
            float dot = (preFallX * gravX + preFallY * gravY + preFallZ * gravZ) / (preMag * nowMag);
            angleDeg = acosf(constrain(dot, -1.0f, 1.0f)) * 57.2957795f;
        }
        // Setting the threshold to 0 disables the test rather than lowering it
        // to "any tilt at all". A plain `>` comparison against 0 would still
        // reject an event whose gravity vector happened not to move, which is
        // not what a disabled check should do -- and the angle is still
        // computed and logged either way, so the evidence keeps accumulating
        // for whenever there is enough of it to turn the test back on.
        bool reoriented = (FALL_ORIENTATION_MIN_DEG <= 0.0f)
                              ? true
                              : (angleDeg > FALL_ORIENTATION_MIN_DEG);

        // One line carrying every number the decision rested on, next to the
        // threshold it was compared against. This is what you calibrate the
        // constants in app_config.h from: drop the watch, read the line, see
        // which test failed and by how much.
        //
        // peakDev is printed alongside, marked with what the old worst-sample
        // rule would have concluded. It decides nothing now, but it lets one
        // calibration session be scored under both rules, which is the only
        // honest way to claim the change was an improvement.
        bool oldRuleStill = confirmMaxDev < FALL_STILLNESS_MAX_DEV_G;
        Serial.printf(" -> [FALL] path=%s peak=%.2fg calm=%u/%u=%.0f%%/%.0f%%%s "
                      "peakDev=%.2f(old:%s) tilt=%.0f/%.0f deg%s => %s\n",
                      entryPathName,
                      peakImpactG,
                      calmSamples, stillSamples,
                      calmRatio * 100.0f, FALL_STILLNESS_MIN_CALM_RATIO * 100.0f,
                      still ? " OK" : " FAIL",
                      confirmMaxDev, oldRuleStill ? "OK" : "FAIL",
                      angleDeg, FALL_ORIENTATION_MIN_DEG, reoriented ? " OK" : " FAIL",
                      (still && reoriented) ? "FALL CONFIRMED" : "dismissed");

        if (still && reoriented) {
            enterAlert("FALL CONFIRMED", 1);
        } else {
            g_watchState.fallState = FALL_STATE_NORMAL;
        }
        confirmStart = 0;
        return;
    }

    if (g_watchState.fallState == FALL_STATE_ALERT) {
        if (now - lastCountdownTick >= 1000) {
            lastCountdownTick = now;
            if (g_watchState.countdownSec > 0) {
                g_watchState.countdownSec--;
            } else {
                g_watchState.fallState = FALL_STATE_SENDING;
                Serial.println(" -> ALERT TIMEOUT! Sending emergency signal...");
            }
        }
    }
}
