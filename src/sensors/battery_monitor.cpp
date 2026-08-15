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

// Exponentially smoothed pack voltage. Zero means "no reading yet".
static float filteredVoltage = 0.0f;

// Charge-trend detector state: the voltage and the millis() at the start of the
// current comparison window, and the verdict it produced.
static unsigned long trendMark = 0;
static float trendVoltage = 0.0f;
static bool  rising = false;

// State of charge from open-circuit voltage.
//
// A LiPo does not discharge in a straight line. It falls quickly from 4.20 V to
// about 3.90 V, then sits on a long flat shelf between 3.90 and 3.70 V that
// covers most of the usable capacity, then collapses below 3.60 V. The straight
// line this replaces therefore lied in both directions at once: it read low
// through the whole plateau, and -- the part that matters on a fall watch -- it
// still showed roughly 25% at 3.50 V, where a real cell has minutes left. The
// wearer would see a quarter tank and get a dead watch.
//
// Piecewise-linear through five points off a typical 3.7 V LiPo discharge curve
// at a light load. Not measured on this cell; good enough to stop the gauge
// from being actively misleading, and the shape is what a low-battery alert
// needs in order to fire at a useful moment.
static uint8_t lipoPercent(float v) {
    struct Point { float volts; float pct; };
    static const Point curve[] = {
        {3.30f,   0.0f},
        {3.60f,   5.0f},
        {3.70f,  20.0f},
        {3.90f,  60.0f},
        {4.05f,  90.0f},
        {4.20f, 100.0f},
    };
    const size_t n = sizeof(curve) / sizeof(curve[0]);

    if (v <= curve[0].volts)     return 0;
    if (v >= curve[n - 1].volts) return 100;

    for (size_t i = 1; i < n; i++) {
        if (v <= curve[i].volts) {
            float span = curve[i].volts - curve[i - 1].volts;
            float t    = (v - curve[i - 1].volts) / span;
            float pct  = curve[i - 1].pct + t * (curve[i].pct - curve[i - 1].pct);
            return (uint8_t)constrain(pct, 0.0f, 100.0f);
        }
    }
    return 100;
}

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

    // Smooth before anything decides on it. A single ADC burst still moves tens
    // of millivolts pass to pass, which is enough to make the percentage jitter
    // and, worse, to fake the rising trend the charge detector below looks for.
    if (filteredVoltage <= 0.0f) filteredVoltage = measuredVoltage;
    filteredVoltage += BATTERY_FILTER_ALPHA * (measuredVoltage - filteredVoltage);
    g_watchState.batteryVoltage = filteredVoltage;

    // No charge-status line is wired to a GPIO, so charging has to be inferred.
    //
    // Voltage alone cannot do it: USB power with no cell fitted parks the rail
    // above 4.15 V and looks exactly like a full battery -- which is how the
    // watch could report 100% forever on the bench and never once fire a
    // low-battery alert. The tell is direction, not level. A cell on charge
    // climbs; a bare rail sits still. So require BOTH a high reading and a
    // measurable rise over BATTERY_TREND_WINDOW_MS.
    unsigned long now = millis();
    if (trendMark == 0) {
        trendMark = now;
        trendVoltage = filteredVoltage;
    } else if (now - trendMark >= BATTERY_TREND_WINDOW_MS) {
        float delta = filteredVoltage - trendVoltage;
        // Charging is only claimed while the rail is high AND still climbing.
        // A full cell on the charger stops climbing, so it drops back to "not
        // charging" and reads 100% from the curve -- which is honest: the watch
        // knows the battery is full, not that current is flowing.
        rising = (delta >= BATTERY_CHARGE_RISE_V);
        trendMark = now;
        trendVoltage = filteredVoltage;
    }

    g_watchState.isCharging = rising && filteredVoltage >= 4.15f;
    g_watchState.batteryPercent = lipoPercent(filteredVoltage);
}
