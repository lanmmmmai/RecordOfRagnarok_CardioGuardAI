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

// Smoothing on the pack voltage. The monitor runs every 5 s, so 0.25 settles
// in roughly a minute -- slow enough to kill ADC jitter, fast enough that the
// gauge still follows a real change.
#define BATTERY_FILTER_ALPHA     0.25f

// How far apart two voltage samples must be before their difference is treated
// as a trend rather than as noise. Two minutes at 5 s per reading.
#define BATTERY_TREND_WINDOW_MS  120000UL

// Minimum rise across that window to call it charging. A cell on a normal
// charger gains far more than 10 mV in two minutes; a bare USB rail gains
// nothing. See the reasoning in battery_monitor.cpp.
#define BATTERY_CHARGE_RISE_V    0.010f

// Low-battery warning. Two thresholds, not one: a pack resting on a single
// boundary drifts across it repeatedly with load and temperature, and each
// crossing would be another message to the family group.
#define BATTERY_LOW_PCT          15
#define BATTERY_CLEAR_PCT        20

// Readings to discard at power-on before the warning may fire. The smoothing
// filter is seeded from the first sample, so that sample carries no smoothing
// at all -- one noisy reading at boot must not send a low-battery alert about
// a full pack. At the 5 s battery tick this is a 20 s hold-off.
#define BATTERY_WARN_MIN_READINGS 4

// ---------------------------------------------------------------------------
// Heart-rate sensing, wrist-worn
// ---------------------------------------------------------------------------
// LED drive current. 0x24 (~7mA) is a finger-on-sensor setting; skin on the
// underside of the wrist has far fewer capillaries and needs more light.
// Raise or lower this against the raw IR values in the serial log.
#define MAX30102_LED_BRIGHTNESS  0x30

// Minimum IR reading that counts as skin contact.
//
// The original 8,000 marked the watch as "in contact with skin" at all times:
// on this unit the sensor sitting on a desk facing open air already floored at
// ~21,900, so nothing could ever fall below the threshold. That floor was
// measured at LED brightness 0x5F and does NOT carry over to the 0x30 in use
// now -- less drive current means a lower open-air floor. 25,000 comes from
// trial on the bench at the current setting, not from a fresh measurement.
// Still provisional: log a session with the watch actually worn, then set this
// midway between the open-air floor and the worn reading.
// Measured on this unit, 2026-08-15: open air reads ~11,000, a finger on the
// sensor reads 123,000-151,000. An order of magnitude apart, so 25,000 sits
// comfortably in the gap and is now backed by a reading rather than a bench
// guess.
#define PPG_CONTACT_IR_THRESHOLD 18000UL

// Contact is harder to lose than to gain. Once the finger is on, IR has to fall
// well below the acquire threshold before contact is doubted -- pressing a
// finger down is never perfectly steady, and the pulsatile component alone
// swings the reading by a few percent every beat.
#define PPG_CONTACT_IR_RELEASE   15000UL

// How many consecutive sub-threshold samples mean the finger really left, as
// opposed to a twitch. 100 samples at 200 Hz is half a second.
//
// This is what makes a four-second SpO2 window achievable at all: that window
// is 800 raw samples, and requiring all 800 to be perfect is a standard no real
// finger meets.
#define PPG_CONTACT_GAP_SAMPLES  100

// Above this accelerometer standard deviation (in g) the arm is moving too
// much for the PPG waveform to mean anything.
#define PPG_MOTION_STD_G         0.08f

// Rate-of-change gate on the displayed heart rate. A beat that differs from
// the current reading by more than this is treated as a mis-detection and
// dropped.
#define PPG_MAX_BPM_STEP         15.0f

// ...but the gate compares each new beat against its own previous output, so
// without a way out it can never recover once reality moves away from the
// number it is holding: every new beat looks like a spike, gets rejected, and
// leaves the stale value in place to reject the next one. That failure is
// worse than the phantom readings the gate was added to stop, because a frozen
// plausible number looks correct. After this many consecutive rejections the
// next beat is accepted unconditionally and the gate re-locks around it.
//
// 8 beats is roughly 5-8 seconds at normal rates -- long enough that noise
// bursts still get filtered, short enough that a real tachycardia onset shows
// up well inside the window that matters.
#define PPG_GATE_ESCAPE_BEATS    8

// Scalar Kalman on the displayed heart rate (DSP stage 5). The median filter
// removes outright spikes; this handles what is left, trading responsiveness
// for smoothness according to how much the two numbers below disagree.
//
// Process noise: how fast a real heart rate changes. Smaller means trust the
// model, which is smoother but slower to follow a genuine change.
#define PPG_KALMAN_Q             0.5f

