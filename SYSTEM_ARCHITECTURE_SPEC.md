# System Architecture Specification (Simplified Edge & Telegram Alert)
## Project: CardioGuardAI (RecordOfRagnarok_CardioGuardAI)

---

## 1. Mục tiêu cốt lõi (Core Focus)

Dự án **CardioGuardAI** tập trung vào hệ thống cảnh báo sức khỏe và té ngã tối giản, tin cậy cao, xử lý hoàn toàn **tại biên (Edge Computing)** trên Đồng hồ ESP32-S3:

1. **Edge Processing (Xử lý tại biên trên Đồng hồ)**:
   - Lọc nhiễu tín hiệu nhịp tim & SpO2 từ cảm biến PPG (**MAX30102**).
   - Phân tích và phát hiện té ngã bằng cảm biến gia tốc 3 chiều X-Y-Z (**QMI8658**).
2. **Cảnh báo tại chỗ (Local Warning)**:
   - Phát hiệu ứng **nhấp nháy màn hình đồng hồ** kèm đếm ngược 15 giây để người đeo hủy nếu ngã nhẹ hoặc ấn nhầm.
3. **Thông báo Khẩn cấp trực tiếp qua Telegram (Direct Telegram Alert)**:
   - Khi hết đếm ngược hoặc bấm SOS $\rightarrow$ Đồng hồ tự động gửi tin nhắn cảnh báo khẩn cấp trực tiếp qua Wi-Fi tới **Group Chat Telegram của Gia đình** để tất cả thành viên gia đình nhận được tức thì.

---

## 2. Đặc tả Kỹ thuật Chi tiết

### A. Xử lý tại biên (Edge Logic - Firmware ESP32-S3)

* **Lọc nhiễu Nhịp tim & SpO2 (MAX30102)**:
  - Sử dụng thuật toán lọc trung bình động (Rolling Average) và kiểm tra cờ nhiễu cử động (`motionArtifact == 0`).
  - Chỉ hiển thị và ghi nhận chỉ số khi cờ `hrValid` và `spo2Valid` đạt tín hiệu tin cậy.
  - Xử lý trạng thái tháo đồng hồ (`skinContact == 0`): Màn hình hiển thị `-- BPM` và `-- % SpO2`.

* **Thuật toán Phát hiện Té ngã 3D (QMI8658 Accelerometer)**:
  - Tính độ lớn gia tốc tổng hợp $SV = \sqrt{a_x^2 + a_y^2 + a_z^2}$.
  - Nhận diện 4 giai đoạn té ngã: *Rơi tự do ($<0.5g$) $\rightarrow$ Va chạm mạnh ($>2.8g$) $\rightarrow$ Đổi góc nghiêng thân người ($>60^\circ$) $\rightarrow$ Bất động*.

* **Hiệu ứng Cảnh báo Màn hình & Đếm ngược 15s**:
  - Khi nghi ngờ té ngã: Màn hình tròn 240x240 chớp nháy viền đỏ (`#FF3B30`) và đen (`#000000`) liên tục.
  - Hiển thị số đếm ngược `15... 14... 13...` và nút **HỦY / CANCEL** kích thước to ở phần dưới màn hình.
  - Nhấn **CANCEL** $\rightarrow$ Trở về bình thường (`✓ ĐÃ HỦY`).
  - Hết 15s không bấm $\rightarrow$ Chuyển sang gửi cảnh báo.

---

### B. Cơ chế Gửi Cảnh báo qua Telegram Bot API (Direct Wi-Fi)

Đồng hồ kết nối Wi-Fi nhà và gọi trực tiếp HTTPS API của Telegram (`https://api.telegram.org/bot<BOT_TOKEN>/sendMessage`):

* **Nội dung tin nhắn Telegram gửi vào Group Gia đình**:
  ```text
  🚨 CẢNH BÁO KHẨN CẤP — TÉ NGÃ PHÁT HIỆN!
  -----------------------------------
  👤 Người đeo: Cụ Nguyễn Văn A
  ⏰ Thời gian: 13:45:00 - 15/08/2026
  ❤️ Nhịp tim lúc xảy ra: 88 BPM
  🫁 SpO2: 97%
  🔋 Pin đồng hồ: 82%
  ⚠️ Trạng thái: Người đeo không ấn HỦY sau 15s đếm ngược!
  -----------------------------------
  👉 Vui lòng kiểm tra người thân ngay lập tức!
  ```

---

## 3. Cấu trúc Giao diện Màn hình Đồng hồ (LVGL 240x240 px)

Kế thừa chuẩn giao diện từ `d:\AIoT\Test`:

1. **HOME**: Hiển thị Giờ (HH:MM:SS), Ngày, Trạng thái Wi-Fi, Pin %, Nhịp tim (BPM), SpO2 (%) và Trạng thái `FALL: OK`.
2. **HEART RATE**: Nhấp chạm icon Tim trên HOME $\rightarrow$ Mở màn hình nhịp tim to + Biểu đồ mini 1 đường.
3. **SpO2**: Nhấp chạm icon SpO2 trên HOME $\rightarrow$ Mở màn hình SpO2 to + Trạng thái tín hiệu.
4. **FALL ALERT**: Màn hình đếm ngược 15s nhấp nháy viền đỏ + Nút CANCEL.
5. **SENDING**: Hiển thị trạng thái `📡 ĐANG GỬI TELEGRAM...` $\rightarrow$ `✓ ĐÃ GỬI` / `❌ THẤT BẠI`.

---

## 4. Cấu trúc Thư mục Mã nguồn Dự án (`RecordOfRagnarok_CardioGuardAI`)

```text
RecordOfRagnarok_CardioGuardAI/
│
├── README.md                           # Giới thiệu & Hướng dẫn nạp code
├── SYSTEM_ARCHITECTURE_SPEC.md         # Đặc tả kiến trúc Edge & Telegram
│
└── firmware/                           # Mã nguồn PlatformIO C++ cho ESP32-S3
    ├── platformio.ini
    ├── include/                        # Cấu hình Pin, Wi-Fi SSID/PASS, Telegram BOT Token & Chat ID
    ├── src/
    │   ├── main.cpp
    │   ├── ui/                         # Giao diện LVGL (Home, HeartRate, SpO2, FallAlert)
    │   ├── sensors/                    # Driver cảm biến MAX30102 & QMI8658
    │   ├── fall_detection/             # Thuật toán té ngã Edge X-Y-Z
    │   └── connectivity/               # Driver Wi-Fi & Telegram Bot API Client
    └── assets/                         # Fonts & Icons LVGL
```
