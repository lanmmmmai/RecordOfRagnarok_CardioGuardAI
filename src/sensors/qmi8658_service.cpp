#include "qmi8658_service.h"

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
        writeRegQMI(0x03, 0x23); // CTRL2: Accel 8g, 250Hz -> 4096 LSB/g
        writeRegQMI(0x04, 0x53); // CTRL3: Gyro 512dps, 250Hz
        writeRegQMI(0x08, 0x03); // CTRL7: Enable Accel & Gyro
        Serial.println(" -> SUCCESS: QMI8658 IMU Service Active.");
    } else {
        qmiDetected = false;
        Serial.printf(" -> ERROR: QMI8658 IMU Not Found! WHO_AM_I=0x%02X\n", id);
    }
    g_watchState.imuOk = qmiDetected;
}

void updateQMI8658Service() {
    if (!qmiDetected) return;

    Wire.beginTransmission(QMI8658_I2C_ADDR);
    Wire.write(0x35); // Accel X LSB register
    if (Wire.endTransmission(false) != 0) {
        g_watchState.imuOk = false;
        return;
    }
    Wire.requestFrom((uint8_t)QMI8658_I2C_ADDR, (uint8_t)12);

    if (Wire.available() < 12) {
        g_watchState.imuOk = false;
        return;
    }

    // Buffer the bytes first. Writing `Wire.read() | (Wire.read() << 8)` leaves
    // the two calls unsequenced, so the compiler may pair the bytes either way.
    uint8_t raw[12];
    for (int i = 0; i < 12; i++) raw[i] = Wire.read();

    g_watchState.accX  = (int16_t)((uint16_t)raw[0]  | ((uint16_t)raw[1]  << 8));
    g_watchState.accY  = (int16_t)((uint16_t)raw[2]  | ((uint16_t)raw[3]  << 8));
    g_watchState.accZ  = (int16_t)((uint16_t)raw[4]  | ((uint16_t)raw[5]  << 8));
    g_watchState.gyroX = (int16_t)((uint16_t)raw[6]  | ((uint16_t)raw[7]  << 8));
    g_watchState.gyroY = (int16_t)((uint16_t)raw[8]  | ((uint16_t)raw[9]  << 8));
    g_watchState.gyroZ = (int16_t)((uint16_t)raw[10] | ((uint16_t)raw[11] << 8));

    g_watchState.imuOk = true;
}
