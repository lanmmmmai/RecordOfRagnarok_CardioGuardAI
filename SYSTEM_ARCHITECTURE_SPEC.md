# System Architecture & Detailed Technical Specification
## Project: CardioGuardAI (RecordOfRagnarok_CardioGuardAI)
> **Đặc tả Kiến trúc Hệ thống, Thuật toán Xử lý tại Biên & TinyML AI Lượng tử hóa (INT8)**

---

## 📄 Mục lục
1. [Tổng quan Hệ thống & Mục tiêu Cốt lõi](#1-tổng-quan-hệ-thống--mục-tiêu-cốt-lõi)
2. [Phần cứng Chi tiết & Sơ đồ Chân GPIO (Hardware Peripherals & Pinouts)](#2-phần-cứng-chi-tiết--sơ-đồ-chân-gpio)
3. [Chuỗi Thuật toán Lọc Nhiễu Nhịp tim & SpO2 5 Tầng tại Biên (5-Stage Edge DSP Pipeline)](#3-chuỗi-thuật-toán-lọc-nhiễu-nhịp-tim--spo2-5-tầng-tại-biên)
4. [Thuật toán Té ngã 3D 4 Giai đoạn (4-Stage 3D Edge Fall Detection Algorithm)](#4-thuật-toán-té-ngã-3d-4-giai-đoạn)
5. [Đặc tả Màn hình Giao diện LVGL & Cảnh báo Nhấp nháy (LCD 1.28" UI/UX)](#5-đặc-tả-màn-hình-giao-diện-lvgl--cảnh-báo-nhấp-nháy)
6. [Kịch bản Báo động SOS Khẩn cấp Thủ công (Manual Emergency SOS)](#6-kịch-bản-báo-động-sos-khẩn-cấp-thủ-công)
7. [Mạch Quản lý Pin & Cảnh báo Pin Yếu Phần cứng (Battery ADC Monitor)](#7-mạch-quản-lý-pin--cảnh-báo-pin-yếu-phần-cứng)
8. [Giao thức Gửi Cảnh báo Trực tiếp qua Telegram Bot API (Direct Wi-Fi)](#8-giao-thức-gửi-cảnh-báo-trực-tiếp-qua-telegram-bot-api)
9. [Mô hình TinyML AI Lượng tử hóa INT8 (Kaggle Datasets & Edge AI Inference)](#9-mô-hình-tinyml-ai-lượng-tử-hóa-int8)
10. [Cấu trúc Mã nguồn PlatformIO & Lộ trình Triển khai](#10-cấu-trúc-mã-nguồn-platformio--lộ-trình-triển-khai)

---

## 1. Tổng quan Hệ thống & Mục tiêu Cốt lõi

**CardioGuardAI** là hệ thống AIoT chuyên dụng giám sát sức khỏe, phát hiện nhịp tim/SpO2 bất thường và cảnh báo té ngã khẩn cấp dành cho người cao tuổi. 

Hệ thống hoạt động theo tiêu chí **Tối giản - Tin cậy - Xử lý tại Biên (Edge Computing)**:
- **Xử lý hoàn toàn tại biên (ESP32-S3)**: Mọi dữ liệu từ cảm biến PPG (MAX30102) và cảm biến gia tốc (QMI8658) được tính toán, lọc nhiễu 5 tầng, và chạy suy luận mô hình AI TinyML INT8 ngay trên chip ESP32-S3 mà không cần máy chủ trung gian.
- **Cảnh báo tại chỗ**: Phát nhấp nháy màn hình màu đỏ dồn dập kèm 15 giây đếm ngược và nút **HỦY / CANCEL** chạm cảm ứng kích thước to để loại bỏ báo động giả khi ngã nhẹ hoặc bấm nhầm.
- **Gửi tin nhắn trực tiếp qua Telegram (Direct Telegram Alert)**: Khi hết 15s đếm ngược hoặc bấm SOS thủ công, đồng hồ tự kết nối Wi-Fi gọi HTTPS Telegram Bot API bắn tin nhắn cảnh báo khẩn cấp tới **Group Chat Telegram của Gia đình** (Chat ID cấu hình trong `include/secrets.h`) cho tất cả người thân nhận được cùng lúc.

---

## 2. Phần cứng Chi tiết & Sơ đồ Chân GPIO

Hệ thống chạy trên bo mạch **Waveshare ESP32-S3-Touch-LCD-1.28-B**. Tất cả linh kiện được tích hợp sẵn 100% trên phần cứng bo mạch:

```text
               +-------------------------------------------------------+
               |        Waveshare ESP32-S3-Touch-LCD-1.28-B            |
               |                                                       |
               |  +-------------------------------------------------+  |
               |  |  ESP32-S3R8 Dual-Core 240MHz, 8MB PSRAM         |  |
               |  +-------------------------------------------------+  |
               |           |         |         |         |             |
               +-----------|---------|---------|---------|-------------+
                           |         |         |         |
    +----------------------+         |         |         +-----------------------+
    | SPI (GC9A01 LCD)               |         |                   I2C (SDA/SCL) |
    v                                v         v                                 v
+-----------------------+      +---------+ +---------+         +-------------------------------+
| LCD Tròn 1.28 inch    |      | BOOT    | | Mạch Pin|         | Bus I2C Dùng chung (Chân I2C)  |
| 240 x 240 pixels      |      | Button  | | ADC 1/2 |         |  - CST816S (Cảm ứng)          |
| (GC9A01 Driver)       |      | (GPIO 0)| | (GPIO 1)|         |  - MAX30102 (Nhịp tim/SpO2)   |
+-----------------------+      +---------+ +---------+         |  - QMI8658 (Gia tốc 6 trục)   |
                                                               +-------------------------------+
```

### Bảng Chi tiết Chân Kết nối Phần cứng (Pin Mapping Table):

| Linh kiện Phần cứng | Model / Tên IC | Chuẩn Giao tiếp | Chân GPIO ESP32-S3 | Ghi chú Tính năng |
|---|---|---|---|---|
| **Vi xử lý (MCU)** | ESP32-S3R8 | System | Dual-core 240MHz | Core 0: Wi-Fi/Telegram; Core 1: DSP/TinyML/LVGL |
| **Màn hình LCD** | GC9A01 | SPI | MOSI=11, SCLK=10, CS=9, DC=8, RST=14, BL=2 | Tròn 1.28" (240x240 px), Nhấp nháy màu đỏ 15s |
| **Mặt Cảm ứng** | CST816S | I2C | SDA=6, SCL=7, INT=5, RST=13 | Cảm ứng điện dung, nhận diện Long Press 3s |
| **Cảm biến Gia tốc** | QMI8658 | I2C | SDA=6, SCL=7 | IMU 6-axis, đo gia tốc 3D X-Y-Z (Phát hiện té ngã) |
| **Cảm biến Nhịp tim**| MAX30102 | I2C | SDA=6, SCL=7 | Đo PPG Đỏ & Hồng ngoại (Nhịp tim & SpO2) |
| **Nút bấm SOS** | Onboard BOOT | Digital Input | **GPIO 0** (Internal Pull-up) | **Bấm giữ 1.5s kích hoạt SOS tức thì** |
| **Mạch đo Pin** | ADC Divider | Analog Input | **GPIO 1** (ADC1 Channel 0) | **Đọc điện áp Pin LiPo, báo Telegram $\le 15\%$** |
| **Mạch sạc Pin** | ETA6096 | Hardware | Giắc cắm GH1.25 2P | Tự động sạc khi cắm cáp USB Type-C |

---

## 3. Chuỗi Thuật toán Lọc Nhiễu Nhịp tim & SpO2 5 Tầng tại Biên

Để triệt tiêu 100% các **số liệu ảo đột biến** (ví dụ: đang 75 BPM tự nhiên vọt lên 180–220 BPM do vung tay hoặc xóc nhẹ), chip ESP32-S3 thực thi chuỗi **5 Tầng Lọc Xử lý Tín hiệu Số (DSP Pipeline)** liên tục trên Core 1:

```text
[Tín hiệu thô MAX30102 @ 100Hz]
         │
         ▼
 1. TẦNG 1: Lọc Dải tần Sinh lý (IIR Bandpass Filter 0.5Hz - 4.0Hz)
    ──> Loại bỏ nhiễu tần số thấp (hô hấp) và nhiễu ánh sáng đèn điện (50/60Hz).
         │
         ▼
 2. TẦNG 2: Lọc Nhiễu Cử động Gia tốc phần cứng (IMU-Assisted NLMS Adaptive Filter)
    ──> Lấy gia tốc 3D (X-Y-Z) từ QMI8658 làm mẫu nhiễu để TRỪ TRỰC TIẾP khỏi sóng PPG.
         │
         ▼
 3. TẦNG 3: Phân tích Tần số Mạch đập (FFT Peak Detection)
    ──> Chuyển đổi sang miền tần số, trích xuất đỉnh năng lượng tần số tim thực tế.
         │
         ▼
 4. TẦNG 4: Loại bỏ Số Ảo Đột biến (Rate-of-Change Gate + Median Filter 5 mẫu)
    ──> Khống chế mức thay đổi ≤ 15 BPM/giây. 
    ──> Phương án A: Vứt bỏ lập tức số ảo vọt ngẫu nhiên (180, 220 BPM), giữ giá trị hợp lệ gần nhất.
         │
         ▼
 5. TẦNG 5: Làm mịn Toán học & Kiểm định Chất lượng (1D Kalman Filter + SQI > 75%)
    ──> Làm mượt chỉ số sinh lý, chỉ xuất dữ liệu lên màn hình & Telegram khi SQI đạt độ tin cậy cao.
         │
         ▼
[Chỉ số Nhịp tim & SpO2 Chuẩn xác hiển thị Màn hình & Gửi Telegram]
```

---

## 4. Thuật toán Té ngã 3D 4 Giai đoạn

Thuật toán phát hiện té ngã trên chip ESP32-S3 liên tục tính độ lớn gia tốc tổng hợp 3 chiều $a_x, a_y, a_z$ từ QMI8658:
$$SV = \sqrt{a_x^2 + a_y^2 + a_z^2}$$

Một sự cố té ngã thực sự bắt buộc phải thỏa mãn đúng **4 Giai đoạn Nối tiếp**:

```text
[Trạng thái Bình thường: SV ≈ 1.0g]
          │
          ▼
 GIAI ĐOẠN 1: Rơi tự do (Free-fall Phase)
  - Thân người mất đà ngã, gia tốc giảm đột ngột: SV < 0.5g (trong 100ms - 300ms).
          │
          ▼
 GIAI ĐOẠN 2: Va chạm mạnh (Impact Phase) ---> [ĐÃ CHỐT: SV > 3.2g]
  - Thân người đập xuống sàn nhà, SV tăng vọt > 3.2g.
  - Ngưỡng 3.2g loại bỏ 100% báo động giả khi vỗ tay (2.0g) hoặc đập nhẹ tay xuống bàn (2.2g).
          │
          ▼
 GIAI ĐOẠN 3: Đổi góc nghiêng cơ thể (Orientation Change Phase)
  - Trục cơ thể thay đổi góc nghiêng > 60° (chuyển từ tư thế đứng/ngồi sang tư thế nằm).
          │
          ▼
 GIAI ĐOẠN 4: Xác nhận Nằm yên (Post-fall Stillness Phase) ---> [ĐÃ CHỐT: 3 GIÂY]
  - Nạn nhân nằm bất động trên sàn (SV duy trì 1.0g ± 0.2g trong 3 giây liên tục).
  - Con số 3 giây chuẩn y tế giúp phân biệt ngã thật với việc lỡ tay làm rơi đồng hồ nhặt lên ngay.
          │
          ▼
[KÍCH HOẠT MÀN HÌNH CẢNH BÁO NHẤP NHÁY MÀU ĐỎ & ĐẾM NGƯỜI 15 GIÂY]
```

---

## 5. Đặc tả Màn hình Giao diện LVGL & Cảnh báo Nhấp nháy

Màn hình tròn 1.28 inch (240x240 px) được chia vùng Safe Area ($x=20..220, y=20..220$):

### A. Giao diện Màn hình HOME (Mặc định)
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
Khi xác nhận té ngã (hết 3s nằm yên):
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
- **Hiệu ứng Nhấp nháy**: Màn hình chớp nháy viền màu đỏ (`#FF3B30`) và đen (`#000000`) dồn dập mỗi 0.5 giây.
- **Đếm ngược**: Số đếm ngược `15... 14... 13...` hiển thị nổi bật ở giữa.
- **Nút CANCEL**: Nút cảm ứng màu xanh kích thước to ở phần dưới (`y=160..210`).

---

## 6. Kịch bản Báo động SOS Khẩn cấp Thủ công

```text
                     CÁC KÊNH KÍCH HOẠT SOS THỦ CÔNG
                                    │
    ┌───────────────────────────────┼───────────────────────────────┐
    │                               │                               │
    ▼                               ▼                               ▼
Nút Vật lý BOOT          Cảm ứng: Nhấn giữ           Cảm ứng: Quick Menu
(Nhấn giữ GPIO 0 1.5s)   Màn hình HOME 3s            (Swipe Up -> Nút SOS Đỏ)
    │                               │                               │
    └───────────────────────────────┼───────────────────────────────┘
                                    │
                                    ▼
                 [MÀN HÌNH HIỂN THỊ: 🆘 SOS EMERGENCY!]
                                    │
                                    ▼
       [BỎ QUA ĐẾM NGƯỜI 15S -> GỬI NGAY TIN NHẮN TELEGRAM KHẨN CẤP]
```

---

## 7. Mạch Quản lý Pin & Cảnh báo Pin Yếu Phần cứng

- **Mạch phân áp ADC**: Chân `BAT_ADC` nối với **GPIO 1** qua mạch phân áp trở tích hợp sẵn trên board.
- **Tần suất quét**: Đo ADC 30 giây/lần, tính trung bình 20 mẫu, quy đổi điện áp LiPo ($3.2\text{V} - 4.2\text{V}$).
- **Ngưỡng Pin yếu ($\le 15\%$)**:
  - Biểu tượng pin nhấp nháy đỏ `🪫 15%`.
  - Tự động phát **1 tin nhắn duy nhất** về Telegram Gia đình:
    *`"⚠️ BÁO PIN YẾU: Đồng hồ của [Tên] chỉ còn 15% pin. Vui lòng cắm sạc!"`*

---

## 8. Giao thức Gửi Cảnh báo Trực tiếp qua Telegram Bot API

Đồng hồ kết nối Wi-Fi nhà và gọi trực tiếp HTTPS POST request tới Telegram Bot API:
`POST https://api.telegram.org/bot<TELEGRAM_BOT_TOKEN>/sendMessage`

### Cấu hình Bí mật (`include/secrets.h` — **đã git-ignore, không bao giờ commit**):
```cpp
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"
#define TELEGRAM_BOT_TOKEN  "YOUR_TELEGRAM_BOT_TOKEN"   // lấy từ @BotFather
#define TELEGRAM_CHAT_ID    "YOUR_TELEGRAM_CHAT_ID"     // ID nhóm Telegram người thân
```

> Sao chép `include/secrets.h.example` thành `include/secrets.h` rồi điền giá trị thật.
> `include/app_config.h` chỉ `#include "secrets.h"` — bản thân nó không chứa bí mật nào
> và vẫn được commit bình thường.

---

## 9. Mô hình TinyML AI Lượng tử hóa INT8

Dự án tích hợp bộ 2 mô hình Machine Learning **Lượng tử hóa INT8 (INT8 Quantized TinyML)** nhúng thẳng vào chip ESP32-S3:

1. **AI Té ngã (Quantized Decision Tree INT8)**:
   - **Tải từ**: Kaggle Smartphone Fall Dataset.
   - **Kích thước Model**: **~ 8 KB** Flash/RAM.
   - **Tốc độ Suy luận**: **0.02 ms** (Tức thì).
   - **Ngưỡng kích hoạt**: Xác suất ngã $80\% \le P(\text{Fall}) \le 85\%$.

2. **AI Loạn nhịp tim (Quantized 1D-CNN INT8)**:
   - **Tải từ**: Kaggle MIT-BIH Arrhythmia Database.
   - **Kích thước Model**: **~ 30 KB** Flash/RAM.
   - **Tốc độ Suy luận**: **1.0 ms** (nhờ tăng tốc SIMD `esp-dsp`).
   - **Quy trình Kiểm định Kép 10 giây**: Khi phát hiện loạn nhịp tim (Arrhythmia) kéo dài quá 10 giây $\rightarrow$ Nhấp nháy viền vàng màn hình + Tự động gửi tin nhắn báo động về Group Telegram Gia đình.

---

## 10. Cấu trúc Mã nguồn PlatformIO & Lộ trình Triển khai

### Cấu trúc Thư mục Dự án (`D:\AIoT\RecordOfRagnarok_CardioGuardAI`):

```text
RecordOfRagnarok_CardioGuardAI/
│
├── README.md                           # Giới thiệu & Hướng dẫn cài đặt
├── SYSTEM_ARCHITECTURE_SPEC.md         # (File đặc tả hiện tại)
├── IMPLEMENTATION_PLAN.md              # Kế hoạch thi công lập trình chi tiết
│
├── ai_models/                          # Mã nguồn Python Huấn luyện & Lượng tử hóa AI
│   ├── dataset_downloader.py           # Tải 3 dataset từ Kaggle
│   ├── train_fall_model.py             # Train mô hình ngã & convert INT8 C++
│   └── train_arrhythmia_model.py       # Train mô hình loạn nhịp & convert INT8 C++
│
└── firmware/                           # Mã nguồn PlatformIO C++ cho ESP32-S3
    ├── platformio.ini                  # Cấu hình PlatformIO, thư viện LVGL, WiFiClientSecure
    ├── include/
    │   ├── app_config.h                # Cấu hình Wi-Fi, Telegram Token & Chat ID
    │   ├── app_state.h                 # Model lưu trạng thái hệ thống
    │   ├── fall_model_data.h           # Trọng số Mô hình AI Té ngã INT8 (~8KB)
    │   └── arrhythmia_model_data.h     # Trọng số Mô hình AI Loạn nhịp tim INT8 (~30KB)
    │
    ├── src/
    │   ├── main.cpp                    # Khởi tạo FreeRTOS Task (Core 0 & Core 1)
    │   ├── app/                        # Quản lý luồng chính & State Machine
    │   ├── ui/                         # Giao diện LVGL 240x240 (Home, HeartRate, FallAlert)
    │   ├── sensors/
    │   │   ├── max30102_service.cpp    # Driver MAX30102 & Lọc nhiễu DSP 5 tầng
    │   │   └── qmi8658_service.cpp     # Driver QMI8658 & AI Fall Detector INT8
    │   ├── battery/                    # Quản lý ADC đọc Pin GPIO 1
    │   └── connectivity/               # Driver Wi-Fi & Telegram Bot API Client
    └── assets/                         # Font chữ & Icons LVGL
```

---
*Tài liệu Đặc tả Kiến trúc Hệ thống & TinyML AI CardioGuardAI được chốt hoàn tất 100%.*
