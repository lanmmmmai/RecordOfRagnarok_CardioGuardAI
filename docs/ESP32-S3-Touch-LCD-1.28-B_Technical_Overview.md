# Waveshare ESP32-S3-Touch-LCD-1.28-B

## 1. Tổng quan

**ESP32-S3-Touch-LCD-1.28-B** là bo mạch phát triển do Waveshare thiết
kế, tích hợp vi điều khiển ESP32-S3R2 cùng màn hình cảm ứng điện dung
1.28 inch, IMU 6 trục, hệ thống quản lý nguồn và các giao tiếp mở rộng.

Thiết bị phù hợp cho các ứng dụng:

-   AIoT
-   IoT
-   Wearable
-   Sensor dashboard
-   Robot controller
-   Motion detection
-   Embedded GUI
-   Thiết bị portable sử dụng pin Lithium

------------------------------------------------------------------------

## 2. Vi điều khiển ESP32-S3R2

Chip xử lý chính:

**ESP32-S3R2**

Thông tin:

-   Kiến trúc: Dual-core Xtensa 32-bit LX7
-   Tần số hoạt động: lên đến 240 MHz
-   Wi-Fi: 2.4 GHz, 802.11 b/g/n
-   Bluetooth: Bluetooth / BLE
-   PSRAM tích hợp: 2 MB

ESP32-S3R2 là thành phần trung tâm điều khiển toàn bộ các module trên
board.

Luồng tổng quát:

``` text
                    ESP32-S3R2
                         |
        +----------------+----------------+
        |                |                |
       LCD              Touch             IMU
        |                |                |
        +----------------+----------------+
                         |
                 Wi-Fi / Bluetooth
                         |
                    Application
```

------------------------------------------------------------------------

## 3. Bộ nhớ

### 3.1. PSRAM

Dung lượng:

**2 MB PSRAM**

PSRAM cung cấp thêm vùng nhớ cho các ứng dụng cần nhiều RAM, chẳng hạn:

-   Frame buffer
-   GUI
-   LVGL
-   Xử lý ảnh
-   Buffer dữ liệu cảm biến
-   Network buffer
-   Các ứng dụng AI/edge nhẹ

### 3.2. Flash

Chip Flash:

**W25Q128JVSIQ**

Dung lượng:

**16 MB NOR Flash**

Flash được sử dụng để lưu:

-   Firmware
-   Chương trình Arduino
-   Cấu hình
-   Font
-   Hình ảnh và tài nguyên giao diện
-   Dữ liệu không mất khi mất nguồn

------------------------------------------------------------------------

## 4. Màn hình

Board tích hợp màn hình:

-   Kích thước: **1.28 inch**
-   Hình dạng: **tròn**
-   Độ phân giải: **240 × 240 pixel**
-   Màu sắc: **65K màu**
-   Loại cảm ứng: **cảm ứng điện dung**

Màn hình phù hợp với:

-   Smartwatch
-   Dashboard
-   Sensor monitor
-   Robot controller
-   Wearable
-   Thiết bị IoT có giao diện trực quan

Ví dụ kiến trúc giao diện:

``` text
        +----------------+
      /                  \
     |       98%          |
     |                    |
     |      SENSOR        |
     |                    |
     |  TEMP: 31 C        |
     |  IMU : OK          |
      \                  /
        +----------------+
```

------------------------------------------------------------------------

## 5. Touchscreen

Màn hình sử dụng cảm ứng điện dung.

Touch có thể được sử dụng để:

-   Nhấn nút
-   Swipe
-   Chuyển màn hình
-   Điều khiển robot
-   Thay đổi thông số
-   Chọn chế độ hoạt động
-   Điều khiển thiết bị IoT

Luồng xử lý:

``` text
Touch
  |
  v
ESP32-S3
  |
  v
X/Y coordinate
  |
  v
UI/Application
  |
  v
Action
```

> Lưu ý: Tài liệu hình ảnh được cung cấp không ghi rõ tên IC touch, vì
> vậy không nên suy luận controller cụ thể chỉ từ các ảnh này.

