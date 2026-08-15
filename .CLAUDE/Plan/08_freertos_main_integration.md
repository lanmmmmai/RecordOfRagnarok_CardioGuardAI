> 🗄️ **FILE NÀY ĐÃ NGHỈ HƯU — mô tả một kiến trúc chưa từng tồn tại. Đừng làm theo.**
> Lý do và bảng đối chiếu với code thật: [README_PLAN_INDEX.md](README_PLAN_INDEX.md)
> Nguồn sự thật: [SYSTEM_ARCHITECTURE_SPEC.md](../../SYSTEM_ARCHITECTURE_SPEC.md) và [src/](../../src/)

---

# Task 8: FreeRTOS Dual-Core Task Integration & Main Program Entry

## Mục tiêu
Tích hợp toàn bộ các mô-đun vào chương trình chính `src/main.cpp`, phân chia chạy đa luồng trên Lõi kép ESP32-S3 (Core 0 & Core 1).

## Các File Cần Tạo / Sửa
- `firmware/src/main.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Khởi tạo Bus I2C, SPI, nút BOOT GPIO 0 và các cảm biến.
- [ ] **Bước 2**: Phân chia Core 0: Xử lý Wi-Fi, Telegram Client & Ngắt nút bấm SOS GPIO 0.
- [ ] **Bước 3**: Phân chia Core 1: Chạy 100Hz DSP 5 Tầng, AI Fall Predictor & LVGL Renderer.
- [ ] **Bước 4**: Đơn vị kiểm thử toàn bộ hệ thống E2E.
