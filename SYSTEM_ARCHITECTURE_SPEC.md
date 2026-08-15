# System Architecture & Detailed Technical Specification
## Project: CardioGuardAI (RecordOfRagnarok_CardioGuardAI)
> **Đặc tả Kiến trúc Hệ thống, Thuật toán Xử lý tại Biên & Lộ trình TinyML**

> **Tài liệu này mô tả phần cứng và phần mềm ĐANG CHẠY THẬT trong `src/`.**
> Mọi hạng mục chưa có trong code đều được đánh dấu **`CHƯA TRIỂN KHAI`** ngay tại chỗ,
> và tập hợp lại ở [§11 — Hạn chế đã biết & Lộ trình](#11-hạn-chế-đã-biết--lộ-trình).
> Nếu bạn thấy một câu trong tài liệu này mà không tìm được dòng code tương ứng, đó là lỗi tài liệu — hãy báo.

---

## 📄 Mục lục
1. [Tổng quan Hệ thống & Mục tiêu Cốt lõi](#1-tổng-quan-hệ-thống--mục-tiêu-cốt-lõi)
2. [Phần cứng Chi tiết & Sơ đồ Chân GPIO](#2-phần-cứng-chi-tiết--sơ-đồ-chân-gpio)
3. [Chuỗi Xử lý Tín hiệu Nhịp tim & SpO2 tại Biên](#3-chuỗi-xử-lý-tín-hiệu-nhịp-tim--spo2-tại-biên)
4. [Thuật toán Té ngã 3D 4 Pha](#4-thuật-toán-té-ngã-3d-4-pha)
5. [Đặc tả Giao diện TFT_eSPI & Cảnh báo Nhấp nháy](#5-đặc-tả-giao-diện-tft_espi--cảnh-báo-nhấp-nháy)
6. [Kịch bản Báo động SOS Khẩn cấp Thủ công](#6-kịch-bản-báo-động-sos-khẩn-cấp-thủ-công)
7. [Mạch Quản lý Pin & Đo Điện áp ADC](#7-mạch-quản-lý-pin--đo-điện-áp-adc)
8. [Giao thức Gửi Cảnh báo Trực tiếp qua Telegram Bot API](#8-giao-thức-gửi-cảnh-báo-trực-tiếp-qua-telegram-bot-api)
9. [Lộ trình TinyML (CHƯA TRIỂN KHAI)](#9-lộ-trình-tinyml-chưa-triển-khai)
10. [Cấu trúc Mã nguồn PlatformIO](#10-cấu-trúc-mã-nguồn-platformio)
11. [Hạn chế đã biết & Lộ trình](#11-hạn-chế-đã-biết--lộ-trình)

---

## 1. Tổng quan Hệ thống & Mục tiêu Cốt lõi

**CardioGuardAI** là hệ thống AIoT giám sát sức khỏe, đo nhịp tim/SpO2 và cảnh báo té ngã khẩn cấp, đeo ở **cổ tay**, hướng tới người cao tuổi.

Hệ thống hoạt động theo tiêu chí **Tối giản – Tin cậy – Xử lý tại Biên (Edge Computing)**:

- **Xử lý hoàn toàn tại biên (ESP32-S3)**: dữ liệu PPG (MAX30102) và gia tốc (QMI8658) được lọc và tính toán ngay trên chip, không cần máy chủ trung gian. *(Phần TinyML trong §9 hiện **CHƯA TRIỂN KHAI**.)*
- **Cảnh báo tại chỗ**: màn hình nhấp nháy đỏ kèm 15 giây đếm ngược và nút **HỦY / CANCEL** cảm ứng kích thước lớn, để loại bỏ báo động giả.
- **Gửi tin nhắn trực tiếp qua Telegram**: hết 15s đếm ngược hoặc bấm SOS thủ công → đồng hồ gọi HTTPS Telegram Bot API bắn tin tới **Group Chat Telegram của Gia đình** (Chat ID cấu hình trong `include/secrets.h`), tất cả người thân nhận cùng lúc. Khi mất mạng, sự kiện được **xếp hàng trong NVS** và gửi lại sau (§8).
- **Kênh BLE song song**: sự kiện té ngã được `notify` qua BLE ngay trong lúc đếm ngược, trước cả Telegram — để nếu Wi-Fi hỏng thì điện thoại vẫn biết.

> ⚠️ **Đây không phải thiết bị y tế.** Không dùng để chẩn đoán, điều trị hay theo dõi bệnh lý. Xem §9 và §11.

---

## 2. Phần cứng Chi tiết & Sơ đồ Chân GPIO

Hệ thống chạy trên bo mạch **Waveshare ESP32-S3-Touch-LCD-1.28-B**.

> **Quan trọng:** cảm biến nhịp tim **MAX30102 KHÔNG có sẵn trên bo mạch**. Tài liệu phần cứng chính hãng
> ([docs/ESP32-S3-Touch-LCD-1.28-B_Technical_Overview.md](docs/ESP32-S3-Touch-LCD-1.28-B_Technical_Overview.md), §6 và §19)
> chỉ liệt kê **một** cảm biến on-board là QMI8658. MAX30102 trong dự án này là **module rời MH-ET LIVE nối ngoài**,
> chạy trên **bus I2C thứ hai** qua chân mở rộng SH1.0.

```text
               +-------------------------------------------------------+
               |        Waveshare ESP32-S3-Touch-LCD-1.28-B            |
               |                                                       |
               |  +-------------------------------------------------+  |
               |  |  ESP32-S3R2 Dual-Core 240MHz, 2MB PSRAM (quad)  |  |
               |  |  Flash ngoài W25Q128JVSIQ 16MB (QIO)            |  |
               |  +-------------------------------------------------+  |
               |        |          |          |            |           |
               +--------|----------|----------|------------|-----------+
                        |          |          |            |
    +-------------------+          |          |            +--------------------+
    | SPI (GC9A01 LCD)             |          |            I2C Bus 2 (SDA/SCL)  |
    v                              v          v                                 v
+---------------------+    +-------------+ +---------+     +-------------------------------+
| LCD Tròn 1.28 inch  |    | I2C Bus 1   | | Mạch Pin|     |  MODULE RỜI (nối ngoài SH1.0) |
| 240 x 240 pixels    |    | SDA=6 SCL=7 | | ADC     |     |  MAX30102 @ 0x57              |
| (GC9A01 Driver)     |    | - CST816S   | | (GPIO 1)|     |  SDA=15, SCL=16               |
+---------------------+    | - QMI8658   | +---------+     |  (Nhịp tim & SpO2)            |
                           +-------------+                 +-------------------------------+
```

### Bảng Chi tiết Chân Kết nối Phần cứng (Pin Mapping Table)

Mọi giá trị dưới đây khớp `#define` trong [include/app_config.h](include/app_config.h) và `build_flags` trong [platformio.ini](platformio.ini).

| Linh kiện Phần cứng | Model / Tên IC | Giao tiếp | Chân GPIO ESP32-S3 | Ghi chú |
|---|---|---|---|---|
| **Vi xử lý (MCU)** | **ESP32-S3R2** | System | Dual-core 240MHz | **2MB PSRAM chế độ quad**, không phải 8MB/octal |
| **Flash ngoài** | W25Q128JVSIQ | QIO SPI | — | 16MB @ 80MHz |
| **Màn hình LCD** | GC9A01 | SPI | MOSI=11, SCLK=10, CS=9, DC=8, RST=14, BL=2, MISO=12 | Tròn 1.28" (240×240 px) |
| **Mặt Cảm ứng** | CST816S | **I2C Bus 1** | SDA=6, SCL=7, INT=5, RST=13 — addr `0x15` | Điện dung. **Tên IC xác minh bằng thực nghiệm**: tài liệu Waveshare §5 không nêu tên IC cảm ứng |
| **Cảm biến Gia tốc** | QMI8658 | **I2C Bus 1** | SDA=6, SCL=7 — addr `0x6B` | 6 trục. Cấu hình **±8g @ 4096 LSB/g**, ODR 250Hz (`CTRL2 = 0x23`) |
| **Cảm biến Nhịp tim** | MAX30102 | **I2C Bus 2** | **SDA=15, SCL=16** — addr `0x57` | **Module rời**, có sẵn trở kéo 4.7k trên module |
| **Mạch đo Pin** | Phân áp trở | Analog Input | **GPIO 1** (ADC1) | Tỉ số phân áp **3.0** — xem §7 |
| **Mạch sạc Pin** | ETA6096 | Hardware | Giắc **MX1.25 2P** | Tự sạc khi cắm USB Type-C |
| **Nút BOOT** | Onboard | Digital Input | GPIO 0 (strapping) | **CHƯA TRIỂN KHAI** — không có `#define` nào cho GPIO 0 trong code |

### Ràng buộc nguồn điện

Bo mạch dùng ổn áp **ME6217C33M5G**, dòng ra khoảng **800 mA**. Đây là **trần ngân sách dòng** cho mọi module gắn thêm: LCD + backlight + Wi-Fi burst + BLE đã chiếm phần lớn, nên MAX30102 (LED drive `0x30` ≈ 12 mA khi cả 2 LED sáng) chiếm một phần dư địa. Thêm module mới phải tính lại.

### Chân mở rộng SH1.0 còn trống

`SH10_EXPANSION_PINS[] = {17, 18, 21, 33}` — GPIO 15 và 16 **đã bị chiếm** bởi I2C Bus 2 của MAX30102.

### Mô hình thực thi: MỘT VÒNG LẶP HỢP TÁC, KHÔNG CÓ FreeRTOS TASK

Không có `xTaskCreatePinnedToCore` nào trong dự án. Toàn bộ hệ thống chạy trong `loop()` của Arduino
([src/main.cpp](src/main.cpp)), chia nhịp bằng `millis()`:

| Việc | Chu kỳ | Gồm |
|---|---|---|
| Cảm ứng | mỗi vòng lặp | `updateCST816SService()` — rẻ, cổng bằng ngắt |
| Cảm biến | **20 ms (50 Hz)** | IMU → phát hiện té ngã → PPG |
| Dispatcher + BLE | mỗi vòng lặp | máy trạng thái không chặn |
| Đồng hồ | 1 s | đọc RTC (timeout 0, không chặn) |
| Pin + Wi-Fi | **5 s** | `updateBatteryMonitor()`, `updateWiFiManager()` |
| Vẽ màn hình | **33 ms (~30 FPS)** | `renderUI()` |
| Log Serial | 2 s | `printSerialLog()` |

Hệ quả cần biết: **mọi thứ chạy trên cùng một core, tuần tự.** Một hàm chặn lâu (ví dụ `getIR()` của
SparkFun chặn tới 250 ms) sẽ làm đứng hình cả giao diện lẫn bộ phát hiện té ngã — đó là lý do
[max30102_service.cpp](src/sensors/max30102_service.cpp) dùng `check()` + `available()` thay vì `getIR()/getRed()`.

---

## 3. Chuỗi Xử lý Tín hiệu Nhịp tim & SpO2 tại Biên

> **Trạng thái thật: tầng 1–4 đã có (tầng 4 vừa hoàn thiện van thoát). Tầng 5 CHƯA TRIỂN KHAI.**
> Mục này mô tả đúng những gì [src/sensors/max30102_service.cpp](src/sensors/max30102_service.cpp) đang làm.

### Tốc độ lấy mẫu thực tế

```cpp
particleSensor.setup(MAX30102_LED_BRIGHTNESS, 4, 2, 100, 411, 4096);
//                    brightness,  sampleAverage=4, ledMode=2, sampleRate=100Hz, ...
```

Cảm biến lấy **100 Hz** rồi **trung bình phần cứng 4 mẫu** → firmware chỉ nhận **25 Hz hiệu dụng**, tức **40 ms/mẫu**.
Đây chính xác là tốc độ mà thuật toán SpO2 của Maxim yêu cầu (100 mẫu @ 25Hz), nên với SpO2 thì đủ.
Nhưng với phân tích **khoảng RR** thì 40 ms là quá thô — xem §9 và §11.

```text
[Tín hiệu thô MAX30102 @ 100Hz  --hardware average x4-->  25Hz hiệu dụng]
         │
         ▼
 1. TẦNG 1: Khử DC + lọc thông thấp   ✅ CÓ (nằm trong thư viện SparkFun)
    ──> `checkForBeat()` nội bộ chạy averageDCEstimator (khử thành phần một chiều)
        rồi lowPassFIRFilter. Đây KHÔNG phải code của dự án — dự án không tự
        viết bộ lọc IIR bandpass nào.
    ──> Lưu ý: không có chuyện "loại bỏ nhiễu đèn điện 50/60Hz" bằng bộ lọc số.
        Nyquist của 25Hz chỉ là 12.5Hz, nên 50/60Hz đã bị aliasing TRƯỚC KHI bất kỳ
        bộ lọc số nào nhìn thấy nó. Việc khử nhiễu đèn do mạch ALC của MAX30102
        làm ở phần cứng.
         │
         ▼
 2. TẦNG 2: Cổng chặn theo Cử động (Motion Gating)   ✅ CÓ
    ──> Vòng đệm 16 mẫu độ lớn gia tốc từ QMI8658. Nếu độ lệch chuẩn > 0.08g
        (`PPG_MOTION_STD_G`) → BỎ QUA hoàn toàn mẫu đó, không dò nhịp, không nhận SpO2.
    ──> ĐÂY KHÔNG PHẢI NLMS adaptive filter. Không có phép trừ nhiễu thích nghi nào;
        chỉ là chặn/không chặn. Đơn giản hơn nhiều, nhưng là lý do quan trọng nhất
        khiến số đo ở cổ tay còn đáng tin.
         │
         ▼
 3. TẦNG 3: Dò đỉnh MIỀN THỜI GIAN   ✅ CÓ
    ──> `checkForBeat(ir)` → đo khoảng giữa 2 nhịp bằng millis() → BPM = 60000/delta.
    ──> Lọc thô: chỉ nhận 45 ≤ BPM ≤ 180. Bốn nhịp gần nhất chuyển sang Tầng 4.
    ──> ĐÂY KHÔNG PHẢI FFT. Không có biến đổi Fourier nào trong dự án. (Cố ý:
        FFT cần cửa sổ 8–16s, làm nhịp tim phản ứng chậm hẳn.)
         │
         ▼
 4. TẦNG 4: Loại Số Ảo Đột biến (Median + Rate-of-Change Gate)   ✅ CÓ
    ──> Trung vị thật trên mảng đã sắp xếp, cộng bộ chặn thay đổi
        > PPG_MAX_BPM_STEP (15 BPM) so với giá trị đang hiển thị.
    ──> Kèm van thoát PPG_GATE_ESCAPE_BEATS: loại 8 nhịp liên tiếp thì buộc
        chấp nhận và bám lại theo nhịp thật. Xem phần ngay dưới — thiếu nó
        thì bộ chặn tự khoá cứng chính mình.
         │
         ▼
 5. TẦNG 5: Làm mịn Kalman 1D + Kiểm định SQI   ❌ CHƯA TRIỂN KHAI
    ──> Kế hoạch: Kalman 1D (~10 dòng) + chặn hiển thị khi SQI dưới ngưỡng.
         │
         ▼
[Nhịp tim & SpO2 hiển thị Màn hình & Gửi Telegram]
```

### Van thoát của Tầng 4 — ĐÃ SỬA, chưa kiểm chứng trên phần cứng

Đây từng là lỗi 🔴 nặng hơn cả thứ mà bộ chặn sinh ra để chống. Ghi lại đầy đủ vì
nó là ví dụ điển hình của một bộ lọc tự tham chiếu.

**Lỗi cũ:** bộ chặn so nhịp mới với `heartRateBPM` — **đầu ra của chính nó**. Khi
giá trị mới bị loại, `heartRateBPM` không đổi, nên nhịp kế tiếp lại bị so với đúng
con số cũ đó và lại bị loại:

```text
Đang hiển thị 75 BPM  →  nhịp thật tăng lên 95 (gắng sức / lên cơn nhịp nhanh)
        │
        ├─ nhịp 1: |95 − 75| = 20 > 15  →  LOẠI   →  heartRateBPM vẫn = 75
        ├─ nhịp 2: |95 − 75| = 20 > 15  →  LOẠI   →  heartRateBPM vẫn = 75
        └─ ... mãi mãi, cho tới khi mất tiếp xúc da (hrValid = false)
```

Màn hình đứng yên ở 75 trong khi tim thật đập 95 — thiết bị **nói dối theo hướng
trấn an**, đúng vào lúc con số có ý nghĩa nhất. Con số ảo 180 BPM thì người dùng
nhìn là biết sai; một con số hợp lý nhưng đông cứng thì không ai phát hiện được.

**Đã sửa** ([max30102_service.cpp](src/sensors/max30102_service.cpp), hằng số ở
[app_config.h](include/app_config.h)):

```cpp
#define PPG_MAX_BPM_STEP      15.0f   // bước nhảy tối đa cho một nhịp
#define PPG_GATE_ESCAPE_BEATS 8       // loại liên tiếp bấy nhiêu lần thì buộc nhận
```

Ba chi tiết quan trọng của bản sửa, thiếu bất kỳ cái nào là van thoát không hoạt động:

| # | Việc phải làm | Vì sao |
|---|---|---|
| 1 | Đếm số lần loại **liên tiếp**; mọi nhịp được chấp nhận reset về 0 | Chỉ chuỗi loại liên tiếp mới là dấu hiệu bị kẹt; nhiễu rời rạc thì không |
| 2 | Khi resync, **xoá sạch `rates[]`** | Giữ lại thì trung vị kéo giá trị về đúng chỗ vừa thoát ra |
| 3 | Khi resync, **gán thẳng `heartRateBPM = bpm`** | Trung vị cần ≥ 2 mẫu mới xuất; trong lúc chờ, `heartRateBPM` vẫn là giá trị kẹt — và đó chính là thứ nhịp kế tiếp bị đem ra so. Thiếu bước này thì van thoát bắn đi bắn lại mà không bao giờ thoát |

8 nhịp ở 60–100 BPM là khoảng 5–8 giây: đủ lâu để chặn nhiễu xung đơn lẻ, đủ nhanh
để không bỏ lỡ một cơn nhịp nhanh thật. Mỗi lần resync in một dòng
`[PPG] Rate gate stuck at ... resyncing to ...` — nếu dòng này xuất hiện dày đặc lúc
ngồi yên thì `PPG_MAX_BPM_STEP` đang quá chặt.

**Còn lại:** kiểm chứng trên người đeo thật — đo lúc ngồi yên rồi vận động nhẹ, xem
số có bám theo không và dòng resync có hiếm không.

### Chỉ số Chất lượng Tín hiệu (SQI) — CHƯA HIỆU CHUẨN

```cpp
float perfusion = (ac / dc) * 100.0f;
g_watchState.signalQuality = constrain((int)(perfusion * 50.0f), 0, 100);
```

Đây là **chỉ số tưới máu (perfusion index) nhân 50 rồi kẹp về 0–100**, tính lại mỗi giây.
Hệ số 50 là số phỏng đoán, **chưa đối chiếu với bất kỳ phép đo tham chiếu nào**.

> ⚠️ Vì thang đo chưa hiệu chuẩn, **ngưỡng "SQI > 75%" hiện không có ý nghĩa**. Khi làm Tầng 5,
> `PPG_MIN_SQI` phải khởi điểm ở **20**, rồi mới nâng dần theo số liệu đo thật — đặt 75 ngay
> thì màn hình sẽ không bao giờ hiện số nào.

### Những thứ code thật có mà đặc tả cũ bỏ sót

- **Ngưỡng tiếp xúc da** `PPG_CONTACT_IR_THRESHOLD = 25000`, dòng LED `0x30`, ngưỡng cử động `PPG_MOTION_STD_G = 0.08` — cả ba đã được chỉnh lại qua nhiều đợt thử trên thiết bị thật để chặn số ảo ~166 BPM khi cảm biến hướng ra không khí.
  > ⚠️ **Bằng chứng hiệu chuẩn cũ không còn áp dụng.** Con số "~21,900 khi để trên bàn" ghi trong [app_config.h](include/app_config.h) được đo ở LED `0x5F`. Dòng LED nay là `0x30` (yếu hơn đáng kể) nên sàn IR khi hở sáng cũng thấp hơn — comment trong file đó cần đo lại rồi viết lại. Ngưỡng 25000 hiện dựa trên thử nghiệm thực nghiệm, chưa có phép đo nền nào đi kèm ở mức sáng mới.
- **Cửa sổ SpO2 trượt**: đầy 100 mẫu → tính → trượt đi 1 giây (`memmove`), thay vì vứt cả cửa sổ.
- **`checkSensorAlive()`**: đọc lại Part ID (`0x15`) mỗi 3 giây. Module nối dây có thể rơi bất kỳ lúc nào; màn hình phải ngừng hiện số cũ trong vài giây, không được giữ mãi.
- **Chặn số cũ (stale guard)**: 6 giây không có nhịp nào → `hrValid = false`.

---

## 4. Thuật toán Té ngã 3D 4 Pha

Thuật toán liên tục tính độ lớn gia tốc tổng hợp từ QMI8658
([src/fall_detection/fall_detector.cpp](src/fall_detection/fall_detector.cpp)), chạy ở 50Hz:

$$SV = \sqrt{a_x^2 + a_y^2 + a_z^2} \qquad (a = \text{giá trị thô} / 4096)$$

> **Cập nhật quan trọng (tag `v0.2-fall-fix`):** phiên bản trước có một lỗi làm **huỷ đúng những cú ngã
> mà nó được sinh ra để bắt** — càng ngã mạnh càng chắc chắn bị loại. Đã sửa. Chi tiết ở §11 mục 1.
> **Bản sửa CHƯA được kiểm chứng trên phần cứng thật.**

### Hai đường vào giai đoạn xác nhận

```text
[Trạng thái Bình thường: SV ≈ 1.0g]
          │
          ├──────────────── ĐƯỜNG A ────────────────┬──────── ĐƯỜNG B ────────┐
          ▼                                          │                         │
 PHA 1: Rơi tự do (Free-fall)                        │      (bỏ qua pha 1)     │
  SV < 0.4g  (FALL_FREEFALL_G)                       │                         │
          │                                          │                         │
          ▼                                          │                         ▼
 PHA 2a: Va chạm trong 1500ms sau rơi tự do          │   PHA 2b: Va chạm ĐỘC LẬP
  SV > 2.5g  (FALL_IMPACT_G)                         │    SV > 3.2g (FALL_IMPACT_STANDALONE_G)
          │                                          │                         │
          └──────────────────────────────────────────┴─────────────────────────┘
                                    │
                                    ▼
                      [FALL_STATE_SUSPECTED — bắt đầu xác nhận 3000ms]
```

**Vì sao có Đường B:** cổ tay **không phải lúc nào cũng đạt 0.4g trong một cú ngã thật.** Người ngã khuỵu,
trượt dọc theo tường, hoặc ngất khi đang ngồi thì cổ tay chưa bao giờ ở trạng thái tự do — mà đó lại là
những kiểu ngã nguy hiểm nhất. Ngưỡng của Đường B cố ý cao hơn (3.2g) để các cử chỉ thường ngày
(đặt mạnh cốc nước, vỗ tay) vẫn phải vượt qua được pha 3 và 4 mới báo động.

### Hai pha xác nhận, đánh giá cùng lúc ở mốc 3000ms

```text
[FALL_STATE_SUSPECTED]
          │
          │  0 ──── 400ms ──────────────────────────────── 3000ms
          │  │        │                                      │
          │  │<- BỎ ->|<------ CỬA SỔ ĐO NẰM YÊN ---------->│
          │  │ QUA    |
          │  └─ FALL_SETTLE_MS: cổ tay còn đang dội và cánh tay đang về vị trí.
          │     KHÔNG tính vào confirmMaxDev. (Đây chính là bản vá lỗi.)
          ▼
 PHA 3: Nằm yên       — max|SV − 1.0g| < 0.35g  (FALL_STILLNESS_MAX_DEV_G)
 PHA 4: Đổi tư thế    — góc giữa vector trọng lực TRƯỚC và SAU sự kiện > 30°
                        (FALL_ORIENTATION_MIN_DEG)
          │
          ▼
   Cả hai ĐẠT ──> [MÀN HÌNH ĐỎ NHẤP NHÁY + ĐẾM NGƯỢC 15 GIÂY]
   Một trong hai TRƯỢT ──> quay về NORMAL, in log giải thích trượt ở đâu
```

Vector trọng lực được ước lượng bằng bộ lọc thông thấp `grav = grav*0.98 + a*0.02` (hằng số thời gian ~1 giây),
nên một mẫu va chạm đơn lẻ gần như không làm nó xê dịch — ảnh chụp "trước khi ngã" vẫn mô tả đúng tư thế cũ.

### Toàn bộ ngưỡng nằm ở một chỗ, và **chưa cái nào được hiệu chuẩn**

Tất cả ở [include/app_config.h](include/app_config.h), khối *Fall detection, wrist-worn*:

| Hằng số | Giá trị | Ý nghĩa |
|---|---|---|
| `FALL_ACCEL_LSB_PER_G` | 4096.0 | Khớp cấu hình ±8g của IMU. Đổi một cái phải đổi cả hai |
| `FALL_FREEFALL_G` | 0.4 | Pha 1 |
| `FALL_IMPACT_G` | 2.5 | Pha 2, đường A |
| `FALL_IMPACT_STANDALONE_G` | 3.2 | Pha 2, đường B |
| `FALL_IMPACT_WINDOW_MS` | 1500 | Rơi tự do và va chạm còn được coi là cùng một sự kiện |
| `FALL_SETTLE_MS` | **400** | Thời gian lặng sau va chạm, không tính vào pha 3 |
| `FALL_CONFIRM_WINDOW_MS` | 3000 | Tổng thời gian xác nhận |
| `FALL_STILLNESS_MAX_DEV_G` | 0.35 | Pha 3 |
| `FALL_ORIENTATION_MIN_DEG` | 30 | Pha 4 |
| `FALL_LOG_RAW_SAMPLES` | 0 | Bật = 1 để in CSV từng mẫu (dữ liệu hiệu chuẩn + train, xem §9) |

**Đây là các giá trị khởi điểm thông dụng cho thiết bị đeo cổ tay, KHÔNG phải số đo trên thiết bị này với người đeo thật.**

### Dòng log quyết định

Mỗi lần xác nhận kết thúc, firmware in đúng một dòng chứa mọi con số mà quyết định dựa vào, cạnh ngưỡng nó bị so:

```
 -> [FALL] path=B standalone impact peak=3.84g stillDev=0.21/0.35 OK tilt=52/30 deg OK => FALL CONFIRMED
 -> [FALL] path=A free-fall+impact peak=2.71g stillDev=0.62/0.35 FAIL tilt=41/30 deg OK => dismissed
```

Đây là thứ dùng để hiệu chuẩn các hằng số trên: thả thiết bị, đọc dòng log, thấy ngay phép thử nào trượt và trượt bao xa.

---

## 5. Đặc tả Giao diện TFT_eSPI & Cảnh báo Nhấp nháy

> **Dự án dùng TFT_eSPI, KHÔNG dùng LVGL.** Không có `lv_conf.h`, không có `lib_deps` LVGL nào.

### Kiến trúc render

- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) 2.5.43** driver GC9A01, SPI 80MHz, `USE_HSPI_PORT`.
- **Sprite double-buffer 240×240** (`TFT_eSprite`, 16-bit → **115 KB RAM**). Vẽ toàn bộ khung vào sprite rồi `pushSprite(0,0)` một lần → không thấy hiện tượng xé hình.
- **Đường lùi**: nếu `createSprite()` thất bại (hết heap), UI Manager tự chuyển sang chế độ vẽ thẳng ra LCD. Xấu hơn nhưng vẫn chạy — được kiểm tra bằng giá trị trả về, không giả định thành công.
- **Font tiếng Việt có dấu**: font VLW (smooth font, khử răng cưa) được **biên dịch thẳng vào firmware** dưới dạng mảng C trong [include/fonts/](include/fonts/), nạp bằng [src/ui/vn_font.cpp](src/ui/vn_font.cpp). Sinh bằng [tools/make_vlw.py](tools/make_vlw.py), kiểm tra vừa Flash bằng [tools/check_fit.py](tools/check_fit.py).
  Font GLCD/Font2/Font4 chỉ có ASCII được giữ làm đường lùi: nếu font mượt không nạp được, chữ mất dấu nhưng vẫn đọc được.
- **NimBLE thay Bluedroid**: tiết kiệm ~50 KB RAM — cần thiết vì sprite đã ăn 115 KB.

### 7 màn hình thật

| Hằng số | File | Vai trò |
|---|---|---|
| `SCREEN_HOME` | [screen_home.cpp](src/ui/screen_home.cpp) | Đồng hồ, ngày, BPM, SpO2, trạng thái té ngã |
| `SCREEN_HEART_RATE` | [screen_heart_rate.cpp](src/ui/screen_heart_rate.cpp) | Nhịp tim + biểu đồ 30 điểm |
| `SCREEN_SPO2` | [screen_spo2.cpp](src/ui/screen_spo2.cpp) | SpO2 chi tiết |
| `SCREEN_FALL_MONITOR` | [screen_fall_monitor.cpp](src/ui/screen_fall_monitor.cpp) | Trạng thái giám sát té ngã |
| `SCREEN_FALL_ALERT` | [screen_fall_alert.cpp](src/ui/screen_fall_alert.cpp) | Cảnh báo đỏ + đếm ngược |
| `SCREEN_NOTIFICATION` | [screen_notification.cpp](src/ui/screen_notification.cpp) | Kết quả gửi cảnh báo |
| `SCREEN_QUICK_MENU` | [screen_quick_menu.cpp](src/ui/screen_quick_menu.cpp) | Menu nhanh, có nút SOS |

### A. Màn hình HOME (Mặc định)

Vùng an toàn của màn tròn: $x=20..220,\ y=20..220$.

```text
       ┌────────────────────────────────────────┐
       │   📶 Wi-Fi   🔵 BLE         🔋 85%    │
       │                                        │
       │               10:30:45                 │
       │              Thứ Bảy                   │
       │             15/08/2026                 │
       │  ────────────────────────────────────  │
       │    ❤️ 72 BPM           🫁 98% SpO2      │
       │            🟢 FALL: OK                 │
       └────────────────────────────────────────┘
```

### B. Màn hình Cảnh báo Té ngã (Fall Alert & Countdown)

```text
       ┌────────────────────────────────────────┐
       │ 🔴 🔴 🔴  FALL ALERT!  🔴 🔴 🔴       │
       │                                        │
       │             ⚠  BẠN CÓ SAO KHÔNG?      │
       │                                        │
       │                  15                    │
       │                seconds                 │
       │                                        │
       │          ┌──────────────────┐          │
       │          │  HỦY / CANCEL    │          │
       │          └──────────────────┘          │
       └────────────────────────────────────────┘
```

- **Nhấp nháy**: viền đỏ (`#FF3B30`) / đen (`#000000`) đổi mỗi 0.5 giây.
- **Đếm ngược**: `15... 14... 13...` ở giữa. Bộ đếm khởi động từ một tick sạch, không để giây đầu bị nuốt.
- **Nút CANCEL**: vùng chạm **`x = 40..200, y = 168..210`** — cố ý đặt hẹp. Nếu nhận "chạm bất kỳ chỗ nào dưới y=150" thì một cú va tay vào cạnh bàn cũng huỷ được báo động thật.
- `cancelFallAlert()` reset đầy đủ: `fallState`, `countdownSec`, cờ rơi tự do, **và** `resetAlertDispatcher()` — bỏ sót cái cuối thì lần cảnh báo sau sẽ kế thừa trạng thái gửi dở dang.

---

## 6. Kịch bản Báo động SOS Khẩn cấp Thủ công

```text
                     CÁC KÊNH KÍCH HOẠT SOS THỦ CÔNG
                                    │
    ┌───────────────────────────────┼───────────────────────────────┐
    │                               │                               │
    ▼                               ▼                               ▼
Cảm ứng: Quick Menu        Lệnh từ điện thoại qua BLE      Nút Vật lý BOOT
(Nút SOS đỏ, vùng chạm     (BLE_CHAR_COMMAND_UUID)         (GPIO 0, giữ 1.5s)
 x=40..86, y=104..148)                                     ❌ CHƯA TRIỂN KHAI
    │                               │                               ✗
    └───────────────────────────────┴───────────────────────────────┘
                                    │
                                    ▼
                 [MÀN HÌNH: 🆘 SOS — enterAlert("MANUAL SOS TRIGGERED", 2)]
                                    │
                                    ▼
              [BLE notify NGAY LẬP TỨC → đếm ngược 15s → Telegram]
```

**Đã có:**
- Nút SOS trong Quick Menu → `triggerSimulatedFall()` ([ui_manager.cpp](src/ui/ui_manager.cpp)).
- Lệnh SOS từ app điện thoại qua BLE ([ble_service.cpp](src/connectivity/ble_service.cpp)).
- SOS thủ công **bỏ qua toàn bộ 4 pha xác nhận** — người đeo đã tự yêu cầu giúp đỡ thì không cần máy xác nhận nữa.

**CHƯA TRIỂN KHAI:**
- ❌ **Nút vật lý BOOT (GPIO 0)** — không có `#define` nào cho GPIO 0. Khi làm: GPIO 0 là **chân strapping**, chỉ được đọc **sau khi `setup()` chạy xong**, không được đọc lúc khởi động.
- ❌ **Nhấn giữ màn hình HOME 3 giây** — không có mã xử lý long-press nào trong [cst816s_service.cpp](src/sensors/cst816s_service.cpp).

### 🔴 Đường BLE không có xác thực

Đặc tả byte-by-byte của cả 4 characteristic nằm ở **[BLE_PROTOCOL.md](BLE_PROTOCOL.md)** —
đó là tài liệu ứng dụng điện thoại phải theo.

Một điểm phải nêu ngay ở đây vì nó liên quan trực tiếp tới kịch bản SOS:
[ble_service.cpp](src/connectivity/ble_service.cpp) đặt `setSecurityAuth(false, false, false)`
và IO capability `NO_INPUT_OUTPUT` — tức **Just Works: không bonding, không mã hoá, không xác thực**.

Đây là đánh đổi có chủ ý (bonding khiến điện thoại phải "Forget Device" thủ công sau mỗi lần nạp
firmware — một hỏng hóc im lặng ở đúng lúc tệ nhất). Nhưng hệ quả phải ghi rõ:

> **Bất kỳ thiết bị BLE nào trong tầm sóng đều ghi được `BLE_CHAR_COMMAND_UUID`**, nghĩa là
> huỷ được một cảnh báo té ngã thật (`0x01`), hoặc bắn một SOS giả (`0x02`).
> Chấp nhận được ở mức *đủ để báo cáo*. **Không chấp nhận được ở mức *dám đeo thật*** —
> xem hai mức nghiệm thu ở §11.

---

## 7. Mạch Quản lý Pin & Đo Điện áp ADC

- **Mạch phân áp ADC**: chân `BAT_ADC` nối **GPIO 1** qua mạch phân áp trên board.
- **Tần suất quét**: **5 giây/lần** ([main.cpp](src/main.cpp)), trung bình **8 mẫu**.
- **Phép đo dùng `analogReadMilliVolts()`, không dùng `analogRead()/4095*3.3`.** ESP32-S3 có hệ số hiệu chuẩn ADC ghi sẵn trong eFuse và đường đặc tuyến không tuyến tính; công thức chia thẳng cho 4095 sai hệ thống.

### Tỉ số phân áp = 3.0 — và vì sao

Tài liệu Waveshare **không ghi** chân đo pin lẫn tỉ số phân áp, nên cả hai đều là suy luận từ đo đạc:

> Với ADC đã hiệu chuẩn, chân này đọc ổn định ~**1.41 V**.
> - Nếu tỉ số là **2:1** → rail = 2.82 V. **Không thể**: ổn áp 3.3 V đã sập từ lâu trước mức đó, mà đồng hồ vẫn chạy bình thường.
> - Nếu tỉ số là **3:1** → rail = **4.23 V**, đúng bằng điện áp VSYS khi cắm USB (ETA6096 đang sạc). ✅

**Cách tự kiểm chứng:** đo vôn kế qua 2 chân BAT, so với số `raw ... mV` trong dòng `[WATCH LOG]`.
Nếu vôn kế đọc $V$ và log đọc $m$ mV thì tỉ số thật là $V / (m/1000)$.

### Cảnh báo Pin yếu — ❌ CHƯA TRIỂN KHAI

Thiết kế dự kiến (Giai đoạn 6): ngưỡng ≤ 15%, biểu tượng pin nhấp nháy đỏ, gửi **một tin duy nhất** về Telegram Gia đình, cờ chỉ reset khi pin vượt trở lại 20% (chống spam khi pin dao động quanh ngưỡng).

*`"⚠️ BÁO PIN YẾU: Đồng hồ của [Tên] chỉ còn 15% pin. Vui lòng cắm sạc!"`*

---

## 8. Giao thức Gửi Cảnh báo Trực tiếp qua Telegram Bot API

Đồng hồ kết nối Wi-Fi nhà và gọi trực tiếp HTTPS POST tới Telegram Bot API:
`POST https://api.telegram.org/bot<TELEGRAM_BOT_TOKEN>/sendMessage`

### Cấu hình Bí mật (`include/secrets.h` — **đã git-ignore, không bao giờ commit**)

```cpp
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"
#define TELEGRAM_BOT_TOKEN  "YOUR_TELEGRAM_BOT_TOKEN"   // lấy từ @BotFather
#define TELEGRAM_CHAT_ID    "YOUR_TELEGRAM_CHAT_ID"     // ID nhóm Telegram người thân
```

> Sao chép `include/secrets.h.example` thành `include/secrets.h` rồi điền giá trị thật.
> `include/app_config.h` chỉ `#include "secrets.h"` — bản thân nó không chứa bí mật nào và vẫn được commit bình thường.

### Máy trạng thái gửi tin ([alert_dispatcher.cpp](src/connectivity/alert_dispatcher.cpp))

Đây là phần đặc tả cũ bỏ sót hoàn toàn, nhưng lại là phần quyết định cảnh báo có tới nơi hay không:

| Cơ chế | Chi tiết |
|---|---|
| **Hàng đợi bền vững** | `Preferences` (NVS), vòng tròn **8 sự kiện** (`ALERT_QUEUE_MAX`). Mất điện / reboot vẫn còn |
| **Đầy thì đẩy** | Hàng đầy → bỏ mục cũ nhất, giữ mục mới nhất |
| **Thử lại** | **3 lần** mỗi sự kiện (`ALERT_MAX_RETRIES`), timeout 8 s (`ALERT_HTTP_TIMEOUT_MS`) |
| **`jsonEscape()`** | Bắt buộc. JSON ghép chuỗi có xuống dòng thô hoặc dấu `"` chưa escape → Telegram trả **HTTP 400** |
| **Không chặn vòng lặp** | `updateAlertDispatcher()` chạy mỗi vòng, không có `delay()` chờ mạng |
| **Nội dung tin** | Thời gian, BPM, SpO2, % pin, loại sự kiện |
| ⚠️ **`client.setInsecure()`** | **TLS KHÔNG xác thực chứng chỉ.** Xem §11 mục 7 |

### Kênh BLE song song

`bleNotifyFallEvent()` được gọi **ngay khi vào trạng thái cảnh báo**, trước cả khi đếm ngược xong — để nếu Wi-Fi hỏng thì điện thoại vẫn nhận được. Định danh GATT khai báo trong [app_config.h](include/app_config.h), 4 characteristic: Vitals / Fall / Status / Command.

---

## 9. Lộ trình TinyML (CHƯA TRIỂN KHAI)

> **⚠️ TOÀN BỘ MỤC 9 HIỆN CHƯA CÓ TRONG CODE.** Không có file model nào, không có thư mục `ai_models/`,
> không có `esp-tflite-micro` trong `lib_deps`. Đây là **thiết kế và lộ trình**, không phải mô tả tính năng hiện có.

**Ngân sách phần cứng:** Waveshare tự định vị 2MB PSRAM của bo mạch này cho *"ứng dụng AI/edge nhẹ"*
(Technical Overview §3.1). Cả hai model dưới đây đều nằm gọn trong ngân sách đó — nhưng cũng chính vì thế
mà không model nào là mạng nơ-ron lớn.

### 9.1 — AI Té ngã: Cây quyết định (Decision Tree)

| Hạng mục | Thiết kế |
|---|---|
| **Loại model** | Cây quyết định / random forest nhỏ |
| **Công cụ xuất** | [`emlearn`](https://github.com/emlearn/emlearn) hoặc `micromlgen` → **sinh C thuần** |
| **Kích thước** | ~8 KB Flash |
| **Tốc độ suy luận** | ~0.02 ms |
| **Ngưỡng kích hoạt** | **`P(Fall) ≥ 0.80`** |
| **Dataset** | **UMAFall** / **FallAllD** — có node đeo **cổ tay** |
| **Vai trò** | Chạy **song song** thuật toán 4 pha ở §4 để đối chiếu, **không thay thế** |

**Vì sao bỏ nhãn "Lượng tử hóa INT8":** cây quyết định là một chuỗi `if (feature < threshold)`. Không có phép nhân
ma trận nào để lượng tử hoá, và TensorFlow Lite Micro cũng không hỗ trợ cây. Con số ~8 KB và 0.02 ms thì **đúng** —
chỉ riêng cái nhãn "INT8 quantized" là sai về mặt kỹ thuật.

**Vì sao đổi dataset:** "Smartphone Fall Dataset" ghi ở túi quần hoặc thắt lưng, tức là đo **chuyển động thân người**.
Thiết bị này đeo **cổ tay**, thấy cánh tay vung lên và động tác chống đỡ — tín hiệu khác hẳn về hình dạng lẫn biên độ.
Train trên dữ liệu thắt lưng rồi suy luận trên cổ tay là sai lệch miền (domain shift) nghiêm trọng.

**🔴 Ràng buộc phần cứng phải xử lý khi train:**

| | Thiết bị này | Dataset công khai điển hình |
|---|---|---|
| Dải đo | **±8g** (4096 LSB/g) | ±16g |
| Tốc độ đọc | **50 Hz** | 200 Hz |

Cú va chạm mạnh sẽ **bão hoà cắt ngọn ở 8g** trên thiết bị này, trong khi dataset ±16g không bị cắt.
Nên trước khi train phải: **resample về 50 Hz**, **rescale**, và **mô phỏng clipping ±8g**.
Bỏ qua bước này thì model học từ những đỉnh gia tốc mà thiết bị vĩnh viễn không bao giờ nhìn thấy.

**Sửa lỗi logic ở đặc tả cũ:** ngưỡng `80% ≤ P(Fall) ≤ 85%` là **lỗi**, vì một cú ngã mà model chắc chắn 90%
sẽ **không** kích hoạt báo động. Đúng phải là `P(Fall) ≥ 0.80`.

**Nguồn dữ liệu tự thu:** bật `FALL_LOG_RAW_SAMPLES = 1` trong [app_config.h](include/app_config.h) → firmware in
một dòng `FALLCSV,...` mỗi mẫu IMU trong suốt cửa sổ xác nhận. Kế hoạch: ~40–50 cú ngã mô phỏng
(ngã trước, ngã ngang, ngã khuỵu, trượt tường) + vài giờ sinh hoạt bình thường làm mẫu âm.

**Đóng góp thật của nhánh này, nói thẳng:** *thay các ngưỡng phỏng đoán ở §4 bằng ngưỡng rút ra từ dữ liệu.*
Không phải "AI thông minh hơn con người" — mà là "ngưỡng có bằng chứng thay cho ngưỡng đoán".

### 9.2 — Sàng lọc Nhịp Bất thường: Đặc trưng Khoảng RR

> **Diễn đạt bắt buộc: đây là "gợi ý / sàng lọc dấu hiệu nhịp bất thường", KHÔNG phải "phát hiện loạn nhịp".**
> Không phải thiết bị chẩn đoán y tế. Mọi tin nhắn cảnh báo phải kèm câu miễn trừ.

| Hạng mục | Thiết kế |
|---|---|
| **Loại model** | Cây nhỏ hoặc hồi quy logistic |
| **Kích thước** | **~2 KB** |
| **Dataset** | **MIT-BIH `afdb`** (Atrial Fibrillation Database — 23 bản ghi 10 giờ, có nhãn từng cơn) |
| **Đặc trưng** | `RMSSD`, `pNN50`, entropy Shannon — trên cửa sổ 30–60 giây |
| **Điều kiện chạy** | Chỉ khi SQI đạt ngưỡng (§3 Tầng 5) |

**Vì sao BỎ mạng 1D-CNN trên dạng sóng:** MIT-BIH là **ECG @ 360Hz**, ghi hoạt động **điện** của tim (phức bộ P-QRS-T).
MAX30102 là cảm biến **quang học**, ghi **thay đổi thể tích máu**. Sóng PPG **không có sóng P, không có phức bộ QRS**.
Một CNN học *hình dạng sóng* sẽ đi tìm những đặc trưng **không tồn tại** trong tín hiệu PPG. Train ECG → suy luận PPG
theo cách này là sai về nguyên lý, không phải chuyện tinh chỉnh được.

**Vì sao đặc trưng khoảng RR thì HỢP LỆ:** khoảng RR là **cùng một đại lượng sinh lý** — thời gian giữa hai lần tim đập —
bất kể đo bằng điện hay bằng ánh sáng. Train trên khoảng RR trích từ `afdb`, rồi suy luận trên khoảng RR trích từ PPG,
là **hợp lệ về mặt vật lý**. Đây cũng là cách y văn làm chuẩn, và là cách Apple Watch / Fitbit sàng lọc rung nhĩ.

**🔴 ĐIỀU KIỆN TIÊN QUYẾT BẮT BUỘC — độ phân giải khoảng RR:**

| | Hiện tại | Cần có |
|---|---|---|
| Tốc độ mẫu PPG hiệu dụng | 25 Hz | **200 Hz** |
| Lượng tử hoá khoảng RR | **40 ms** | **5 ms** |
| Nguồn mốc thời gian | `millis()` lúc **vòng lặp chạy** ([max30102_service.cpp](src/sensors/max30102_service.cpp)) → **+20 ms jitter** | Suy từ **chỉ số mẫu trong FIFO × chu kỳ mẫu** |

`RMSSD` của người bình thường nằm trong khoảng **20–50 ms**. Với lượng tử hoá 40 ms, **sai số đo lớn bằng đúng
đại lượng cần đo** — mọi con số RMSSD tính ra ở cấu hình hiện tại đều vô nghĩa. **Phải hoàn thành nâng cấp 200Hz
(Giai đoạn 5 / Phần B) trước khi bắt đầu nhánh này.**

**🔴 Phụ thuộc DSP Tầng 4+5:** một nhịp bị bỏ sót tạo ra khoảng RR **gấp đôi** — nhìn y hệt rung nhĩ.
Cảnh báo này bắn thẳng vào nhóm Telegram gia đình, nên phải lọc sạch số ảo trước.

### 9.3 — Thứ tự phụ thuộc

```text
A4 (log FALLCSV)  ─────────────────────────────────> 9.1  AI Té ngã

B1a (median + chặn 15 BPM/nhịp) ✅ ĐÃ CÓ ─┐
B1b (van thoát 8 nhịp)          ✅ ĐÃ CÓ ─┴─> Tầng 4 ─┐
                                                      ├─> 9.2  Sàng lọc khoảng RR
B2  (Kalman + chặn theo SQI)    ❌ THIẾU ──> Tầng 5 ──┤
                                                      │
B0  (PPG 200 Hz + mốc từ FIFO)  ❌ THIẾU ─────────────┘
```

> Đọc sơ đồ: 9.2 **chưa thể bắt đầu** — còn thiếu B0 và B2.
> B0 là điều kiện tiên quyết cứng (độ phân giải 40 ms không đo nổi RMSSD 20–50 ms);
> B2 là điều kiện về độ sạch (một nhịp sai làm khoảng RR gấp đôi → dương tính giả rung nhĩ).

---

## 10. Cấu trúc Mã nguồn PlatformIO

Cấu trúc **thật** của repo (không có thư mục `firmware/`, không có `assets/` — mã nguồn nằm thẳng ở gốc theo quy ước PlatformIO):

```text
RecordOfRagnarok_CardioGuardAI/
│
├── platformio.ini                      # env: esp32-s3-touch-lcd-128, framework = arduino
├── .gitignore                          # include/secrets.h nằm ở đây — KHÔNG BAO GIỜ commit secrets.h
├── README.md
├── SYSTEM_ARCHITECTURE_SPEC.md         # (File đặc tả hiện tại)
├── IMPLEMENTATION_PLAN.md              # ĐÃ THAY THẾ — xem đầu file đó
│
├── include/
│   ├── app_config.h                    # Chân GPIO + MỌI ngưỡng. #include "secrets.h"
│   ├── app_state.h                     # struct g_watchState — trạng thái toàn hệ thống
│   ├── ui_config.h                     # Màu sắc, toạ độ giao diện
│   ├── secrets.h.example               # Mẫu; sao chép thành secrets.h (git-ignored)
│   └── fonts/
│       ├── vn_font_large.h             # Font VLW tiếng Việt, biên dịch thẳng vào firmware
│       ├── vn_font_medium.h
│       └── vn_font_small.h
│
├── src/
│   ├── main.cpp                        # setup() + loop() hợp tác — KHÔNG có FreeRTOS task
│   ├── app/app_state.cpp               # Định nghĩa g_watchState
│   ├── sensors/
│   │   ├── max30102_service.cpp        # PPG: motion gating + checkForBeat + SpO2 (§3)
│   │   ├── qmi8658_service.cpp         # IMU raw I2C, ±8g @ 250Hz
│   │   ├── cst816s_service.cpp         # Cảm ứng, cổng bằng ngắt
│   │   └── battery_monitor.cpp         # ADC hiệu chuẩn, phân áp 3.0 (§7)
│   ├── fall_detection/
│   │   └── fall_detector.cpp           # Thuật toán 4 pha, 2 đường vào (§4)
│   ├── connectivity/
│   │   ├── wifi_manager.cpp            # Kết nối không chặn + NTP
│   │   ├── alert_dispatcher.cpp        # Hàng đợi NVS + retry + jsonEscape (§8)
│   │   └── ble_service.cpp             # NimBLE, 4 characteristic
│   └── ui/
│       ├── ui_manager.cpp              # Sprite 240x240 + định tuyến chạm
│       ├── vn_font.cpp                 # Nạp font VLW từ mảng C
│       └── screen_*.cpp                # 7 màn hình (§5)
│
├── tools/
│   ├── make_vlw.py                     # TTF -> VLW -> mảng C header
│   └── check_fit.py                    # Kiểm tra font vừa Flash
│
├── docs/
│   ├── ESP32-S3-Touch-LCD-1.28-B_Technical_Overview.md   # Tài liệu phần cứng gốc
│   ├── UI_SPEC_Smart_Health_Fall_Detection_Watch.md
│   └── HARDWARE_TEST_GUIDE.md
│
└── .CLAUDE/Plan/                       # Ghi chú kế hoạch theo từng tác vụ (lịch sử)
```

**Chưa tồn tại, sẽ tạo ở Giai đoạn 7:** `ai_models/` (script train Python + notebook; dataset thì **không** commit — đã có trong `.gitignore`).

---

## 11. Hạn chế đã biết & Lộ trình

> Mục này liệt kê **mọi khoảng cách đã biết giữa tài liệu và thực tế**, xếp theo mức nghiêm trọng.
> Nó tồn tại để không ai — kể cả tác giả sau 3 tháng — nhầm tưởng dự án đã xong.

### 🔴 Nghiêm trọng

**1. Lỗi phát hiện té ngã — ĐÃ SỬA, CHƯA KIỂM CHỨNG TRÊN PHẦN CỨNG**

Phiên bản trước bắt đầu đo "nằm yên" **ngay tại mẫu va chạm**, trong khi cổ tay còn dội 100–300 ms và cánh tay
đang về vị trí. Chu kỳ mẫu là 20 ms, nên chỉ **một** mẫu vượt 0.35g là đủ đóng đinh `confirmMaxDev` trên ngưỡng
→ báo động bị huỷ. Hệ quả nguy hiểm nhất: **cú ngã càng mạnh thì càng chắc chắn bị loại bỏ** — chính xác là ngược
với thứ cần có.

**Đã sửa** (tag `v0.2-fall-fix`): thêm `FALL_SETTLE_MS = 400`; nới cửa sổ xác nhận 2000 → 3000 ms.
Nay có 2600 ms quan sát thật sự thay vì một phép đo bị nhiễu bởi chính cú va chạm.

**Còn lại:** cần **kiểm chứng trên phần cứng thật** — thả thiết bị xuống đệm ~10 lần theo 4 kiểu ngã,
đọc dòng log quyết định (§4), xác nhận báo động bắn. Cho tới lúc đó, đây vẫn là mã chưa được kiểm chứng
trên tính năng cốt lõi nhất của sản phẩm.

### 🟠 Cao

**2. PPG 25 Hz không đủ cho phân tích khoảng RR.** 40 ms lượng tử hoá so với RMSSD 20–50 ms → sai số bằng chính
đại lượng đo. Cộng thêm ~20 ms jitter do lấy mốc bằng `millis()` lúc vòng lặp chạy thay vì lúc mẫu được lấy.
**Chặn hoàn toàn §9.2.** Khắc phục: Giai đoạn 5 / Phần B — nâng 200 Hz + suy mốc thời gian từ chỉ số FIFO.

**3. Số nhịp ảo — ĐÃ XỬ LÝ, chưa kiểm chứng trên người đeo.** Dải hợp lệ siết còn
`45 ≤ BPM ≤ 180`, thêm trung vị thật, bộ chặn 15 BPM/nhịp và van thoát 8 nhịp. Lần sửa đầu
tiên của bộ chặn từng tạo ra một lỗi 🔴 nặng hơn (nhịp tim khoá cứng vĩnh viễn); van thoát
đã đóng lỗ đó — diễn giải đầy đủ: [§3](#3-chuỗi-xử-lý-tín-hiệu-nhịp-tim--spo2-tại-biên).
Còn lại: đo trên người đeo thật để chỉnh `PPG_MAX_BPM_STEP` nếu dòng resync xuất hiện quá dày.

**4. DSP Tầng 5 chưa tồn tại** (Kalman + chặn hiển thị theo SQI). Xem §3.

### ⚪ Trung bình / Thấp

**5. Toàn bộ ngưỡng té ngã chưa hiệu chuẩn.** Mọi hằng số ở §4 và thang SQI ở §3 là số phỏng đoán — giá trị
khởi điểm thông dụng, không phải số đo trên thiết bị này với người đeo thật.

Nhóm PPG (`PPG_CONTACT_IR_THRESHOLD = 25000`, `MAX30102_LED_BRIGHTNESS = 0x30`,
`PPG_MOTION_STD_G = 0.08`) **đã** qua nhiều đợt chỉnh trên thiết bị thật, nên đáng tin hơn nhóm té ngã.
Nhưng chúng được chỉnh theo tiêu chí "hết báo số ảo khi hở sáng", chưa đối chiếu với thiết bị đo tham chiếu
khi đeo — và comment giải thích trong [app_config.h](include/app_config.h) vẫn là bằng chứng đo ở mức LED cũ
`0x5F`, cần đo lại rồi viết lại.

**6. Tính năng còn thiếu** (Giai đoạn 6 / Phần C):
- ❌ Nút SOS vật lý BOOT (GPIO 0) — §6
- ❌ Cảnh báo pin yếu ≤ 15% — §7
- ❌ Cảnh báo ngưỡng sinh lý (HR > 130 / < 45, SpO2 < 90) — **phải làm sau Tầng 5**, không thì spam cảnh báo từ số rác
- ❌ Nhấn giữ HOME 3 giây để SOS — §6

**7. `client.setInsecure()`** ([alert_dispatcher.cpp](src/connectivity/alert_dispatcher.cpp)) — kết nối TLS
**không xác thực chứng chỉ máy chủ**. Dữ liệu vẫn được mã hoá, nhưng thiết bị không kiểm tra nó đang nói chuyện
với ai. Trong mạng Wi-Fi nhà thì rủi ro thấp; muốn chặt chẽ thì phải nhúng CA root của Telegram và xử lý việc
chứng chỉ hết hạn.

**8. 🟠 BLE không có xác thực — ai trong tầm sóng cũng huỷ được cảnh báo thật.** `setSecurityAuth(false,
false, false)` + `IO_NO_INPUT_OUTPUT` (Just Works, không ghép cặp). Bất kỳ thiết bị nào cũng ghi được vào
`BLE_CHAR_COMMAND_UUID` để huỷ báo động té ngã đang đếm lùi, hoặc bắn SOS giả. Đây là **đánh đổi có chủ ý**
(nếu bật bonding thì mỗi lần nạp firmware phải "Forget Device" trên điện thoại), nhưng phải sửa trước khi
dùng thật. Chi tiết và hướng khắc phục: [BLE_PROTOCOL.md](BLE_PROTOCOL.md) §6.

**9. Toàn bộ §9 TinyML chưa tồn tại.** Không có model, không có `ai_models/`, không có `esp-tflite-micro`.

**10. Phụ thuộc phần cứng nối dây tay.** MAX30102 là module rời nối bằng dây (§2). Đây là điểm hỏng cơ khí có thật —
`checkSensorAlive()` tồn tại chính vì lý do đó.

### Hai mức nghiệm thu — hãy chọn đúng mức khi phát biểu về dự án

| | **Đủ để báo cáo đồ án** | **Dám cho người thật đeo** |
|---|---|---|
| Lỗi 🔴 | Đã sửa trong code | Đã sửa **và kiểm chứng trên phần cứng** |
| Té ngã | Ma trận nhầm lẫn, độ nhạy / độ đặc hiệu, đường ROC trên tập test | Thử nghiệm thực địa **nhiều ngày** trên người đeo thật, đếm báo động giả/ngày |
| Nhịp tim | So sánh với thiết bị tham chiếu ở tư thế ngồi yên | Ổn định khi vận động; Tầng 5 hoạt động; van thoát Tầng 4 đã đo trên người đeo |
| BLE | Kết nối và đọc được từ điện thoại | Có ghép cặp/bonding — lệnh huỷ báo động không thể bị giả mạo |
| Ngưỡng | Ghi rõ là chưa hiệu chuẩn | Đã hiệu chuẩn bằng số đo |
| Cảnh báo | Gửi được tin thật một lần | Kiểm chứng đường mất mạng: hàng đợi NVS, retry, khôi phục sau reboot |
| Diễn đạt | "sàng lọc / gợi ý", có miễn trừ | "sàng lọc / gợi ý", có miễn trừ **+ hướng dẫn sử dụng rõ ràng** |

**Trạng thái hiện tại: đang ở giữa hai mức.** Code chạy thật trên phần cứng thật, nhưng lỗi 🔴 vừa sửa chưa được
kiểm chứng, và các ngưỡng vẫn là số phỏng đoán.

---

## Lộ trình còn lại

| Giai đoạn | Nội dung | Trạng thái |
|---|---|---|
| 0 | Xử lý rò rỉ bí mật + `.gitignore` | ✅ Xong |
| 1 | Chuyển firmware vào repo, tag `v0.1-arduino-baseline` | ✅ Xong |
| 3 (Phần A) | Sửa lỗi té ngã 🔴, tag `v0.2-fall-fix` | ✅ Xong code, ⏳ chờ kiểm chứng phần cứng |
| 2 | Viết lại tài liệu cho đúng sự thật | ✅ Xong (tài liệu này) |
| 2b | README, `BLE_PROTOCOL.md`, cho `.CLAUDE/Plan/` nghỉ hưu | ✅ Xong |
| 5a | Van thoát Tầng 4 (`PPG_GATE_ESCAPE_BEATS`) | ✅ Xong code, ⏳ chờ kiểm chứng |
| 5 (Phần B) | DSP Tầng 5 (Kalman + chặn theo SQI) + nâng PPG 200 Hz | ⏳ Kế tiếp |
| 6 (Phần C) | Nút SOS BOOT, cảnh báo pin yếu, cảnh báo ngưỡng sinh lý | ⏳ |
| 7 (Phần D) | TinyML: cây té ngã + sàng lọc khoảng RR | ⏳ |
| 8 | Đánh giá: ma trận nhầm lẫn, độ nhạy/đặc hiệu, ROC, so với baseline 4 pha | ⏳ |
| 4 | Đổi `framework = arduino, espidf` (nhánh riêng, **làm sau cùng**) | ⏳ |

---

*Tài liệu này mô tả trạng thái thật của mã nguồn tại tag `v0.2-fall-fix`.
Khi code thay đổi mà tài liệu không đổi theo, tài liệu là bên sai.*