// Measurement noise: how wrong a single beat measurement can be. Larger means
// filter harder. At Q=0.5 and R=4.0 the steady-state gain is about 0.28, so
// roughly a quarter of each new measurement is taken -- a few beats to move
// 10 BPM, which is faster than any physiological change worth showing.
#define PPG_KALMAN_R             4.0f

// Signal-quality floor below which no heart rate is displayed at all.
//
// START AT 20, NOT 75. The SQI scale here is `perfusion * 50` -- an
// uncalibrated ratio, not a meaningful percentage. Setting 75 before the scale
// has been read off a real wrist would mean the screen never shows a number.
// Read the actual SQI values from the log in Giai đoạn 3b, then tighten.
#define PPG_MIN_SQI              20

// ---------------------------------------------------------------------------
// Physiological threshold alerts
// ---------------------------------------------------------------------------

// Screening thresholds, not diagnostic ones. Deliberately wide: a resting adult
// sits well inside them, and exercise or a nap should not page the family.
#define VITAL_HR_HIGH            130
#define VITAL_HR_LOW             45
#define VITAL_SPO2_LOW           90

// The condition must hold continuously for this long. One stray reading is not
// a medical event, and the timer resets the moment the condition clears.
#define VITAL_SUSTAIN_MS         5000UL

// Minimum gap between two messages about the same condition. Somebody whose
// SpO2 sits low for an hour needs help, not hundreds of identical alerts --
// and a flooded chat is a chat people stop reading.
#define VITAL_REPEAT_MS          600000UL

// ---------------------------------------------------------------------------
// Physical SOS button
// ---------------------------------------------------------------------------

// The BOOT button, reused as SOS.
//
// ⚠️ GPIO 0 is a strapping pin: its level at reset decides whether the ESP32
// enters download mode or runs the firmware. Do not pinMode() or read it until
// setup() has finished -- touching it earlier interferes with that decision.
#define SOS_BUTTON_PIN           0

// How long the button must be held. Long enough that a knock against a
// doorframe or a pocket cannot raise a false alarm, short enough that somebody
// who has fallen does not have to keep holding while they wait.
#define SOS_HOLD_MS              1500UL

// ---------------------------------------------------------------------------
// Fall detection, wrist-worn
// ---------------------------------------------------------------------------
// NONE OF THESE ARE CALIBRATED. They are the usual starting points for a
// wrist detector, not values measured on this watch on a real wearer. Treat
// every number below as provisional until a logging session says otherwise --
// see FALL_LOG_RAW_SAMPLES at the bottom of this block.

// Accelerometer full scale. qmi8658_service.cpp configures +/-8g, so the
// int16 range 32768 maps to 8g. Change both together or every threshold here
// silently shifts.
#define FALL_ACCEL_LSB_PER_G      4096.0f

// How many times updateQMI8658Service() reads the sensor per 20 ms tick,
// keeping the sample with the largest acceleration magnitude.
//
// The sensor runs at 235 Hz -- a new sample every 4.3 ms -- while the loop
// ticks every 20 ms. Reading once per tick therefore discards three samples out
// of four, and the one kept is chosen by loop timing rather than by what the
// wrist was doing. A floor impact lasts a few milliseconds, so it usually fell
// in a discarded sample and the recorded peak was a random point on the slope.
//
// Four reads at 4300 us cover 12.9 ms of the 20 ms tick. Cost is roughly
// 4 x 12 bytes over I2C at 400 kHz plus the waits, well inside the tick budget
// shared with the 200 Hz PPG sampling and the display.
#define QMI_SAMPLES_PER_TICK      4
#define QMI_SAMPLE_GAP_US         4300

// Phase 1 -- free fall. Below this total acceleration the wrist is unsupported.
#define FALL_FREEFALL_G           0.4f

// Phase 2 -- impact, when a free fall preceded it. Lower than the standalone
// figure below because free fall has already made a fall likely.
#define FALL_IMPACT_G             2.5f

// Phase 2, second entry path -- impact with NO preceding free fall.
//
// A wrist does not always reach 0.4g in a real fall: someone who slumps into
// a chair, slides down a wall or faints while already seated never gets the
// free-fall signature, and those are among the more dangerous falls. This
// threshold is deliberately higher than FALL_IMPACT_G so that everyday
// gestures -- setting a mug down hard, clapping -- still have to survive the
// stillness and orientation phases before anything is raised.
#define FALL_IMPACT_STANDALONE_G  3.2f

// How long after a free fall an impact still counts as belonging to it.
#define FALL_IMPACT_WINDOW_MS     1500UL

