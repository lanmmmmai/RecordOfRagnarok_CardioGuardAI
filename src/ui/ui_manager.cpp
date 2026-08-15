#include "ui_manager.h"
#include "screen_home.h"
#include "screen_heart_rate.h"
#include "screen_spo2.h"
#include "screen_fall_monitor.h"
#include "screen_fall_alert.h"
#include "screen_notification.h"
#include "screen_quick_menu.h"
#include "vn_font.h"
#include "fall_detection/fall_detector.h"

TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite spr = TFT_eSprite(&tft);

// False when the 115 KB frame buffer could not be allocated. Every draw call
// below would otherwise write through a null pointer.
static bool spriteReady = false;

void initUIManager() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(UI_COLOR_BLACK);

    spr.setColorDepth(16);
    // 240 x 240 x 16bpp = 115,200 bytes. With Wi-Fi and BLE also claiming heap
    // this can fail, and TFT_eSPI signals that only through the return value.
    spriteReady = (spr.createSprite(240, 240) != nullptr);

    if (spriteReady) {
        spr.fillSprite(UI_COLOR_BLACK);
        Serial.println(" -> SUCCESS: UI Manager & 240x240 Double Buffer Sprite Initialized.");
        initVnFonts(spr);
    } else {
        Serial.printf(" -> WARNING: Sprite allocation FAILED (free heap %u B). "
                      "Falling back to direct rendering.\n", (unsigned)ESP.getFreeHeap());
    }
}

void handleUITouchInput(int touchX, int touchY, uint8_t gestureID) {
    UIScreen screen = g_watchState.currentScreen;

    // Ignore invalid / empty touch pulses (0x00)
    if (gestureID == 0x00) return;

    Serial.printf(" [UI NAV] Current Screen: %d | Gesture: 0x%02X | Touch (%d, %d)\n",
                  (int)screen, gestureID, touchX, touchY);

    // 1. Gesture Navigation
    //
    // Every screen except the fall alert sits on one horizontal carousel, in
    // this order, and it wraps at both ends:
    //
    //   HOME -> HEART_RATE -> SPO2 -> FALL_MONITOR -> NOTIFICATION
    //        -> QUICK_MENU -> back to HOME
    //
    // Notifications and the quick menu used to be reachable only by swiping up
    // or down *from home*. On a 240 px round face an older wearer's swipe lands
    // diagonally as often as not, so two of the seven screens were effectively
    // behind a gesture that half the time did nothing at all. Putting them on
    // the same carousel means one repeated gesture reaches everything: swipe
    // left enough times and you arrive, whichever screen you started on.
    //
    // The vertical gestures are kept as the shortcuts they always were, so
    // nobody who learned them has to relearn anything.
    static const UIScreen carousel[] = {
        SCREEN_HOME, SCREEN_HEART_RATE, SCREEN_SPO2, SCREEN_FALL_MONITOR,
        SCREEN_NOTIFICATION, SCREEN_QUICK_MENU,
    };
    const int carouselLen = sizeof(carousel) / sizeof(carousel[0]);

    // -1 when the current screen is not on the carousel, which today means the
    // fall alert. That screen is excluded on purpose: it is the one screen the
    // wearer must not be able to swipe away from during a countdown.
    int pos = -1;
    for (int i = 0; i < carouselLen; i++) {
        if (carousel[i] == screen) { pos = i; break; }
    }

    if (gestureID == 0x03) { // SWIPE LEFT -- forward, wrapping
        if (pos >= 0) g_watchState.currentScreen = carousel[(pos + 1) % carouselLen];
    }
    else if (gestureID == 0x04) { // SWIPE RIGHT -- back one step, wrapping
        // Steps back rather than jumping straight home. Swiping right used to
        // mean "home" from anywhere, which made the carousel a one-way street:
        // overshoot by one and the only way back was all the way around. Home
        // is still at most a few swipes away in either direction, and the
        // gesture is now the mirror image of swipe-left, which is what a
        // sideways swipe leads someone to expect.
        if (pos >= 0) g_watchState.currentScreen = carousel[(pos + carouselLen - 1) % carouselLen];
    }
    else if (gestureID == 0x01) { // SWIPE UP (Quick Menu shortcut)
        if (screen == SCREEN_HOME) g_watchState.currentScreen = SCREEN_QUICK_MENU;
    }
    else if (gestureID == 0x02) { // SWIPE DOWN (Notifications shortcut)
        if (screen == SCREEN_HOME) g_watchState.currentScreen = SCREEN_NOTIFICATION;
        else if (screen == SCREEN_QUICK_MENU) g_watchState.currentScreen = SCREEN_HOME;
    }
    else if (gestureID == 0x05) { // SINGLE CLICK / TAP ONLY
        if (screen == SCREEN_HOME) {
            // Tap BPM Area (Left) -> Open Heart Rate Screen
            if (touchX >= 20 && touchX <= 115 && touchY >= 140 && touchY <= 185) {
                g_watchState.currentScreen = SCREEN_HEART_RATE;
            }
            // Tap SpO2 Area (Right) -> Open SpO2 Screen
            else if (touchX >= 125 && touchX <= 220 && touchY >= 140 && touchY <= 185) {
                g_watchState.currentScreen = SCREEN_SPO2;
            }
            // Tap Fall Status Pill -> Open Fall Monitor
            else if (touchY >= 186 && touchY <= 230) {
                g_watchState.currentScreen = SCREEN_FALL_MONITOR;
            }
        }
        else if (screen == SCREEN_FALL_ALERT) {
            if (g_watchState.fallState == FALL_STATE_ALERT) {
                // Only the CANCEL button cancels, and it is checked against the
                // rectangle actually drawn in screen_fall_alert.cpp: origin
                // (40,166), size 160x40, so (40,166)-(200,206). A loose
                // "anywhere below Y=150" test lets a knock against a table
                // dismiss a real alert during the countdown. Keep these two
                // numbers in step -- the button moved once already and the
                // hit box was left behind, which made the top of the button
                // dead and the strip below it live.
                if (touchX >= 40 && touchX <= 200 && touchY >= 166 && touchY <= 206) {
                    cancelFallAlert();
                }
            }
            else if (g_watchState.fallState == FALL_STATE_SENT ||
                     g_watchState.fallState == FALL_STATE_FAILED) {
                // Acknowledging the outcome. Queued messages survive this:
                // the dispatcher keeps retrying from NVS.
                cancelFallAlert();
            }
        }
        else if (screen == SCREEN_QUICK_MENU) {
            // SOS button, matching the rect drawn at (40,104) size 46x44.
            if (touchX >= 40 && touchX <= 86 && touchY >= 104 && touchY <= 148) {
                triggerSimulatedFall();
            } else {
                g_watchState.currentScreen = SCREEN_HOME;
            }
        }
        else if (screen != SCREEN_HOME && screen != SCREEN_FALL_ALERT) {
            // Tap anywhere on detail screens -> Return HOME
            g_watchState.currentScreen = SCREEN_HOME;
        }
    }
}

