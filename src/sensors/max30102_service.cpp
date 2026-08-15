#include "max30102_service.h"

#include <heartRate.h>
#include <spo2_algorithm.h>
#include <math.h>

#include "../dsp/rr_analysis.h"

static MAX30105 particleSensor;
static bool maxDetected = false;

// Beat-to-beat intervals, averaged over a few beats so a single mis-detection
// does not swing the displayed value.
static const uint8_t RATE_SIZE = 4;
static uint8_t rates[RATE_SIZE];
static uint8_t rateIndex = 0;
static unsigned long lastBeatMs = 0;

// Samples pulled from the FIFO since the last reset, and the count at which the
// previous beat landed.
//
// Beat timing used to come from millis() read at the moment the loop got around
// to the sample -- but the loop runs every 20 ms and drains several samples per
// pass, so every sample in one pass carried the SAME timestamp. That is ~20 ms
// of jitter added to each RR interval, larger than the RMSSD the interval is
// meant to reveal. The FIFO index is tied to when the SENSOR took the reading,
// which is what the interval actually describes.
static uint32_t sampleIndex = 0;
static uint32_t lastBeatSample = 0;

// 200 Hz. Keep in step with the sampleRate argument to particleSensor.setup().
#define PPG_SAMPLE_PERIOD_MS 5.0f

// Consecutive beats thrown out by the rate-of-change gate. Reaching
// PPG_GATE_ESCAPE_BEATS forces the next one through; see the gate itself.
static uint8_t gateRejections = 0;

// The Maxim SpO2 routine wants 100 samples at 25 Hz. FreqS and BUFFER_SIZE are
// #defines inside the SparkFun library, so that rate is not negotiable -- but
// the sensor now runs at 200 Hz for the sake of RR-interval resolution. The two
// are reconciled by averaging PPG_DECIMATE raw samples into each buffer slot.
static uint32_t irBuffer[BUFFER_SIZE];
static uint32_t redBuffer[BUFFER_SIZE];
static int bufferCount = 0;

// 200 Hz / 8 = 25 Hz, which is exactly FreqS.
//
// Averaging rather than taking every eighth sample: decimating by selection
// folds anything above 12.5 Hz back down into the band the SpO2 routine cares
// about. Averaging attenuates it instead.
#define PPG_DECIMATE 8
static uint8_t  decimateCount = 0;
static uint32_t irAccum = 0, redAccum = 0;

// Rolling AC/DC measurement for the signal-quality figure.
static uint32_t irMin = 0xFFFFFFFF, irMax = 0;
static unsigned long lastQualityCalc = 0;

// Counts the once-a-second quality passes, so the HRV log fires every tenth.
static uint8_t hrvLogTick = 0;

// Short history of total acceleration, used to reject samples taken while the
// arm is swinging. Wrist PPG is dominated by motion artefact otherwise, and
// this is the single most important reason wrist readings can be trusted.
static const uint8_t MOTION_WINDOW = 16;
static float motionRing[MOTION_WINDOW];
static uint8_t motionIndex = 0;

static void updateMotionEstimate() {
    float ax = g_watchState.accX / 4096.0f;
    float ay = g_watchState.accY / 4096.0f;
    float az = g_watchState.accZ / 4096.0f;
    motionRing[motionIndex] = sqrtf(ax * ax + ay * ay + az * az);
    motionIndex = (motionIndex + 1) % MOTION_WINDOW;

    float mean = 0.0f;
    for (uint8_t i = 0; i < MOTION_WINDOW; i++) mean += motionRing[i];
    mean /= MOTION_WINDOW;

    float var = 0.0f;
    for (uint8_t i = 0; i < MOTION_WINDOW; i++) {
        float d = motionRing[i] - mean;
        var += d * d;
    }
    g_watchState.motionArtifact = sqrtf(var / MOTION_WINDOW) > PPG_MOTION_STD_G;
}

