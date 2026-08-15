# System Architecture & Feature Specification
## Project: CardioGuardAI (RecordOfRagnarok_CardioGuardAI)

---

## 1. Tổng quan hệ thống (System Overview)

**CardioGuardAI** là hệ thống AIoT chăm sóc sức khỏe và cảnh báo sự cố khẩn cấp (Phát hiện té ngã, bất thường nhịp tim, nồng độ SpO2) toàn diện dành cho người cao tuổi và gia đình.

Hệ thống kết hợp 3 thành phần nòng cốt:
1. **Hardware (Wearable Watch)**: Đồng hồ thông minh dựa trên board **Waveshare ESP32-S3-Touch-LCD-1.28-B**, trang bị cảm biến nhịp tim/SpO2 (**MAX30102**) và cảm biến gia tốc 6 trục/IMU (**QMI8658**). Màn hình tròn 1.28" (240x240 px) hiển thị thời gian, nhịp tim, SpO2, trạng thái Wi-Fi & Bluetooth.
2. **Mobile Application (Flutter App)**: Ứng dụng di động đa nền tảng (iOS & Android) phân quyền 2 Role (Người dùng / Người thân & Admin quản trị) kết nối trực tiếp với Đồng hồ qua BLE và qua Cloud Server.
3. **Backend & Cloud Intelligence (Firebase & CardioGuard AI)**: Nền tảng Firebase (Auth, Firestore, Cloud Messaging FCM, Storage, Cloud Functions) xử lý đồng bộ dữ liệu thời gian thực, gửi cảnh báo khẩn cấp và mô hình AI phân tích nguy cơ sức khỏe.

---

## 2. Phân tích & Kế thừa từ Prototype mẫu (`d:\AIoT\Test`)

Hệ thống CardioGuardAI kế thừa các tiêu chuẩn kỹ thuật đã đạt được ở bản prototype:
- **Thiết kế UI Đồng hồ**: Màn hình tròn 240x240px, safe area (x=20..220, y=20..220), Dark UI tương phản cao, tối giản, hiển thị 1-2 giây đọc ngay thông số (Giờ, Ngày, Nhịp tim, SpO2, Wi-Fi, Pin, Trạng thái Fall Monitor).
- **Giao thức kết nối kép BLE + Wi-Fi**:
  - Khi có Wi-Fi: Đồng hồ đẩy dữ liệu và cảnh báo trực tiếp lên Firebase Cloud / Webhook khẩn cấp.
  - Khi mất Wi-Fi, còn BLE: Đồng hồ gửi gói tin `Fall Event` (12-byte Indicate) qua BLE tới App Flutter trên điện thoại của người dùng để App đứng ra làm Gateway gửi cảnh báo.
  - Khi mất cả 2: Lưu sự kiện vào bộ nhớ Flash NVS của đồng hồ để gửi bù ngay khi khôi phục mạng; App Flutter kích hoạt **Watchdog** (nếu quá 60 giây không nhận gói heartbeat `Device Status` sẽ tự báo động "Người thân đi quá xa hoặc đồng hồ mất nguồn").

---

## 3. Đề xuất Ý tưởng & Chức năng Nâng cao hoàn thiện Hệ thống (Enhanced Features Blueprint)

Dựa trên yêu cầu của bạn, dưới đây là bộ giải pháp mở rộng chi tiết và chuyên nghiệp để biến `CardioGuardAI` thành một sản phẩm thương mại hoàn chỉnh.

---

### A. Thiết bị Đồng hồ Thông minh (ESP32-S3 Firmware)

1. **Thuật toán Phát hiện Té ngã 3 Chiều (3-Axis Accelerometer AI)**:
   - Sử dụng cảm biến gia tốc X, Y, Z để tính độ lớn gia tốc tổng hợp $SV = \sqrt{a_x^2 + a_y^2 + a_z^2}$.
   - **Quy trình 4 giai đoạn nhận diện té ngã**:
     - *Giai đoạn 1 (Rơi tự do)*: $SV < 0.5g$ trong thời gian ngắn (100–300ms).
     - *Giai đoạn 2 (Va chạm mạnh)*: $SV > 2.8g - 3.5g$ ngay sau giai đoạn rơi tự do.
     - *Giai đoạn 3 (Biến đổi tư thế)*: Góc nghiêng của cơ thể thay đổi $> 60^\circ$ so với trước khi ngã.
     - *Giai đoạn 4 (Nằm yên không cử động)*: Gia tốc ổn định ở mức $1g$ trong 2–3 giây sau va chạm.
   - **Xử lý Báo động Giả (False Alarm Cancellation)**: Màn hình hiển thị đếm ngược 15 giây kèm chuông/rung. Người đeo có thể nhấn trực tiếp nút **CANCEL** trên màn hình cảm ứng để hủy bỏ nếu chỉ là sự cố vô ý (ngã nhẹ hoặc làm rơi đồng hồ).

