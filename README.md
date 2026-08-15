# CardioGuardAI 🛡️❤️

**Đồng hồ đeo tay phát hiện té ngã và theo dõi sức khoẻ, chạy trên Waveshare ESP32-S3-Touch-LCD-1.28-B.**
Khi phát hiện cú ngã, đồng hồ đếm ngược 15 giây trên màn hình cảm ứng; nếu người đeo không bấm HUỶ,
nó gửi cảnh báo vào nhóm Telegram của gia đình và báo sang điện thoại qua BLE.

> ⚠️ **Đây là đồ án học tập, không phải thiết bị y tế.** Các chỉ số nhịp tim và SpO₂ chỉ để tham khảo,
> không dùng để chẩn đoán hay điều trị. Xem [mục Miễn trừ](#-miễn-trừ) ở cuối trang.

---

## 📊 Trạng thái thật của dự án

Bảng này là **sự thật về code trong `src/`**, không phải danh sách mong muốn.
Mỗi dòng "Đang chạy" đều trỏ được tới file cụ thể.

| Hạng mục | Trạng thái | Ở đâu |
|---|---|---|
| Giao diện 7 màn hình, cảm ứng, font tiếng Việt có dấu | ✅ Đang chạy | [src/ui/](src/ui/) |
| Phát hiện té ngã 4 pha + 2 đường vào | ✅ Đang chạy | [fall_detector.cpp](src/fall_detection/fall_detector.cpp) |
| Đếm ngược 15s + nút HUỶ + SOS thủ công | ✅ Đang chạy | [screen_fall_alert.cpp](src/ui/screen_fall_alert.cpp) (vẽ) · [ui_manager.cpp](src/ui/ui_manager.cpp) (chạm) |
| Gửi Telegram + hàng đợi NVS 8 sự kiện + retry 3 lần | ✅ Đang chạy | [alert_dispatcher.cpp](src/connectivity/alert_dispatcher.cpp) |
| BLE GATT: vitals / fall / status / command | ✅ Đang chạy | [ble_service.cpp](src/connectivity/ble_service.cpp) · [BLE_PROTOCOL.md](BLE_PROTOCOL.md) |
| Đo nhịp tim + SpO₂ (PPG) | ✅ Đang chạy | [max30102_service.cpp](src/sensors/max30102_service.cpp) |
| Đo pin qua ADC | ✅ Đang chạy | [battery_monitor.cpp](src/sensors/battery_monitor.cpp) |
| **Hiệu chuẩn ngưỡng té ngã** | ⚠️ Chưa — toàn số phỏng đoán | [app_config.h §Fall](include/app_config.h) |
| **DSP tầng 4** — trung vị + chặn 15 BPM/s | ⚠️ Có, nhưng **thiếu van thoát** — xem cảnh báo dưới | [max30102_service.cpp:153](src/sensors/max30102_service.cpp#L153) |
| **DSP tầng 5** (Kalman + chặn hiển thị theo SQI) | ❌ Chưa triển khai | Kế hoạch: SPEC §11 |
| **Lấy mẫu PPG 200 Hz** (hiện 25 Hz hiệu dụng) | ❌ Chưa triển khai | Kế hoạch: SPEC §11 |
| **Mô hình TinyML** (té ngã + sàng lọc nhịp) | ❌ Chưa triển khai | Kế hoạch: SPEC §9 |
| **Nút SOS vật lý, cảnh báo pin yếu, cảnh báo ngưỡng sinh lý** | ❌ Chưa triển khai | Kế hoạch: SPEC §11 |

> 🔴 **Lỗi đã biết, chưa sửa — bộ chặn nhịp tim có thể khoá cứng vĩnh viễn.**
> [max30102_service.cpp:154](src/sensors/max30102_service.cpp#L154) loại mọi nhịp lệch quá
> 15 BPM so với giá trị đang hiển thị, nhưng **không có van thoát**. Nếu nhịp tim thật tăng
> vọt (lên cơn nhịp nhanh, gắng sức), mọi nhịp mới đều bị loại và số trên màn hình **đứng yên
> mãi ở giá trị cũ** cho tới khi mất tiếp xúc da. Đúng lúc cần đo nhất thì thiết bị lại nói dối.
> Cách sửa: đếm số lần bị loại liên tiếp, quá 8 lần thì buộc chấp nhận giá trị mới. Xếp vào Giai đoạn 5.

**Danh sách hạn chế đầy đủ, xếp theo mức nghiêm trọng:**
[SYSTEM_ARCHITECTURE_SPEC.md §11](SYSTEM_ARCHITECTURE_SPEC.md#11-hạn-chế-đã-biết--lộ-trình)

Firmware hiện tại chiếm **RAM 17.3%** và **Flash 40.5%**.

---

## 🔩 Phần cứng

| Thành phần | Chi tiết |
|---|---|
| Bo mạch | Waveshare **ESP32-S3-Touch-LCD-1.28-B** |
| MCU | ESP32-S3**R2** — 2 MB PSRAM chế độ **quad** (không phải octal) |
| Flash | W25Q128JVSIQ 16 MB, QIO @ 80 MHz |
| Màn hình | GC9A01 tròn 240×240, SPI |
| Cảm ứng | CST816S @ `0x15` (I2C bus 1) |
| IMU | QMI8658 6 trục @ `0x6B` (I2C bus 1), cấu hình ±8g |
| **PPG** | **MAX30102 — module rời MH-ET LIVE, KHÔNG có sẵn trên bo** |
| Pin | LiPo, đầu nối **MX1.25 2P**, sạc qua ETA6096 |
| Ổn áp | ME6217C33M5G, ~800 mA — đây là trần ngân sách dòng khi gắn thêm module |

### Đấu nối module MAX30102

Bo Waveshare **không có** cảm biến PPG. Phải gắn thêm một module MAX30102 rời vào
đầu nối mở rộng **SH1.0**, chạy trên một bus I2C thứ hai bằng phần mềm:

| Chân module MAX30102 | Nối vào SH1.0 | Ghi chú |
|---|---|---|
| VIN | `3V3` | Module MH-ET LIVE đã có sẵn LDO 3.3V |
| GND | `GND` | |
| SDA | `GPIO 15` | I2C bus 2 |
| SCL | `GPIO 16` | I2C bus 2 |
| INT | *không nối* | Firmware đọc theo kiểu polling |

Địa chỉ I2C của cảm biến là `0x57`. Bốn chân mở rộng còn lại (`17`, `18`, `21`, `33`) vẫn trống.

> 💡 Nếu bạn không gắn module này, firmware vẫn boot và chạy bình thường — màn hình
> sẽ báo `PPG: FAIL` và nhịp tim/SpO₂ hiện `--`. Phát hiện té ngã không phụ thuộc vào PPG.

---

## 🚀 Build & nạp

Cần [PlatformIO](https://platformio.org/) (khuyên dùng qua VS Code).

**1. Tạo file bí mật.** Repo cố tình không chứa mật khẩu:

```bash
cp include/secrets.h.example include/secrets.h
```

Mở `include/secrets.h` và điền SSID Wi-Fi, mật khẩu, token bot Telegram và Chat ID.
File này nằm trong `.gitignore` — **đừng bao giờ commit nó**.

**2. Sửa cổng COM** trong [platformio.ini](platformio.ini) (`upload_port` / `monitor_port`) cho khớp máy bạn.

**3. Build và nạp:**

```bash
pio run --target upload
pio device monitor
```

**4. Đối chiếu log khởi động.** Cứ 2 giây firmware in một khối trạng thái.
Đây là công cụ chẩn đoán chính, và cũng là nơi lấy số để hiệu chuẩn:

```text
==========================================================================
 [WATCH LOG] 2026-08-15 14:22:07 | WiFi: CONNECTED (192.168.1.42) | BLE: AWAY | BAT: 87% (4.02V, raw 1340 mV)
--------------------------------------------------------------------------
 [SENSOR] IMU: OK | PPG: OK | TOUCH: OK | Free heap: 198432 B
 [HEALTH] Status: NO SKIN CONTACT (IR: 21900, threshold: 25000)
 [MOTION] Accel (   120,   -84,  4050) | Gyro (    -2,     5,     1)
 [FALL]   Status: OK (NORMAL) | Countdown: 15s | Queued alerts: 0
 [TOUCH]  State: IDLE
==========================================================================
```

Cả ba cảm biến phải hiện `OK`. Nếu `PPG: FAIL` → kiểm tra lại đấu nối GPIO 15/16 ở trên.

---

## 🛠️ Luồng hoạt động

```text
+-----------------------------------------------------------------------+
|                    ESP32-S3 (một vòng lặp hợp tác)                    |
|  QMI8658 @ 50Hz  ->  Té ngã 4 pha: rơi tự do -> va chạm ->            |
|                      nằm yên 3s -> đổi tư thế >30 deg                 |
|  MAX30102 @ 25Hz ->  Nhịp tim + SpO2 (DSP tầng 1-3)                   |
+-----------------------------------------------------------------------+
                                   |
        (Té ngã đã xác nhận  |  SOS thủ công  |  Lệnh SOS từ điện thoại)
                                   v
+-----------------------------------------------------------------------+
|            Màn hình đỏ nhấp nháy + đếm ngược 15 giây                  |
|                  [ Nút HUỶ lớn trên cảm ứng ]                         |
|         (đồng thời gửi ngay BLE INDICATE cho điện thoại)              |
+-----------------------------------------------------------------------+
           |                                             |
     (Bấm HUỶ)                                    (Hết 15 giây)
           v                                             v
+-----------------------+          +------------------------------------+
| ✓ Trở về bình thường  |          |  Telegram Bot API qua HTTPS        |
+-----------------------+          |  - thất bại: retry tối đa 3 lần    |
                                   |  - mất mạng: xếp hàng vào NVS (8)  |
                                   +------------------------------------+
                                                         |
                                                         v
                                          +--------------------------+
                                          |  📱 Nhóm Telegram gia đình |
                                          +--------------------------+
```

Cảnh báo đi ra **hai đường độc lập**: BLE bắn ngay khi bắt đầu đếm ngược (để điện thoại
biết cả khi Wi-Fi chết), Telegram bắn khi hết 15 giây. Ngoài ra khi điện thoại rời khỏi
tầm BLE quá 60 giây, đồng hồ tự gửi Telegram báo mất kết nối.

---

## 📁 Cấu trúc thư mục

```text
RecordOfRagnarok_CardioGuardAI/
├── platformio.ini              # Cấu hình build, chân TFT_eSPI, lib_deps
├── include/
│   ├── app_config.h            # Chân GPIO + TOÀN BỘ ngưỡng, kèm ghi chú hiệu chuẩn
│   ├── app_state.h             # struct WatchState — trạng thái dùng chung
│   ├── ui_config.h             # Bảng màu ngữ nghĩa
│   ├── secrets.h.example       # Mẫu; copy thành secrets.h (đã git-ignore)
│   └── fonts/                  # Font VLW tiếng Việt biên dịch thành mảng C
├── src/
│   ├── main.cpp                # setup() + một loop() hợp tác, không FreeRTOS task
│   ├── sensors/                # battery, qmi8658, max30102, cst816s
│   ├── fall_detection/         # Máy trạng thái té ngã 4 pha
│   ├── connectivity/           # wifi_manager, alert_dispatcher, ble_service
│   └── ui/                     # ui_manager + 7 màn hình + vn_font
├── tools/
│   ├── make_vlw.py             # Sinh font VLW tiếng Việt
│   └── check_fit.py            # Kiểm tra chuỗi có tràn màn hình tròn không
└── docs/                       # Tài liệu phần cứng, hướng dẫn test, đặc tả UI
```

Không có FreeRTOS task nào. `loop()` chạy hợp tác theo mốc thời gian:
cảm ứng mỗi vòng · cảm biến 20 ms · vẽ 33 ms (~30 FPS) · pin + Wi-Fi 5 s · log 2 s.

---

## 📖 Tài liệu

| File | Nội dung |
|---|---|
| [SYSTEM_ARCHITECTURE_SPEC.md](SYSTEM_ARCHITECTURE_SPEC.md) | **Nguồn sự thật về kiến trúc.** 11 mục, mỗi khẳng định đều trỏ được tới dòng code — hoặc đánh dấu rõ `CHƯA TRIỂN KHAI` |
| [BLE_PROTOCOL.md](BLE_PROTOCOL.md) | Đặc tả GATT cho ứng dụng điện thoại: UUID, byte offset, ý nghĩa từng bit |
| [docs/ESP32-S3-Touch-LCD-1.28-B_Technical_Overview.md](docs/ESP32-S3-Touch-LCD-1.28-B_Technical_Overview.md) | Phân tích phần cứng bo mạch |
| [docs/HARDWARE_TEST_GUIDE.md](docs/HARDWARE_TEST_GUIDE.md) | Quy trình kiểm tra từng cảm biến |
| [docs/UI_SPEC_Smart_Health_Fall_Detection_Watch.md](docs/UI_SPEC_Smart_Health_Fall_Detection_Watch.md) | Đặc tả giao diện |
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | 🗄️ Đã nghỉ hưu — giữ lại để tra cứu bài học |
| [.CLAUDE/Plan/](.CLAUDE/Plan/) | 🗄️ Đã nghỉ hưu — mô tả kiến trúc LVGL/FreeRTOS chưa từng tồn tại |

---

## 🗺️ Lộ trình

| Giai đoạn | Nội dung |
|---|---|
| ~~0–3~~ | ~~Xử lý bí mật rò rỉ · đưa code vào repo · sửa lỗi té ngã · viết lại tài liệu~~ ✅ |
| **3b** | Hiệu chuẩn ngưỡng té ngã bằng thử nghiệm thật (thả xuống đệm, đọc dòng quyết định trong log) |
| **5** | DSP tầng 4+5 · nâng PPG lên 200 Hz để đo được khoảng RR |
| **6** | Nút SOS vật lý · cảnh báo pin yếu · cảnh báo ngưỡng sinh lý |
| **7** | TinyML: cây quyết định té ngã (UMAFall/FallAllD + dữ liệu tự thu) · sàng lọc khoảng RR (MIT-BIH afdb) |
| **8** | Đánh giá: ma trận nhầm lẫn, độ nhạy/đặc hiệu, ROC, so sánh với baseline 4 pha |
| **4** | Chuyển framework sang `arduino, espidf` (làm sau cùng) |

Chi tiết kỹ thuật và lý do của từng hạng mục nằm ở
[SYSTEM_ARCHITECTURE_SPEC.md §9 và §11](SYSTEM_ARCHITECTURE_SPEC.md).

---

## ⚕️ Miễn trừ

CardioGuardAI **không phải thiết bị y tế** và chưa được kiểm định lâm sàng.

- Nhịp tim và SpO₂ đo bằng cảm biến quang học ở cổ tay — vị trí có ít mao mạch,
  dễ nhiễu do cử động, và độ chính xác thấp hơn máy đo kẹp ngón tay rõ rệt.
- Ngưỡng phát hiện té ngã **chưa được hiệu chuẩn**. Thiết bị sẽ vừa bỏ sót cú ngã thật,
  vừa báo động nhầm. Đừng dựa vào nó như biện pháp an toàn duy nhất cho người thân.
- Mọi tính năng liên quan tới nhịp tim bất thường là **gợi ý sàng lọc**, không phải chẩn đoán.
  Có triệu chứng thì đi khám, đừng hỏi cái đồng hồ.

---

*Dự án CardioGuardAI — Record of Ragnarok.*
