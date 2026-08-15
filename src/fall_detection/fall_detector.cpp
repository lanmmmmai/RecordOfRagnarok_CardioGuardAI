#include "fall_detector.h"
#include "connectivity/alert_dispatcher.h"
#include "connectivity/ble_service.h"
#include <math.h>

// Accelerometer is configured for +/-8g in qmi8658_service.cpp, so full scale
// 32768 maps to 8g.
static const float ACCEL_LSB_PER_G = 4096.0f;

// Phase thresholds. The impact and free-fall numbers are the usual starting
// point for wrist-worn detectors; the confirmation numbers below are what keep
// everyday gestures from raising an alarm.
static const float FREEFALL_G          = 0.4f;
static const float IMPACT_G            = 2.5f;
static const unsigned long IMPACT_WINDOW_MS  = 1500;
static const unsigned long CONFIRM_WINDOW_MS = 2000;
static const float STILLNESS_MAX_DEV_G = 0.35f;  // max |totalG - 1g| while still
static const float ORIENTATION_MIN_DEG = 30.0f;  // posture change after the fall

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

    float ax = g_watchState.accX / ACCEL_LSB_PER_G;
    float ay = g_watchState.accY / ACCEL_LSB_PER_G;
    float az = g_watchState.accZ / ACCEL_LSB_PER_G;
    float totalG = sqrtf(ax * ax + ay * ay + az * az);

    // Low-pass the acceleration to track the gravity direction.
    gravX = gravX * 0.98f + ax * 0.02f;
    gravY = gravY * 0.98f + ay * 0.02f;
    gravZ = gravZ * 0.98f + az * 0.02f;

    unsigned long now = millis();

    if (g_watchState.fallState == FALL_STATE_NORMAL) {
        // Phase 1: free fall.
        if (totalG < FREEFALL_G && !freeFallDetected) {
            freeFallDetected = true;
            freeFallTimestamp = now;
            preFallX = gravX; preFallY = gravY; preFallZ = gravZ;
        }

        if (freeFallDetected) {
            if (now - freeFallTimestamp > IMPACT_WINDOW_MS) {
                freeFallDetected = false;              // no impact followed
            } else if (totalG > IMPACT_G) {
                // Phase 2: impact. Not an alarm yet — start confirming.
                freeFallDetected = false;
                g_watchState.fallState = FALL_STATE_SUSPECTED;
                confirmStart = now;
                confirmMaxDev = 0.0f;
                Serial.println(" -> Impact detected, confirming...");
            }
        }
        return;
    }

    if (g_watchState.fallState == FALL_STATE_SUSPECTED) {
        // Phase 3: the wearer should be lying still. Track the worst deviation
        // from 1g across the whole window; a clap or a slammed hand keeps
        // moving and blows past the threshold immediately.
        float dev = fabsf(totalG - 1.0f);
        if (dev > confirmMaxDev) confirmMaxDev = dev;

        if (now - confirmStart < CONFIRM_WINDOW_MS) return;

        bool still = confirmMaxDev < STILLNESS_MAX_DEV_G;

        // Phase 4: posture must actually have changed. Angle between the
        // gravity vector before the event and the one after it.
        float preMag = sqrtf(preFallX * preFallX + preFallY * preFallY + preFallZ * preFallZ);
        float nowMag = sqrtf(gravX * gravX + gravY * gravY + gravZ * gravZ);
        float angleDeg = 0.0f;
        if (preMag > 0.1f && nowMag > 0.1f) {
            float dot = (preFallX * gravX + preFallY * gravY + preFallZ * gravZ) / (preMag * nowMag);
            angleDeg = acosf(constrain(dot, -1.0f, 1.0f)) * 57.2957795f;
        }
        bool reoriented = angleDeg > ORIENTATION_MIN_DEG;

        if (still && reoriented) {
            enterAlert("FALL CONFIRMED", 1);
        } else {
            g_watchState.fallState = FALL_STATE_NORMAL;
            Serial.printf(" -> Impact dismissed (still=%d, tilt=%.0f deg). Not a fall.\n",
                          still ? 1 : 0, angleDeg);
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
