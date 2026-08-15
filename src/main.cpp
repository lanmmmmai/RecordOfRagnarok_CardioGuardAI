#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "app_config.h"
#include "app_state.h"
#include "ui_config.h"

#include "sensors/battery_monitor.h"
#include "sensors/qmi8658_service.h"
#include "sensors/max30102_service.h"
#include "sensors/cst816s_service.h"

#include "fall_detection/fall_detector.h"
#include "input/sos_button.h"
#include "health/vital_monitor.h"
#include "connectivity/wifi_manager.h"
#include "connectivity/alert_dispatcher.h"
#include "connectivity/ble_service.h"
#include "ui/ui_manager.h"

// Sensors are sampled on a fixed 50 Hz tick rather than as fast as the loop
// happens to run. The fall detector measures variance over a window, so a
// steady rate keeps its thresholds meaningful, and the PPG motion estimate
// needs one entry per genuine IMU reading rather than per loop iteration.
static const unsigned long SENSOR_PERIOD_MS = 20;

// Rendering and the sensor tick share this one thread, so a frame that lands
// mid-tick pushes that tick back by however long the LCD write took. At 33 ms
// the two periods collide almost every time and the sensor tick settles at
// ~48 ms -- measured, not estimated: a capture showed median 48 ms with p95 at
// 49 ms, and a spread that tight is two fixed-cost jobs queueing behind each
// other rather than serial backpressure, which would jitter.
//
// That matters because the fall detector reads a 235 Hz sensor through this
// tick. At 48 ms it sees 12.9 ms out of every 48 and is blind for the other
// 35, so a sharp impact peak -- the exact thing it exists to catch -- is more
// likely to land in the gap than not, and every recorded peak reads low by an
// unknown amount.
//
// While raw logging is on, the capture is the product and the UI is not, so
// frames give way to the tick. 10 FPS is still legible to anyone watching the
// watch during a recording.
#if FALL_LOG_RAW_SAMPLES
static const unsigned long RENDER_PERIOD_MS = 100;  // 10 FPS while capturing
#else
static const unsigned long RENDER_PERIOD_MS = 33;   // ~30 FPS
#endif

static unsigned long lastSensorTick = 0;
static unsigned long lastRenderTick = 0;
static unsigned long lastUptimeTick = 0;
static unsigned long lastBatteryTick = 0;
static unsigned long lastLogTick = 0;

#if FALL_LOG_TICK_PROFILE
// See FALL_LOG_TICK_PROFILE in app_config.h for why this exists and when to
// delete it. Each entry keeps the largest single duration that job has cost
// since the last report; the reporter prints them and zeroes them.
struct JobPeak {
    const char* name;
    uint32_t peakUs;
};
static JobPeak jobPeaks[] = {
    {"touch",  0},  // 0
    {"poll",   0},  // 1
    {"imu",    0},  // 2
    {"fall",   0},  // 3
    {"ppg",    0},  // 4
    {"sos",    0},  // 5
    {"alert",  0},  // 6
    {"ble",    0},  // 7
    {"sec",    0},  // 8  once-a-second block
    {"bat",    0},  // 9  once-per-5s block
    {"render", 0},  // 10
    {"log",    0},  // 11
};
static uint32_t loopPeakUs = 0;

// A macro rather than a function taking a callable: this wraps calls inside
// the hot loop, and the timing must not cost more than what it measures.
#define PROFILE_JOB(idx, call)                                      \
    do {                                                            \
        uint32_t _t0 = micros();                                    \
        call;                                                       \
        uint32_t _dt = micros() - _t0;                              \
        if (_dt > jobPeaks[idx].peakUs) jobPeaks[idx].peakUs = _dt; \
    } while (0)

static void reportTickProfile() {
    Serial.print("TICKPROF");
    Serial.printf(",loop=%lu", (unsigned long)loopPeakUs);
    for (auto& j : jobPeaks) {
        Serial.printf(",%s=%lu", j.name, (unsigned long)j.peakUs);
        j.peakUs = 0;
    }
    Serial.println();
    loopPeakUs = 0;
}
#else
#define PROFILE_JOB(idx, call) call
#endif

