#include "wifi_manager.h"

static unsigned long lastWifiAttempt = 0;

void syncNTPTimeService() {
    if (g_watchState.wifiConnected && !g_watchState.timeSynced) {
        configTime(GMT_OFFSET_SECONDS, DAYLIGHT_OFFSET_SECONDS, NTP_SERVER_PRIMARY);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 1000)) {
            g_watchState.timeSynced = true;
            Serial.println(" -> SUCCESS: Real-Time Clock Synchronized via NTP.");
        }
    }
}

void initWiFiManager() {
    Serial.printf("\n -> Connecting to Wi-Fi SSID: '%s' (in background)...\n", WIFI_SSID_DEFAULT);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);

    // No blocking wait here. The watch must show its face immediately; the
    // connection settles in the background and updateWiFiManager() picks it up.
    g_watchState.wifiConnected = false;
    g_watchState.wifiStatusStr = "CONNECTING";
    g_watchState.ipAddress = "No IP";
    g_watchState.rssi = 0;
    lastWifiAttempt = millis();
}

void updateWiFiManager() {
    if (WiFi.status() == WL_CONNECTED) {
        bool justConnected = !g_watchState.wifiConnected;
        g_watchState.wifiConnected = true;
        g_watchState.wifiStatusStr = "CONNECTED";
        g_watchState.ipAddress = WiFi.localIP().toString();
        g_watchState.rssi = WiFi.RSSI();
        if (justConnected) {
            Serial.printf(" -> Wi-Fi SUCCESS! IP: %s | RSSI: %d dBm\n",
                          g_watchState.ipAddress.c_str(), g_watchState.rssi);
        }
        if (!g_watchState.timeSynced) syncNTPTimeService();
    } else {
        g_watchState.wifiConnected = false;
        g_watchState.wifiStatusStr = "OFFLINE";
        g_watchState.ipAddress = "No IP";
        g_watchState.rssi = 0;

        if (millis() - lastWifiAttempt >= 8000) {
            lastWifiAttempt = millis();
            Serial.printf(" [Wi-Fi] Retrying connection to '%s'...\n", WIFI_SSID_DEFAULT);
            WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
        }
    }
}
