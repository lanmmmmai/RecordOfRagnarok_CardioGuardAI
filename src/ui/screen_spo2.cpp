#include "screen_spo2.h"
#include "vn_font.h"

static const uint8_t SPO2_MIN_QUALITY = 45;

void renderSpO2Screen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_SPO2, UI_COLOR_BLACK);
    spr.drawString("SpO2", SCREEN_CENTER_X, 28, 2);
    vnFont(spr, VN_SMALL);
    spr.drawString("(tham khảo)", SCREEN_CENTER_X, 46, 1);

    // Lungs / Oxygen Icon Symbol
    spr.drawCircle(SCREEN_CENTER_X - 12, 64, 8, UI_COLOR_SPO2);
    spr.drawCircle(SCREEN_CENTER_X + 12, 64, 8, UI_COLOR_SPO2);

    char spo2Buf[16];
    bool trustworthy = g_watchState.spo2Valid &&
                       g_watchState.signalQuality >= SPO2_MIN_QUALITY &&
                       !g_watchState.motionArtifact;

    if (!g_watchState.hrSensorOk) {
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("--%", SCREEN_CENTER_X, 98, 4);
        vnFont(spr, VN_MEDIUM);
        spr.drawString("LỖI CẢM BIẾN", SCREEN_CENTER_X, 130, 2);
    } else if (trustworthy) {
        snprintf(spo2Buf, sizeof(spo2Buf), "%d%%", g_watchState.spo2Percent);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString(spo2Buf, SCREEN_CENTER_X, 98, 4);

        vnFont(spr, VN_MEDIUM);
        if (g_watchState.spo2Percent < 92) {
            spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
            spr.drawString("THẤP - ĐO LẠI", SCREEN_CENTER_X, 130, 2);
        } else {
            spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
            spr.drawString("BÌNH THƯỜNG", SCREEN_CENTER_X, 130, 2);
        }

        char qBuf[24];
        snprintf(qBuf, sizeof(qBuf), "Tín hiệu: %d%%", g_watchState.signalQuality);
        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString(qBuf, SCREEN_CENTER_X, 154, 1);
    } else {
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("--%", SCREEN_CENTER_X, 98, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        if (g_watchState.motionArtifact) {
            spr.drawString("GIỮ YÊN TAY", SCREEN_CENTER_X, 130, 2);
        } else if (!g_watchState.skinContact) {
            spr.drawString("CHƯA CHẠM DA", SCREEN_CENTER_X, 130, 2);
        } else {
            spr.drawString("TÍN HIỆU YẾU", SCREEN_CENTER_X, 130, 2);
        }

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Đeo chặt, giữ yên tay", SCREEN_CENTER_X, 154, 1);
    }

    vnFont(spr, VN_SMALL);
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("Không dùng cho y tế", SCREEN_CENTER_X, 180, 1);
    spr.drawString("← Trang chủ", SCREEN_CENTER_X, 210, 1);
}
