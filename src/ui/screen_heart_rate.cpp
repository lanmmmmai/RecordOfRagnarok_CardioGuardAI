#include "screen_heart_rate.h"
#include "vn_font.h"

void renderHeartRateScreen(TFT_eSprite& spr) {
    spr.fillSprite(UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    // Header Title
    vnFont(spr, VN_MEDIUM);
    spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
    spr.drawString("NHỊP TIM & SpO2", SCREEN_CENTER_X, 22, 2);

    // Dual Icons (Heart + Oxygen)
    spr.fillCircle(SCREEN_CENTER_X - 18, 40, 6, UI_COLOR_HEART);
    spr.fillCircle(SCREEN_CENTER_X - 6, 40, 6, UI_COLOR_HEART);
    spr.fillTriangle(SCREEN_CENTER_X - 23, 42, SCREEN_CENTER_X - 1, 42, SCREEN_CENTER_X - 12, 54, UI_COLOR_HEART);

    spr.drawCircle(SCREEN_CENTER_X + 12, 46, 7, UI_COLOR_SPO2);

    char bpmBuf[16];
    char spo2Buf[16];

    if (!g_watchState.hrSensorOk) {
        spr.setTextColor(UI_COLOR_CRITICAL, UI_COLOR_BLACK);
        vnFont(spr, VN_LARGE);
        spr.drawString("--", SCREEN_CENTER_X, 82, 4);
        vnFont(spr, VN_MEDIUM);
        spr.drawString("LỖI CẢM BIẾN", SCREEN_CENTER_X, 114, 2);
    } else if (g_watchState.hrValid || g_watchState.spo2Valid) {
        // Render BPM
        snprintf(bpmBuf, sizeof(bpmBuf), "%d", g_watchState.heartRateBPM);
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
        spr.drawString(bpmBuf, 78, 80, 4);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_HEART, UI_COLOR_BLACK);
        spr.drawString("BPM", 78, 106, 1);

        // Render SpO2
        if (g_watchState.spo2Valid && g_watchState.spo2Percent > 0) {
            snprintf(spo2Buf, sizeof(spo2Buf), "%d%%", g_watchState.spo2Percent);
        } else {
            snprintf(spo2Buf, sizeof(spo2Buf), "--%%");
        }
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_SPO2, UI_COLOR_BLACK);
        spr.drawString(spo2Buf, 162, 80, 4);

        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SPO2, UI_COLOR_BLACK);
        spr.drawString("SpO2", 162, 106, 1);

        // Clinical Evaluation
        vnFont(spr, VN_MEDIUM);
        if (g_watchState.heartRateBPM > 100) {
            spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
            spr.drawString("NHỊP HƠI NHANH", SCREEN_CENTER_X, 130, 2);
        } else if (g_watchState.heartRateBPM < 55 && g_watchState.heartRateBPM > 0) {
            spr.setTextColor(UI_COLOR_BLUE, UI_COLOR_BLACK);
            spr.drawString("NHỊP HƠI CHẬM", SCREEN_CENTER_X, 130, 2);
        } else {
            spr.setTextColor(UI_COLOR_NORMAL, UI_COLOR_BLACK);
            spr.drawString("CHỈ SỐ BÌNH THƯỜNG", SCREEN_CENTER_X, 130, 2);
        }
    } else {
        vnFont(spr, VN_LARGE);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        spr.drawString("--", 78, 80, 4);
        spr.drawString("--%", 162, 80, 4);

        vnFont(spr, VN_SMALL);
        spr.drawString("BPM", 78, 106, 1);
        spr.drawString("SpO2", 162, 106, 1);

        vnFont(spr, VN_MEDIUM);
        spr.setTextColor(UI_COLOR_WARNING, UI_COLOR_BLACK);
        if (g_watchState.motionArtifact) {
            spr.drawString("GIỮ YÊN TAY", SCREEN_CENTER_X, 130, 2);
        } else if (!g_watchState.skinContact) {
            spr.drawString("CHƯA CHẠM DA", SCREEN_CENTER_X, 130, 2);
        } else {
            spr.drawString("ĐANG TÍNH...", SCREEN_CENTER_X, 130, 2);
        }
    }

    // Signal Quality Progress Bar
    if (g_watchState.hrSensorOk) {
        int qw = (g_watchState.signalQuality * 110) / 100;
        if (qw > 110) qw = 110;
        uint16_t qcol = g_watchState.signalQuality >= 60 ? UI_COLOR_NORMAL
                      : g_watchState.signalQuality >= 30 ? UI_COLOR_WARNING
                                                         : UI_COLOR_CRITICAL;
        spr.drawRoundRect(65, 150, 110, 6, 3, UI_COLOR_BEZEL);
        if (qw > 2) spr.fillRoundRect(65, 150, qw, 6, 3, qcol);
        
        vnFont(spr, VN_SMALL);
        spr.setTextColor(UI_COLOR_SECONDARY, UI_COLOR_BLACK);
        char qBuf[32];
        snprintf(qBuf, sizeof(qBuf), "Chất lượng SQI: %d%%", g_watchState.signalQuality);
        spr.drawString(qBuf, SCREEN_CENTER_X, 166, 1);
    }

    // Mini Trend Line Chart
    int chartX = 65;
    int chartY = 178;
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
    spr.drawString("← Trang chủ", SCREEN_CENTER_X, 210, 1);
}