static void resetMeasurement() {
    bufferCount = 0;
    decimateCount = 0;
    irAccum = redAccum = 0;
    rateIndex = 0;
    lastBeatMs = 0;
    sampleIndex = 0;
    lastBeatSample = 0;
    gateRejections = 0;
    // Drop the RR history too. It is called on every loss of skin contact, and
    // an interval spanning a gap where the finger was off the sensor is not a
    // heartbeat interval -- it would enter the window as one huge outlier and
    // drag RMSSD up for the next hundred beats.
    resetRRAnalysis();
    memset(rates, 0, sizeof(rates));
    irMin = 0xFFFFFFFF;
    irMax = 0;
    g_watchState.heartRateBPM = 0;
    g_watchState.spo2Percent = 0;
    g_watchState.hrValid = false;
    g_watchState.spo2Valid = false;
    g_watchState.signalQuality = 0;
}

void initMAX30102Service() {
    // No internal pull-ups here. The ESP32's are around 45k, far too weak for
    // I2C; the breakout module carries its own 4.7k resistors.
    Wire1.begin(I2C2_SDA_PIN, I2C2_SCL_PIN, 400000);

    for (uint8_t i = 0; i < MOTION_WINDOW; i++) motionRing[i] = 1.0f;

    if (particleSensor.begin(Wire1, I2C_SPEED_FAST, MAX30102_I2C_ADDR)) {
        maxDetected = true;
        // brightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange
        //
        // 200 Hz with NO hardware averaging (second argument 1), giving 5 ms per
        // sample. The old 100 Hz averaged by 4 worked out to 25 Hz -- 40 ms of
        // quantisation against an RMSSD of 20-50 ms, which made the measurement
        // error the same size as the quantity being measured. RR analysis was
        // impossible until this line changed. SpO2 still gets its 25 Hz via
        // PPG_DECIMATE.
        particleSensor.setup(MAX30102_LED_BRIGHTNESS, 1, 2, 200, 411, 4096);
        Serial.println(" -> SUCCESS: MAX30102 Service Active (wrist PPG).");
    } else {
        maxDetected = false;
        Serial.println(" -> ERROR: MAX30102 Sensor Not Found on I2C2 (SDA:15, SCL:16)!");
    }
    g_watchState.hrSensorOk = maxDetected;
    resetMeasurement();
}

// Re-probe the part ID periodically. A hand-wired module can lose contact at
// any moment, and the plan's own acceptance test is pulling SDA while running:
// the screen has to stop showing vitals within a few seconds, not keep the last
// number on display forever.
static unsigned long lastHealthCheck = 0;

static void checkSensorAlive() {
    if (millis() - lastHealthCheck < 3000) return;
    lastHealthCheck = millis();

    bool alive = (particleSensor.readPartID() == 0x15);  // MAX30102 part ID

    if (!alive && maxDetected) {
        maxDetected = false;
        g_watchState.hrSensorOk = false;
        g_watchState.skinContact = false;
        resetMeasurement();
        Serial.println(" -> ERROR: MAX30102 stopped responding on I2C2. Vitals disabled.");
    } else if (alive && !maxDetected) {
        maxDetected = true;
        g_watchState.hrSensorOk = true;
        // Must match initMAX30102Service() exactly -- a sensor that comes back
        // at a different rate would silently corrupt every timing calculation.
        particleSensor.setup(MAX30102_LED_BRIGHTNESS, 1, 2, 200, 411, 4096);
        resetMeasurement();
        Serial.println(" -> MAX30102 back online. Vitals resumed.");
    }
}

