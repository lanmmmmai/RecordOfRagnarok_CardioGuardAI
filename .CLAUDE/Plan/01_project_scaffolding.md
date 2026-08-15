# Task 1: Project Scaffolding & Configuration Setup

## Mục tiêu
Khởi tạo cấu trúc dự án PlatformIO cho vi điều khiển Waveshare ESP32-S3-Touch-LCD-1.28 và cấu hình các file header trung tâm.

## Các File Cần Tạo / Sửa
- `firmware/platformio.ini`
- `firmware/include/app_config.h`
- `firmware/include/app_state.h`
- `firmware/include/ui_config.h`

## Các Bước Triển Khai
- [ ] **Bước 1**: Viết file `platformio.ini` nạp đủ thư viện LVGL 8.x, MAX30105, ArduinoJson.
- [ ] **Bước 2**: Viết `app_config.h` chứa sơ đồ chân GPIO. Thông số Wi-Fi, Telegram Bot Token & Chat ID nằm ở `include/secrets.h` (đã git-ignore, **không commit**).
- [ ] **Bước 3**: Viết `app_state.h` định nghĩa model lưu trạng thái đồng hồ `WatchState` & enum `FallState`.
- [ ] **Bước 4**: Viết `ui_config.h` định nghĩa màu sắc semantic.
- [ ] **Bước 5**: Kiểm thử biên dịch khung code.
