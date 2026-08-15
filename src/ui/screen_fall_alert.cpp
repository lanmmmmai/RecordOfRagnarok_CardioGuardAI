#include "screen_fall_alert.h"
#include "vn_font.h"

void renderFallAlertScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    if (g_watchState.fallState == FALL_STATE_ALERT) {
        // ==========================================
        // 15-SECOND EMERGENCY SOS COUNTDOWN
        // ==========================================
        spr.fillTriangle(SCREEN_CENTER_X, 20, SCREEN_CENTER_X - 18, 48, SCREEN_CENTER_X + 18, 48, UI_COLOR_CRITICAL);
        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("!", SCREEN_CENTER_X, 39, 2);

        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("CẢNH BÁO KHẨN CẤP!", SCREEN_CENTER_X, 64, 2);

        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Bạn có cần giúp đỡ?", SCREEN_CENTER_X, 90, 2);

        char countStr[16];
        snprintf(countStr, sizeof(countStr), "%d", g_watchState.countdownSec);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString(countStr, SCREEN_CENTER_X, 122, 4);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("giây", SCREEN_CENTER_X, 144, 1);

        int btnW = 160;
        int btnH = 40;
        int btnX = SCREEN_CENTER_X - (btnW / 2); // 40
        int btnY = 166;
        spr.fillRoundRect(btnX, btnY, btnW, btnH, 20, UI_COLOR_CRITICAL);
        spr.drawRoundRect(btnX, btnY, btnW, btnH, 20, UI_COLOR_WHITE);
        
        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("HỦY CẢNH BÁO", SCREEN_CENTER_X, btnY + 20, 2);

    } else if (g_watchState.fallState == FALL_STATE_SENDING) {
        // ==========================================
        // SENDING EMERGENCY ALERT
        // ==========================================
        spr.drawCircle(SCREEN_CENTER_X, 58, 6, UI_COLOR_WARNING);
        spr.fillCircle(SCREEN_CENTER_X, 58, 3, UI_COLOR_WARNING);
        spr.drawCircle(SCREEN_CENTER_X, 58, 14, UI_COLOR_WARNING);
        spr.drawCircle(SCREEN_CENTER_X, 58, 22, UI_COLOR_BEZEL);

        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString("ĐANG GỬI...", SCREEN_CENTER_X, 104, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Đang phát tín hiệu SOS", SCREEN_CENTER_X, 144, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Đang báo cho người thân", SCREEN_CENTER_X, 168, 1);

    } else if (g_watchState.fallState == FALL_STATE_SENT) {
        // ==========================================
        // ALERT SENT SUCCESS
        // ==========================================
        spr.fillCircle(SCREEN_CENTER_X, 60, 24, UI_COLOR_NORMAL);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_NORMAL);
        spr.drawString("V", SCREEN_CENTER_X, 60, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
        spr.drawString("ĐÃ PHÁT TÍN HIỆU", SCREEN_CENTER_X, 108, 2);

        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Đã báo người thân", SCREEN_CENTER_X, 146, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Chạm để về", SCREEN_CENTER_X, 196, 1);

    } else if (g_watchState.fallState == FALL_STATE_FAILED) {
        // ==========================================
        // DELIVERY FAILED
        // ==========================================
        spr.fillCircle(SCREEN_CENTER_X, 58, 22, UI_COLOR_CRITICAL);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("X", SCREEN_CENTER_X, 58, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("CHƯA GỬI ĐƯỢC", SCREEN_CENTER_X, 102, 2);

        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Không có mạng!", SCREEN_CENTER_X, 126, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        char qBuf[40];
        snprintf(qBuf, sizeof(qBuf), "Đã lưu %d - sẽ gửi lại", g_watchState.queuedAlerts);
        spr.drawString(qBuf, SCREEN_CENTER_X, 148, 1);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("HÃY GỌI CỨU HỘ", SCREEN_CENTER_X, 172, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Chạm để về", SCREEN_CENTER_X, 198, 1);
    }
}
