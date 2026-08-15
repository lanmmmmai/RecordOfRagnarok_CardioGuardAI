> 🗄️ **FILE NÀY ĐÃ NGHỈ HƯU — mô tả một kiến trúc chưa từng tồn tại. Đừng làm theo.**
> Lý do và bảng đối chiếu với code thật: [README_PLAN_INDEX.md](README_PLAN_INDEX.md)
> Nguồn sự thật: [SYSTEM_ARCHITECTURE_SPEC.md](../../SYSTEM_ARCHITECTURE_SPEC.md) và [src/](../../src/)

---

# Task 2: Hardware Battery Voltage Monitor Module (GPIO 1 ADC)

## Mục tiêu
Lập trình module đo dung lượng pin sạc LiPo 3.7V từ mạch phân áp ADC phần cứng trên chân GPIO 1 và tự động kích hoạt cảnh báo pin yếu.

## Các File Cần Tạo / Sửa
- `firmware/src/battery/battery_monitor.h`
- `firmware/src/battery/battery_monitor.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Tạo header `battery_monitor.h` khai báo class `BatteryMonitor`.
- [ ] **Bước 2**: Viết `battery_monitor.cpp` đo 20 mẫu ADC liên tiếp, tính điện áp V_bat = V_adc * 2.0, nội suy % pin (3.3V - 4.15V).
- [ ] **Bước 3**: Tạo cờ kiểm tra pin yếu <= 15% (`lowBatAlertSent`).
- [ ] **Bước 4**: Đơn vị kiểm thử đo điện áp pin.
