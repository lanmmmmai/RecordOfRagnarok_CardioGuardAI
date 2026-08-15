# .CLAUDE/Plan/ — 🗄️ ĐÃ NGHỈ HƯU (SUPERSEDED)

> **Toàn bộ 9 file trong thư mục này không còn hiệu lực. Đừng làm theo chúng.**
>
> **Nguồn sự thật về kiến trúc:** [SYSTEM_ARCHITECTURE_SPEC.md](../../SYSTEM_ARCHITECTURE_SPEC.md)
> **Nguồn sự thật về hành vi:** mã nguồn trong [src/](../../src/) và [include/](../../include/)
> **Việc còn lại:** [SYSTEM_ARCHITECTURE_SPEC.md §11](../../SYSTEM_ARCHITECTURE_SPEC.md#11-hạn-chế-đã-biết--lộ-trình)

---

## Vì sao nghỉ hưu

8 file task này là bản chia nhỏ của [IMPLEMENTATION_PLAN.md](../../IMPLEMENTATION_PLAN.md) —
bản kế hoạch **chưa từng build được và chưa từng chạy trên phần cứng**.
Chúng mô tả một hệ thống khác hẳn firmware đang chạy thật trong `src/`:

| File | Mô tả trong plan | Thực tế trong code |
|---|---|---|
| `01_project_scaffolding.md` | Đường dẫn `firmware/src/...` | Không có thư mục `firmware/`. Code nằm thẳng ở [src/](../../src/) |
| `02_battery_monitor.md` | Cảnh báo pin yếu | **Chưa triển khai.** Chỉ có đo điện áp; đường cong pin còn là ánh xạ tuyến tính |
| `03_max30102_dsp_pipeline.md` | "DSP 5 tầng" đã hoàn thành | Chỉ có tầng 1–3. **Tầng 4+5 chưa triển khai** — xem SPEC §3 |
| `04_qmi8658_fall_detection.md` | Thuật toán 4 giai đoạn | Code thật làm **đủ 4 pha + 2 đường vào**; plan chỉ làm 2 pha |
| `05_tinyml_ai_models.md` | Kaggle + TFLite INT8 | **Chưa triển khai.** Hướng đi đã đổi: `emlearn` sinh C thuần, dataset UMAFall/FallAllD + MIT-BIH `afdb` — xem SPEC §9 |
| `06_lvgl_ui_screens.md` | LVGL v8 | **Không dùng LVGL.** Giao diện chạy trên TFT_eSPI + sprite 240×240 + font VLW tiếng Việt |
| `07_wifi_telegram_client.md` | Gửi Telegram đơn giản | Code thật có thêm hàng đợi NVS 8 sự kiện, retry 3 lần, `jsonEscape()` — plan bỏ hết |
| `08_freertos_main_integration.md` | Đa luồng FreeRTOS lõi kép | **Không có FreeRTOS task nào.** [main.cpp](../../src/main.cpp) là một vòng lặp hợp tác duy nhất |

Chi tiết đầy đủ về những lỗi khiến bản kế hoạch không dùng được (thiếu `lv_conf.h`,
`psram_type = opi` sai chế độ PSRAM, `WiFi.begin()` không được gọi, `_ax/_ay/_az` không
bao giờ được đọc, JSON có newline thô…) nằm ở
[IMPLEMENTATION_PLAN.md](../../IMPLEMENTATION_PLAN.md).

---

## Vì sao giữ lại, không xoá

Ba lý do:

1. **Bằng chứng lịch sử.** Nó cho thấy chính xác kiểu sai lầm nào xuất hiện khi kế hoạch
   được viết ra mà không bao giờ được biên dịch thử.
2. **Tra cứu cho Giai đoạn 4.** Lỗi `psram_type = opi` sẽ quay lại đúng lúc chuyển sang
   ESP-IDF nếu không cẩn thận — bo này dùng PSRAM **quad**, không phải octal.
3. **Vẫn còn ba ý dùng được**, đã được đưa vào lộ trình thật:
   - Chặn tốc độ đổi nhịp ≤ 15 BPM/s + trung vị 5 mẫu → **DSP tầng 4**, Giai đoạn 5
   - Kalman 1D + kiểm định SQI → **DSP tầng 5**, cùng giai đoạn
   - Nút SOS vật lý BOOT (GPIO 0) giữ 1.5s → **Giai đoạn 6**

---

## Danh sách file (đều đã nghỉ hưu)

1. [01_project_scaffolding.md](01_project_scaffolding.md)
2. [02_battery_monitor.md](02_battery_monitor.md)
3. [03_max30102_dsp_pipeline.md](03_max30102_dsp_pipeline.md)
4. [04_qmi8658_fall_detection.md](04_qmi8658_fall_detection.md)
5. [05_tinyml_ai_models.md](05_tinyml_ai_models.md)
6. [06_lvgl_ui_screens.md](06_lvgl_ui_screens.md)
7. [07_wifi_telegram_client.md](07_wifi_telegram_client.md)
8. [08_freertos_main_integration.md](08_freertos_main_integration.md)
