#include "wifi_manager.h"

static unsigned long lastWifiAttempt = 0;

// Has configTime() been handed to the SNTP client yet? It starts the request
// and returns immediately; the answer arrives on its own task.
static bool ntpRequested = false;

void syncNTPTimeService() {
    if (!g_watchState.wifiConnected || g_watchState.timeSynced) return;

    if (!ntpRequested) {
        ntpRequested = true;
        setenv("TZ", "ICT-7", 1);
        tzset();
        configTime(7 * 3600, 0, "vn.pool.ntp.org", "pool.ntp.org", "time.google.com");
        Serial.println(" -> NTP request sent (Vietnam UTC+7 ICT-7), waiting for the clock in the background.");
        return;
    }

    // Zero timeout. The old code passed 1000 ms here, which blocks the whole
    // cooperative loop for a full second: about fifty missed IMU samples,
    // thirty dropped frames, and a third of the fall confirmation window gone
    // if the two happen to coincide. Poll instead -- this runs every five
    // seconds anyway, and the clock is set the moment SNTP answers.
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0) && timeinfo.tm_year > 120) {  // past 2020
        g_watchState.timeSynced = true;
        Serial.println(" -> SUCCESS: Real-Time Clock Synchronized via NTP.");
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

        // The SNTP request went out over a link that is now gone. Arm it again
        // so the next successful connection re-sends it; without this the watch
        // would wait forever for a reply to a request nobody received.
        if (!g_watchState.timeSynced) ntpRequested = false;

        if (millis() - lastWifiAttempt >= 8000) {
            lastWifiAttempt = millis();
            Serial.printf(" [Wi-Fi] Retrying connection to '%s'...\n", WIFI_SSID_DEFAULT);
            WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
        }
    }
}
