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

// Minimum time between two accepted navigations, in milliseconds.
//
// This is a second gate, not a replacement for the per-contact latch in
// cst816s_service.cpp. That latch already guarantees one navigation per touch,
// so it is not what lets a single swipe run away. What it cannot see is one
// physical swipe that the controller splits into two contacts: a finger
// dragging across the glass momentarily lightens, points drops to 0, and the
// re-press is a new contact by every test the latch has. Two screens from one
// swipe, and the wearer's hand never left the glass.
//
// 350 ms is chosen from the two intervals it sits between. A deliberate
// repeat -- swipe, lift, swipe again to walk the carousel -- takes an
// unhurried wearer well over half a second, so nothing intentional is
// swallowed. A split contact re-presses within a few tens of milliseconds,
// since it is one continuous movement. The gap between those two is wide, and
// 350 ms sits in it rather than close to either edge, so neither a slightly
// faster wearer nor a slightly slower split lands on the wrong side.
//
// Raising this makes the UI feel heavier and eventually starts dropping real
// swipes; the first sign is a wearer swiping twice on purpose and arriving one
// screen short, which is the failure this whole path already had once and is
// worth not reintroducing from the other direction.
#define TOUCH_GESTURE_MIN_GAP_MS  350

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
#define MAX30102_LED_BRIGHTNESS  0x40

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
#define PPG_CONTACT_IR_THRESHOLD 25000UL

// Contact is harder to lose than to gain. Once the finger is on, IR has to fall
// well below the acquire threshold before contact is doubted -- pressing a
// finger down is never perfectly steady, and the pulsatile component alone
// swings the reading by a few percent every beat.
#define PPG_CONTACT_IR_RELEASE   20000UL

// How many consecutive sub-threshold samples mean the finger really left, as
// opposed to a twitch. 200 samples at 200 Hz is one second.
//
// This is what makes a four-second SpO2 window achievable at all: that window
// is 800 raw samples, and requiring all 800 to be perfect is a standard no real
// finger meets.
//
// Was 100 here while max30102_service.cpp compared against a literal 200. The
// firmware's one-second debounce was the deliberate behaviour -- a wrist shifts
// under the strap constantly and half a second dropped the reading to zero on
// ordinary movement -- so the constant is corrected to the value in use rather
// than the code to the constant. Before this, editing the figure below changed
// nothing at all.
#define PPG_CONTACT_GAP_SAMPLES  200

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
#define PPG_MIN_SQI              1

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

