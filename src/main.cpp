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

    // Touch is interrupt-gated and cheap, so it is serviced every pass to keep
    // the UI feeling immediate.
    updateCST816SService();

    // Every pass, because the IMU runs at 235 Hz and the tick at 50 Hz. The
    // call is a timestamp comparison until a new sensor sample is actually due,
    // so the cost of asking often is small and the peak of each tick gets found
    // no matter where in the loop the impact lands.
    pollQMI8658Service();

    if (now - lastSensorTick >= SENSOR_PERIOD_MS) {
        lastSensorTick = now;
        updateQMI8658Service();
        updateFallDetector();
        updateMAX30102Service();
        // On the sensor tick rather than every pass: a single digitalRead is
        // cheap, but 50 Hz already resolves a 1500 ms hold to within 2%.
        updateSOSButton();
    }

    updateAlertDispatcher();
    updateBLEService();

    if (now - lastUptimeTick >= 1000) {
        lastUptimeTick = now;
        g_watchState.uptimeSec = now / 1000;

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
    }

    if (now - lastBatteryTick >= 5000) {
        lastBatteryTick = now;
        updateBatteryMonitor();
        updateWiFiManager();
    }

    if (now - lastRenderTick >= RENDER_PERIOD_MS) {
        lastRenderTick = now;
        renderUI();
    }

    if (now - lastLogTick >= 2000) {
        lastLogTick = now;
        printSerialLog();
    }
}
