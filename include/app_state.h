#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>

// Fall Detection State Machine States
enum FallState {
    FALL_STATE_NORMAL = 0,
    FALL_STATE_SUSPECTED,
    FALL_STATE_ALERT,
    FALL_STATE_SENDING,
    FALL_STATE_SENT,
    FALL_STATE_FAILED
};

// UI Screen Enumeration
enum UIScreen {
    SCREEN_HOME = 0,
    SCREEN_HEART_RATE,
    SCREEN_SPO2,
    SCREEN_FALL_MONITOR,
    SCREEN_FALL_ALERT,
    SCREEN_NOTIFICATION,
    SCREEN_QUICK_MENU
};

// Central WatchState Model (Section 16 UI Spec)
struct WatchState {
    // System & Power
    uint8_t batteryPercent;
    bool isCharging;
    float batteryVoltage;

    // Sensor health. These must reflect live status, not just boot-time probe
    // results: a hand-soldered module can lose contact at any moment, and a
    // watch that keeps showing "FALL: OK" after the IMU dies is worse than
    // one that shows nothing.
    bool imuOk;
    bool hrSensorOk;
    bool touchOk;

    // Connectivity
    bool wifiConnected;
    String wifiStatusStr;
    String ipAddress;
    int32_t rssi;
    bool bleConnected;
    uint8_t queuedAlerts;

    // Time & Date (RTC)
    bool timeSynced;
    int year, month, day, dayOfWeek;
    int hour, minute, second;
    uint32_t uptimeSec;

    // Health Sensors (MAX30102, worn against the underside of the wrist)
    bool skinContact;
    uint16_t heartRateBPM;
    uint8_t spo2Percent;
    bool hrValid;
    bool spo2Valid;
    uint8_t signalQuality;  // 0-100, AC amplitude relative to DC baseline
    bool motionArtifact;    // IMU says the arm is moving too much to trust PPG
    uint32_t redRaw;
    uint32_t irRaw;
    uint16_t heartRateHistory[30]; // 30-sample trend chart buffer
    uint8_t historyIndex;

    // Fall Detection & IMU 6-Axis Data (QMI8658)
    int16_t accX, accY, accZ;
    int16_t gyroX, gyroY, gyroZ;
    bool fallMonitoringActive;
    bool fallDetected;
    FallState fallState;
    uint8_t countdownSec; // 15-second countdown timer
    unsigned long fallTimestamp;

    // Touch & Gesture Input
    bool touched;
    int touchX, touchY;
    uint8_t gestureID;
    String gestureName;

    // Active UI Screen
    UIScreen currentScreen;
    UIScreen previousScreen;
    bool screenNeedsFullRedraw;

    // Notifications
    bool notificationPending;
    String lastNotificationMsg;
    String lastNotificationTime;

    // Constructor with safe defaults
    WatchState() {
        batteryPercent = 100;
        isCharging = false;
        batteryVoltage = 4.2f;

        imuOk = false;
        hrSensorOk = false;
        touchOk = false;

        wifiConnected = false;
        wifiStatusStr = "CONNECTING";
        ipAddress = "0.0.0.0";
        rssi = 0;
        bleConnected = false;
        queuedAlerts = 0;

        timeSynced = false;
        year = 2026; month = 7; day = 13; dayOfWeek = 4;
        hour = 10; minute = 30; second = 0;
        uptimeSec = 0;

        // No plausible-looking placeholder vitals: an unmeasured reading must
        // read as unmeasured, never as a healthy number.
        skinContact = false;
        heartRateBPM = 0;
        spo2Percent = 0;
        hrValid = false;
        spo2Valid = false;
        signalQuality = 0;
        motionArtifact = false;
        redRaw = 0; irRaw = 0;
        memset(heartRateHistory, 0, sizeof(heartRateHistory));
        historyIndex = 0;

        fallMonitoringActive = true;
        fallDetected = false;
        fallState = FALL_STATE_NORMAL;
        countdownSec = 15;
        fallTimestamp = 0;

        touched = false;
        touchX = 0; touchY = 0;
        gestureID = 0;
        gestureName = "NONE";

        currentScreen = SCREEN_HOME;
        previousScreen = SCREEN_HOME;
        screenNeedsFullRedraw = true;

        notificationPending = false;
        lastNotificationMsg = "Hệ thống sẵn sàng";
        lastNotificationTime = "--:--";
    }
};

extern WatchState g_watchState;

#endif // APP_STATE_H