// Minimum spacing between IMU reads, in microseconds. The sensor runs at
// 235 Hz -- a new sample every 4.3 ms -- while the sensor tick is 20 ms.
// Reading once per tick would discard three samples out of four, and the one
// kept would be chosen by loop timing rather than by what the wrist was doing.
// A floor impact lasts a few milliseconds, so it usually fell in a discarded
// sample and the recorded peak was a random point on the slope.
//
// pollQMI8658Service() runs every pass of loop() and reads whenever this much
// time has passed, holding the largest sample until the tick collects it. It
// compares against micros() rather than waiting: an earlier version spaced the
// reads with delayMicroseconds() inside a single tick, which cost 12.9 ms of
// stalled CPU per tick and stretched the tick itself to a measured 48 ms.
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
//
// Raised 3.2 -> 3.5 on 2026-08-16 from the two fall captures. The 13 recorded
// falls peak at 3.53g and above; the loudest non-fall movement in the same
// recordings reaches 3.09g. At 3.2 the gap between them was 0.11g, which is
// less than the run-to-run spread of a single gesture. 3.5 keeps all 13 falls
// and widens the margin to 0.41g, so it costs nothing measured and buys room.
//
// WHAT THIS PATH STILL DOES NOT COVER, and why it is not fixed here.
//
// The slump and the slide down a wall named above are the reason path B exists,
// but re-reading fall_features.csv shows the captures never contained one. All
// 13 labelled falls have min_g at or below 0.82g and 11 of them go below 0.4g,
// so every recorded fall passed through free fall and would have entered by
// path A regardless. They were dropped or thrown, peaking at 3.53-11.7g. A slow
// collapse is absent from the data entirely.
//
// So the detector has no measured example of the case this path was written
// for, and a third entry path for slow falls cannot be designed from what is
// here: every threshold in it would be invented. The three labelled non-falls
// peak at 1.55g, 2.30g and 3.09g, which sets the trap concretely -- a slow fall
// produces no sharp impact, so any bar low enough to catch one sits below 3.09g
// and starts admitting ordinary movement. With three negatives there is no way
// to estimate what that would cost.
//
// This is the same trap FALL_ORIENTATION_MIN_DEG below was disabled to avoid:
// keeping a feature whose behaviour is unmeasured and tuning it until the
// recordings pass. The honest position is that slow falls are currently NOT
// detected, and that closing the gap needs captures containing one -- a
// controlled slump onto a mat, and the long negative session that would show
// what a lower bar costs.
//
// Raised 3.5 -> 5.0 -> 7.0 on 2026-08-16, from the first negative capture that
// exists.
// Ten minutes worn on the wrist, arms swung deliberately hard, produced 60
// events; 16 entered by path B and every one of them confirmed. Path B was
// therefore responsible for 14 of the session's 18 false alarms while, across
// every capture ever taken, deciding zero real falls.
//
// The reason it fires so freely is that nothing downstream stops it. Once past
// this threshold the only live test is calm_ratio >= 0.5, because
// FALL_ORIENTATION_MIN_DEG is 0 and the orientation check is compiled to an
// unconditional pass. So path B currently means "hit harder than this, then
// hold reasonably still for 3 s" -- which is also a description of swinging an
// arm and letting it rest.
//
// 7.0 is a chosen operating point, not a fitted value, and peak_g cannot be
// fitted here in any case: the hardest arm swing in the capture peaks at 9.24g
// and the lightest labelled fall at 3.53g, so the two distributions overlap
// across their whole width. Every value on this axis is a trade, and the trade
// was made toward fewer false alarms. Scoring this capture, path B false
// entries go 16 (at 3.5) -> 6 (at 5.0) -> 3 (at 7.0) -> 1 (at 9.5).
//
// It costs no measured detection. All 13 labelled falls dip into free fall
// (min_g <= 0.82g, 11 below 0.4g), so all 13 enter by path A at 2.5g and never
// consult this constant -- which is what makes moving it cheap, and also what
// should temper any confidence drawn from that.
//
// Because the thing being traded away is not visible in the data. 7.0 sits
// above 9 of the 13 recorded falls, so path B is now a backstop only for
// impacts harder than most real falls produce. The case it was written for --
// a collapse with no free-fall phase, which path A cannot see -- is precisely
// the case that has never been captured, so raising the bar past it costs
// nothing measurable while plausibly costing something real. That asymmetry is
// worth stating plainly rather than reading the unchanged fall count as
// evidence the move was free.
//
// What actually separates the two sets is min_g -- 11 of 13 falls below 0.4g,
// against 1 of 16 path B false entries. Requiring free-fall evidence here would
// remove 15 of the 16 while leaving the bar low, which is the fix this
// threshold ladder is a substitute for: it would cut false alarms further than
// 7.0 does and keep path B able to catch a light impact, instead of buying the
// first by giving up the second. It is not a threshold move but a change to
// what path B means, so it stays a deliberate decision rather than something
// folded in here. This remains the next thing to do to path B.
#define FALL_IMPACT_STANDALONE_G  7.0f

// How long after a free fall an impact still counts as belonging to it.
#define FALL_IMPACT_WINDOW_MS     1500UL

