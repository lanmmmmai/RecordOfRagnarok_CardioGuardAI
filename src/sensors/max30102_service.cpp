#include "max30102_service.h"

#include <heartRate.h>
#include <spo2_algorithm.h>
#include <math.h>

#include "../dsp/rr_analysis.h"

static MAX30105 particleSensor;
static bool maxDetected = false;

// Beat-to-beat intervals, averaged over 5 beats for rock-solid stability
static const uint8_t RATE_SIZE = 5;
static uint8_t rates[RATE_SIZE];
static uint8_t rateIndex = 0;
static unsigned long lastBeatMs = 0;

static const uint8_t SPO2_SIZE = 5;
static uint8_t spo2History[SPO2_SIZE];
static uint8_t spo2Index = 0;

static uint32_t sampleIndex = 0;
static uint32_t lastBeatSample = 0;

// 200 Hz sampling (5.0ms per sample)
#define PPG_SAMPLE_PERIOD_MS 5.0f

static uint8_t gateRejections = 0;

// Cycle min/max tracking for true peak-to-peak AC calculation
static uint32_t cycleMinIr = 0xFFFFFFFF, cycleMaxIr = 0;
static uint32_t cycleMinRed = 0xFFFFFFFF, cycleMaxRed = 0;

static uint16_t contactGapSamples = 0;
static uint32_t irMin = 0xFFFFFFFF, irMax = 0;

// Motion artifact estimate
static float motionStdG = 0.0f;

static void updateMotionEstimate() {
    float gx = (float)g_watchState.accX / 4096.0f;
    float gy = (float)g_watchState.accY / 4096.0f;
    float gz = (float)g_watchState.accZ / 4096.0f;
    float currentMag = sqrtf(gx * gx + gy * gy + gz * gz);

    static float baselineMag = 1.0f;
    baselineMag = 0.98f * baselineMag + 0.02f * currentMag;
    float dev = fabsf(currentMag - baselineMag);

    motionStdG = 0.90f * motionStdG + 0.10f * dev;
    g_watchState.motionArtifact = (motionStdG > PPG_MOTION_STD_G);
}

// Stage 5 Kalman filter on displayed heart rate
static float kalmanX = 0.0f;
static float kalmanP = 1.0f;

static float kalmanUpdate(float measurement) {
    kalmanP += PPG_KALMAN_Q;
    float k = kalmanP / (kalmanP + PPG_KALMAN_R);
    kalmanX += k * (measurement - kalmanX);
    kalmanP *= (1.0f - k);
    return kalmanX;
}

// Lowpass DC Estimator (cutoff ~0.2Hz at 200Hz)
static float g_dcEstIr = 0.0f;
static float g_dcEstRed = 0.0f;
static float g_prevAc = 0.0f;
static float g_peakAc = 120.0f;
static uint32_t g_samplesSinceBeat = 100;
static bool g_rising = false;

static bool detectBeatAdaptive(uint32_t ir, uint32_t red) {
    if (g_dcEstRed == 0.0f) g_dcEstRed = (float)red;
    g_dcEstRed = 0.992f * g_dcEstRed + 0.008f * (float)red;

    if (g_dcEstIr == 0.0f) g_dcEstIr = (float)ir;
    g_dcEstIr = 0.992f * g_dcEstIr + 0.008f * (float)ir;

    // Track cycle peak and valley
    if (ir < cycleMinIr) cycleMinIr = ir;
    if (ir > cycleMaxIr) cycleMaxIr = ir;
    if (red < cycleMinRed) cycleMinRed = red;
    if (red > cycleMaxRed) cycleMaxRed = red;

    // Highpass AC Signal
    float acSignal = (float)ir - g_dcEstIr;

    // Adaptive peak tracking
    g_peakAc *= 0.994f;
    if (g_peakAc < 40.0f) g_peakAc = 40.0f;
    if (acSignal > g_peakAc) {
        g_peakAc = acSignal;
    }
    float threshold = g_peakAc * 0.35f;

    g_samplesSinceBeat++;

    // Peak slope detector with 500ms (100 samples) dicrotic refractory block
    bool beatDetected = false;
    if (acSignal > g_prevAc) {
        g_rising = true;
    } else if (g_rising && acSignal < g_prevAc) {
        if (g_prevAc > threshold && g_prevAc > 40.0f && g_samplesSinceBeat >= 100) {
            beatDetected = true;
            g_samplesSinceBeat = 0;
            g_peakAc = g_prevAc;
        }
        g_rising = false;
    }
    g_prevAc = acSignal;
    return beatDetected;
}