------------------------------------------------------------------------

## 6. IMU QMI8658

Board tích hợp:

**QMI8658**

Đây là cảm biến IMU 6 trục, gồm:

### Accelerometer

Gia tốc kế 3 trục:

``` text
X
Y
Z
```

Có thể dùng để phát hiện:

-   Gia tốc
-   Nghiêng
-   Rung
-   Chuyển động
-   Orientation

### Gyroscope

Con quay hồi chuyển 3 trục:

``` text
X
Y
Z
```

Có thể dùng để phát hiện:

-   Xoay
-   Rotation
-   Angular velocity
-   Gesture
-   Motion

Tổng quan:

``` text
QMI8658
|
+-- Accelerometer
|   +-- X
|   +-- Y
|   +-- Z
|
+-- Gyroscope
    +-- X
    +-- Y
    +-- Z
```

------------------------------------------------------------------------

## 7. USB-UART CH343P

Board sử dụng:

**CH343P**

Đây là chip chuyển đổi USB sang UART.

Kiến trúc:

``` text
Computer
   |
 USB
   |
   v
CH343P
   |
 UART
   |
   v
ESP32-S3
```

USB-UART được sử dụng cho:

-   Nạp firmware
-   Serial Monitor
-   Debug
-   In log
-   Giao tiếp giữa máy tính và ESP32

------------------------------------------------------------------------

## 8. USB Type-C

Board có cổng:

**USB Type-C**

Cổng USB Type-C được sử dụng để:

-   Cấp nguồn
-   Ghi chương trình
-   Kết nối máy tính
-   In nhật ký/log

Workflow Arduino:

``` text
Arduino IDE
    |
    v
USB Type-C
    |
    v
CH343P
    |
    v
ESP32-S3R2
```

------------------------------------------------------------------------

## 9. Hệ thống nguồn

Board tích hợp nhiều thành phần quản lý nguồn.

### 9.1. ME6217C33M5G

Chip:

**ME6217C33M5G**

Thông tin được cung cấp:

-   Dòng đầu ra: khoảng 800 mA
-   Độ sụt áp thấp
-   Hiệu suất loại bỏ cao

Vai trò chính là ổn áp cho hệ thống.

``` text
Nguồn đầu vào
     |
     v
ME6217
     |
     v
3.3V
     |
     +-- ESP32
     +-- LCD
     +-- IMU
     +-- Các mạch khác
```

### 9.2. ETA6096

Chip:

**ETA6096**

Vai trò:

**Bộ quản lý sạc pin Lithium hiệu suất cao.**

Board hỗ trợ pin Lithium 3.7V và có khả năng sạc/xả thông qua hệ thống
quản lý nguồn tích hợp.

------------------------------------------------------------------------

## 10. Đầu nối pin

Board sử dụng:

**MX1.25 2P**

Thông tin:

-   Loại đầu nối: MX1.25 2P
-   Pin hỗ trợ: Lithium 3.7V
-   Hỗ trợ sạc và xả

Khu vực pin trên board được đánh dấu:

``` text
+   -
BAT
```

Kiến trúc nguồn có thể hình dung:

``` text
             USB-C
                |
                v
             ETA6096
          Battery Charger
                |
                v
          Li-ion 3.7V
                |
                v
              VSYS
                |
                v
            Regulator
                |
                v
              3.3V
```

------------------------------------------------------------------------

## 11. GPIO mở rộng

Board có đầu nối:

**SH1.0**

Theo thông tin trong ảnh, có 6 GPIO được đưa ra ngoài:

``` text
GPIO15
GPIO16
GPIO17
GPIO18
GPIO21
GPIO33
```

Ngoài ra header còn có các chân hệ thống:

``` text
GND
VSYS
RESET
BOOT
GND
3V3
```

Các GPIO mở rộng có thể dùng để kết nối:

-   Sensor
-   LED
-   Relay
-   Motor controller
-   UART device
-   I2C device
-   SPI device
-   GPIO interrupt
-   Module IoT

