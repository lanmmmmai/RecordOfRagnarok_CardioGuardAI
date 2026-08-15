# IMPLEMENTATION_PLAN.md — 🗄️ ĐÃ THAY THẾ (SUPERSEDED)

> **Tài liệu này không còn hiệu lực. Đừng làm theo nó.**
>
> Bản kế hoạch cũ (610 dòng, sinh ngày 15/08/2026) mô tả một firmware **chưa từng build được**
> và **chưa từng chạy trên phần cứng**. Trong khi đó một firmware khác đã được viết, đã chạy thật
> trên chính bo mạch này, và nay nằm trong `src/` của repo.
>
> **Nguồn sự thật duy nhất về kiến trúc:** [SYSTEM_ARCHITECTURE_SPEC.md](SYSTEM_ARCHITECTURE_SPEC.md)
> **Nguồn sự thật duy nhất về hành vi:** mã nguồn trong [src/](src/) và [include/](include/)
> **Danh sách việc còn lại:** [SYSTEM_ARCHITECTURE_SPEC.md §11](SYSTEM_ARCHITECTURE_SPEC.md#11-hạn-chế-đã-biết--lộ-trình)

---

## Vì sao cho nghỉ hưu

File được giữ lại (không xoá) vì lịch sử của nó có giá trị: nó cho thấy chính xác kiểu sai lầm nào
xuất hiện khi kế hoạch được viết ra mà **không bao giờ được biên dịch thử**. Bảng dưới đây là để tra cứu
khi làm Giai đoạn 4 (đổi framework) — vài lỗi trong này sẽ quay lại nếu không cẩn thận.

| Loại | Vấn đề cụ thể trong bản kế hoạch cũ |
|---|---|
| **Không build được** | `lib_deps` khai `lvgl@^8.3.11` nhưng **không có `lv_conf.h`** — LVGL không biên dịch được nếu thiếu file cấu hình này. Không có driver GC9A01 nào trong `lib_deps` (LVGL chỉ vẽ vào buffer, không tự đẩy ra màn hình). Không có `build_flags` khai chân SPI. Không có bảng phân vùng cho Flash 16MB. |
| **Không boot được** | `board_build.psram_type = opi` — bo mạch dùng **ESP32-S3R2, PSRAM chế độ quad**, không phải octal. Cấu hình sai chỗ này thì chip panic ngay lúc khởi động. ⚠️ *Đây đúng là loại lỗi cần đặc biệt cẩn thận ở Giai đoạn 4.* |
| **Không chạy được** | `main.cpp` **không hề gọi `WiFi.begin()`** — không có một dòng kết nối Wi-Fi nào, nhưng `TelegramBot::sendEmergencyAlert()` lại mở đầu bằng `if (WiFi.status() != WL_CONNECTED) return false;` → **mọi cảnh báo im lặng thất bại**. `batteryMon.update()` không bao giờ được gọi (chỉ có `begin()`) → phần trăm pin đứng yên vĩnh viễn. |
| **Cảm biến không đọc** | `QMI8658Service::update()` tính `sv = sqrt(_ax²+_ay²+_az²)` nhưng **không có lệnh đọc I2C nào** — `_ax/_ay/_az` không bao giờ được gán. `sv` luôn bằng hằng số khởi tạo → **phát hiện té ngã không bao giờ kích hoạt**. |
| **Logic hỏng** | Không có timer đếm ngược: `countdownRemaining` được gán một lần rồi **không bao giờ giảm**. `cancel_btn_cb()` chỉ gọi `UI::showHomeScreen()` mà **không reset `fallState`** → hệ thống kẹt vĩnh viễn ở `FALL_ALERT_COUNTDOWN`, không bao giờ phát hiện được cú ngã thứ hai. |
| **Telegram hỏng** | Chuỗi `message` chứa **ký tự xuống dòng thô** (`"\n"` trong C++ là byte 0x0A thật), rồi được nhét thẳng vào `"\"text\":\"" + message + "\""`. Xuống dòng thô bên trong một chuỗi JSON là **JSON không hợp lệ** → Telegram trả **HTTP 400**. Không có hàm escape nào. |
| **Bảo mật** | Token Telegram và mật khẩu Wi-Fi đặt thẳng trong `app_config.h` rồi commit lên repo **công khai**. Đây chính là nguyên nhân phải làm Giai đoạn 0 (viết lại lịch sử git + `.gitignore`). |
| **Giả mạo hoàn thành** | "DSP 5 tầng" trong `MAX30102Service::update()` thực chất là `uint16_t rawBPM = 75;` với comment `// Computed from PPG peak interval`, cộng `_cleanSpO2 = 98; _sqi = 90;` — **ba giá trị hardcode**. Bộ lọc trung vị chạy trên một hằng số. |
| **Thụt lùi so với code thật** | Bản kế hoạch loại bỏ những thứ code thật đã có và đã hoạt động: `jsonEscape()`, hàng đợi NVS 8 sự kiện, cơ chế retry 3 lần, `checkSensorAlive()`, motion gating theo gia tốc, sprite double-buffer, font VLW tiếng Việt. |

### Điểm đáng chú ý nhất

Thuật toán té ngã: đặc tả yêu cầu **4 pha nối tiếp**. Code thật ([fall_detector.cpp](src/fall_detection/fall_detector.cpp))
làm **đủ cả 4** (rơi tự do → va chạm → nằm yên → đổi tư thế), còn bản kế hoạch chỉ làm **2** — bỏ hẳn pha rơi tự do
và pha đổi tư thế. Bản kế hoạch tự nhận đã hoàn thành hạng mục này.

---

## Những gì còn dùng được từ file cũ

Không phải mọi thứ đều sai. Ba ý dưới đây đã được **giữ lại và đưa vào lộ trình thật**:

1. **Rate-of-Change Gate ≤ 15 BPM/s + Median 5 mẫu** → trở thành **DSP Tầng 4**, Giai đoạn 5 / Phần B.
   (Có bổ sung "van thoát 8 lần liên tiếp" mà bản cũ thiếu — nếu không, đúng lúc lên cơn nhịp nhanh thiết bị sẽ khoá cứng ở số cũ.)
2. **Kalman 1D + kiểm định SQI** → trở thành **DSP Tầng 5**, cùng giai đoạn.
3. **Nút SOS vật lý BOOT (GPIO 0) giữ 1.5s** → Giai đoạn 6 / Phần C.
   (Kèm cảnh báo bản cũ không nêu: GPIO 0 là **chân strapping**, chỉ được đọc **sau khi `setup()` xong**.)

---

## Bài học rút ra

Một bản kế hoạch chưa được biên dịch thử thì **không phải bằng chứng** rằng thứ gì đó chạy được.
Từ nay, mọi hạng mục trong [SYSTEM_ARCHITECTURE_SPEC.md](SYSTEM_ARCHITECTURE_SPEC.md) hoặc trỏ tới
một dòng code cụ thể, hoặc được đánh dấu rõ **`CHƯA TRIỂN KHAI`**. Không có vùng xám ở giữa.

---

*Nội dung gốc 610 dòng vẫn còn trong lịch sử git. Xem:*
`git show v0.1-arduino-baseline:IMPLEMENTATION_PLAN.md`
