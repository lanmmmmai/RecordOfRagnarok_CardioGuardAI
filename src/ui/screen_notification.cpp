#include "screen_notification.h"
#include "vn_font.h"

void renderNotificationScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
    spr.drawString("THÔNG BÁO", SCREEN_CENTER_X, 35, 2);

    spr.fillRoundRect(30, 65, 180, 110, 10, UI_COLOR_DARK_CARD);
    spr.drawRoundRect(30, 65, 180, 110, 10, UI_COLOR_BLUE);

    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_DARK_CARD);
    spr.drawString(g_watchState.lastNotificationMsg.c_str(), SCREEN_CENTER_X, 90, 2);

    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d  %02d/%02d/%04d",
             g_watchState.hour, g_watchState.minute, g_watchState.day, g_watchState.month + 1, g_watchState.year);
    vnFont(spr, VN_SMALL);
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_DARK_CARD);
    spr.drawString(timeBuf, SCREEN_CENTER_X, 118, 1);

    // Delivery state comes from the queue depth, not a hardcoded checkmark.
    // Claiming "Delivered" when messages are still queued would be a lie about
    // the one thing this device exists to do.
    vnFont(spr, VN_SMALL);
    if (g_watchState.queuedAlerts > 0) {
        char pend[40];
        snprintf(pend, sizeof(pend), "%d đang chờ gửi", g_watchState.queuedAlerts);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_DARK_CARD);
        // Vector clock face rather than an emoji glyph.
        spr.drawCircle(74, 147, 7, UI_COLOR_WARNING);
        spr.drawFastVLine(74, 142, 5, UI_COLOR_WARNING);
        spr.drawFastHLine(74, 147, 4, UI_COLOR_WARNING);
        spr.setTextDatum(ML_DATUM);
        spr.drawString(pend, 88, 147, 1);
        spr.setTextDatum(MC_DATUM);
    } else {
        spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_DARK_CARD);
        // Vector checkmark: two lines, no font dependency.
        spr.drawLine(70, 147, 75, 152, UI_COLOR_NORMAL);
        spr.drawLine(75, 152, 84, 141, UI_COLOR_NORMAL);
        spr.drawLine(70, 148, 75, 153, UI_COLOR_NORMAL);
        spr.drawLine(75, 153, 84, 142, UI_COLOR_NORMAL);
        spr.setTextDatum(ML_DATUM);
        spr.drawString("Đã gửi xong", 92, 147, 1);
        spr.setTextDatum(MC_DATUM);
    }

    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("← Trang chủ", SCREEN_CENTER_X, 216, 1);
}