------------------------------------------------------------------------

## 12. Nút RESET

Board có nút:

**RESET**

Nút RESET dùng để khởi động lại ESP32-S3.

Luồng:

``` text
RESET
  |
  v
ESP32-S3 Restart
  |
  v
setup()
  |
  v
loop()
```

Trong quá trình phát triển Arduino, nút này rất hữu ích để reset chương
trình và kiểm tra trạng thái firmware.

------------------------------------------------------------------------

## 13. Nút BOOT

Board có nút:

**BOOT**

Nút BOOT được sử dụng để đưa ESP32-S3 vào chế độ download/programming
khi cần.

Workflow:

``` text
BOOT / RESET
      |
      v
Download Mode
      |
      v
Arduino IDE
      |
      v
Upload Firmware
```

------------------------------------------------------------------------

## 14. VSYS và 3V3

Board có các đường nguồn:

### VSYS

Là đường nguồn hệ thống của board.

### 3V3

Là nguồn 3.3V được sử dụng cho các mạch logic tương ứng.

Cần phân biệt VSYS và 3V3 khi kết nối module bên ngoài.

GPIO của ESP32-S3 sử dụng mức logic 3.3V, do đó cần kiểm tra mức logic
của thiết bị ngoại vi trước khi kết nối.

------------------------------------------------------------------------

## 15. Pinout được thể hiện trong tài liệu

### Nguồn và điều khiển

``` text
GND
VSYS
RESET
BOOT
GND
3V3
```

### GPIO mở rộng

``` text
GPIO15
GPIO16
GPIO17
GPIO18
GPIO21
GPIO33
```

> Đây là các chân được thể hiện trong ảnh pinout mà người dùng cung cấp.
> Không nên suy ra toàn bộ GPIO nội bộ của LCD, Touch hoặc IMU chỉ từ
> ảnh này.

------------------------------------------------------------------------

## 16. Sơ đồ kiến trúc phần cứng

``` text
                         +----------------------+
                         |      ESP32-S3R2      |
                         |                      |
                         | Dual-core Xtensa LX7 |
                         | Up to 240 MHz        |
                         | 2 MB PSRAM           |
                         | Wi-Fi / Bluetooth    |
                         +----------+-----------+
                                    |
             +----------------------+----------------------+
             |                      |                      |
             v                      v                      v
      +-------------+        +-------------+        +-------------+
      | LCD 1.28"   |        | Capacitive  |        |  QMI8658    |
      | 240 x 240   |        |   Touch     |        |   6-axis    |
      | 65K colors  |        |             |        |             |
      +-------------+        +-------------+        +-------------+
             |
             |
      +------+----------------------------------------------+
      |                                                     |
      v                                                     v
+-------------+                                     +-------------+
| 16MB Flash  |                                     | 2MB PSRAM   |
| W25Q128     |                                     |             |
+-------------+                                     +-------------+

                         +----------------------+
                         |     Connectivity     |
                         |                      |
                         | USB Type-C           |
                         | CH343P USB-UART      |
                         | Wi-Fi                |
                         | Bluetooth             |
                         +----------------------+

                         +----------------------+
                         |    Power System      |
                         |                      |
                         | ETA6096              |
                         | ME6217C33M5G          |
                         | Li-ion 3.7V          |
                         | MX1.25 2P            |
                         +----------------------+

                         +----------------------+
                         |    Expansion GPIO    |
                         |                      |
                         | GPIO15               |
                         | GPIO16               |
                         | GPIO17               |
                         | GPIO18               |
                         | GPIO21               |
                         | GPIO33               |
                         +----------------------+
```

------------------------------------------------------------------------

## 17. Nhìn dưới góc độ lập trình viên

Có thể chia board thành 5 tầng:

