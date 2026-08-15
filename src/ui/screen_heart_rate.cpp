#include "screen_heart_rate.h"
#include "vn_font.h"

// This screen shows the heart rate and nothing else. SpO2 has its own screen
// one swipe to the left, and duplicating it here cost the BPM figure half the
// display: at 240 px across, two large numbers side by side left each of them
// about 80 px of usable width on a round face that has already narrowed to
// ~150 px at y=80. The wearer this watch is for reads the number from arm's
// length without glasses, so the number gets the whole width back.
void renderHeartRateScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    // Header Title
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
    spr.drawString("NHỊP TIM", SCREEN_CENTER_X, 28, 2);

    // Heart icon, centred: two lobes and a point.
    spr.fillCircle(SCREEN_CENTER_X - 6, 54, 6, UI_COLOR_HEART);
    spr.fillCircle(SCREEN_CENTER_X + 6, 54, 6, UI_COLOR_HEART);
    spr.fillTriangle(SCREEN_CENTER_X - 11, 56, SCREEN_CENTER_X + 11, 56,
                     SCREEN_CENTER_X, 68, UI_COLOR_HEART);

    char bpmBuf[16];

    if (!g_watchState.hrSensorOk) {
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        vnFont(spr, VN_LARGE);
        spr.drawString("--", SCREEN_CENTER_X, 100, 4);
        vnFont(spr, VN_MEDIUM);
        spr.drawString("LỖI CẢM BIẾN", SCREEN_CENTER_X, 134, 2);
    } else if (g_watchState.hrValid) {
        // hrValid alone, not hrValid || spo2Valid. The old condition let a
        // valid SpO2 reading pull this screen into its "measured" branch and
        // print heartRateBPM -- whatever stale value it happened to hold --
        // as though it had just been measured.
        snprintf(bpmBuf, sizeof(bpmBuf), "%d", g_watchState.heartRateBPM);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString(bpmBuf, SCREEN_CENTER_X, 100, 4);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
        spr.drawString("BPM", SCREEN_CENTER_X, 126, 1);

        // Clinical Evaluation
        vnFont(spr, VN_MEDIUM);
        if (g_watchState.heartRateBPM > 100) {
            spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
            spr.drawString("NHỊP HƠI NHANH", SCREEN_CENTER_X, 146, 2);
        } else if (g_watchState.heartRateBPM < 55 && g_watchState.heartRateBPM > 0) {
            spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
            spr.drawString("NHỊP HƠI CHẬM", SCREEN_CENTER_X, 146, 2);
        } else {
            spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
            spr.drawString("NHỊP TIM BÌNH THƯỜNG", SCREEN_CENTER_X, 146, 2);
        }
    } else {
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("--", SCREEN_CENTER_X, 100, 4);

        vnFont(spr, VN_SMALL);
        spr.drawString("BPM", SCREEN_CENTER_X, 126, 1);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        if (g_watchState.motionArtifact) {
            spr.drawString("GIỮ YÊN TAY", SCREEN_CENTER_X, 146, 2);
        } else if (!g_watchState.skinContact) {
            spr.drawString("CHƯA CHẠM DA", SCREEN_CENTER_X, 146, 2);
        } else {
            spr.drawString("ĐANG TÍNH...", SCREEN_CENTER_X, 146, 2);
        }
    }

    // Signal Quality Progress Bar
    if (g_watchState.hrSensorOk) {
        int qw = (g_watchState.signalQuality * 110) / 100;
        if (qw > 110) qw = 110;
        uint16_t qcol = g_watchState.signalQuality >= 60 ? UI_COLOR_NORMAL
                      : g_watchState.signalQuality >= 30 ? UI_COLOR_WARNING
                                                         : UI_COLOR_CRITICAL;
        spr.drawRoundRect(65, 166, 110, 6, 3, UI_COLOR_BEZEL);
        if (qw > 2) spr.fillRoundRect(65, 166, qw, 6, 3, qcol);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        char qBuf[32];
        snprintf(qBuf, sizeof(qBuf), "Chất lượng: %d%%", g_watchState.signalQuality);
        spr.drawString(qBuf, SCREEN_CENTER_X, 182, 1);
    }

    // Mini Trend Line Chart
    int chartX = 65;
    int chartY = 192;
    int chartW = 110;
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
    spr.drawString("← Trang chủ", SCREEN_CENTER_X, 218, 1);
}