static void resetMeasurement() {
    g_dcEstIr = 0.0f;
    g_dcEstRed = 0.0f;
    g_prevAc = 0.0f;
    g_peakAc = 120.0f;
    g_samplesSinceBeat = 100;
    g_rising = false;
    rateIndex = 0;
    spo2Index = 0;
    lastBeatMs = 0;
    sampleIndex = 0;
    lastBeatSample = 0;
    gateRejections = 0;
    kalmanX = 0.0f;
    kalmanP = 1.0f;
    cycleMinIr = 0xFFFFFFFF; cycleMaxIr = 0;
    cycleMinRed = 0xFFFFFFFF; cycleMaxRed = 0;
    resetRRAnalysis();
    memset(rates, 0, sizeof(rates));
    memset(spo2History, 0, sizeof(spo2History));
    irMin = 0xFFFFFFFF;
    irMax = 0;
    g_watchState.heartRateBPM = 0;
    g_watchState.spo2Percent = 0;
    g_watchState.hrValid = false;
    g_watchState.spo2Valid = false;
    g_watchState.signalQuality = 0;
}

void initMAX30102Service() {
    Serial.println(" -> Initializing MAX30102 Pulse Oximeter & Heart Rate Sensor...");

    // Claim the pins before handing the bus to the library.
    //
    // MAX30105::begin() calls _i2cPort->begin() with no arguments, so Wire1
    // would come up on the core's default pins rather than the ones named in
    // app_config.h. It worked only because those defaults happen to match this
    // board; I2C2_SDA_PIN and I2C2_SCL_PIN were decorative, and editing them
    // would have moved nothing. TwoWire::begin() is idempotent on ESP32 -- the
    // library's later pinless call finds the bus already up and leaves the pin
    // assignment alone -- so binding here makes the config the single source of
    // truth without fighting the library.
    Wire1.begin(I2C2_SDA_PIN, I2C2_SCL_PIN, 400000);

    if (particleSensor.begin(Wire1, I2C_SPEED_FAST)) {
        maxDetected = true;
        particleSensor.setup(MAX30102_LED_BRIGHTNESS, 1, 2, 200, 411, 4096);
        Serial.println(" -> SUCCESS: MAX30102 Service Active (wrist PPG).");
    } else {
        maxDetected = false;
        Serial.println(" -> ERROR: MAX30102 Sensor Not Found on I2C2!");
    }
    g_watchState.hrSensorOk = maxDetected;
    resetMeasurement();
}

static unsigned long lastHealthCheck = 0;

static void checkSensorAlive() {
    if (millis() - lastHealthCheck < 3000) return;
    lastHealthCheck = millis();

    bool alive = (particleSensor.readPartID() == 0x15);
    if (!alive && maxDetected) {
        maxDetected = false;
        g_watchState.hrSensorOk = false;
        g_watchState.skinContact = false;
        resetMeasurement();
        Serial.println(" -> ERROR: MAX30102 stopped responding on I2C2. Vitals disabled.");
    } else if (alive && !maxDetected) {
        maxDetected = true;
        g_watchState.hrSensorOk = true;
        particleSensor.setup(MAX30102_LED_BRIGHTNESS, 1, 2, 200, 411, 4096);
        resetMeasurement();
        Serial.println(" -> MAX30102 back online. Vitals resumed.");
    }
}