2. **Lọc nhiễu & Tăng độ chính xác PPG (Nhịp tim & SpO2)**:
   - Tích hợp bộ lọc trung bình động (Rolling Average) và kiểm tra cờ cử động (`motionArtifact`).
   - Chỉ cập nhật chỉ số chính xác khi cờ `hrValid` và `spo2Valid` được xác nhận.

3. **Quản lý Năng lượng Thông minh**:
   - Tự động hạ độ sáng / tắt màn hình sau 15 giây không tương tác.
   - Tính năng **Raise-to-Wake**: Sử dụng ngắt gia tốc từ QMI8658 để bật sáng màn hình khi người dùng đưa tay lên xem giờ.

---

### B. Ứng dụng Di động Flutter (Flutter Mobile App)

Ứng dụng được thiết kế tối ưu UX/UI với 2 phân quyền người dùng chính (Role-Based Access Control - RBAC):

#### 1. Role 1: Người dùng / Người thân (User / Caregiver Role)

* **Xác thực & Hồ sơ Cá nhân (Auth & Medical Profile)**:
  - Đăng ký / Đăng nhập qua Số điện thoại (OTP SMS), Email, hoặc Google Sign-In.
  - Lưu trữ thông tin cá nhân đầy đủ: **Họ tên, Ngày tháng năm sinh, Số CCCD/CMND, Ảnh đại diện, Nhóm máu, Tiền sử bệnh lý (Huyết áp, Tim mạch, Tiểu đường), Danh sách thuốc dị ứng**.
* **Quản lý Nhóm Gia đình (Family Management)**:
  - **Tạo nhóm gia đình**: Mỗi người dùng có thể tạo một hoặc nhiều "Nhóm Gia Đình" (Ví dụ: *Gia đình Nội*, *Gia đình Ngoại*).
  - **Thêm thành viên**: Mời thành viên bằng Mã QR hoặc Số điện thoại.
  - **Phân quyền trong gia đình**:
    - *Người giám sát chính (Primary Caregiver)*: Nhận thông báo đầu tiên, được sửa cấu hình đồng hồ.
    - *Thành viên gia đình (Family Members)*: Nhận thông báo đồng thời khi có biến động.
* **Mạng lưới Kết bạn & Liên lạc Khẩn cấp (Friend & Emergency Contacts)**:
  - Cho phép kết bạn với người ngoài gia đình (hàng xóm, bác sĩ riêng, bạn bè thân thiết).
  - Nếu xảy ra sự cố khẩn cấp mà các thành viên gia đình không phản hồi/xác nhận trong vòng 30 giây, ứng dụng tự động gửi thông báo khẩn cấp đến danh sách **Bạn bè / Liên hệ dự phòng**.
* **Theo dõi Thời gian thực & Lịch sử Sức khỏe (Real-time Live Vitals & Trends)**:
  - Hiển thị thông số trực tiếp: Nhịp tim (BPM), SpO2 (%), Phần trăm pin, Trạng thái kết nối (BLE Direct / Firebase Cloud).
  - Đồ thị diễn biến sức khỏe theo ngày/tuần/tháng (Sử dụng biểu đồ đường tương tác).
* **Màn hình Cảnh báo Khẩn cấp Kích hoạt Chuông báo (Critical Alert UI)**:
  - Khi nhận tín hiệu Té ngã / Bấm nút SOS: Điện thoại phát chuông báo động với âm lượng tối đa (kể cả khi điện thoại ở chế độ Im lặng/Do Not Disturb).
  - Hiển thị vị trí GPS của người đeo đồng hồ trên bản đồ (tận dụng GPS của điện thoại hoặc Geolocation Wi-Fi).
  - Nút bấm nhanh gọi **115 (Cấp cứu)** hoặc gọi trực tiếp cho Người thân.
