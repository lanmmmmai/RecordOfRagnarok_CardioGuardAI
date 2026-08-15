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

// Confirmation-window accumulators.
static unsigned long confirmStart = 0;
static float confirmMaxDev = 0.0f;

// Which of the two entry paths brought us into FALL_STATE_SUSPECTED, and how
// hard the impact was. Reported on the decision line so a dismissed event can
// be traced back to the threshold that dismissed it.
static const char* entryPathName = "";
static float peakImpactG = 0.0f;

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

    if (g_watchState.fallState == FALL_STATE_NORMAL) {
        // Two ways into the confirmation phase. Path A is the classic
        // free-fall-then-impact signature. Path B exists because insisting on
        // free fall silently misses the falls that matter most -- a slump, a
        // slide down a wall, fainting from a chair -- where the wrist never
        // becomes unsupported. Path B compensates with a higher impact bar.
        const char* entryPath = nullptr;

        // Phase 1: free fall.
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
            // No free fall means no earlier snapshot of which way was up, so
            // take one now. The gravity estimate is low-passed with a time
            // constant near a second, so a single impact sample has barely
            // moved it and it still describes the pre-impact posture.
            preFallX = gravX; preFallY = gravY; preFallZ = gravZ;
            entryPath = "B standalone impact";
        }

        if (entryPath != nullptr) {
            g_watchState.fallState = FALL_STATE_SUSPECTED;
            confirmStart = now;
            confirmMaxDev = 0.0f;
            peakImpactG = totalG;
            entryPathName = entryPath;
            Serial.printf(" -> [FALL] Impact %.2fg via %s. Confirming for %lu ms...\n",
                          totalG, entryPath, (unsigned long)FALL_CONFIRM_WINDOW_MS);
        }
        return;
    }

    if (g_watchState.fallState == FALL_STATE_SUSPECTED) {
        unsigned long elapsed = now - confirmStart;

        if (totalG > peakImpactG) peakImpactG = totalG;

#if FALL_LOG_RAW_SAMPLES
        // One line per sample, for offline calibration and as training data
        // for the fall model. Prefixed so it can be grepped straight into CSV.
        Serial.printf("FALLCSV,%lu,%d,%d,%d,%d,%d,%d,%.3f\n",
                      elapsed,
                      g_watchState.accX,  g_watchState.accY,  g_watchState.accZ,
                      g_watchState.gyroX, g_watchState.gyroY, g_watchState.gyroZ,
                      totalG);
#endif

        // Phase 3: the wearer should be lying still. Track the worst deviation
        // from 1g; a clap or a slammed hand keeps moving and blows past the
        // threshold.
        //
        // Nothing in the first FALL_SETTLE_MS counts. The impact itself, the
        // wrist rebounding off the floor and the arm coming to rest all land
        // inside that window, and at a 20 ms sample period they are several
        // samples wide. Measuring from the impact sample -- as this did before
        // -- meant one rebound sample pinned confirmMaxDev above the limit and
        // the alert was thrown away. It got worse the harder the fall was.
        if (elapsed >= FALL_SETTLE_MS) {
            float dev = fabsf(totalG - 1.0f);
            if (dev > confirmMaxDev) confirmMaxDev = dev;
        }

        if (elapsed < FALL_CONFIRM_WINDOW_MS) return;

        bool still = confirmMaxDev < FALL_STILLNESS_MAX_DEV_G;

        // Phase 4: posture must actually have changed. Angle between the
        // gravity vector before the event and the one after it.
        float preMag = sqrtf(preFallX * preFallX + preFallY * preFallY + preFallZ * preFallZ);
        float nowMag = sqrtf(gravX * gravX + gravY * gravY + gravZ * gravZ);
        float angleDeg = 0.0f;
        if (preMag > 0.1f && nowMag > 0.1f) {
            float dot = (preFallX * gravX + preFallY * gravY + preFallZ * gravZ) / (preMag * nowMag);
            angleDeg = acosf(constrain(dot, -1.0f, 1.0f)) * 57.2957795f;
        }
        bool reoriented = angleDeg > FALL_ORIENTATION_MIN_DEG;

        // One line carrying every number the decision rested on, next to the
        // threshold it was compared against. This is what you calibrate the
        // constants in app_config.h from: drop the watch, read the line, see
        // which test failed and by how much.
        Serial.printf(" -> [FALL] path=%s peak=%.2fg stillDev=%.2f/%.2f%s "
                      "tilt=%.0f/%.0f deg%s => %s\n",
                      entryPathName,
                      peakImpactG,
                      confirmMaxDev, FALL_STILLNESS_MAX_DEV_G, still ? " OK" : " FAIL",
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