// How far back the pre-fall posture is read from, for path B only.
//
// Path B sees no free fall, so there is no earlier event to snapshot on: by the
// time the impact registers, the body is already down. The old code snapshotted
// the live gravity estimate on the impact sample, which is a posture already
// partway through the fall.
//
// Sizing this needs the rate the gravity filter actually runs at, which is NOT
// the IMU sampling rate. pollQMI8658Service() samples at 235 Hz to catch impact
// peaks, but updateFallDetector() -- and with it the 0.98/0.02 low-pass -- runs
// on the 20 ms sensor tick, 50 Hz. So the filter's time constant is
// 1/(50 * 0.02) = 1.0 s, and a fall of 400-800 ms moves it by roughly a third
// to a half. Enough to matter, and less than it would be at the IMU rate.
//
// 600 ms is a compromise between two failure modes rather than a measured
// figure. Too short and the entry is still mid-fall, which is the bug. Too long
// and it reaches back past the fall into whatever came before -- bending to
// pick something up would then read as a posture change by itself. At 600 ms
// the estimate retains about 55% of the standing orientation, so the ring entry
// is meaningfully closer to upright than the impact-moment value without
// reaching into unrelated activity.
//
// UNCALIBRATED, like every other constant in this block, and resting on the
// filter time constant plus published fall durations rather than a measurement
// from this device. The tilt check it feeds is currently disabled
// (FALL_ORIENTATION_MIN_DEG 0.0), so today it only affects the logged angle --
// which is exactly the number a calibration session needs to be trustworthy
// before that check can be judged.
#define FALL_PREFALL_DELAY_MS     600UL

// Ring capacity, in ticks. Only a bound on how far back the search can reach --
// the lookback itself is set by FALL_PREFALL_DELAY_MS above, because entries
// carry timestamps and are selected by age rather than by counting backwards.
//
// It has to hold FALL_PREFALL_DELAY_MS worth of ticks in the worst case, which
// is the FASTEST the tick can run: a short tick means more entries per second.
// updateFallDetector() runs on the 20 ms sensor tick, so 600 ms is 30 entries
// at nominal speed. It cannot run faster than that, since the tick is
// millis()-gated, and it often runs slower -- a measured capture showed ~48 ms
// with a frame in flight, which needs only 13 entries for the same 600 ms.
//
// 64 doubles the nominal requirement so that lowering SENSOR_PERIOD_MS or
// raising FALL_PREFALL_DELAY_MS moderately does not silently truncate the
// lookback. If either changes a lot, check that 64 * SENSOR_PERIOD_MS still
// exceeds FALL_PREFALL_DELAY_MS; if it does not, the search runs out of
// entries and falls back to the live estimate, which is the bug this was
// written to fix. Four arrays of 64 cost 1 kB.
#define FALL_PREFALL_RING_LEN     64

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
// tolerating the two or three seconds of trying to get up.
//
// 0.70 was that starting point. Replaced with 0.50 on 2026-08-16 from the
// sessions the old comment asked for: across 13 recorded falls the calm ratio
// runs 0.59 to 1.00, so 0.70 threw away the one fall at 0.59 -- a wearer who
// kept struggling, which is not evidence against having fallen. Nothing is
// given up by the change: the three non-fall events never reach confirmation
// at all, so no ratio of theirs is ever tested. 0.50 sits below the lowest
// real fall with margin and still means "motionless for most of the window".
#define FALL_STILLNESS_MIN_CALM_RATIO  0.50f

// Phase 4 -- posture must actually have changed, in degrees between the
// gravity direction before the event and after it.
//
// DISABLED (0) on 2026-08-16. Zero switches the test off entirely rather than
// lowering it; fall_detector.cpp special-cases it, and the angle is still
// computed and printed on every confirmation line.
//
// The measurement that forced this. Two capture sessions, same firmware, same
// 30 deg threshold, thirteen real falls between them:
//
//     log_nga_   arms down    6 falls   tilt  13-27 deg    1 of 6 caught
//     log_nga2_  arms out     7 falls   tilt 139-176 deg   7 of 7 caught
//
// The threshold was not measuring whether the wearer fell. It was measuring
// where their arm happened to be, and it separated the two sessions almost
// perfectly while separating falls from non-falls not at all. An elderly
// person going down does not get to choose their arm position, so a check
// that only passes for one of them is worse than no check: it fails silently,
// and it fails hardest on the arms-down posture a real collapse produces.
//
// Lowering it to ~12 deg would have scored 12 of 13 on this data, and that is
// the tempting move. It is rejected on purpose -- it keeps a feature whose
// measured behaviour is posture-dependent and merely tunes the dependence
// until this particular pair of recordings passes.
//
// What replaces it: nothing yet, and that is the honest state. With tilt off,
// impact peak plus the stillness ratio give 13 of 13 falls and 0 of 3 false
// alarms on everything recorded so far. Three negatives is far too few to
// claim a false-alarm rate, so the number that is actually unknown is how
// often this fires during an ordinary day. Turning the check back on -- or
// replacing it with something that measures the fall rather than the wrist --
// needs a long capture of normal living first. See the note on
// FALL_LOG_RAW_SAMPLES below.
#define FALL_ORIENTATION_MIN_DEG  0.0f

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
// Currently OFF. The 2026-08-16 negative session is captured and analysed;
// log_negative_10min.txt holds it. Note that turning this on also compiles
// renderUI() out of main.cpp, so the display goes dark and the fall alert's
// 15 s cancel button goes with it -- which is why alert_dispatcher.cpp
// suppresses sending entirely while it is 1.
#define FALL_LOG_RAW_SAMPLES      0