// Minimal direct-to-LCD rendering used when the sprite could not be allocated.
// It flickers, but a flickering fall alert beats a black screen.
//
// Text here is Vietnamese but unaccented, which is deliberate. The smooth
// fonts live in the sprite, and this path runs precisely when there was not
// enough memory to allocate one -- loading a second copy of the fonts onto
// `tft` just for the low-memory fallback would defeat its purpose. So the
// wearer still reads Vietnamese words through the built-in ASCII font, just
// without the diacritics.
static void renderDirectFallback() {
    static uint8_t lastState = 0xFF;
    static int lastSecond = -1;

    bool alerting = (g_watchState.fallState == FALL_STATE_ALERT ||
                     g_watchState.fallState == FALL_STATE_SENDING ||
                     g_watchState.fallState == FALL_STATE_FAILED);

    if (lastState != (uint8_t)g_watchState.fallState) {
        lastState = (uint8_t)g_watchState.fallState;
        lastSecond = -1;
        tft.fillScreen(alerting ? UI_COLOR_CRITICAL : UI_COLOR_BLACK);
    }
    if (lastSecond == g_watchState.second) return;
    lastSecond = g_watchState.second;

    tft.setTextDatum(MC_DATUM);
    char buf[24];
    if (alerting) {
        tft.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        tft.drawString("TE NGA!", SCREEN_CENTER_X, 90, 4);
        snprintf(buf, sizeof(buf), "%d  ", g_watchState.countdownSec);
        tft.drawString(buf, SCREEN_CENTER_X, 140, 4);
    } else {
        tft.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 g_watchState.hour, g_watchState.minute, g_watchState.second);
        tft.drawString(buf, SCREEN_CENTER_X, 110, 4);
        tft.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        tft.drawString("CHE DO TIET KIEM RAM", SCREEN_CENTER_X, 150, 2);
    }
}

void renderUI() {
    // Take over the screen for states the wearer must see. FALL_STATE_SUSPECTED
    // is deliberately excluded: it is a two-second internal confirmation window
    // that must not flash an alarm at every bumped wrist.
    if (g_watchState.currentScreen != SCREEN_FALL_ALERT &&
        (g_watchState.fallState == FALL_STATE_ALERT ||
         g_watchState.fallState == FALL_STATE_SENDING ||
         g_watchState.fallState == FALL_STATE_SENT ||
         g_watchState.fallState == FALL_STATE_FAILED)) {
        g_watchState.currentScreen = SCREEN_FALL_ALERT;
    }

    if (!spriteReady) {
        renderDirectFallback();
        return;
    }

    switch (g_watchState.currentScreen) {
        case SCREEN_HOME:         renderHomeScreen(spr);         break;
        case SCREEN_HEART_RATE:   renderHeartRateScreen(spr);    break;
        case SCREEN_SPO2:         renderSpO2Screen(spr);         break;
        case SCREEN_FALL_MONITOR: renderFallMonitorScreen(spr);  break;
        case SCREEN_FALL_ALERT:   renderFallAlertScreen(spr);    break;
        case SCREEN_NOTIFICATION: renderNotificationScreen(spr); break;
        case SCREEN_QUICK_MENU:   renderQuickMenuScreen(spr);    break;
    }

    spr.pushSprite(0, 0);
}