void updateMAX30102Service() {
    checkSensorAlive();
    if (!maxDetected) return;

    updateMotionEstimate();

    // Never getIR()/getRed(): each blocks up to 250 ms waiting for a sample.
    // check() pulls whatever the FIFO holds and returns immediately.
    particleSensor.check();

    while (particleSensor.available()) {
        uint32_t ir  = particleSensor.getFIFOIR();
        uint32_t red = particleSensor.getFIFORed();
        particleSensor.nextSample();
        sampleIndex++;

        g_watchState.irRaw = ir;
        g_watchState.redRaw = red;

        if (ir < PPG_CONTACT_IR_THRESHOLD) {
            if (g_watchState.skinContact) {
                g_watchState.skinContact = false;
                resetMeasurement();
            }
            continue;
        }
        g_watchState.skinContact = true;

        if (ir < irMin) irMin = ir;
        if (ir > irMax) irMax = ir;

        // Beat detection. Skipped entirely while the arm is moving: a bad
        // reading is worse than no reading on a device someone relies on.
        if (!g_watchState.motionArtifact && checkForBeat(ir)) {
            unsigned long now = millis();
            if (lastBeatSample > 0) {
                // Interval measured in samples, not milliseconds: see
                // sampleIndex for why the clock is the wrong instrument here.
                uint32_t deltaSamples = sampleIndex - lastBeatSample;
                float deltaMs = (float)deltaSamples * PPG_SAMPLE_PERIOD_MS;
                float bpm = 60000.0f / deltaMs;
                if (bpm >= 45.0f && bpm <= 180.0f) {
                    // Reject beats that jump too far from the current reading:
                    // a mis-detected beat halves or doubles the interval, which
                    // is a much bigger step than any real heart makes.
                    //
                    // The gate measures against its own output, so it needs a
                    // way out -- see PPG_GATE_ESCAPE_BEATS. Without it the
                    // display locks on a stale value the moment the true rate
                    // moves away from it, and every subsequent beat is rejected
                    // by comparison against that same stale value.
                    bool isSpike = g_watchState.hrValid &&
                                   fabsf(bpm - g_watchState.heartRateBPM) > PPG_MAX_BPM_STEP;

                    if (isSpike && gateRejections >= PPG_GATE_ESCAPE_BEATS) {
                        // Reality has disagreed with us this many times in a
                        // row. Assume the reading is what drifted, not the
                        // heart: drop the history and re-lock around the beat
                        // we are being handed. Clearing rates[] matters --
                        // keeping it would let the median drag the value
                        // straight back to where it was stuck.
                        Serial.printf(" [PPG] Rate gate stuck at %u BPM for %u beats"
                                      " -- resyncing to %.0f BPM.\n",
                                      g_watchState.heartRateBPM,
                                      (unsigned)gateRejections, bpm);
                        memset(rates, 0, sizeof(rates));
                        rateIndex = 0;
                        // Move the reference straight away. The median below
                        // needs two samples before it will publish anything,
                        // and until it does, heartRateBPM would still hold the
                        // stuck value -- which is exactly what the next beat
                        // gets compared against. Leaving it would make the
                        // escape hatch fire over and over without ever
                        // escaping.
                        g_watchState.heartRateBPM = (uint16_t)bpm;
                        isSpike = false;
                    }

                    if (isSpike) {
                        gateRejections++;
                    } else {
                        gateRejections = 0;

                        // Only intervals that got past the gate. A missed beat
                        // doubles the interval and looks exactly like atrial
                        // fibrillation -- and that alert goes straight to the
                        // family group.
                        pushRRInterval(deltaMs);

                        rates[rateIndex] = (uint8_t)bpm;
                        rateIndex = (rateIndex + 1) % RATE_SIZE;

                        uint8_t temp[RATE_SIZE];
                        uint8_t valid = 0;
                        for (uint8_t i = 0; i < RATE_SIZE; i++) {
                            if (rates[i] > 0) { temp[valid++] = rates[i]; }
                        }
                        if (valid >= 2) {
                            // Sort for Median Filter
                            for (uint8_t i = 0; i < valid - 1; i++) {
                                for (uint8_t j = i + 1; j < valid; j++) {
                                    if (temp[i] > temp[j]) {
                                        uint8_t t = temp[i]; temp[i] = temp[j]; temp[j] = t;
                                    }
                                }
                            }
                            // True Median Filter selection from sorted temp array
                            g_watchState.heartRateBPM = temp[valid / 2];
                            g_watchState.hrValid = true;
                            g_watchState.heartRateHistory[g_watchState.historyIndex] =
                                g_watchState.heartRateBPM;
                            g_watchState.historyIndex = (g_watchState.historyIndex + 1) % 30;
                        }
                    }
                }
            }
            lastBeatSample = sampleIndex;
            // Still kept: the stale-reading guard at the end of this function
            // works in wall-clock time, not in samples.
            lastBeatMs = now;
        }

        // Fill the SpO2 window, then slide it by one second so the value
        // refreshes without discarding four seconds of good signal.
        //
        // Beat detection above sees every one of the 200 samples a second; this
        // branch only takes one slot per PPG_DECIMATE of them, because the Maxim
        // routine has 25 Hz baked into it. Feeding it 200 Hz would make it read
        // every interval as eight times longer than it is.
        irAccum  += ir;
        redAccum += red;
        if (++decimateCount < PPG_DECIMATE) continue;

        irBuffer[bufferCount]  = irAccum  / PPG_DECIMATE;
        redBuffer[bufferCount] = redAccum / PPG_DECIMATE;
        bufferCount++;
        decimateCount = 0;
        irAccum = redAccum = 0;

        if (bufferCount >= BUFFER_SIZE) {
            int32_t spo2 = 0, hr = 0;
            int8_t spo2Valid = 0, hrValidFlag = 0;
            maxim_heart_rate_and_oxygen_saturation(
                irBuffer, BUFFER_SIZE, redBuffer, &spo2, &spo2Valid, &hr, &hrValidFlag);

            // Wrist SpO2 is a reference figure at best, so it is only shown
            // when the algorithm is confident and the arm was still.
            if (spo2Valid && spo2 >= 90 && spo2 <= 100) {
                g_watchState.spo2Percent = (uint8_t)spo2;
                g_watchState.spo2Valid = true;
            } else {
                g_watchState.spo2Valid = false;
            }
            if (hrValidFlag && hr >= 45 && hr <= 180 && !g_watchState.hrValid) {
                g_watchState.heartRateBPM = (uint16_t)hr;
                g_watchState.hrValid = true;
            }


            const int slide = FreqS;  // one second
            memmove(irBuffer,  irBuffer  + slide, (BUFFER_SIZE - slide) * sizeof(uint32_t));
            memmove(redBuffer, redBuffer + slide, (BUFFER_SIZE - slide) * sizeof(uint32_t));
            bufferCount = BUFFER_SIZE - slide;
        }
    }

    // Signal quality: pulsatile amplitude against the DC baseline, recomputed
    // once a second. Roughly a perfusion index, scaled to 0-100.
    if (millis() - lastQualityCalc >= 1000) {
        lastQualityCalc = millis();
        if (g_watchState.skinContact && irMax > irMin && irMin != 0xFFFFFFFF) {
            float ac = (float)(irMax - irMin);
            float dc = (float)irMin;
            float perfusion = (dc > 0.0f) ? (ac / dc) * 100.0f : 0.0f;
            g_watchState.signalQuality = (uint8_t)constrain((int)(perfusion * 50.0f), 0, 100);
        } else {
            g_watchState.signalQuality = 0;
        }
        irMin = 0xFFFFFFFF;
        irMax = 0;

        // Stale reading guard: if no beat has landed for several seconds the
        // displayed number no longer describes the present.
        if (g_watchState.hrValid && lastBeatMs > 0 && millis() - lastBeatMs > 6000) {
            g_watchState.hrValid = false;
            g_watchState.heartRateBPM = 0;
        }

        // HRV every tenth pass through this once-a-second block. Purely for
        // observation right now -- these three numbers are the input the rhythm
        // model of Giai đoạn 7 will be trained against, so they need watching on
        // a real wrist long before anything is allowed to raise an alert.
        if (++hrvLogTick >= 10) {
            hrvLogTick = 0;
            float rmssd, pnn50, ent;
            if (getRRFeatures(&rmssd, &pnn50, &ent)) {
                Serial.printf(" [HRV] n=%u RMSSD=%.1fms pNN50=%.1f%% H=%.2f\n",
                              getRRCount(), rmssd, pnn50, ent);
            }
        }
    }
}
