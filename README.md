# RecordOfRagnarok_CardioGuardAI 🛡️❤️

> **CardioGuardAI** - Smart AIoT Health Monitoring & Fall Detection System (ESP32-S3 Watch + Flutter App + Firebase Cloud).

---

## 📌 Project Overview
**CardioGuardAI** là hệ thống AIoT giám sát sức khỏe toàn diện và cảnh báo khẩn cấp (Phát hiện té ngã, nhịp tim bất thường, SpO2 thấp) dành cho người thân và gia đình.

### Các thành phần chính:
1. **Wearable Smartwatch (ESP32-S3)**: Màn hình tròn 1.28" (240x240 px), cảm biến **MAX30102** (Nhịp tim & SpO2) + **QMI8658** (Gia tốc 6 trục / Phát hiện té ngã X-Y-Z).
2. **Flutter Mobile App**: Ứng dụng di động phân quyền **User (Người dùng/Người thân)** và **Admin (Quản trị hệ thống & Bảo hành)**.
3. **Firebase Cloud Backend**: Firebase Auth, Firestore Database, Firebase Cloud Messaging (FCM), Cloud Functions & AI Anomaly Detection.

---

## 📄 Documentation & Specifications
Tài liệu kiến trúc chi tiết và đặc tả hệ thống được lưu tại:
- 📖 [SYSTEM_ARCHITECTURE_SPEC.md](SYSTEM_ARCHITECTURE_SPEC.md) - Đặc tả toàn bộ hệ thống, thuật toán té ngã 3D, kiến trúc Flutter App, sơ đồ Firestore & gói bảo hành.

---

## 🛠️ System Architecture

```text
+-----------------------+           BLE (Notify/Indicate)           +-----------------------+
|  ESP32-S3 Smartwatch  | <=======================================> |  Flutter Mobile App   |
| (MAX30102 + QMI8658)  |                                           | (User & Admin Roles)  |
+-----------------------+                                           +-----------------------+
            |                                                                   |
            | Wi-Fi (Direct Direct Alert)                                       | Firebase SDK
            v                                                                   v
+-------------------------------------------------------------------------------------------+
|                                  Firebase Cloud Platform                                  |
|  - Firebase Auth (Custom Claims Role)       - Firestore Realtime Database                 |
|  - Cloud Messaging FCM (Sound Alarm)        - Cloud Functions (SMS Escalation & Cron)     |
+-------------------------------------------------------------------------------------------+
```

---

## 🚀 Getting Started

### 1. Firmware (ESP32-S3)
```bash
cd firmware
pio run --target upload
```

### 2. Mobile App (Flutter)
```bash
cd app
flutter pub get
flutter run
```

---
*Developed for RecordOfRagnarok CardioGuardAI Project.*