void updateMAX30102Service() {
    checkSensorAlive();
    if (!maxDetected) return;

    updateMotionEstimate();
    particleSensor.check();

    while (particleSensor.available()) {
        uint32_t ir  = particleSensor.getFIFOIR();
        uint32_t red = particleSensor.getFIFORed();
        particleSensor.nextSample();

        // Counts every sample the FIFO produced, including the ones dropped
        // below as out-of-contact. It used to be incremented after the contact
        // check, which made it a count of *processed* samples while
        // g_samplesSinceBeat inside detectBeatAdaptive counted *delivered*
        // ones -- two clocks running at different rates.
        //
        // RR intervals are measured in this unit and converted with a fixed
        // 5 ms per sample, so any sample the counter skipped shortened the
        // measured interval without shortening the real one, and a shortened
        // interval reads as a faster heart. The 45-180 gate and the median
        // filter hid the small cases, which is what made it worth fixing: a
        // systematic bias toward high BPM that never looks like an error.
        //
        // The sensor free-runs at a fixed 200 Hz regardless of whether a
        // finger is present, so counting the dropped samples is what makes
        // the fixed-period conversion true.
        sampleIndex++;

        g_watchState.irRaw = ir;
        g_watchState.redRaw = red;

        // Contact detection with 1.0s release debounce
        if (ir < (g_watchState.skinContact ? PPG_CONTACT_IR_RELEASE : PPG_CONTACT_IR_THRESHOLD)) {
            if (g_watchState.skinContact) {
                if (++contactGapSamples >= PPG_CONTACT_GAP_SAMPLES) {
                    g_watchState.skinContact = false;
                    contactGapSamples = 0;
                    resetMeasurement();
                }
            }
            continue;
        }

        if (!g_watchState.skinContact) {
            g_dcEstIr = (float)ir;
            g_dcEstRed = (float)red;
            g_peakAc = 120.0f;
            g_samplesSinceBeat = 100;
            cycleMinIr = ir; cycleMaxIr = ir;
            cycleMinRed = red; cycleMaxRed = red;
            // The quality figure belongs to the session that just ended, and
            // every other line of this block is here to discard exactly that.
            // Leaving it out meant a finger returning to the sensor found the
            // whole DSP chain restarted from nothing while the quality bar
            // still showed what the previous finger scored.
            g_watchState.signalQuality = 0;
        }
        g_watchState.skinContact = true;
        contactGapSamples = 0;

        if (ir < irMin) irMin = ir;
        if (ir > irMax) irMax = ir;

        // Validated Beat Detection
        if (!g_watchState.motionArtifact && detectBeatAdaptive(ir, red)) {
            unsigned long now = millis();

            if (lastBeatSample == 0) {
                // Beat 1: Record anchor timestamp
                lastBeatSample = sampleIndex;
                // No quality is asserted here. Perfusion is measured over a
                // completed cardiac cycle, and this is the beat that starts the
                // first one -- there is nothing yet to measure. This used to
                // claim 90, which put the highest confidence figure the device
                // can report at the moment it knows least.
                g_watchState.signalQuality = 0;
                g_watchState.spo2Valid = false;
            } else {
                // Beat 2+: Compute true interval
                uint32_t deltaSamples = sampleIndex - lastBeatSample;
                float deltaMs = (float)deltaSamples * PPG_SAMPLE_PERIOD_MS;
                float bpm = 60000.0f / deltaMs;

                if (bpm >= 45.0f && bpm <= 180.0f) {
                    bool isSpike = g_watchState.hrValid &&
                                   fabsf(bpm - g_watchState.heartRateBPM) > PPG_MAX_BPM_STEP;

                    if (isSpike && gateRejections >= PPG_GATE_ESCAPE_BEATS) {
                        memset(rates, 0, sizeof(rates));
                        rateIndex = 0;
                        g_watchState.heartRateBPM = (uint16_t)bpm;
                        kalmanX = bpm;
                        kalmanP = 1.0f;
                        isSpike = false;
                    }

                    if (isSpike) {
                        gateRejections++;
                    } else {
                        gateRejections = 0;
                        pushRRInterval(deltaMs);

                        rates[rateIndex] = (uint8_t)(bpm + 0.5f);
                        rateIndex = (rateIndex + 1) % RATE_SIZE;

                        // 5-Point Median Filter for Heart Rate
                        uint8_t tempRates[RATE_SIZE];
                        uint8_t validRates = 0;
                        for (uint8_t i = 0; i < RATE_SIZE; i++) {
                            if (rates[i] > 0) tempRates[validRates++] = rates[i];
                        }

                        if (validRates >= 1) {
                            for (uint8_t i = 0; i < validRates - 1; i++) {
                                for (uint8_t j = i + 1; j < validRates; j++) {
                                    if (tempRates[i] > tempRates[j]) {
                                        uint8_t t = tempRates[i]; tempRates[i] = tempRates[j]; tempRates[j] = t;
                                    }
                                }
                            }
                            float medianBpm = (float)tempRates[validRates / 2];
                            if (kalmanX == 0.0f) {
                                kalmanX = medianBpm;
                                kalmanP = 1.0f;
                            }
                            float smoothedBpm = kalmanUpdate(medianBpm);
                            g_watchState.heartRateBPM = (uint16_t)(smoothedBpm + 0.5f);
                            g_watchState.hrValid = true;

                            g_watchState.heartRateHistory[g_watchState.historyIndex] = g_watchState.heartRateBPM;
                            g_watchState.historyIndex = (g_watchState.historyIndex + 1) % 30;

                            // Clinical True Peak-to-Peak SpO2 Extraction
                            if (g_dcEstRed > 1000.0f && g_dcEstIr > 1000.0f && cycleMaxIr > cycleMinIr && cycleMaxRed > cycleMinRed) {
                                float acRedPp = (float)(cycleMaxRed - cycleMinRed);
                                float acIrPp  = (float)(cycleMaxIr - cycleMinIr);

                                if (acIrPp > 25.0f && acRedPp > 20.0f) {
                                    float rRatio = (acRedPp / g_dcEstRed) / (acIrPp / g_dcEstIr);
                                    float instantSpo2 = 104.0f - 17.0f * rRatio;

                                    // Out of range means discard, not substitute.
                                    //
                                    // This was `if (instantSpo2 < 80) instantSpo2 = 92`,
                                    // labelled "bounds". It is not a bound -- a bound
                                    // on 80 is 80. 92 is a value invented for a
                                    // measurement that failed, and it sits two points
                                    // above VITAL_SPO2_LOW, so a computation returning
                                    // 60 -- whether from a noisy waveform or from real
                                    // hypoxia -- reached the wearer as a reassuring
                                    // "92%" and reached updateVitalMonitor() as a
                                    // number too high to alert on. Of all the
                                    // directions this device can be wrong, silently
                                    // reporting a healthy oxygen level is the worst
                                    // one available.
                                    //
                                    // Clamping to 80 instead would be honest about the
                                    // arithmetic but still dishonest about the
                                    // measurement: below 80 from wrist PPG is
                                    // overwhelmingly a bad waveform rather than a
                                    // dying wearer, and either way the one true
                                    // statement is that this beat produced nothing
                                    // usable. So the sample is dropped and the
                                    // previous median stands, which is the same thing
                                    // that already happens when the AC amplitude is
                                    // too small to work with a few lines above.
                                    if (instantSpo2 > 100.0f) instantSpo2 = 100.0f;

                                    // Only the SpO2 update is skipped. The beat
                                    // itself was fine and the heart rate derived
                                    // from it is already committed above.
                                    if (instantSpo2 >= 80.0f) {
                                        spo2History[spo2Index] = (uint8_t)(instantSpo2 + 0.5f);
                                        spo2Index = (spo2Index + 1) % SPO2_SIZE;

                                        // Median Filter on SpO2 History
                                        uint8_t tempSpo2[SPO2_SIZE];
                                        uint8_t validSpo2 = 0;
                                        for (uint8_t s = 0; s < SPO2_SIZE; s++) {
                                            if (spo2History[s] > 0) tempSpo2[validSpo2++] = spo2History[s];
                                        }
                                        if (validSpo2 >= 1) {
                                            for (uint8_t i = 0; i < validSpo2 - 1; i++) {
                                                for (uint8_t j = i + 1; j < validSpo2; j++) {
                                                    if (tempSpo2[i] > tempSpo2[j]) {
                                                        uint8_t t = tempSpo2[i]; tempSpo2[i] = tempSpo2[j]; tempSpo2[j] = t;
                                                    }
                                                }
                                            }
                                            g_watchState.spo2Percent = tempSpo2[validSpo2 / 2];
                                            g_watchState.spo2Valid = true;
                                        }
                                    }
                                }
                            }

                            // Signal Quality (SQI) from Perfusion Index.
                            //
                            // The floor used to be `if (sqi < 40) sqi = 75`, which
                            // inverted the scale over its lower half: weak perfusion
                            // -- the state where the waveform is least trustworthy --
                            // reported *better* quality than moderate perfusion, and
                            // 75 meant either "genuinely good" or "too bad to
                            // measure" with no way to tell which.
                            //
                            // That mattered beyond the progress bar. PPG_MIN_SQI
                            // gates the heart-rate threshold alerts in
                            // vital_monitor.cpp, and it is currently 1 with a comment
                            // urging whoever reads real values to raise it. Doing so
                            // would have made the gate run backwards: every scrap of
                            // noise entering at 75 while honest mid-perfusion beats
                            // at 40-74 were rejected. The floor had to go before that
                            // number can be tuned against anything.
                            //
                            // Now monotonic and clamped only at the top. Expect the
                            // displayed figure to drop -- that is the real scale
                            // becoming visible, not a regression.
                            float pi = ((float)(cycleMaxIr - cycleMinIr) / g_dcEstIr) * 100.0f;
                            int sqi = (int)(pi * 50.0f);
                            if (sqi > 100) sqi = 100;
                            if (sqi < 0) sqi = 0;
                            g_watchState.signalQuality = (uint8_t)sqi;
                        }
                    }
                }
            }

            // Reset cardiac cycle min/max for next pulse wave
            cycleMinIr = ir; cycleMaxIr = ir;
            cycleMinRed = red; cycleMaxRed = red;
            lastBeatSample = sampleIndex;
            lastBeatMs = now;
        }
    }

    // Staleness guard: a number nobody has confirmed for 3.5 s stops being a
    // reading and becomes a memory.
    //
    // This used to require skinContact, which made it unreachable in the one
    // case it most needed to cover. When the wrist leaves the sensor,
    // skinContact goes false and the guard stops running, so the last BPM sat
    // on the screen indefinitely. resetMeasurement() in the contact-loss branch
    // was the only thing clearing it, and that needs 200 consecutive
    // sub-threshold samples -- so an IR level hovering around the threshold,
    // which is exactly what a wrist shifting under a strap produces, kept
    // resetting the counter and never got there. The wearer saw a stale heart
    // rate from a sensor touching nothing.
    //
    // Dropping the skinContact term makes elapsed time alone the test, which is
    // what "stale" means. It runs in both states now.
    if (lastBeatMs > 0 && (millis() - lastBeatMs >= 3500)) {
        g_watchState.hrValid = false;
        g_watchState.spo2Valid = false;
        // Quality expires with the reading it describes. Without this the log
        // showed the halfway state the guard exists to prevent: "Pulse: -- |
        // Quality: 21%", a confidence figure for a measurement that had already
        // been withdrawn. It is computed per cardiac cycle, so once beats stop
        // arriving there is nothing left keeping it current.
        g_watchState.signalQuality = 0;
    }
}
