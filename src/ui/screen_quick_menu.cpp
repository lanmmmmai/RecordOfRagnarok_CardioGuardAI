#include "screen_quick_menu.h"
#include "vn_font.h"

void renderQuickMenuScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
    spr.drawString("MENU NHANH", SCREEN_CENTER_X, 32, 2);

    // 2x3 Grid Menu Buttons (48x42 px tiles)
    // Row 1 (Y = 48..90): WiFi | BLE | Báo
    vnFont(spr, VN_SMALL);
    spr.fillRoundRect(38, 48, 48, 42, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(38, 48, 48, 42, 8, UI_COLOR_BLUE);
    spr.setTextColor(g_watchState.wifiConnected ? UI_COLOR_NORMAL : UI_COLOR_WARNING, UI_COLOR_DARK_CARD);
    spr.drawString("WiFi", 62, 69, 1);

    spr.fillRoundRect(96, 48, 48, 42, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(96, 48, 48, 42, 8, UI_COLOR_BEZEL);
    spr.setTextColor(g_watchState.bleConnected ? UI_COLOR_NORMAL : UI_COLOR_SECONDARY, UI_COLOR_DARK_CARD);
    spr.drawString("BLE", 120, 69, 1);

    spr.fillRoundRect(154, 48, 48, 42, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(154, 48, 48, 42, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_DARK_CARD);
    spr.drawString("BÁO", 178, 69, 1);

    // Row 2 (Y = 98..140): SOS | Sáng | T.Tin
    spr.fillRoundRect(38, 98, 48, 42, 8, UI_COLOR_CRITICAL);
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
    spr.drawString("SOS", 62, 119, 2);

    vnFont(spr, VN_SMALL);
    spr.fillRoundRect(96, 98, 48, 42, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(96, 98, 48, 42, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_DARK_CARD);
    spr.drawString("SÁNG", 120, 119, 1);

    spr.fillRoundRect(154, 98, 48, 42, 8, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(154, 98, 48, 42, 8, UI_COLOR_BEZEL);
    spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_DARK_CARD);
    spr.drawString("T.TIN", 178, 119, 1);

    // Sensor Status Indicator Strip
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("IMU", 54, 156, 1);
    spr.fillCircle(72, 156, 4, g_watchState.imuOk ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);

    spr.drawString("PPG", 106, 156, 1);
    spr.fillCircle(124, 156, 4, g_watchState.hrSensorOk ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);

    spr.drawString("CHẠM", 158, 156, 1);
    spr.fillCircle(182, 156, 4, g_watchState.touchOk ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL);

    spr.drawString("CardioGuard AI v1.0", SCREEN_CENTER_X, 184, 1);
    spr.drawString("Vuốt xuống để về", SCREEN_CENTER_X, 208, 1);
}
