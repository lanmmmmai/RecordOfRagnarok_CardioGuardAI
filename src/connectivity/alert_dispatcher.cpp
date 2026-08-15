#include "alert_dispatcher.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

// TLS handshakes block for a second or more, which would freeze the countdown
// and the display at exactly the moment the wearer is looking at them. The
// send therefore runs on its own FreeRTOS task, and the main loop only reads
// the result flags below.

enum DispatchResult { DISPATCH_IDLE = 0, DISPATCH_INFLIGHT, DISPATCH_DELIVERED, DISPATCH_FAILED };

static volatile DispatchResult fallDispatch = DISPATCH_IDLE;
static bool fallQueued = false;

// millis() at which the watch entered FALL_STATE_SENT or FALL_STATE_FAILED.
// Zero means it is in neither. Drives the auto-clear that gets the watch back
// to monitoring without needing the wearer to touch it.
static unsigned long outcomeSince = 0;

static Preferences prefs;
static SemaphoreHandle_t storeMutex = nullptr;
static TaskHandle_t alertTask = nullptr;

// ---------------------------------------------------------------------------
// Persistent outbox
//
// A fall that happens while the network is down must not evaporate. Entries
// live in NVS as a ring so they survive a reboot, and are removed only after
// Telegram confirms delivery.
// ---------------------------------------------------------------------------

static void storeKey(char* buf, size_t len, uint32_t slot) {
    snprintf(buf, len, "m%lu", (unsigned long)(slot % ALERT_QUEUE_MAX));
}

// Publish the outbox depth for the UI and the BLE status packet.
//
// This is written from alertTask and read from the main loop, so it is kept to
// a single byte written in one instruction: a torn read is not possible and no
// lock is needed on the reader side. What it must NOT do is re-read NVS --
// Preferences is not documented thread-safe, and every caller already knows the
// new count, so hand it in instead of asking the flash again.
static void publishQueueDepth(uint8_t count) {
    g_watchState.queuedAlerts = count;
}

static void storePush(const String& text) {
    if (xSemaphoreTake(storeMutex, portMAX_DELAY) != pdTRUE) return;

    uint32_t head  = prefs.getUInt("head", 0);
    uint8_t  count = prefs.getUChar("count", 0);

    if (count >= ALERT_QUEUE_MAX) {
        // Outbox full. Drop the oldest so the newest event always survives.
        head = (head + 1) % ALERT_QUEUE_MAX;
        count = ALERT_QUEUE_MAX - 1;
        prefs.putUInt("head", head);
        Serial.println(" [ALERT] Outbox full, dropped oldest entry.");
    }

    char key[8];
    storeKey(key, sizeof(key), head + count);
    prefs.putString(key, text);
    prefs.putUChar("count", count + 1);
    publishQueueDepth(count + 1);

    xSemaphoreGive(storeMutex);
}

static bool storePeek(String& out) {
    bool found = false;
    if (xSemaphoreTake(storeMutex, portMAX_DELAY) != pdTRUE) return false;

    if (prefs.getUChar("count", 0) > 0) {
        char key[8];
        storeKey(key, sizeof(key), prefs.getUInt("head", 0));
        out = prefs.getString(key, "");
        found = out.length() > 0;
    }

    xSemaphoreGive(storeMutex);
    return found;
}

static void storePopFront() {
    if (xSemaphoreTake(storeMutex, portMAX_DELAY) != pdTRUE) return;

    uint8_t count = prefs.getUChar("count", 0);
    if (count > 0) {
        uint32_t head = prefs.getUInt("head", 0);
        char key[8];
        storeKey(key, sizeof(key), head);
        prefs.remove(key);
        prefs.putUInt("head", (head + 1) % ALERT_QUEUE_MAX);
        prefs.putUChar("count", count - 1);
        publishQueueDepth(count - 1);
    }

    xSemaphoreGive(storeMutex);
}

// ---------------------------------------------------------------------------
// Telegram
// ---------------------------------------------------------------------------