void printSerialLog() {
    Serial.println("==========================================================================");
    Serial.printf(" [WATCH LOG] %04d-%02d-%02d %02d:%02d:%02d | WiFi: %s (%s) | BLE: %s | BAT: %d%% (%.2fV, raw %lu mV)\n",
                  g_watchState.year, g_watchState.month + 1, g_watchState.day,
                  g_watchState.hour, g_watchState.minute, g_watchState.second,
                  g_watchState.wifiStatusStr.c_str(), g_watchState.ipAddress.c_str(),
                  g_watchState.bleConnected ? "LINKED" : "AWAY",
                  g_watchState.batteryPercent, g_watchState.batteryVoltage,
                  (unsigned long)getBatteryAdcMillivolts());
    Serial.println("--------------------------------------------------------------------------");

    Serial.printf(" [SENSOR] IMU: %s | PPG: %s | TOUCH: %s | Free heap: %u B\n",
                  g_watchState.imuOk      ? "OK" : "FAIL",
                  g_watchState.hrSensorOk ? "OK" : "FAIL",
                  g_watchState.touchOk    ? "OK" : "FAIL",
                  (unsigned)ESP.getFreeHeap());

    if (g_watchState.skinContact) {
        // Raw IR and RED are printed on purpose: they are what you calibrate
        // PPG_CONTACT_IR_THRESHOLD and MAX30102_LED_BRIGHTNESS against.
        Serial.printf(" [HEALTH] Pulse: %s | SpO2: %s | Quality: %d%% | Motion: %s | IR: %u | RED: %u\n",
                      g_watchState.hrValid   ? String(g_watchState.heartRateBPM).c_str() : "--",
                      g_watchState.spo2Valid ? String(g_watchState.spo2Percent).c_str()  : "--",
                      g_watchState.signalQuality,
                      g_watchState.motionArtifact ? "YES" : "no",
                      g_watchState.irRaw, g_watchState.redRaw);
    } else {
        Serial.printf(" [HEALTH] Status: NO SKIN CONTACT (IR: %u, threshold: %lu)\n",
                      g_watchState.irRaw, (unsigned long)PPG_CONTACT_IR_THRESHOLD);
    }

    Serial.printf(" [MOTION] Accel (%6d, %6d, %6d) | Gyro (%6d, %6d, %6d)\n",
                  g_watchState.accX, g_watchState.accY, g_watchState.accZ,
                  g_watchState.gyroX, g_watchState.gyroY, g_watchState.gyroZ);

    const char* fallText = "OK (NORMAL)";
    switch (g_watchState.fallState) {
        case FALL_STATE_SUSPECTED: fallText = "CONFIRMING IMPACT"; break;
        case FALL_STATE_ALERT:     fallText = "ALERT - COUNTDOWN"; break;
        case FALL_STATE_SENDING:   fallText = "SENDING";           break;
        case FALL_STATE_SENT:      fallText = "SENT";              break;
        case FALL_STATE_FAILED:    fallText = "SEND FAILED";       break;
        default: break;
    }
    Serial.printf(" [FALL]   Status: %s | Countdown: %ds | Queued alerts: %d\n",
                  fallText, g_watchState.countdownSec, g_watchState.queuedAlerts);

    if (g_watchState.touched) {
        Serial.printf(" [TOUCH]  State: TOUCHED | X,Y: (%3d, %3d) | Gesture: %s\n",
                      g_watchState.touchX, g_watchState.touchY, g_watchState.gestureName.c_str());
    } else {
        Serial.println(" [TOUCH]  State: IDLE");
    }

    Serial.println("==========================================================================\n");
}

void setup() {
    // 921600 rather than the usual 115200 because the fall-detection CSV log
    // is written from inside the 20 ms sensor tick and Serial.printf blocks
    // until the bytes are out. A measured session at 115200 ran the loop at
    // 48 ms per tick -- the IMU was configured for 235 Hz and sampled four
    // times a tick, then sat idle for 43 ms, so the effective rate was 21 Hz
    // and a floor impact landed in one or two samples. The link speed was the
    // whole of the difference; the same figure already works for uploads.
    Serial.begin(921600);
    delay(1000);

    Serial.println("==========================================================================");
    Serial.println(" Waveshare ESP32-S3 Smart Health & Fall Detection Watch Firmware ");
    Serial.println("==========================================================================");

    // Initialize Main Fast I2C1 Bus (400kHz)
    Wire.begin(I2C1_SDA_PIN, I2C1_SCL_PIN, 400000);

    // Initialize Services
    initBatteryMonitor();
    initCST816SService();
    initQMI8658Service();
    initMAX30102Service();
    initFallDetector();
    initVitalMonitor();
    initAlertDispatcher();

    // Initialize UI Manager & Double Buffer Sprite
    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, HIGH);
    initUIManager();

    // Connect Wi-Fi (non-blocking) and start the BLE link to the phone
    initWiFiManager();
    initBLEService();

    // Deliberately the last line of setup(). GPIO 0 is a strapping pin and
    // must not be driven or read until the boot-mode decision is long past --
    // see SOS_BUTTON_PIN in app_config.h.
    initSOSButton();

    Serial.println("\n>>> All Hardware & Health Watch Services Active <<<\n");
}

