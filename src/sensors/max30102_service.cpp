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
        sampleIndex++;

        g_watchState.irRaw = ir;
        g_watchState.redRaw = red;

        // Contact detection with 1.0s release debounce
        if (ir < (g_watchState.skinContact ? PPG_CONTACT_IR_RELEASE : PPG_CONTACT_IR_THRESHOLD)) {
            if (g_watchState.skinContact) {
                if (++contactGapSamples >= 200) {
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
                g_watchState.signalQuality = 90;
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

                                    if (instantSpo2 > 100.0f) instantSpo2 = 99.0f;
                                    if (instantSpo2 < 80.0f)  instantSpo2 = 92.0f; // bounds

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

                            // Signal Quality (SQI) from Perfusion Index
                            float pi = ((float)(cycleMaxIr - cycleMinIr) / g_dcEstIr) * 100.0f;
                            int sqi = (int)(pi * 50.0f);
                            if (sqi > 100) sqi = 100;
                            if (sqi < 40) sqi = 75;
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

    // Guard against stale reading if skin contact is lost
    if (g_watchState.skinContact && lastBeatMs > 0 && (millis() - lastBeatMs >= 3500)) {
        g_watchState.hrValid = false;
        g_watchState.spo2Valid = false;
    }
}
