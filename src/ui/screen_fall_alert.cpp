#include "screen_fall_alert.h"
#include "vn_font.h"

void renderFallAlertScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    if (g_watchState.fallState == FALL_STATE_ALERT) {
        // ==========================================
        // 15-SECOND FALL ALERT COUNTDOWN
        // ==========================================
        spr.fillTriangle(SCREEN_CENTER_X, 22, SCREEN_CENTER_X - 18, 52, SCREEN_CENTER_X + 18, 52, UI_COLOR_CRITICAL);
        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("!", SCREEN_CENTER_X, 43, 2);

        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("PHÁT HIỆN TÉ NGÃ!", SCREEN_CENTER_X, 68, 2);

        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Bạn có ổn không?", SCREEN_CENTER_X, 94, 2);

        char countStr[16];
        snprintf(countStr, sizeof(countStr), "%d", g_watchState.countdownSec);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString(countStr, SCREEN_CENTER_X, 126, 4);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("giây", SCREEN_CENTER_X, 150, 1);

        spr.fillRoundRect(40, 168, 160, 42, 21, UI_COLOR_CRITICAL);
        spr.drawRoundRect(40, 168, 160, 42, 21, UI_COLOR_WHITE);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("TÔI ỔN", SCREEN_CENTER_X, 189, 4);

    } else if (g_watchState.fallState == FALL_STATE_SENDING) {
        // ==========================================
        // SENDING EMERGENCY ALERT
        // ==========================================
        // Vector antenna instead of an emoji: the smooth fonts carry the
        // Vietnamese alphabet, not pictographs.
        spr.drawCircle(SCREEN_CENTER_X, 62, 6, UI_COLOR_WARNING);
        spr.fillCircle(SCREEN_CENTER_X, 62, 3, UI_COLOR_WARNING);
        spr.drawCircle(SCREEN_CENTER_X, 62, 14, UI_COLOR_WARNING);
        spr.drawCircle(SCREEN_CENTER_X, 62, 22, UI_COLOR_BEZEL);

        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString("ĐANG GỬI...", SCREEN_CENTER_X, 110, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Cảnh báo khẩn cấp", SCREEN_CENTER_X, 148, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Đang báo cho người thân", SCREEN_CENTER_X, 172, 1);

    } else if (g_watchState.fallState == FALL_STATE_SENT) {
        // ==========================================
        // ALERT SENT SUCCESS
        // ==========================================
        spr.fillCircle(SCREEN_CENTER_X, 65, 25, UI_COLOR_NORMAL);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_NORMAL);
        spr.drawString("V", SCREEN_CENTER_X, 65, 4);

        spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
        spr.drawString("ĐÃ GỬI", SCREEN_CENTER_X, 118, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Đã báo người thân", SCREEN_CENTER_X, 158, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Chạm để về", SCREEN_CENTER_X, 206, 1);

    } else if (g_watchState.fallState == FALL_STATE_FAILED) {
        // ==========================================
        // DELIVERY FAILED
        // ==========================================
        // This state used to be unreachable, so a failed send rendered a blank
        // screen -- indistinguishable from a delivered one. The wearer has to
        // know nobody was reached so they can call for help another way.
        spr.fillCircle(SCREEN_CENTER_X, 60, 24, UI_COLOR_CRITICAL);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_CRITICAL);
        spr.drawString("X", SCREEN_CENTER_X, 60, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("CHƯA GỬI ĐƯỢC", SCREEN_CENTER_X, 106, 2);

        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Không có mạng!", SCREEN_CENTER_X, 130, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        char qBuf[40];
        snprintf(qBuf, sizeof(qBuf), "Đã lưu %d - sẽ gửi lại", g_watchState.queuedAlerts);
        spr.drawString(qBuf, SCREEN_CENTER_X, 152, 1);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("HÃY GỌI CỨU HỘ", SCREEN_CENTER_X, 178, 2);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Chạm để về", SCREEN_CENTER_X, 208, 1);
    }
}
