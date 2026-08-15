#include "rr_analysis.h"

#include <math.h>
#include <string.h>

// Sliding window, compacted by half when full rather than wrapped.
//
// A wrapping ring buffer would be cheaper, but RMSSD is built from the
// difference between ADJACENT intervals -- so the wrap point would produce one
// bogus difference between the newest interval and the oldest one, every time
// around. Compacting keeps the array in true chronological order at the cost of
// one memmove per hundred beats.
static float    buf[RR_BUFFER_SIZE];
static uint16_t count = 0;

void initRRAnalysis()  { count = 0; }
void resetRRAnalysis() { count = 0; }

uint16_t getRRCount() { return count; }

void pushRRInterval(float ms) {
    // Same physiological range as the tier-3 filter: 333 ms is 180 BPM, 1333 ms
    // is 45 BPM. Anything outside is a missed or spurious beat, not data. Kept
    // deliberately in step with the gate in max30102_service.cpp -- two filters
    // that disagree about what counts as a heartbeat would be worse than one.
    if (ms < 333.0f || ms > 1333.0f) return;

    if (count >= RR_BUFFER_SIZE) {
        memmove(buf, buf + RR_BUFFER_SIZE / 2,
                (RR_BUFFER_SIZE / 2) * sizeof(float));
        count = RR_BUFFER_SIZE / 2;
    }
    buf[count++] = ms;
}

// Shannon entropy over a 16-bin histogram of the intervals.
//
// A sinus rhythm concentrates its intervals in a narrow band, so the histogram
// is peaked and entropy is low. Atrial fibrillation scatters them, so the
// histogram flattens and entropy rises. This is the second-strongest
// discriminator after RMSSD, and it captures something RMSSD misses: RMSSD sees
// beat-to-beat jumps, entropy sees the shape of the whole distribution.
static float shannonEntropy() {
    const uint8_t BINS = 16;
    uint16_t hist[BINS] = {0};

    float lo = buf[0], hi = buf[0];
    for (uint16_t i = 1; i < count; i++) {
        if (buf[i] < lo) lo = buf[i];
        if (buf[i] > hi) hi = buf[i];
    }
    // A perfectly regular rhythm carries no information to measure, and would
    // divide by zero below.
    if (hi - lo < 1.0f) return 0.0f;

    for (uint16_t i = 0; i < count; i++) {
        uint8_t b = (uint8_t)((buf[i] - lo) / (hi - lo) * (float)(BINS - 1));
        hist[b]++;
    }

    float h = 0.0f;
    for (uint8_t b = 0; b < BINS; b++) {
        if (hist[b] == 0) continue;  // 0 * log(0) is 0, but log(0) is not
        float p = (float)hist[b] / (float)count;
        h -= p * log2f(p);
    }
    return h;
}

bool getRRFeatures(float* rmssd, float* pnn50, float* entropy) {
    if (count < RR_MIN_INTERVALS) return false;

    // RMSSD: root mean square of successive differences. The standard
    // short-window HRV measure, and the one most sensitive to fibrillation.
    //
    // pNN50: share of successive pairs differing by more than 50 ms. Coarser
    // than RMSSD but far less affected by one outlying interval, so the two
    // disagreeing is itself informative.
    float sumSq = 0.0f;
    uint16_t nn50 = 0;
    for (uint16_t i = 1; i < count; i++) {
        float d = buf[i] - buf[i - 1];
        sumSq += d * d;
        if (fabsf(d) > 50.0f) nn50++;
    }

    *rmssd   = sqrtf(sumSq / (float)(count - 1));
    *pnn50   = 100.0f * (float)nn50 / (float)(count - 1);
    *entropy = shannonEntropy();
    return true;
}
