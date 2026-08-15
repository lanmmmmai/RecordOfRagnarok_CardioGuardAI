# Task 6: LVGL UI Engine & Flashing Red 15s Countdown Alert Screen

## Mục tiêu
Thiết kế giao diện màn hình tròn 240x240 px trên LVGL v8 bao gồm màn hình HOME, Nhịp tim/SpO2 và màn hình Nhấp nháy viền màu đỏ đếm ngược 15s kèm Nút CANCEL chạm cảm ứng.

## Các File Cần Tạo / Sửa
- `firmware/src/ui/ui.h`
- `firmware/src/ui/ui.cpp`
- `firmware/src/ui/screen_home.cpp`
- `firmware/src/ui/screen_fall_alert.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Khởi tạo LVGL display buffer và driver GC9A01 LCD.
- [ ] **Bước 2**: Dựng màn hình HOME (Giờ, Ngày, Wi-Fi, Pin %, Nhịp tim, SpO2, FALL: OK).
- [ ] **Bước 3**: Dựng màn hình đếm ngược 15s chớp nháy màu đỏ/đen + Nút CANCEL cảm ứng kích thước lớn.
- [ ] **Bước 4**: Bấm CANCEL -> Hiện '✓ ĐÃ HỦY' trong 1.5s và quay về HOME.