// Phase 3 -- how long the wearer must lie still, measured from the impact.
#define FALL_CONFIRM_WINDOW_MS    3000UL

// Settling time after the impact, during which motion is NOT held against the
// wearer.
//
// This exists because of a real defect: stillness used to be measured from the
// impact sample itself, while the wrist is still bouncing and the arm is still
// coming to rest. At a 20 ms sample period the very next reading is part of
// the same impact, so a single sample would pin confirmMaxDev above the limit
// and the alert would be dismissed. The harder the fall, the more certainly it
// was thrown away -- exactly backwards. A wrist takes roughly 100-300 ms to
// settle, so nothing in the first FALL_SETTLE_MS counts.
#define FALL_SETTLE_MS            400UL

// Phase 3 -- how far from 1g a single settled sample may sit and still count
// as calm.
#define FALL_STILLNESS_MAX_DEV_G  0.35f

// Phase 3 -- what fraction of the settled samples must be calm for the wearer
// to count as lying still.
//
// The test used to be on the worst single sample: one reading over the limit
// anywhere in 2.6 seconds dismissed the alert. That asks somebody who has just
// hit the floor to lie perfectly still, and a conscious person does the
// opposite -- pushes up on an elbow, rolls over, reaches for something to pull
// against. All of it happens at the wrist. The old rule therefore grew more
// certain to stay silent the harder the wearer tried to help themselves.
//
// A ratio keeps the meaning (mostly motionless, not walking around) while
// tolerating the two or three seconds of trying to get up. 0.70 is a starting
// point, not a measurement: it says roughly two thirds of the window must be
// quiet. The confirmation log prints calm/total on every event, so a session
// of real falls gives the number to replace it with.
#define FALL_STILLNESS_MIN_CALM_RATIO  0.70f

// Phase 4 -- posture must actually have changed, in degrees between the
// gravity direction before the event and after it.
#define FALL_ORIENTATION_MIN_DEG  30.0f

// Print a one-line note when acceleration passes this without being large
// enough to start confirmation, at most once per FALL_NEARMISS_LOG_MS.
//
// Set below FALL_IMPACT_G so both entry paths have a margin above it. Its job
// is to distinguish "nothing happened" from "the thresholds are too high to
// catch anything" -- two cases that otherwise produce identical silence.
#define FALL_NEARMISS_LOG_G       1.8f
#define FALL_NEARMISS_LOG_MS      500UL

// Set to 1 to stream one CSV line per IMU tick, continuously, in every state.
//
// This is the training set for the fall model, and it has to run continuously
// rather than only during confirmation: a classifier shown nothing but falls
// learns to call everything a fall. The ordinary hours in between -- walking,
// eating, clapping, setting a mug down -- are the negative examples, and they
// exist only if the log never stops.
//
// Off by default: about 60 bytes per 20 ms tick is ~3 kB/s of blocking serial
// writes. Turn it on for dedicated collection sessions, and turn it back off
// afterwards. Lines are prefixed FALLCSV; the columns are
//   millis, accX, accY, accZ, gyroX, gyroY, gyroZ, totalG, fallState
// Currently ON: a data-collection session is in progress. Set back to 0 once
// the fall recordings are done.
#define FALL_LOG_RAW_SAMPLES      1

// ---------------------------------------------------------------------------
// Alerting
// ---------------------------------------------------------------------------
#define TELEGRAM_API_HOST     "api.telegram.org"
#define TELEGRAM_API_PORT     443
#define ALERT_HTTP_TIMEOUT_MS 8000
#define ALERT_MAX_RETRIES     3
#define ALERT_QUEUE_MAX       8      // events held in NVS while offline

// How long the outcome screen stays up before the watch returns itself to
// normal monitoring.
//
// This exists because the only other way out of FALL_STATE_SENT is a finger on
// the glass -- and the wearer this device is built for is, by definition,
// possibly unconscious. Without a timeout the watch parks on the alert screen
// at full brightness until the battery dies, and detects nothing further:
// updateFallDetector() only looks for impacts while the state is NORMAL, so a
// second fall after an unacknowledged first one would go unnoticed.
//
// Five minutes is long enough for someone who is conscious to read the screen
// and understand help was called, short enough that the watch is back to
// watching well within the window where a follow-up event still matters.
#define ALERT_SENT_AUTO_CLEAR_MS   300000UL   // 5 minutes

// The failed screen is held longer: the outbox is still retrying every ten
// seconds behind it, and "not delivered yet" is information the wearer acts on
// differently from "delivered". It still clears on its own for the same reason
// as above.
#define ALERT_FAILED_AUTO_CLEAR_MS 900000UL   // 15 minutes

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
