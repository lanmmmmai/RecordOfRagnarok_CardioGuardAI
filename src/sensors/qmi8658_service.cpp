#include "qmi8658_service.h"
#include <string.h>

static bool qmiDetected = false;

static uint8_t readRegQMI(uint8_t reg) {
    Wire.beginTransmission(QMI8658_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)QMI8658_I2C_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0x00;
}

static void writeRegQMI(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(QMI8658_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

void initQMI8658Service() {
    uint8_t id = readRegQMI(0x00); // WHO_AM_I is 0x00
    if (id == 0x05) {
        qmiDetected = true;
        // CTRL1 bit 6 (0x40) = address auto-increment, bit 5 (0x20) = big-endian.
        // Only auto-increment may be set: the burst read below assembles words
        // little-endian, so enabling big-endian would swap every sample.
        writeRegQMI(0x02, 0x40);
        // CTRL2: aFS<6:4>=010 -> +/-8g (32768/8 = 4096 LSB/g, see
        // FALL_ACCEL_LSB_PER_G). aODR<3:0>=0101 -> 235 Hz in 6DOF mode.
        //
        // The rate matters more than it looks. Datasheet note 13: with both
        // sensors enabled the ODR comes from the gyroscope, so CTRL3 below is
        // what actually sets the pace and the two must agree. This used to be
        // 0x23 (aODR=0011) with a comment claiming 250 Hz; the real figure for
        // that code is 940 Hz, so the sensor was producing 19 samples for every
        // one the 20 ms loop read.
        //
        // 235 Hz is chosen, not maximal, because a wrist cannot move faster
        // than its own mechanics: arm motion lives below ~20 Hz and an impact
        // spreads to roughly 50-100 Hz, all of it inside Nyquist here. The
        // extra 700 Hz was mostly case resonance and noise, bought with 27% of
        // every loop tick spent on I2C.
        writeRegQMI(0x03, 0x25); // CTRL2: Accel +/-8g, 235 Hz
        writeRegQMI(0x04, 0x55); // CTRL3: Gyro +/-512 dps, 235 Hz
        writeRegQMI(0x08, 0x03); // CTRL7: Enable Accel & Gyro
        Serial.println(" -> SUCCESS: QMI8658 IMU Service Active.");
    } else {
        qmiDetected = false;
        Serial.printf(" -> ERROR: QMI8658 IMU Not Found! WHO_AM_I=0x%02X\n", id);
    }
    g_watchState.imuOk = qmiDetected;
}

// One 12-byte burst from the output registers: accel XYZ then gyro XYZ.
// Returns false on a bus error, leaving the caller's buffer untouched.
static bool readSample(int16_t out[6]) {
    Wire.beginTransmission(QMI8658_I2C_ADDR);
    Wire.write(0x35); // Accel X LSB register
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom((uint8_t)QMI8658_I2C_ADDR, (uint8_t)12);
    if (Wire.available() < 12) return false;

    // Buffer the bytes first. Writing `Wire.read() | (Wire.read() << 8)` leaves
    // the two calls unsequenced, so the compiler may pair the bytes either way.
    uint8_t raw[12];
    for (int i = 0; i < 12; i++) raw[i] = Wire.read();

    for (int i = 0; i < 6; i++) {
        out[i] = (int16_t)((uint16_t)raw[i * 2] | ((uint16_t)raw[i * 2 + 1] << 8));
    }
    return true;
}

// Squared magnitude in raw LSB units. Squared, so no sqrtf per sample, and
// int32 rather than float: three axes at full scale reach 3 * 32767^2, which
// fits with room to spare and compares exactly.
static inline uint32_t magSquared(const int16_t s[6]) {
    int32_t x = s[0], y = s[1], z = s[2];
    return (uint32_t)(x * x) + (uint32_t)(y * y) + (uint32_t)(z * z);
}

// Running peak across the current tick, written by pollQMI8658Service() and
// consumed -- then cleared -- by updateQMI8658Service().
static int16_t peakSample[6];
static uint32_t peakMag = 0;
static bool havePeak = false;
static uint32_t lastPollMicros = 0;
static bool busError = false;

void pollQMI8658Service() {
    if (!qmiDetected) return;

    // Sample on the sensor's schedule, not the loop's, and never block waiting
    // for it. Each call asks one question -- has a new sensor sample had time
    // to arrive? -- and returns immediately if not.
    //
    // The previous version answered the same need with delayMicroseconds():
    // four reads spaced 4300 us apart, all inside one tick. That spacing was
    // right, but the waiting was not. delayMicroseconds() does not yield, so
    // 12.9 ms of every tick was the CPU standing still, and the tick stretched
    // to a measured 48 ms. The reads then covered 12.9 ms out of 48 and the
    // detector was blind for the other 35 -- a wider gap than the plain 20 ms
    // it was introduced to close.
    //
    // Comparing against micros() instead costs a subtraction. The loop keeps
    // running, the IMU still gets read every 4.3 ms, and the peak-holding that
    // motivated the original change is preserved exactly.
    uint32_t nowUs = micros();
    // Unsigned arithmetic, so the 71-minute micros() rollover subtracts
    // correctly without a special case.
    if (havePeak && (nowUs - lastPollMicros) < QMI_SAMPLE_GAP_US) return;
    lastPollMicros = nowUs;

    int16_t s[6];
    if (!readSample(s)) {
        busError = true;
        return;
    }
    busError = false;

    // Keep the largest sample of the tick. A wrist hitting the floor peaks for
    // only a few milliseconds; whichever single reading happened to line up
    // with the tick boundary would be a random point on the slope, always low
    // by an amount that changed every time. Thresholds calibrated against that
    // are calibrated against noise.
    //
    // Magnitude decides the winner and the whole 6-axis sample travels
    // together -- pairing the accelerometer from one instant with the gyroscope
    // from another would describe a motion that never happened.
    uint32_t m = magSquared(s);
    if (!havePeak || m > peakMag) {
        peakMag = m;
        memcpy(peakSample, s, sizeof(peakSample));
        havePeak = true;
    }
}

void updateQMI8658Service() {
    if (!qmiDetected) return;

    // Nothing arrived this tick. Only a bus error is an outage: an empty tick
    // right after boot, before the first poll, is not.
    if (!havePeak) {
        if (busError) g_watchState.imuOk = false;
        return;
    }

    g_watchState.accX  = peakSample[0];
    g_watchState.accY  = peakSample[1];
    g_watchState.accZ  = peakSample[2];
    g_watchState.gyroX = peakSample[3];
    g_watchState.gyroY = peakSample[4];
    g_watchState.gyroZ = peakSample[5];

    g_watchState.imuOk = true;

    // Start a fresh peak for the next tick. Without this the tick would report
    // the largest sample since boot forever after the first impact.
    havePeak = false;
    peakMag = 0;
}