// Temporary instrumentation for the tick budget. Prints a TICKPROF line every
// 2 s giving the worst single duration each job in loop() has cost since the
// last report, in microseconds.
//
// It exists to answer one open question: after the IMU stopped blocking, the
// tick median reached its 20 ms target but p95 stayed at 41 ms and max at 51 --
// one tick in twenty taking twice as long, with the cause unknown. Render was
// the obvious suspect and was ruled out by measurement, since that capture
// already had frames at 10 FPS.
//
// Worst case rather than mean, because an average over a hundred ticks hides
// exactly the rare stall being hunted.
//
// Set back to 0 once the answer is in. The timing calls are cheap but the
// report is a blocking serial write, so it competes with what it measures.
#define FALL_LOG_TICK_PROFILE     0

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

// ---------------------------------------------------------------------------
// Clinical Thresholds for Alerts & Color Coding (Strictly for Notifications, NEVER Hardcoded Measurements)
// ---------------------------------------------------------------------------
//
// NOT WIRED UP. Every constant in this block is unreferenced by the firmware --
// verified by grepping the whole of src/ -- and the web dashboard classifies
// from its own numbers rather than reading these. Editing them changes nothing
// on the device today.
//
// What actually decides an alert is the older set above:
//
//     VITAL_HR_HIGH   130      vs  ALERT_HR_TACHYCARDIA_BPM      110
//     VITAL_HR_LOW    45       vs  ALERT_HR_BRADYCARDIA_BPM      50
//     VITAL_SPO2_LOW  90       vs  ALERT_SPO2_MILD_HYPOXIA_PCT   94
//     BATTERY_LOW_PCT 15       vs  ALERT_BATTERY_LOW_PCT         20
//
// The two sets disagree in the same direction every time: the live thresholds
// are the more permissive ones, so the watch is quieter than this block reads
// as promising. A tachycardia at 115 BPM or an SpO2 of 92% passes in silence.
//
// They are kept rather than deleted because the graded scheme is the better
// design -- mild versus severe hypoxia deserve different responses, which the
// single VITAL_SPO2_LOW cannot express. Adopting it is a clinical decision
// about who gets woken at 3 a.m., not a tidy-up, so it is left for whoever
// makes that call. Until then this comment is here so nobody reads the block
// and believes the device behaves as it describes.
#define ALERT_HR_BRADYCARDIA_BPM      50    // Heart rate below 50 BPM -> Bradycardia warning
#define ALERT_HR_TACHYCARDIA_BPM      110   // Heart rate above 110 BPM -> Tachycardia warning
#define ALERT_HR_CRITICAL_HIGH_BPM    130   // Heart rate above 130 BPM -> Critical high warning
#define ALERT_SPO2_MILD_HYPOXIA_PCT   94    // SpO2 below 95% -> Mild hypoxia warning
#define ALERT_SPO2_SEVERE_HYPOXIA_PCT 90    // SpO2 below 90% -> Critical hypoxia emergency
#define ALERT_BATTERY_LOW_PCT         20    // Battery below 20% -> Low battery notification


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