* **Cảnh báo Vùng an toàn & BLE Watchdog (Geofencing & Distance Alert)**:
  - Báo động khi người đeo đồng hồ di chuyển ra khỏi tầm kết nối Bluetooth của điện thoại hoặc đi ra khỏi vùng Wi-Fi nhà (>60 giây không có tín hiệu).

---

#### 2. Role 2: Admin Quản trị Hệ thống (System Admin Role)

* **Quản lý Người dùng & Xác thực Danh tính**:
  - Tra cứu, tìm kiếm tài khoản người dùng theo Tên, Số điện thoại, CCCD.
  - Phê duyệt / Khóa tài khoản nếu phát hiện vi phạm hoặc gian lận.
  - Kiểm tra thông tin hồ sơ sức khỏe và trạng thái liên kết đồng hồ.
* **Dashboard Giám sát Vận hành (Operations Dashboard)**:
  - Thống kê tổng số tài khoản đang hoạt động.
  - Thống kê **Số nhóm gia đình (Group Count)** đang hoạt động.
  - Thống kê số lượng thiết bị đồng hồ đang kết nối trực tuyến (Active Devices).
  - Nhật ký sự kiện cảnh báo toàn hệ thống (System Emergency Logs & Resolution Rate).
* **Quản lý Gói Bảo hành & Dịch vụ (Subscription & Warranty Management)**:
  - **Quản lý Serial Number & Đồng hồ**: Ánh xạ mã Serial đồng hồ với thông tin khách hàng và ngày kích hoạt bảo hành.
  - **Quản lý Gói Dịch vụ / Bảo hành**:
    - *Gói Cơ bản (Free)*: Lưu dữ liệu 7 ngày, gửi cảnh báo qua App Push Notification.
    - *Gói Cao cấp (Premium Family)*: Lưu dữ liệu không giới hạn, cảnh báo cuộc gọi/SMS tự động, hỗ trợ kỹ thuật 24/7.
  - **Tích hợp Cổng Thanh toán (Payment Gateway)**:
    - Cho phép người dùng gia hạn gói bảo hành/dịch vụ ngay trên App qua **VietQR (SeABank/Vietcombank/MBBank), MoMo, ZaloPay**.
    - Admin theo dõi doanh thu, hóa đơn thanh toán, danh sách gói sắp hết hạn và gửi thông báo nhắc gia hạn tự động.

---

### C. Kiến trúc Backend Firebase & Cloud Functions

1. **Firebase Authentication**:
   - Quản lý phiên đăng nhập bảo mật (JWT Tokens).
   - Tích hợp Custom Claims để phân quyền Role (`role: 'user'` vs `role: 'admin'`).
2. **Cloud Firestore (NoSQL Database)**:
   - `users`: `{ uid, name, dob, cccd, phone, medicalProfile, role, createdAt }`
   - `families`: `{ familyId, name, ownerId, memberIds[], friendIds[], createdAt }`
   - `devices`: `{ deviceId, serialNumber, pairedUserId, status, battery, lastSeen }`
   - `health_logs`: `{ logId, deviceId, timestamp, heartRate, spo2, motionState }`
   - `alerts`: `{ alertId, deviceId, userId, type ('FALL'/'SOS'), status, timestamp, location }`
   - `subscriptions`: `{ subId, userId, packageType, startDate, endDate, paymentStatus, transactionId }`
3. **Firebase Cloud Messaging (FCM)**:
   - Gửi thông báo đẩy Data Payload ưu tiên cao nhất (`priority: high`).
   - Tùy chỉnh Sound Channel trên Android/iOS để vượt qua chế độ im lặng.
4. **Firebase Cloud Functions (Node.js)**:
   - **Escalation Engine**: Khi có sự kiện Té ngã $\rightarrow$ Gửi FCM cho Nhóm Gia đình. Nếu sau 30 giây không ai ấn "Đã tiếp nhận" $\rightarrow$ Tự động chuyển tiếp sang danh sách Bạn bè + Gửi SMS khẩn cấp (qua Twilio hoặc Zalo ZNS API).
   - **Cron Job**: Tự động kiểm tra các gói bảo hành sắp hết hạn trước 7 ngày để gửi thông báo nhắc thanh toán.

