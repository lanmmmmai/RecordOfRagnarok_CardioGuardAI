#include "screen_heart_rate.h"
#include "vn_font.h"

void renderHeartRateScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    // Header Title
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
    spr.drawString("NHỊP TIM", SCREEN_CENTER_X, 30, 2);

    // Heart Icon Symbol
    spr.fillCircle(SCREEN_CENTER_X - 10, 58, 8, UI_COLOR_HEART);
    spr.fillCircle(SCREEN_CENTER_X + 10, 58, 8, UI_COLOR_HEART);
    spr.fillTriangle(SCREEN_CENTER_X - 17, 60, SCREEN_CENTER_X + 17, 60, SCREEN_CENTER_X, 78, UI_COLOR_HEART);

    // Vertical rhythm: the smooth fonts have taller line boxes than the ASCII
    // fonts they replaced (37 px at VN_LARGE, 24 at VN_MEDIUM), so the rows
    // below are spaced by those boxes, not by the old 26/16 px ones.
    char bpmBuf[16];
    if (!g_watchState.hrSensorOk) {
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        vnFont(spr, VN_LARGE);
        spr.drawString("--", SCREEN_CENTER_X, 100, 4);
        vnFont(spr, VN_MEDIUM);
        spr.drawString("LỖI CẢM BIẾN", SCREEN_CENTER_X, 130, 2);
        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("Kiểm tra dây MAX30102", SCREEN_CENTER_X, 154, 1);
    } else if (g_watchState.hrValid) {
        snprintf(bpmBuf, sizeof(bpmBuf), "%d", g_watchState.heartRateBPM);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString(bpmBuf, SCREEN_CENTER_X, 100, 4);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("BPM", SCREEN_CENTER_X, 130, 2);

        if (g_watchState.heartRateBPM > 100) {
            spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
            spr.drawString("HƠI CAO", SCREEN_CENTER_X, 154, 2);
        } else if (g_watchState.heartRateBPM < 60) {
            spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
            spr.drawString("HƠI THẤP", SCREEN_CENTER_X, 154, 2);
        } else {
            spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
            spr.drawString("BÌNH THƯỜNG", SCREEN_CENTER_X, 154, 2);
        }
    } else {
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("--", SCREEN_CENTER_X, 100, 4);
        vnFont(spr, VN_MEDIUM);
        spr.drawString("BPM", SCREEN_CENTER_X, 130, 2);

        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        if (g_watchState.motionArtifact) {
            spr.drawString("GIỮ YÊN TAY", SCREEN_CENTER_X, 154, 2);
        } else if (!g_watchState.skinContact) {
            spr.drawString("ĐEO CHẶT HƠN", SCREEN_CENTER_X, 154, 2);
        } else {
            spr.drawString("ĐANG ĐO...", SCREEN_CENTER_X, 154, 2);
        }
    }

    // Signal quality bar. The wearer needs a way to tell a trustworthy reading
    // from a lucky one, and at the wrist that difference is large.
    if (g_watchState.hrSensorOk) {
        int qw = (g_watchState.signalQuality * 120) / 100;
        if (qw > 120) qw = 120;
        uint16_t qcol = g_watchState.signalQuality >= 60 ? UI_COLOR_NORMAL
                      : g_watchState.signalQuality >= 30 ? UI_COLOR_WARNING
                                                         : UI_COLOR_CRITICAL;
        spr.drawRoundRect(60, 168, 120, 8, 4, UI_COLOR_BEZEL);
        if (qw > 2) spr.fillRoundRect(60, 168, qw, 8, 4, qcol);
        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        char qBuf[24];
        snprintf(qBuf, sizeof(qBuf), "Tín hiệu %d%%", g_watchState.signalQuality);
        spr.drawString(qBuf, SCREEN_CENTER_X, 186, 1);
    }

    // Mini Trend Chart. heartRateHistory is a ring written at historyIndex, so
    // the oldest sample lives there and reading must start from it — walking
    // 0..29 linearly would draw a chart with a jump at the write cursor.
    //
    // Narrower and shorter than before: at y=210 the round screen is only
    // ~158 px wide, so the old 140 px box starting at x=50 had its bottom
    // corners outside the glass.
    int chartX = 62;
    int chartY = 196;
    int chartW = 116;
    int chartH = 14;

    spr.drawRoundRect(chartX, chartY, chartW, chartH, 4, UI_COLOR_BEZEL);

    for (int i = 0; i < 29; i++) {
        uint16_t v1 = g_watchState.heartRateHistory[(g_watchState.historyIndex + i) % 30];
        uint16_t v2 = g_watchState.heartRateHistory[(g_watchState.historyIndex + i + 1) % 30];

        if (v1 > 0 && v2 > 0) {
            int y1 = chartY + chartH - constrain(map(v1, 50, 120, 2, chartH - 2), 2, chartH - 2);
            int y2 = chartY + chartH - constrain(map(v2, 50, 120, 2, chartH - 2), 2, chartH - 2);
            int x1 = chartX + (i * chartW / 29);
            int x2 = chartX + ((i + 1) * chartW / 29);
            spr.drawLine(x1, y1, x2, y2, UI_COLOR_HEART);
        }
    }

    // Back Footer
    vnFont(spr, VN_SMALL);
    spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
    spr.drawString("← Trang chủ", SCREEN_CENTER_X, 220, 1);
}
