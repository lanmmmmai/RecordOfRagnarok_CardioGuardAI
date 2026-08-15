> 🗄️ **FILE NÀY ĐÃ NGHỈ HƯU — mô tả một kiến trúc chưa từng tồn tại. Đừng làm theo.**
> Lý do và bảng đối chiếu với code thật: [README_PLAN_INDEX.md](README_PLAN_INDEX.md)
> Nguồn sự thật: [SYSTEM_ARCHITECTURE_SPEC.md](../../SYSTEM_ARCHITECTURE_SPEC.md) và [src/](../../src/)

---

# Task 4: QMI8658 3D IMU Driver & 4-Stage Edge Fall Detection Algorithm

## Mục tiêu
Lập trình Driver cảm biến gia tốc QMI8658 và Thuật toán phát hiện té ngã 3D 4 giai đoạn tại biên.

## Các File Cần Tạo / Sửa
- `firmware/src/sensors/qmi8658_service.h`
- `firmware/src/sensors/qmi8658_service.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Tạo `qmi8658_service.h` khai báo class `QMI8658Service`.
- [ ] **Bước 2**: Viết `qmi8658_service.cpp` tính gia tốc SV = sqrt(ax^2 + ay^2 + az^2).
- [ ] **Bước 3**: Lập trình 4 giai đoạn: Rơi tự do (<0.5g) -> Va chạm mạnh (>3.2g) -> Đổi góc nghiêng (>60 deg) -> Nằm yên 3 giây.
- [ ] **Bước 4**: Kích hoạt cờ `fallDetected` khi đủ 4 giai đoạn.
