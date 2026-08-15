# Hướng Dẫn Test Board ESP32-S3-Touch-LCD-1.28-B bằng PlatformIO

Dự án này chứa file cấu hình `platformio.ini` và chương trình kiểm tra toàn bộ phần cứng (`src/main.cpp`) cho bo mạch **Waveshare ESP32-S3-Touch-LCD-1.28-B**.

---

## 1. Các Tính Năng Được Kiểm Tra Trong Mã Nguồn

Chương trình test sẽ tự động thực hiện các tác vụ sau khi khởi động:

1. **Kiểm tra bộ nhớ (Internal RAM & 2MB PSRAM):** 
   - Kiểm tra xem 2MB PSRAM có được nhận diện hay không.
   - Thử nghiệm cấp phát động (malloc) 500KB trên vùng nhớ PSRAM.
2. **Quét bus I2C (SDA = GPIO6, SCL = GPIO7):**
   - Tự động dò tìm địa chỉ của chip cảm ứng **CST816S** (địa chỉ `0x15`).
   - Tự động dò tìm địa chỉ của chip cảm biến IMU **QMI8658** (địa chỉ `0x6B`).
3. **Kiểm tra Màn hình LCD GC9A01 (1.28" 240x240):**
   - Đèn nền Backlight PWM (GPIO2).
   - Đổi màu màn hình (Đỏ, Xanh lá, Xanh dương, Đen).
   - Vẽ giao diện đồng hồ tròn với văn bản và trạng thái hệ thống.
4. **Kiểm tra Màn hình cảm ứng CST816S:**
   - Đọc tọa độ chạm $(X, Y)$ và cử chỉ (gesture).
   - Hiển thị điểm chạm màu đỏ trực tiếp trên màn hình và xuất qua Serial Monitor.
5. **Kiểm tra Cảm biến IMU QMI8658 (6 trục):**
   - Đọc thanh ghi `WHO_AM_I` (`0x05`).
   - Khởi tạo và đọc dữ liệu Gia tốc kế (Accelerometer $X, Y, Z$) theo thời gian thực.
6. **Kiểm tra 6 chân GPIO Mở rộng (Cổng SH1.0):**
   - Đảo trạng thái HIGH/LOW định kỳ 1 giây trên các chân `GPIO15`, `GPIO16`, `GPIO17`, `GPIO18`, `GPIO21`, `GPIO33`.

---

## 2. Cấu Trúc Thư Mục Dự Án PlatformIO

```text
d:/AIoT/Test/
├── platformio.ini         # File cấu hình PlatformIO (Cài đặt board, PSRAM, USB CDC, thư viện)
├── src/
│   └── main.cpp           # Mã nguồn C++ kiểm tra toàn bộ phần cứng
└── README.md              # Tài liệu hướng dẫn sử dụng
```

---

## 3. Hướng Dẫn Biên Dịch Và Nạp Code

### **Bước 1: Mở dự án trong VS Code**
1. Mở **VS Code**, cài đặt extension **PlatformIO IDE**.
2. Chọn `File` -> `Open Folder...` và chọn thư mục `d:\AIoT\Test`.

### **Bước 2: Cắm cáp USB Type-C**
1. Cắm bo mạch vào máy tính qua cổng USB Type-C.
2. Nếu máy tính không tự nhận cổng COM, đưa bo mạch vào **Download Mode**:
   - Nhấn giữ nút **BOOT**.
   - Nhấn rồi thả nút **RESET**.
   - Thả nút **BOOT**.

### **Bước 3: Biên dịch & Nạp Code (Build & Upload)**
- Bấm icon **Checkmark (✔)** ở thanh công cụ phía dưới VS Code để **Build**.
- Bấm icon **Mũi tên sang phải (➔)** để **Upload** chương trình lên ESP32-S3.
- Hoặc chạy lệnh CLI:
  ```bash
  pio run -t upload
  ```

### **Bước 4: Xem Serial Monitor**
- Bấm icon **Zắc cắm/Plug (🔌)** hoặc chạy lệnh:
  ```bash
  pio device monitor
  ```
- Tốc độ Baud: **115200**.

---

## 4. Kết Quả Kỳ Vọng Trên Serial Monitor

```text
==================================================
 Waveshare ESP32-S3-Touch-LCD-1.28 Test Firmware 
==================================================

--- [1] Checking Memory & PSRAM ---
Internal Free Heap: 325412 bytes
PSRAM Found! Total Size: 2097152 bytes (2.00 MB)
Free PSRAM: 2097152 bytes
SUCCESS: Allocated 500 KB in PSRAM successfully!

--- [2] Scanning I2C Bus (SDA=6, SCL=7) ---
I2C device found at address 0x15 (CST816S Touch Controller)
I2C device found at address 0x6B (QMI8658 6-Axis IMU)
Scan finished. Found 2 device(s).

--- [3] Initializing QMI8658 IMU ---
QMI8658 WHO_AM_I Reg (0x05): 0x05 (Expected: 0x05)
SUCCESS: QMI8658 IMU initialized successfully.

--- [4] Initializing GC9A01 LCD ---
SUCCESS: LCD initial test completed.

--- [5] Configuring Expansion GPIO Pins ---
GPIO 15 set as OUTPUT
GPIO 16 set as OUTPUT
...
>>> All hardware initialization complete. Entering main loop... <<<

[SYSTEM #0001] FreeHeap: 320140 B | FreePSRAM: 2097152 B | Accel: X=  -124 Y=   452 Z= 16210
[TOUCH] X: 118 | Y: 120 | Gesture ID: 0
```
