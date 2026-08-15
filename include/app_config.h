#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Wi-Fi credentials and Telegram bot token live in secrets.h, which is
// git-ignored. Copy secrets.h.example if the file is missing.
#include "secrets.h"

// NTP Server & Timezone (Vietnam UTC+7)
#define NTP_SERVER_PRIMARY "pool.ntp.org"
#define GMT_OFFSET_SECONDS (7 * 3600)
#define DAYLIGHT_OFFSET_SECONDS 0

// I2C Bus 1: Touch Controller (CST816S) & 6-Axis IMU (QMI8658)
#define I2C1_SDA_PIN      6
#define I2C1_SCL_PIN      7
#define CST816S_I2C_ADDR  0x15
#define QMI8658_I2C_ADDR  0x6B

// Dedicated I2C Bus 2: Pulse Oximeter & Heart Rate Sensor (MAX30102)
#define I2C2_SDA_PIN      15
#define I2C2_SCL_PIN      16
#define MAX30102_I2C_ADDR 0x57

// Touch Control Pins
#define TOUCH_RST_PIN     13
#define TOUCH_INT_PIN      5

// Display Backlight
#define LCD_BL_PIN         2

// Battery Voltage ADC Sense Pin
#define BATTERY_ADC_PIN    1

// Resistor divider between the cell and BATTERY_ADC_PIN.
//
// The hardware overview does not document either the sense pin or the divider,
// so both are inferred. The evidence for 3.0: with the calibrated ADC the pin
// measures a steady ~1.41 V. At a 2:1 ratio that would mean a 2.82 V rail --
// impossible, because the 3.3 V regulator feeding the board would have dropped
// out long before that and the watch is running fine. At 3:1 it means 4.23 V,
// which is exactly what VSYS sits at with USB attached (ETA6096 charging).
//
// Verify before trusting the gauge: put a multimeter across the BAT pads and
// compare with the "raw" figure in the [WATCH LOG] line. If the meter reads V
// and the log reads m mV, the true ratio is V / (m / 1000).
#define BATTERY_DIVIDER_RATIO 3.0f

// ---------------------------------------------------------------------------
// Heart-rate sensing, wrist-worn
// ---------------------------------------------------------------------------
// LED drive current. 0x24 (~7mA) is a finger-on-sensor setting; skin on the
// underside of the wrist has far fewer capillaries and needs more light.
// Raise or lower this against the raw IR values in the serial log.
#define MAX30102_LED_BRIGHTNESS  0x5F

// Minimum IR reading that counts as skin contact.
//
// Measured on this unit with LED brightness 0x5F: the sensor sitting on a desk
// facing open air already floors at ~21,900, so the original 8,000 marked the
// watch as "in contact with skin" at all times. Provisional value pending a
// worn measurement -- log a session with the watch on the wrist, then set this
// midway between the ~21,900 floor and the worn reading.
#define PPG_CONTACT_IR_THRESHOLD 35000UL

// Above this accelerometer standard deviation (in g) the arm is moving too
// much for the PPG waveform to mean anything.
#define PPG_MOTION_STD_G         0.06f

// ---------------------------------------------------------------------------
// Alerting
// ---------------------------------------------------------------------------
#define TELEGRAM_API_HOST     "api.telegram.org"
#define TELEGRAM_API_PORT     443
#define ALERT_HTTP_TIMEOUT_MS 8000
#define ALERT_MAX_RETRIES     3
#define ALERT_QUEUE_MAX       8      // events held in NVS while offline

// How long the phone may be out of BLE range before the watch reports it.
#define BLE_LEASH_TIMEOUT_MS  60000UL

// BLE GATT identifiers. Documented for the companion app in BLE_PROTOCOL.md.
#define BLE_DEVICE_NAME        "HealthWatch"
#define BLE_SERVICE_UUID       "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_CHAR_VITALS_UUID   "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_CHAR_FALL_UUID     "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_CHAR_STATUS_UUID   "6e400004-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_CHAR_COMMAND_UUID  "6e400005-b5a3-f393-e0a9-e50e24dcca9e"

// Expansion SH1.0 GPIO Pins (Pins 15 & 16 reserved for I2C2 MAX30102)
const int SH10_EXPANSION_PINS[] = {17, 18, 21, 33};
const int NUM_SH10_PINS = sizeof(SH10_EXPANSION_PINS) / sizeof(SH10_EXPANSION_PINS[0]);

#endif // APP_CONFIG_H
