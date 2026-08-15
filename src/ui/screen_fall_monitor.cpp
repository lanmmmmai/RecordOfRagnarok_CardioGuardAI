#include "screen_fall_monitor.h"
#include "vn_font.h"

void renderFallMonitorScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
    spr.drawString("THEO DÕI NGÃ", SCREEN_CENTER_X, 38, 2);

    // The indicator reflects the live IMU flag. Previously this screen printed
    // "STATUS: ACTIVE" unconditionally, which stayed reassuring even with the
    // sensor unplugged.
    bool active = g_watchState.imuOk && g_watchState.fallMonitoringActive;
    uint16_t col = active ? UI_COLOR_NORMAL : UI_COLOR_CRITICAL;

    spr.fillCircle(SCREEN_CENTER_X, 82, 22, UI_COLOR_DARK_CARD);
    spr.drawCircle(SCREEN_CENTER_X, 82, 22, col);
    spr.fillCircle(SCREEN_CENTER_X, 82, 10, col);

    spr.setTextColor(col, UI_COLOR_BLACK);
    spr.drawString(active ? "ĐANG HOẠT ĐỘNG" : "ĐÃ NGỪNG", SCREEN_CENTER_X, 120, 2);

    vnFont(spr, VN_SMALL);
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    if (active) {
        // 235 Hz, not 50: QMI_SAMPLE_GAP_US is 4300 us and the IMU is polled
        // every pass of loop(), so this is the rate the detector actually sees.
        // The old "50Hz" was off by a factor of nearly five and would have sent
        // anyone reading it here looking for a sample-rate bug that is not
        // there.
        spr.drawString("IMU 235Hz - lọc 4 bước", SCREEN_CENTER_X, 142, 1);
    } else {
        spr.drawString("QMI8658 không phản hồi", SCREEN_CENTER_X, 142, 1);
    }

    spr.drawFastHLine(50, 158, 140, UI_COLOR_BEZEL);

    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("Sự kiện gần nhất:", SCREEN_CENTER_X, 170, 1);

    vnFont(spr, VN_MEDIUM);
    if (g_watchState.fallDetected) {
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        spr.drawString("PHÁT HIỆN NGÃ!", SCREEN_CENTER_X, 192, 2);
    } else if (g_watchState.fallState == FALL_STATE_SUSPECTED) {
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString("Đang xác nhận", SCREEN_CENTER_X, 192, 2);
    } else {
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString("Không có", SCREEN_CENTER_X, 192, 2);
    }

    // Pending alerts matter here: a queued alert means someone was told
    // nothing yet.
    vnFont(spr, VN_SMALL);
    if (g_watchState.queuedAlerts > 0) {
        char qBuf[36];
        snprintf(qBuf, sizeof(qBuf), "Còn %d chờ gửi", g_watchState.queuedAlerts);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        spr.drawString(qBuf, SCREEN_CENTER_X, 214, 1);
    } else {
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("← Trang chủ", SCREEN_CENTER_X, 214, 1);
    }
}