void loop() {
    unsigned long now = millis();

#if FALL_LOG_TICK_PROFILE
    uint32_t loopT0 = micros();
#endif

    // Touch is interrupt-gated and cheap, so it is serviced every pass to keep
    // the UI feeling immediate.
    PROFILE_JOB(0, updateCST816SService());

    // Every pass, because the IMU runs at 235 Hz and the tick at 50 Hz. The
    // call is a timestamp comparison until a new sensor sample is actually due,
    // so the cost of asking often is small and the peak of each tick gets found
    // no matter where in the loop the impact lands.
    PROFILE_JOB(1, pollQMI8658Service());

    // True when the sensor work has already run this pass, which is what tells
    // the render block below to stand aside.
    bool sensorTickRan = false;

    if (now - lastSensorTick >= SENSOR_PERIOD_MS) {
        lastSensorTick = now;
        sensorTickRan = true;
        PROFILE_JOB(2, updateQMI8658Service());
        PROFILE_JOB(3, updateFallDetector());
        PROFILE_JOB(4, updateMAX30102Service());
        // On the sensor tick rather than every pass: a single digitalRead is
        // cheap, but 50 Hz already resolves a 1500 ms hold to within 2%.
        PROFILE_JOB(5, updateSOSButton());
    }

    PROFILE_JOB(6, updateAlertDispatcher());
    PROFILE_JOB(7, updateBLEService());

    if (now - lastUptimeTick >= 1000) {
        lastUptimeTick = now;
        g_watchState.uptimeSec = now / 1000;

#if FALL_LOG_TICK_PROFILE
        uint32_t secT0 = micros();
#endif
        // Once a second is plenty: the shortest thing it can react to is a
        // five-second sustained breach.
        updateVitalMonitor();

        struct tm timeinfo;
        // Zero timeout: NTP has already written the RTC, so this is a local
        // read. The default 5 s timeout would stall the whole loop.
        if (g_watchState.timeSynced && getLocalTime(&timeinfo, 0)) {
            g_watchState.year      = timeinfo.tm_year + 1900;
            g_watchState.month     = timeinfo.tm_mon;
            g_watchState.day       = timeinfo.tm_mday;
            g_watchState.dayOfWeek = timeinfo.tm_wday;
            g_watchState.hour      = timeinfo.tm_hour;
            g_watchState.minute    = timeinfo.tm_min;
            g_watchState.second    = timeinfo.tm_sec;
        } else {
            uint32_t sec = g_watchState.uptimeSec;
            g_watchState.hour   = (sec / 3600) % 24;
            g_watchState.minute = (sec / 60) % 60;
            g_watchState.second = sec % 60;
        }
#if FALL_LOG_TICK_PROFILE
        uint32_t secDt = micros() - secT0;
        if (secDt > jobPeaks[8].peakUs) jobPeaks[8].peakUs = secDt;
#endif
    }

    if (now - lastBatteryTick >= 5000) {
        lastBatteryTick = now;
#if FALL_LOG_TICK_PROFILE
        uint32_t batT0 = micros();
#endif
        updateBatteryMonitor();
        updateWiFiManager();
#if FALL_LOG_TICK_PROFILE
        uint32_t batDt = micros() - batT0;
        if (batDt > jobPeaks[9].peakUs) jobPeaks[9].peakUs = batDt;
#endif
    }

    // Render only on a pass that did no sensor work. renderUI() ends in a
    // blocking pushSprite of 240x240 at 16 bpp -- 115 kB down the SPI bus,
    // measured at 31 ms, which is longer than the entire 20 ms tick it would
    // otherwise share a pass with.
    //
    // Deferring by one pass costs nothing, because the loop runs far faster
    // than either period: the next pass arrives within microseconds, still
    // well inside the frame budget. What it buys is that the sensor tick is
    // never queued behind a frame, so a fall impact cannot land in a 31 ms
    // blind spot.
    //
    // Rate limiting stays: this decides *whether* a due frame may run now,
    // not how often frames are due.
    if (!sensorTickRan && now - lastRenderTick >= RENDER_PERIOD_MS) {
        lastRenderTick = now;
        PROFILE_JOB(10, renderUI());
    }

    // Same reasoning, and it matters more than it looks: printSerialLog() is a
    // ~6 ms blocking write, and the worst ticks in the profile were the ones
    // where it landed on the same pass as a frame.
    if (!sensorTickRan && now - lastLogTick >= 2000) {
        lastLogTick = now;
        PROFILE_JOB(11, printSerialLog());
    }

#if FALL_LOG_TICK_PROFILE
    // Measured before the report itself, so the printf below is excluded --
    // it only runs on the tick that reports and would otherwise show up as a
    // phantom spike in the very number being investigated.
    uint32_t loopDt = micros() - loopT0;
    if (loopDt > loopPeakUs) loopPeakUs = loopDt;

    // Every 2 s, alongside the existing log rather than on its own timer, so
    // the report costs one print per two seconds instead of one per loop.
    static unsigned long lastProfileTick = 0;
    if (now - lastProfileTick >= 2000) {
        lastProfileTick = now;
        reportTickProfile();
    }
#endif
}
