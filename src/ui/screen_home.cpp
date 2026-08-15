#include "screen_home.h"
#include "vn_font.h"

void renderHomeScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Double Concentric Bezel Rings (Cyber Luxe Theme)
    spr.drawCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 119, UI_COLOR_BEZEL);
    spr.drawCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 117, UI_COLOR_BLUE);
    spr.drawCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, 115, UI_COLOR_BEZEL);

    spr.setTextDatum(MC_DATUM);

    // 1. TOP HEADER: link status & battery
    vnFont(spr, VN_SMALL);
    // The label stays put and the dot carries the state: at y=30 the circle
    // leaves only ~130 px of width, and a label that changed length between
    // states pushed past the bezel on one of them.
    spr.setTextColor(g_watchState.wifiConnected ? UI_COLOR_NORMAL : UI_COLOR_WARNING,
                     UI_COLOR_BLACK);
    spr.drawString("WiFi", 72, 30, 1);
    if (g_watchState.wifiConnected) {
        spr.fillCircle(94, 30, 3, UI_COLOR_NORMAL);
    } else {
        spr.drawCircle(94, 30, 3, UI_COLOR_WARNING);
    }

    // BLE state sits next to Wi-Fi: the wearer needs to see at a glance that
    // at least one path to help is still open.
    spr.setTextColor(g_watchState.bleConnected ? UI_COLOR_NORMAL : UI_COLOR_SECONDARY,
                     UI_COLOR_BLACK);
    spr.drawString("BLE", 112, 30, 1);

    char batBuf[16];
    if (g_watchState.isCharging) {
        snprintf(batBuf, sizeof(batBuf), "SẠC");
        spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
    } else {
        snprintf(batBuf, sizeof(batBuf), "%d%%", g_watchState.batteryPercent);
        spr.setTextColor(g_watchState.batteryPercent > 20 ? UI_COLOR_WHITE : UI_COLOR_CRITICAL, UI_COLOR_BLACK);
    }
    spr.drawString(batBuf, 164, 30, 1);

    // 2. CENTER: DIGITAL CLOCK & DATE
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
             g_watchState.hour, g_watchState.minute, g_watchState.second);
    vnFont(spr, VN_LARGE);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
    spr.drawString(timeStr, SCREEN_CENTER_X, 74, 4);

    // Clean digital clock display

    const char* daysVN[] = {"CHỦ NHẬT", "THỨ HAI", "THỨ BA", "THỨ TƯ",
                            "THỨ NĂM", "THỨ SÁU", "THỨ BẢY"};
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
    spr.drawString(daysVN[g_watchState.dayOfWeek % 7], SCREEN_CENTER_X, 111, 2);

    char dateStr[20];
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", g_watchState.day, g_watchState.month + 1, g_watchState.year);
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString(dateStr, SCREEN_CENTER_X, 134, 2);

    spr.drawFastHLine(50, 148, 140, UI_COLOR_BEZEL);

    // 3. BOTTOM: HEALTH METRICS
    // A number is shown only when the algorithm actually produced one. A stale
    // or unmeasured value displayed as if it were current is worse than dashes.
    char hrStr[16];
    if (!g_watchState.hrSensorOk) {
        snprintf(hrStr, sizeof(hrStr), "LỖI ĐO");
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
    } else if (g_watchState.hrValid) {
        snprintf(hrStr, sizeof(hrStr), "%d BPM", g_watchState.heartRateBPM);
        spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
    } else {
        snprintf(hrStr, sizeof(hrStr), "-- BPM");
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    }
    spr.drawString(hrStr, 76, 166, 2);

    char spo2Str[16];
    if (g_watchState.spo2Valid) {
        snprintf(spo2Str, sizeof(spo2Str), "%d%% SpO2", g_watchState.spo2Percent);
        spr.setTextColor(UI_COLOR_SPO2, UI_COLOR_BLACK);
    } else {
        snprintf(spo2Str, sizeof(spo2Str), "--%% SpO2");
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    }
    spr.drawString(spo2Str, 164, 166, 2);

    // 4. FALL STATUS BADGE
    // The pill sits at y=185..209 rather than lower down: this far from the
    // centre the round screen has already narrowed to about 155 usable pixels,
    // which is what the wording below is sized against.
    // 4. FALL STATUS BADGE (Centered 150px Capsule Pill)
    int pillW = 150;
    int pillH = 26;
    int pillX = SCREEN_CENTER_X - (pillW / 2); // 120 - 75 = 45
    int pillY = 186;

    if (!g_watchState.imuOk) {
        spr.fillRoundRect(pillX, pillY, pillW, pillH, 13, UI_COLOR_CRITICAL);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("MẤT CẢM BIẾN", SCREEN_CENTER_X, pillY + 12, 2);
    } else if (g_watchState.fallState == FALL_STATE_NORMAL) {
        spr.fillRoundRect(pillX, pillY, pillW, pillH, 13, UI_COLOR_DARK_CARD);
        spr.drawRoundRect(pillX, pillY, pillW, pillH, 13, UI_COLOR_NORMAL);
        spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_DARK_CARD);
        spr.drawString("THEO DÕI: BẬT", SCREEN_CENTER_X, pillY + 12, 2);
    } else {
        spr.fillRoundRect(pillX, pillY, pillW, pillH, 13, UI_COLOR_CRITICAL);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("PHÁT HIỆN NGÃ", SCREEN_CENTER_X, pillY + 12, 2);
    }
}
