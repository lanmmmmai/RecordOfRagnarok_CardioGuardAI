#include "battery_monitor.h"

// GPIO1 is ADC1_CH0, so this reading is unaffected by Wi-Fi (only ADC2 is).
//
// The raw analogRead()/4095*3.3 formula it replaced is wrong on ESP32-S3: the
// ADC saturates well below 3.3 V and its transfer curve is noticeably non
// linear. analogReadMilliVolts() applies the per-chip calibration burned into
// eFuse, which is the only way to get a trustworthy voltage here -- and a
// battery gauge that lies is a real problem on a watch someone depends on.
static const uint8_t ADC_SAMPLES = 8;

// Last raw pin reading, kept for the serial log. See BATTERY_DIVIDER_RATIO in
// app_config.h for why the ratio is what it is and how to check it.
static uint32_t lastPinMillivolts = 0;

uint32_t getBatteryAdcMillivolts() { return lastPinMillivolts; }

void initBatteryMonitor() {
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);  // full 0..~3.1 V span
}

void updateBatteryMonitor() {
    uint32_t mv = 0;
    for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
        mv += analogReadMilliVolts(BATTERY_ADC_PIN);
    }
    mv /= ADC_SAMPLES;
    lastPinMillivolts = mv;

    float measuredVoltage = (mv / 1000.0f) * BATTERY_DIVIDER_RATIO;
    g_watchState.batteryVoltage = measuredVoltage;

    // There is no charge-status line wired to a GPIO, so "charging" here is
    // only an inference from the rail sitting above a full cell. A board
    // running on USB with no cell fitted at all looks the same.
    if (measuredVoltage >= 4.15f) {
        g_watchState.isCharging = true;
        g_watchState.batteryPercent = 100;
    } else {
        g_watchState.isCharging = false;
        // Straight-line map over the usable LiPo range. Crude, but it never
        // claims more charge than the cell holds.
        float pct = ((measuredVoltage - 3.3f) / (4.1f - 3.3f)) * 100.0f;
        g_watchState.batteryPercent = (uint8_t)constrain(pct, 0.0f, 100.0f);
    }
}
