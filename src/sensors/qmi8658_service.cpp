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

void updateQMI8658Service() {
    if (!qmiDetected) return;

    // Take several samples per tick and keep the strongest, instead of whatever
    // single reading happened to line up with the 20 ms loop.
    //
    // A wrist hitting the floor peaks for only a few milliseconds. Sampling
    // once every 20 ms, the odds of landing on that peak are poor, so the
    // recorded impact was a point somewhere on the slope -- always lower than
    // the truth, and lower by an amount that changed every time. Thresholds
    // calibrated against that are calibrated against noise.
    //
    // Keeping the largest of QMI_SAMPLES_PER_TICK readings bounds the error:
    // the peak can still be missed, but only by the gap between samples rather
    // than the whole 20 ms window. Magnitude decides which sample wins, and the
    // whole 6-axis sample is kept together -- mixing the accelerometer from one
    // instant with the gyroscope from another would describe a motion that
    // never happened.
    int16_t best[6];
    uint32_t bestMag = 0;
    bool haveAny = false;

    for (uint8_t i = 0; i < QMI_SAMPLES_PER_TICK; i++) {
        int16_t s[6];
        if (!readSample(s)) continue;

        uint32_t m = magSquared(s);
        if (!haveAny || m > bestMag) {
            bestMag = m;
            memcpy(best, s, sizeof(best));
            haveAny = true;
        }

        // Space the reads out so they fall on different sensor samples. At
        // 235 Hz a new one arrives every 4.3 ms; without this the burst would
        // finish inside a single sensor period and re-read one value.
        if (i + 1 < QMI_SAMPLES_PER_TICK) delayMicroseconds(QMI_SAMPLE_GAP_US);
    }

    if (!haveAny) {
        g_watchState.imuOk = false;
        return;
    }

    g_watchState.accX  = best[0];
    g_watchState.accY  = best[1];
    g_watchState.accZ  = best[2];
    g_watchState.gyroX = best[3];
    g_watchState.gyroY = best[4];
    g_watchState.gyroZ = best[5];

    g_watchState.imuOk = true;
}