static String jsonEscape(const String& in) {
    String out;
    out.reserve(in.length() + 16);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': break;
            default:   out += c;      break;
        }
    }
    return out;
}

static bool sendTelegram(const String& text) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    // Certificate pinning would break silently whenever Telegram rotates its
    // CA, and a fall alert that fails closed is worse than one sent over an
    // unverified link. The payload carries no secrets beyond the bot token.
    client.setInsecure();
    client.setTimeout(ALERT_HTTP_TIMEOUT_MS / 1000);

    if (!client.connect(TELEGRAM_API_HOST, TELEGRAM_API_PORT)) {
        Serial.println(" [ALERT] TLS connect failed.");
        return false;
    }

    String body = String("{\"chat_id\":\"") + TELEGRAM_CHAT_ID +
                  "\",\"text\":\"" + jsonEscape(text) + "\"}";

    String req = String("POST /bot") + TELEGRAM_BOT_TOKEN + "/sendMessage HTTP/1.1\r\n" +
                 "Host: " + TELEGRAM_API_HOST + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + body.length() + "\r\n" +
                 "Connection: close\r\n\r\n" + body;

    client.print(req);

    unsigned long deadline = millis() + ALERT_HTTP_TIMEOUT_MS;
    String statusLine;
    while (client.connected() && millis() < deadline) {
        if (client.available()) {
            statusLine = client.readStringUntil('\n');
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    client.stop();

    bool ok = statusLine.indexOf("200") > 0;
    if (!ok) {
        Serial.printf(" [ALERT] Telegram rejected the request: %s\n", statusLine.c_str());
    }
    return ok;
}

// ---------------------------------------------------------------------------
// Worker task
// ---------------------------------------------------------------------------

static void alertTaskFn(void*) {
    for (;;) {
        String entry;
        if (WiFi.status() == WL_CONNECTED && storePeek(entry)) {
            // Entries are tagged so the UI can report the outcome of the fall
            // alert specifically, not of whatever happens to be at the front.
            bool isFall = entry.startsWith("F|");
            String text = entry.substring(2);

            bool ok = false;
            for (int attempt = 0; attempt < ALERT_MAX_RETRIES && !ok; attempt++) {
                if (attempt > 0) vTaskDelay(pdMS_TO_TICKS(1500));
                ok = sendTelegram(text);
            }

            if (ok) {
                storePopFront();
                Serial.println(" [ALERT] Delivered via Telegram.");
                if (isFall) fallDispatch = DISPATCH_DELIVERED;
            } else {
                // Keep the entry queued and report the failure honestly. It
                // will go out on a later pass once the network recovers.
                Serial.println(" [ALERT] Delivery failed, entry stays queued.");
                if (isFall) fallDispatch = DISPATCH_FAILED;
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void queueAlertMessage(const String& message) {
    storePush("N|" + message);
}

void resetAlertDispatcher() {
    fallDispatch = DISPATCH_IDLE;
    fallQueued = false;
    outcomeSince = 0;
}

void initAlertDispatcher() {
    storeMutex = xSemaphoreCreateMutex();
    prefs.begin("alerts", false);
    // Safe to touch NVS directly here: alertTask does not exist yet.
    publishQueueDepth(prefs.getUChar("count", 0));

    if (g_watchState.queuedAlerts > 0) {
        Serial.printf(" -> NOTE: %d undelivered alert(s) recovered from storage.\n",
                      g_watchState.queuedAlerts);
    }

    // Core 0 alongside the Wi-Fi stack, leaving core 1 for the render loop.
    xTaskCreatePinnedToCore(alertTaskFn, "alertTask", 10240, nullptr, 1, &alertTask, 0);
    Serial.println(" -> SUCCESS: Alert Dispatcher Active (Telegram + NVS outbox).");
}

void updateAlertDispatcher() {
    // A failed send is not final: the entry stays in the outbox and the worker
    // keeps retrying every ten seconds. When one of those retries lands, the
    // screen has to stop saying "CHUA GUI DUOC" -- otherwise the watch under-
    // reports in the other direction and the wearer calls for help that has
    // already been summoned.
    if (g_watchState.fallState == FALL_STATE_FAILED) {
        if (fallDispatch == DISPATCH_DELIVERED) {
            g_watchState.fallState = FALL_STATE_SENT;
            outcomeSince = millis();
            g_watchState.notificationPending = true;
            g_watchState.lastNotificationMsg = "Đã gửi lại thành công";
        }
    }

    // Return the watch to monitoring on its own. Every other exit from these
    // two states needs a finger on the glass, which is not something a wearer
    // who has just fallen can be assumed to provide -- and while the state is
    // not NORMAL the fall detector ignores new impacts entirely. See
    // ALERT_SENT_AUTO_CLEAR_MS.
    if (g_watchState.fallState == FALL_STATE_SENT ||
        g_watchState.fallState == FALL_STATE_FAILED) {
        unsigned long limit = (g_watchState.fallState == FALL_STATE_SENT)
                                  ? ALERT_SENT_AUTO_CLEAR_MS
                                  : ALERT_FAILED_AUTO_CLEAR_MS;
        if (outcomeSince != 0 && millis() - outcomeSince >= limit) {
            Serial.println(" [ALERT] Outcome screen timed out, resuming monitoring.");
            outcomeSince = 0;
            fallQueued = false;
            fallDispatch = DISPATCH_IDLE;
            g_watchState.fallDetected = false;
            g_watchState.fallState = FALL_STATE_NORMAL;
            g_watchState.countdownSec = 15;
            g_watchState.currentScreen = SCREEN_HOME;
            g_watchState.screenNeedsFullRedraw = true;
            // Anything still queued stays queued. The worker keeps retrying it
            // in the background; clearing the screen is not the same as
            // abandoning the message.
        }
        return;
    }

    if (g_watchState.fallState != FALL_STATE_SENDING) {
        if (g_watchState.fallState == FALL_STATE_NORMAL) fallQueued = false;
        return;
    }

    if (!fallQueued) {
        fallQueued = true;
        fallDispatch = DISPATCH_INFLIGHT;

        char msg[256];
        snprintf(msg, sizeof(msg),
                 "F|🚨 CẢNH BÁO TÉ NGÃ KHẨN CẤP 🚨\n"
                 "⏰ Thời gian: %02d:%02d:%02d - %02d/%02d/%04d\n"
                 "❤️ Nhịp tim: %s\n"
                 "🫁 SpO2: %s\n"
                 "🔋 Pin: %d%%\n"
                 "📍 Trạng thái: Cần hỗ trợ ngay lập tức!",
                 g_watchState.hour, g_watchState.minute, g_watchState.second,
                 g_watchState.day, g_watchState.month + 1, g_watchState.year,
                 g_watchState.hrValid ? (String(g_watchState.heartRateBPM) + " BPM").c_str() : "Không đo được",
                 g_watchState.spo2Valid ? (String(g_watchState.spo2Percent) + "%").c_str() : "Không đo được",
                 g_watchState.batteryPercent);

        storePush(msg);
        Serial.println(" [ALERT] Fall alert handed to dispatcher.");
    }

    if (fallDispatch == DISPATCH_DELIVERED) {
        g_watchState.fallState = FALL_STATE_SENT;
        outcomeSince = millis();
        g_watchState.notificationPending = true;
        g_watchState.lastNotificationMsg = "Đã báo người thân";
    } else if (fallDispatch == DISPATCH_FAILED) {
        g_watchState.fallState = FALL_STATE_FAILED;
        outcomeSince = millis();
        g_watchState.notificationPending = true;
        g_watchState.lastNotificationMsg = "CHƯA GỬI ĐƯỢC - đã lưu";
    } else {
        return;
    }

    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %02d/%02d/%04d",
             g_watchState.hour, g_watchState.minute,
             g_watchState.day, g_watchState.month + 1, g_watchState.year);
    g_watchState.lastNotificationTime = timeBuf;
}
