# RecordOfRagnarok_CardioGuardAI 🛡️❤️

> **CardioGuardAI** - Hệ thống Cảnh báo Té ngã, Loạn nhịp tim & Lọc Nhiễu Tín hiệu tại Biên (ESP32-S3 Watch + TinyML AI INT8 + Direct Telegram Alert).

---

## 📌 Cấu trúc Thư mục Dự án Chi tiết (Project Structure)

Dưới đây là cấu trúc thư mục hoàn chỉnh của dự án tại `D:\AIoT\RecordOfRagnarok_CardioGuardAI` để lập trình viên và người dùng dễ dàng theo dõi và làm theo:

```text
D:\AIoT\RecordOfRagnarok_CardioGuardAI\
├── .CLAUDE/                                # Thư mục chứa Kế hoạch Triển khai Mô-đun hóa
│   └── Plan/
│       ├── README_PLAN_INDEX.md            # Chỉ mục tổng hợp các Plan
│       ├── 01_project_scaffolding.md       # Task 1: Cấu hình khung PlatformIO & Header
│       ├── 02_battery_monitor.md           # Task 2: Đo Pin ADC GPIO 1 phần cứng
│       ├── 03_max30102_dsp_pipeline.md     # Task 3: Lọc nhiễu PPG 5 tầng tại biên
│       ├── 04_qmi8658_fall_detection.md    # Task 4: Thuật toán té ngã 3D 4 giai đoạn
│       ├── 05_tinyml_ai_models.md          # Task 5: Python AI Trainer & Exporter INT8
│       ├── 06_lvgl_ui_screens.md           # Task 6: LVGL UI & Màn hình nhấp nháy 15s đếm ngược
│       ├── 07_wifi_telegram_client.md      # Task 7: Wi-Fi & Telegram Bot API Client
│       └── 08_freertos_main_integration.md # Task 8: Tích hợp Đa luồng FreeRTOS Lõi kép
│
├── ai_models/                              # Mã nguồn Python Huấn luyện & Lượng tử hóa AI
│   ├── dataset_downloader.py               # Tải 3 tập dữ liệu từ Kaggle
│   ├── train_fall_model.py                 # Train AI Té ngã & convert INT8 (~8KB)
│   └── train_arrhythmia_model.py           # Train AI Loạn nhịp tim & convert INT8 (~30KB)
│
├── firmware/                               # Mã nguồn PlatformIO C++ cho Đồng hồ ESP32-S3
│   ├── platformio.ini                      # File cấu hình nạp firmware & thư viện
│   │
│   ├── include/                            # Các file Header C++ cấu hình & Trọng số AI
│   │   ├── app_config.h                    # Cấu hình GPIO, Wi-Fi, Telegram Token & Chat ID
│   │   ├── app_state.h                     # Cấu trúc lưu trạng thái WatchState & FallState
│   │   ├── ui_config.h                     # Bảng màu sắc semantic giao diện LVGL
│   │   ├── fall_model_data.h               # Trọng số Mô hình AI Té ngã INT8 (~8KB)
│   │   └── arrhythmia_model_data.h         # Trọng số Mô hình AI Loạn nhịp tim INT8 (~30KB)
│   │
│   ├── src/                                # Mã nguồn xử lý chính C++
│   │   ├── main.cpp                        # Khởi tạo Lõi kép FreeRTOS Core 0 & Core 1
│   │   │
│   │   ├── battery/                        # Đọc dung lượng Pin ADC GPIO 1
│   │   │   ├── battery_monitor.h
│   │   │   └── battery_monitor.cpp
│   │   │
│   │   ├── sensors/                        # Driver Cảm biến & Lọc nhiễu DSP 5 tầng
│   │   │   ├── max30102_service.h          # MAX30102 PPG Driver & 5-Stage DSP Noise Filter
│   │   │   ├── max30102_service.cpp
│   │   │   ├── qmi8658_service.h           # QMI8658 3D IMU Driver & Edge AI Fall Predictor
│   │   │   └── qmi8658_service.cpp
│   │   │
│   │   ├── ui/                             # Giao diện Màn hình tròn 240x240 LVGL
│   │   │   ├── ui.h
│   │   │   ├── ui.cpp
│   │   │   ├── screen_home.cpp             # Màn hình HOME chính
│   │   │   ├── screen_heart_rate.cpp       # Màn hình đo nhịp tim chi tiết
│   │   │   ├── screen_spo2.cpp             # Màn hình đo SpO2 chi tiết
│   │   │   └── screen_fall_alert.cpp       # Màn hình nhấp nháy đỏ 15s đếm ngược + Nút CANCEL
│   │   │
│   │   └── connectivity/                   # Kết nối Wi-Fi & Telegram Bot API Client
│   │       ├── telegram_bot.h              # Gọi Telegram HTTPS API POST
│   │       └── telegram_bot.cpp
│   │
│   └── assets/                             # Font chữ & Icons LVGL
│
├── README.md                               # Giới thiệu tổng quan dự án CardioGuardAI
└── SYSTEM_ARCHITECTURE_SPEC.md             # Bản Đặc tả Kiến trúc Kỹ thuật Toàn diện
```

---

## 🛠️ Luồng hoạt động (System Workflow)

```text
+-----------------------------------------------------------------------+
|                       ESP32-S3 Smartwatch (Edge)                      |
|  - Cảm biến MAX30102 (Lọc nhiễu Tín hiệu DSP 5 Tầng)                 |
|  - Cảm biến QMI8658 + TinyML AI INT8 (Dự đoán Té ngã & Loạn nhịp tim) |
+-----------------------------------------------------------------------+
                                   |
              (Nghi ngờ Té ngã / Loạn nhịp tim / Bấm SOS)
                                   v
+-----------------------------------------------------------------------+
|               Cảnh báo Nhấp nháy Màn hình (15s Đếm ngược)             |
|             [ Nút HỦY / CANCEL to trên Màn hình Cảm ứng ]             |
+-----------------------------------------------------------------------+
           |                                             |
    (Bấm CANCEL)                                   (Hết 15s Timeout)
           v                                             v
+-----------------------+               +-------------------------------+
|  ✓ Trở về bình thường |               | Gửi Telegram Bot API (Wi-Fi)  |
+-----------------------+               +-------------------------------+
                                                         |
                                                         v
                                        +-------------------------------+
                                        |  📱 Telegram Group Gia đình  |
                                        | (Tất cả thành viên nhận tin)  |
                                        +-------------------------------+
```

---

## 📖 Tài liệu Hướng dẫn Chi tiết
- 📄 **[`SYSTEM_ARCHITECTURE_SPEC.md`](SYSTEM_ARCHITECTURE_SPEC.md)** - Bản Đặc tả Kiến trúc Kỹ thuật Toàn diện.
- 📂 **[`.CLAUDE/Plan/README_PLAN_INDEX.md`](.CLAUDE/Plan/README_PLAN_INDEX.md)** - Kế hoạch Triển khai Lập trình Mô-đun hóa.

---
*Developed for RecordOfRagnarok CardioGuardAI Project.*
