# RecordOfRagnarok_CardioGuardAI 🛡️❤️

> **CardioGuardAI** - Hệ thống Cảnh báo Té ngã & Nhịp tim tại Biên (ESP32-S3 Watch + Direct Telegram Alert).

---

## 📌 Tổng quan Hệ thống
**CardioGuardAI** tập trung hoàn toàn vào sự tối giản, độ tin cậy và xử lý thời gian thực tại biên:

1. **Xử lý tại biên (Edge Computing on ESP32-S3)**:
   - **MAX30102**: Đo & Lọc nhiễu tín hiệu nhịp tim & SpO2.
   - **QMI8658 (3-Axis Accelerometer)**: Thuật toán phát hiện té ngã X-Y-Z ngay trên đồng hồ.
2. **Cảnh báo tại chỗ (Local Flashing Alert)**:
   - Màn hình đếm ngược 15s chớp nháy màu đỏ + Nút chạm **CANCEL** hủy cảnh báo giả.
3. **Cảnh báo Khẩn cấp trực tiếp qua Telegram (Direct Telegram Bot Alert)**:
   - Đồng hồ kết nối Wi-Fi tự động gửi tin nhắn báo động chi tiết tới **Group Chat Telegram của Gia đình** khi có sự cố.

---

## 📄 Tài liệu Đặc tả
- 📖 [SYSTEM_ARCHITECTURE_SPEC.md](SYSTEM_ARCHITECTURE_SPEC.md) - Chi tiết thuật toán Edge, cấu trúc UI và giao thức Telegram Bot API.

---

## 🛠️ Luồng hoạt động (Workflow)

```text
+-----------------------------------------------------------------------+
|                       ESP32-S3 Smartwatch (Edge)                      |
|  - Cảm biến MAX30102 (Lọc nhiễu PPG)                                  |
|  - Cảm biến QMI8658 (Thuật toán Té ngã Gia tốc 3D X-Y-Z)              |
+-----------------------------------------------------------------------+
                                   |
                  (Nghi ngờ Té ngã / Bấm SOS)
                                   v
+-----------------------------------------------------------------------+
|               Cảnh báo Nhấp nháy Màn hình (15s Đếm ngược)             |
|                  [ Nút HỦY / CANCEL trên Màn hình ]                   |
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
*Developed for RecordOfRagnarok CardioGuardAI Project.*
