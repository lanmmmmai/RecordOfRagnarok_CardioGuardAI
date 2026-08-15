# Task 3: MAX30102 PPG Driver & 5-Stage Edge DSP Noise Filter Pipeline

## Mục tiêu
Lập trình Driver cảm biến MAX30102 tích hợp Chuỗi 5 tầng Lọc nhiễu Tín hiệu Số (DSP Pipeline) trực tiếp tại biên ESP32-S3 để triệt tiêu số ảo đột biến.

## Các File Cần Tạo / Sửa
- `firmware/src/sensors/max30102_service.h`
- `firmware/src/sensors/max30102_service.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Tạo `max30102_service.h` khai báo class `MAX30102Service`.
- [ ] **Bước 2**: Viết `max30102_service.cpp` cài đặt 5 tầng lọc DSP:
  1. IIR Bandpass Filter (0.5 – 4.0 Hz)
  2. NLMS Adaptive Filter (Loại bỏ nhiễu cử động tay từ QMI8658)
  3. FFT Peak Detection
  4. Outlier Rejection (Rate-of-Change Gate <= 15 BPM/s + Median Filter 5 mẫu)
  5. 1D Kalman Filter + SQI Check (>75%)
- [ ] **Bước 3**: Kiểm tra cờ tiếp xúc da `skinContact` (hiển thị `-- BPM` khi tháo đồng hồ).
- [ ] **Bước 4**: Kiểm thử đơn vị các tầng lọc DSP.
