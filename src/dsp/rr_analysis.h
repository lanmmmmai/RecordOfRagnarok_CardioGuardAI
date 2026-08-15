#ifndef RR_ANALYSIS_H
#define RR_ANALYSIS_H

#include <Arduino.h>

// Heart-rate variability from beat-to-beat (RR) intervals.
//
// This lives apart from max30102_service.cpp on purpose: that file already
// reads the FIFO, detects beats and runs SpO2, and HRV is a fourth concern
// with its own state and its own window length.
//
// The features here are the input to the rhythm-screening model (Giai đoạn 7).
// They are meaningful only because an RR interval is the same physiological
// quantity whether it was measured electrically or optically -- which is what
// makes it legitimate to train on ECG records and infer on PPG.

// One minute at 180 BPM is 180 intervals. 200 leaves headroom.
#define RR_BUFFER_SIZE   200

// Below this many intervals every HRV statistic is noise rather than
// measurement. 30 intervals is roughly 30 seconds at a resting rate, the
// shortest window the literature treats as meaningful.
#define RR_MIN_INTERVALS 30

void initRRAnalysis();
void resetRRAnalysis();

// Feed one filtered beat-to-beat interval, in milliseconds. Only pass intervals
// that already survived the tier-4 gate: a missed beat produces a doubled
// interval that looks exactly like atrial fibrillation.
void pushRRInterval(float ms);

// Returns false until RR_MIN_INTERVALS intervals have accumulated.
bool getRRFeatures(float* rmssd, float* pnn50, float* entropy);

// How many intervals are currently held. For logging and for the model's own
// readiness check.
uint16_t getRRCount();

#endif  // RR_ANALYSIS_H