---

## 4. Cấu trúc Thư mục Dự án Đề xuất (`RecordOfRagnarok_CardioGuardAI`)

Để dự án phát triển sạch sẻ, dễ bảo trì và mở rộng, cấu trúc mã nguồn được phân chia thành các thư mục độc lập:

```text
RecordOfRagnarok_CardioGuardAI/
│
├── README.md                           # Giới thiệu tổng quan dự án & Hướng dẫn cài đặt
├── SYSTEM_ARCHITECTURE_SPEC.md         # (File hiện tại) Đặc tả kiến trúc hệ thống
│
├── firmware/                           # Mã nguồn Firmware ESP32-S3 (PlatformIO / C++)
│   ├── platformio.ini
│   ├── include/                        # File cấu hình pin, BLE UUIDs, Wi-Fi
│   ├── src/
│   │   ├── main.cpp
│   │   ├── app/                        # Quản lý trạng thái & luồng chính
│   │   ├── ui/                         # Giao diện màn hình LVGL 240x240
│   │   ├── sensors/                    # Driver MAX30102 & QMI8658
│   │   ├── fall_detection/             # Thuật toán nhận diện té ngã X-Y-Z
│   │   └── connectivity/               # Quản lý Wi-Fi, BLE Service & Push Alert
│   └── assets/                         # Font chữ & Icon LVGL
│
├── app/                                # Mã nguồn Ứng dụng Di động (Flutter)
│   ├── pubspec.yaml
│   ├── android/
│   ├── ios/
│   └── lib/
│       ├── main.dart
│       ├── core/                       # Theme, Constants, Route, BLE Manager, Firebase Service
│       ├── models/                     # User, HealthData, Family, Alert, Subscription Models
│       ├── providers/                  # State Management (Riverpod / Provider / BLoC)
│       └── views/
│           ├── auth/                   # Màn hình Đăng ký, Đăng nhập, Quên mật khẩu
│           ├── user/                   # Dashboard Người dùng, Nhóm gia đình, Sức khỏe, Báo động
│           └── admin/                  # Dashboard Admin, Quản lý tài khoản, Bảo hành & Thanh toán
│
├── backend/                            # Firebase Infrastructure & Cloud Functions
│   ├── firebase.json
│   ├── firestore.rules                 # Quy tắc bảo mật Firestore RBAC
│   ├── firestore.indexes.json
│   └── functions/                      # Node.js Cloud Functions (SMS Escalation, Cron Job)
│
└── docs/                               # Tài liệu hướng dẫn & Thiết kế UI
    ├── ble-protocol.md                 # Đặc tả giao thức truyền nhận BLE
    ├── database-schema.md              # Sơ đồ CSDL Firebase Firestore
    └── api-docs.md                     # Tài liệu API tích hợp Cổng thanh toán
```

---

## 5. Lộ trình Triển khai (Implementation Roadmap)

1. **Giai đoạn 1: Chuẩn hóa Firmware & Giao thức BLE/Wi-Fi** (Hoàn thiện mã nguồn C++ trên đồng hồ ESP32-S3 dựa theo spec `d:\AIoT\Test`).
2. **Giai đoạn 2: Xây dựng Backend Firebase & Database Schema** (Khởi tạo Firebase Auth, Firestore Rules, Cloud Messaging).
3. **Giai đoạn 3: Phát triển App Flutter - Role User** (Xây dựng UI/UX, Đăng ký/Đăng nhập, Tạo nhóm gia đình, Nhận thông báo đẩy FCM, Đồng bộ BLE).
4. **Giai đoạn 4: Phát triển App Flutter - Role Admin** (Xây dựng UI Dashboard Admin, Quản lý CCCD người dùng, Quản lý Gói bảo hành & Tích hợp VietQR/MoMo).
5. **Giai đoạn 5: Tích hợp Hệ thống, AI Anomaly Detection & Kiểm thử E2E** (Test kịch bản giả lập té ngã, ngắt mạng, gửi SMS khẩn cấp và thanh toán thực tế).