``` text
+--------------------------------------+
|            APPLICATION               |
|  AIoT / Robot / Wearable / IoT       |
+--------------------------------------+
|               GUI                    |
|          LCD + Touch + LVGL          |
+--------------------------------------+
|             SENSORS                  |
|             QMI8658                  |
+--------------------------------------+
|          CONNECTIVITY                |
|        Wi-Fi / Bluetooth             |
+--------------------------------------+
|               MCU                    |
|            ESP32-S3R2                |
+--------------------------------------+
|             HARDWARE                 |
| USB / Power / GPIO / Battery         |
+--------------------------------------+
```

Cách phân tầng này giúp dễ thiết kế phần mềm:

``` text
Application
    |
    v
Service / Business Logic
    |
    v
Sensor / Display / Communication Driver
    |
    v
ESP32 Hardware
```

------------------------------------------------------------------------

## 18. Các project phù hợp

### 18.1. AIoT

``` text
Sensor
   |
   v
ESP32-S3
   |
   +--> AI / Rule
   |
   +--> LCD
   |
   +--> Wi-Fi
          |
          v
       Backend
```

### 18.2. Wearable

``` text
QMI8658
   |
   v
Motion
   |
   v
ESP32-S3
   |
   v
Touch UI
   |
   v
LCD
```

### 18.3. Robot Controller

``` text
Touch LCD
     |
     v
ESP32-S3
     |
     +--> Wi-Fi
     |
     +--> Bluetooth
     |
     v
   Robot
```

### 18.4. Sensor Dashboard

``` text
Temperature ----+
IMU -------------+
Battery ----------+--> ESP32-S3 --> LCD
Other Sensors ----+              |
                                  v
                                Wi-Fi
```

------------------------------------------------------------------------

## 19. Bảng thông số tổng hợp

  Thành phần            Thông tin
  --------------------- ------------------------------------------------
  Model                 ESP32-S3-Touch-LCD-1.28-B
  Nhà sản xuất          Waveshare
  MCU                   ESP32-S3R2
  CPU                   Dual-core Xtensa 32-bit LX7
  Tần số                Up to 240 MHz
  Wi-Fi                 2.4 GHz, 802.11 b/g/n
  Bluetooth             Bluetooth / BLE
  PSRAM                 2 MB
  Flash                 16 MB NOR Flash
  Flash IC              W25Q128JVSIQ
  Display               1.28 inch
  Display shape         Tròn
  Resolution            240 × 240
  Color                 65K
  Touch                 Capacitive Touch
  IMU                   QMI8658
  Accelerometer         3 trục
  Gyroscope             3 trục
  USB-UART              CH343P
  USB                   Type-C
  Charger               ETA6096
  Regulator             ME6217C33M5G
  Regulator output      800 mA theo thông tin cung cấp
  Battery               Li-ion 3.7V
  Battery connector     MX1.25 2P
  Expansion connector   SH1.0
  GPIO mở rộng          GPIO15, GPIO16, GPIO17, GPIO18, GPIO21, GPIO33
  Control buttons       RESET, BOOT

------------------------------------------------------------------------

## 20. Kết luận

**ESP32-S3-Touch-LCD-1.28-B** có thể xem là một nền tảng embedded khá
hoàn chỉnh thay vì chỉ là một module ESP32.

Các thành phần quan trọng nhất là:

``` text
ESP32-S3R2
    +
16MB Flash
    +
2MB PSRAM
    +
1.28" 240x240 Touch LCD
    +
QMI8658 6-axis IMU
    +
Wi-Fi / Bluetooth
    +
USB Type-C / CH343P
    +
Li-ion Battery Management
    +
GPIO Expansion
```

Điểm mạnh của board nằm ở việc **MCU + giao diện người dùng + cảm biến
chuyển động + kết nối không dây + nguồn pin** đã được tích hợp trên một
thiết bị nhỏ gọn.

Với hướng phát triển AIoT/embedded, board này có thể đóng vai trò **edge
device**, nhận dữ liệu cảm biến, xử lý cục bộ, hiển thị trạng thái trên
LCD, sau đó truyền dữ liệu qua Wi-Fi/Bluetooth tới backend hoặc thiết bị
khác.
