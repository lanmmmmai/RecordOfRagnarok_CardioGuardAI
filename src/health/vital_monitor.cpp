#include "vital_monitor.h"

#include <Arduino.h>

#include "app_config.h"
#include "app_state.h"
#include "connectivity/alert_dispatcher.h"

// Three independent conditions, each with its own sustain timer and its own
// repeat cooldown. Sharing one timer between them would let a brief tachycardia
// reset the clock on an ongoing desaturation, which is the more serious of the
// two.
enum VitalKind { VITAL_HR_HIGH_KIND, VITAL_HR_LOW_KIND, VITAL_SPO2_LOW_KIND, VITAL_KIND_COUNT };

// millis() when the condition first became true, 0 while it is false.
static unsigned long breachSince[VITAL_KIND_COUNT] = {0};

// millis() of the last message sent for this condition, 0 if never.
static unsigned long lastSent[VITAL_KIND_COUNT] = {0};

void initVitalMonitor() {
    for (uint8_t i = 0; i < VITAL_KIND_COUNT; i++) {
        breachSince[i] = 0;
        lastSent[i] = 0;
    }
}

// One condition's state machine: hold for VITAL_SUSTAIN_MS, then send at most
// once per VITAL_REPEAT_MS.
static void evaluate(VitalKind kind, bool breached, const char* message) {
    if (!breached) {
        breachSince[kind] = 0;
        return;
    }

    unsigned long now = millis();
    if (breachSince[kind] == 0) {
        breachSince[kind] = now;
        return;
    }

    // A single stray reading is not a medical event. The condition has to hold
    // continuously; any moment it clears, the timer above resets to zero.
    if (now - breachSince[kind] < VITAL_SUSTAIN_MS) return;

    // Rate limit. Someone whose SpO2 sits at 88% for an hour needs help, not
    // seven hundred identical messages -- and a flooded chat is a chat people
    // stop reading.
    if (lastSent[kind] != 0 && now - lastSent[kind] < VITAL_REPEAT_MS) return;

    lastSent[kind] = now;
    Serial.printf(" [VITAL] Threshold alert: %s\n", message);
    queueAlertMessage(String(message));
}

void updateVitalMonitor() {
    // Nothing worth saying without a trustworthy number. Every gate matters:
    // hrValid alone would still let through a reading taken off a poorly
    // seated sensor, PPG_MIN_SQI is what stage 5 uses for the display, and the
    // beat count covers the case both of those miss -- a detector whose peak
    // reference has not converged yet reports a confident number from a
    // threshold that is still wrong. That is how the 2026-08-13 00:00 session
    // sent a high-heart-rate alert 11 seconds after boot, with hrValid set and
    // SQI reading 52.
    bool hrTrusted = g_watchState.hrValid &&
                     g_watchState.signalQuality >= PPG_MIN_SQI &&
                     g_watchState.skinContact &&
                     g_watchState.beatsSinceContact >= PPG_WARMUP_BEATS;

    if (!hrTrusted) {
        breachSince[VITAL_HR_HIGH_KIND] = 0;
        breachSince[VITAL_HR_LOW_KIND] = 0;
    } else {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "🔴 CẢNH BÁO NHỊP TIM CAO: %u BPM (ngưỡng %d). "
                 "Vui lòng kiểm tra người đeo.",
                 g_watchState.heartRateBPM, VITAL_HR_HIGH);
        evaluate(VITAL_HR_HIGH_KIND, g_watchState.heartRateBPM > VITAL_HR_HIGH, msg);

        snprintf(msg, sizeof(msg),
                 "🔴 CẢNH BÁO NHỊP TIM THẤP: %u BPM (ngưỡng %d). "
                 "Vui lòng kiểm tra người đeo.",
                 g_watchState.heartRateBPM, VITAL_HR_LOW);
        evaluate(VITAL_HR_LOW_KIND, g_watchState.heartRateBPM < VITAL_HR_LOW, msg);
    }

    bool spo2Trusted = g_watchState.spo2Valid &&
                       g_watchState.signalQuality >= PPG_MIN_SQI &&
                       g_watchState.skinContact &&
                       !g_watchState.motionArtifact;

    if (!spo2Trusted) {
        breachSince[VITAL_SPO2_LOW_KIND] = 0;
    } else {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "🔴 CẢNH BÁO SpO2 THẤP: %u%% (ngưỡng %d). "
                 "Đây là số đo ở cổ tay, chỉ mang tính tham khảo -- "
                 "hãy đo lại bằng máy kẹp ngón tay.",
                 g_watchState.spo2Percent, VITAL_SPO2_LOW);
        evaluate(VITAL_SPO2_LOW_KIND, g_watchState.spo2Percent < VITAL_SPO2_LOW, msg);
    }
}
