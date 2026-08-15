#include "websocket_service.h"
#include "fall_detection/fall_detector.h"
#include <WebSocketsServer.h>
#include <WiFi.h>

static WebSocketsServer webSocket = WebSocketsServer(8080);
static bool serverStarted = false;
static unsigned long lastBroadcast = 0;

static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf(" [WebSocket] Client #%u Disconnected.\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf(" [WebSocket] Client #%u Connected from %s\n", num, ip.toString().c_str());
            break;
        }
        case WStype_TEXT: {
            String text = String((char*)payload);
            if (text.indexOf("cancel_sos") >= 0) {
                Serial.println(" [WebSocket] Command: CANCEL SOS received from Web.");
                cancelFallAlert();
            } else if (text.indexOf("trigger_sos") >= 0) {
                Serial.println(" [WebSocket] Command: TRIGGER SOS received from Web.");
                triggerSimulatedFall();
            }
            break;
        }
        default:
            break;
    }
}

void initWebSocketService() {
    serverStarted = false;
}

void updateWebSocketService() {
    if (!g_watchState.wifiConnected) return;

    if (!serverStarted) {
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
        serverStarted = true;
        Serial.printf(" -> SUCCESS: WebSocket Server Active on port 8080 (ws://%s:8080).\n",
                      g_watchState.ipAddress.c_str());
    }

    webSocket.loop();

    // Broadcast comprehensive telemetry JSON at 10 Hz (every 100ms) or immediately on touch event
    bool forceSend = g_watchState.touched;
    if (forceSend || (millis() - lastBroadcast >= 100)) {
        lastBroadcast = millis();

        if (webSocket.connectedClients() > 0) {
            char jsonBuf[640];
            snprintf(jsonBuf, sizeof(jsonBuf),
                "{\"type\":\"telemetry\",\"pulse\":%u,\"spo2\":%u,\"quality\":%u,\"skinContact\":%s,"
                "\"hrValid\":%s,\"spo2Valid\":%s,\"motionArtifact\":%s,"
                "\"raw\":{\"ir\":%lu,\"red\":%lu},"
                "\"accel\":{\"x\":%d,\"y\":%d,\"z\":%d},\"gyro\":{\"x\":%d,\"y\":%d,\"z\":%d},"
                "\"sensors\":{\"imuOk\":%s,\"hrOk\":%s,\"touchOk\":%s},"
                "\"touch\":{\"touched\":%s,\"x\":%d,\"y\":%d,\"gesture\":\"%s\"},"
                "\"screen\":%d,"
                "\"fallState\":%d,\"countdown\":%u,\"battery\":%u,\"voltage\":%.2f,\"charging\":%s,"
                "\"rssi\":%d,\"uptime\":%lu,\"ip\":\"%s\"}",
                (unsigned int)g_watchState.heartRateBPM,
                (unsigned int)g_watchState.spo2Percent,
                (unsigned int)g_watchState.signalQuality,
                g_watchState.skinContact ? "true" : "false",
                g_watchState.hrValid ? "true" : "false",
                g_watchState.spo2Valid ? "true" : "false",
                g_watchState.motionArtifact ? "true" : "false",
                (unsigned long)g_watchState.irRaw, (unsigned long)g_watchState.redRaw,
                (int)g_watchState.accX, (int)g_watchState.accY, (int)g_watchState.accZ,
                (int)g_watchState.gyroX, (int)g_watchState.gyroY, (int)g_watchState.gyroZ,
                g_watchState.imuOk ? "true" : "false",
                g_watchState.hrSensorOk ? "true" : "false",
                g_watchState.touchOk ? "true" : "false",
                g_watchState.touched ? "true" : "false",
                (int)g_watchState.touchX, (int)g_watchState.touchY,
                g_watchState.gestureName.c_str(),
                (int)g_watchState.currentScreen,
                (int)g_watchState.fallState,
                (unsigned int)g_watchState.countdownSec,
                (unsigned int)g_watchState.batteryPercent,
                g_watchState.batteryVoltage,
                g_watchState.isCharging ? "true" : "false",
                (int)g_watchState.rssi,
                (unsigned long)g_watchState.uptimeSec,
                g_watchState.ipAddress.c_str()
            );

            webSocket.broadcastTXT(jsonBuf);
        }
    }
}
