#include "screen_quick_menu.h"
#include "vn_font.h"

void renderQuickMenuScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
    spr.drawString("MENU NHANH", SCREEN_CENTER_X, 36, 2);

    // 2x3 Grid Menu Buttons. The hit zones in ui_manager.cpp are derived from
    // these rectangles -- keep the two in step when moving anything.
    // Labels stay short on purpose: each tile is only 46 px wide, so a full
    // phrase like "BAO THUC" would run over the tile edge.
    // Row 1 (Y = 52..96): WiFi | BLE | Bao thuc
    vnFont(spr, VN_SMALL);
    spr.fillRoundRect(40, 52, 46, 44, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(40, 52, 46, 44, 8, UI_COLOR_BLUE);
    spr.setTextColor(g_watchState.wifiConnected ? UI_COLOR_NORMAL : UI_COLOR_WARNING, UI_COLOR_DARK_CARD);
    spr.drawString("WiFi", 63, 74, 1);

    spr.fillRoundRect(97, 52, 46, 44, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(97, 52, 46, 44, 8, UI_COLOR_BEZEL);
    spr.setTextColor(g_watchState.bleConnected ? UI_COLOR_NORMAL : UI_COLOR_SECONDARY, UI_COLOR_DARK_CARD);
    spr.drawString("BLE", 120, 74, 1);

    spr.fillRoundRect(154, 52, 46, 44, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(154, 52, 46, 44, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_DARK_CARD);
    spr.drawString("BÁO", 177, 74, 1);

    // Row 2 (Y = 104..148): SOS Emergency | Do sang | Thong tin
    spr.fillRoundRect(40, 104, 46, 44, 8, UI_COLOR_CRITICAL);
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
    spr.drawString("SOS", 63, 126, 2);

    vnFont(spr, VN_SMALL);
    spr.fillRoundRect(97, 104, 46, 44, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(97, 104, 46, 44, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_DARK_CARD);
    spr.drawString("SÁNG", 120, 126, 1);

    spr.fillRoundRect(154, 104, 46, 44, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(154, 104, 46, 44, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_DARK_CARD);
    spr.drawString("T.TIN", 177, 126, 1);

    // Sensor health strip: three dots, one per sensor, so a fault is visible
    // without digging through the serial log. The labels stay as the sensor
    // abbreviations -- they are part names, not words to translate.
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.setTextDatum(ML_DATUM);
    spr.drawString("IMU", 58, 166, 1);
    spr.drawString("PPG", 104, 166, 1);
    spr.drawString("CHẠM", 150, 166, 1);
    spr.setTextDatum(MC_DATUM);
    spr.fillCircle(88,  166, 4, g_watchState.imuOk      ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);
    spr.fillCircle(134, 166, 4, g_watchState.hrSensorOk ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);
    spr.fillCircle(196, 166, 4, g_watchState.touchOk    ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);

    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("Đồng hồ ESP32-S3", SCREEN_CENTER_X, 188, 1);
    spr.drawString("Vuốt xuống để về", SCREEN_CENTER_X, 206, 1);
}
