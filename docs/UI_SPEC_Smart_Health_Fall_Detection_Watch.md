# Smart Health & Fall Detection Watch - UI Design Specification

## 1. Mục tiêu

Đặc tả UI/UX cho Antigravity/Codex/GPT triển khai giao diện trên **Waveshare ESP32-S3-Touch-LCD-1.28-B**.

Mục tiêu sản phẩm:
- Đo nhịp tim và SpO2 từ MH-ET LIVE MAX30102.
- Phát hiện té ngã từ QMI8658.
- Hiển thị cảnh báo trên đồng hồ.
- Gửi cảnh báo tới người nhà.
- Giao diện tối giản, dễ đọc trên màn hình tròn **1.28 inch, 240×240 px**.

> Không tự ý thêm phần cứng hoặc chức năng ngoài scope.

---

# 2. Quy tắc thiết kế bắt buộc

## 2.1 Kích thước

```text
Display: Circular
Resolution: 240 × 240 px
Physical size: 1.28 inch
Aspect ratio: 1:1
Coordinate space: x=0..239, y=0..239
```

Không thiết kế UI chữ nhật rồi crop thành hình tròn.

## 2.2 Safe area

Vùng nội dung chính:

```text
x ≈ 20..220
y ≈ 20..220
```

Không đặt nội dung quan trọng sát mép tròn.

## 2.3 Phong cách

- Modern smartwatch
- Dark UI
- High contrast
- Minimal
- Health/safety oriented
- Không đồng hồ cơ
- Không kim giờ/phút
- Không số La Mã
- Không hiệu ứng 3D nặng
- Animation nhẹ

Màu semantic:

```text
Background: #000000
Primary: #FFFFFF
Blue: #2196F3
Heart rate: #FF3B30
SpO2: #00A8FF
Normal: #39D353
Warning: #FFB020
Critical: #FF3B30
Secondary text: #9E9E9E
```

---

# 3. Scope chức năng

## Bắt buộc

```text
- Digital clock
- Date
- Wi-Fi status
- Battery percentage
- Heart rate
- SpO2
- Fall monitoring
- Fall alert
- Notification status
- Touch navigation
```

## Không đưa vào MVP

```text
- Body temperature
- GPS
- Step counter
- Sleep tracking
- Weather
- Music
- Phone call
- General smartphone notifications
- Camera
- Complex animations
```

> Không hiển thị nhiệt độ cơ thể vì thiết bị hiện tại không có cảm biến nhiệt độ cơ thể chuyên dụng.

---

# 4. Kiến trúc màn hình

```text
HOME
 ├── HEART RATE
 ├── SpO2
 ├── FALL MONITOR
 ├── FALL ALERT
 └── NOTIFICATION
```

Điều hướng:

```text
HOME
 ├── Tap BPM    → HEART RATE
 ├── Tap SpO2   → SpO2
 ├── Swipe Left → FALL MONITOR
 └── Swipe Down → NOTIFICATION

HEART RATE  ── Swipe Right → HOME
SpO2        ── Swipe Right → HOME
FALL MONITOR── Swipe Right → HOME
NOTIFICATION── Swipe Right → HOME

FALL ALERT
 ├── Tap CANCEL → HOME
 └── Timeout    → SENDING → ALERT SENT / SEND FAILED
```

---

# 5. HOME SCREEN

## Mục tiêu

Người dùng phải hiểu trạng thái đồng hồ trong 1–2 giây.

## Thông tin

```text
Top:
- Wi-Fi
- Battery %

Center:
- HH:MM:SS
- Day
- DD/MM/YYYY

Bottom:
- Heart rate
- SpO2
- Fall status
```

## Wireframe

```text
          WiFi       Battery

              10:30:45

               THỨ NĂM
              13/08/2026

        ─────────────────────

          ❤️ 72      🫁 98%
          BPM        SpO2

          🟢 FALL: OK
```

## Layout

Header:
```text
Wi-Fi: x≈28, y≈22
Battery: x≈185, y≈22
```

Clock:
```text
x≈35..205
y≈55..105
```

Date:
```text
THỨ NĂM
13/08/2026
```

Health row:
```text
Left:  ❤️ 72 BPM
Right: 🫁 98% SpO2
```

Fall status:
```text
🟢 FALL: OK
```

Alert state:
```text
🔴 FALL: ALERT
```

---

# 6. HEART RATE SCREEN

```text
          ← HEART RATE

              ❤️

              72
             BPM

           NORMAL

       ─────────────
       Mini chart
       ─────────────

             BACK
```

States:

```text
MEASURING
NORMAL
HIGH
LOW
NO SIGNAL
```

Chart:
- 1 line
- 20–40 recent samples
- Không dùng chart phức tạp
- Không legend dài

---

# 7. SpO2 SCREEN

```text
             SpO2

             🫁

             98%

            NORMAL

          Signal: GOOD

             ←
```

States:

```text
MEASURING
NORMAL
LOW
NO SIGNAL
```

Giá trị % là thành phần nổi bật nhất.

---

# 8. FALL MONITOR SCREEN

```text
        🚶 FALL MONITOR

         STATUS: ACTIVE

             ◉
        monitoring

          Last event:
           No event

             BACK
```

States:

```text
ACTIVE
PAUSED
DISABLED
ALERT
```

---

# 9. FALL ALERT SCREEN

Khi phát hiện té ngã:

```text
              ⚠

          FALL ALERT

        Are you OK?

              15
           seconds

            CANCEL
```

Countdown:

```text
15
14
13
...
3
2
1
```

Nút CANCEL phải lớn và dễ chạm.

Nếu người dùng hủy:

```text
             ✓

      ALERT CANCELLED

       You are safe
```

Sau 1–2 giây quay về HOME.

Nếu timeout:

```text
FALL ALERT
   ↓
SENDING...
   ↓
ALERT SENT
```

---

# 10. SENDING SCREEN

```text
            📡

         SENDING...

      Emergency alert

     Sending to caregiver
```

Nếu lỗi:

```text
            ⚠

       SEND FAILED

       Check connection
```

---

# 11. NOTIFICATION SCREEN

```text
          🔔 NOTIFICATION

       Fall alert sent

           17:32
        13/08/2026

        ✓ Delivered

            BACK
```

MVP chỉ cần thông báo của hệ thống cảnh báo, chưa cần notification tổng quát từ điện thoại.

---

# 12. QUICK MENU

Mở bằng swipe up:

```text
       ┌─────┬─────┬─────┐
       │ WiFi│ ⚙️  │ ⏰  │
       ├─────┼─────┼─────┤
       │ SOS │ ☀️  │  ℹ️ │
       └─────┴─────┴─────┘
```

Chức năng:
- WiFi
- Settings
- Alarm
- Emergency
- Brightness
- About

---

# 13. TOUCH INTERACTION

```text
Tap         → Open / Select / Confirm
Swipe Left  → Next page
Swipe Right → Back
Swipe Up    → Quick menu
Swipe Down  → Notifications
```

Long press chỉ triển khai nếu cần. Không dùng long press trong MVP nếu không có yêu cầu.

---

# 14. Animation

Được phép:
- Fade
- Slide
- Pulse
- Countdown
- Icon state change

Không nên:
- 3D
- Parallax
- Blur nặng
- Particle
- Shader phức tạp

---

# 15. Typography

Ưu tiên font sans-serif dễ đọc.

Clock lớn nhất.
BPM/SpO2 lớn thứ hai.
Label nhỏ hơn.

Không dùng font trang trí hoặc quá mảnh.

Tối đa 1–2 font family.

---

# 16. UI State Model

UI không tự đọc sensor.

```cpp
struct WatchState {
    uint8_t batteryPercent;
    bool wifiConnected;

    uint16_t heartRate;
    uint8_t spo2;

    bool fallMonitoring;
    bool fallDetected;

    bool notificationPending;
};
```

Luồng:

```text
Sensors
   ↓
WatchState
   ↓
UI Renderer
   ↓
LVGL
```

---

# 17. Fall State Machine

```text
NORMAL
  |
  | suspicious motion
  v
FALL_SUSPECTED
  |
  | 15s countdown
  |
  +---- User cancel ----> NORMAL
  |
  +---- Timeout --------> SENDING
                              |
                              +---- success --> ALERT_SENT
                              |
                              +---- failure --> SEND_FAILED
```

Logic phát hiện té phải độc lập với UI.

---

# 18. Data Source Mapping

| UI field | Data source |
|---|---|
| Time | RTC / system time |
| Date | RTC / system time |
| WiFi | WiFi manager |
| Battery | Battery monitor |
| Heart Rate | MAX30102 service |
| SpO2 | MAX30102 service |
| Fall Status | Fall Detection service |
| Notification | Alert manager |

UI chỉ nhận dữ liệu từ service/state layer.

---

# 19. Project Structure

```text
smart-health-watch/
│
├── README.md
├── UI_SPEC.md
├── platformio.ini
│
├── include/
│   ├── app_config.h
│   ├── app_state.h
│   └── ui_config.h
│
├── src/
│   ├── main.cpp
│   │
│   ├── app/
│   │   ├── app.cpp
│   │   ├── app.h
│   │   └── app_state.cpp
│   │
│   ├── ui/
│   │   ├── ui.cpp
│   │   ├── ui.h
│   │   ├── ui_theme.cpp
│   │   ├── ui_theme.h
│   │   ├── screen_home.cpp
│   │   ├── screen_home.h
│   │   ├── screen_heart_rate.cpp
│   │   ├── screen_heart_rate.h
│   │   ├── screen_spo2.cpp
│   │   ├── screen_spo2.h
│   │   ├── screen_fall_monitor.cpp
│   │   ├── screen_fall_monitor.h
│   │   ├── screen_fall_alert.cpp
│   │   ├── screen_fall_alert.h
│   │   ├── screen_notification.cpp
│   │   ├── screen_notification.h
│   │   ├── screen_quick_menu.cpp
│   │   └── screen_quick_menu.h
│   │
│   ├── sensors/
│   │   ├── max30102/
│   │   │   ├── max30102_service.cpp
│   │   │   └── max30102_service.h
│   │   └── qmi8658/
│   │       ├── qmi8658_service.cpp
│   │       └── qmi8658_service.h
│   │
│   ├── health/
│   │   ├── heart_rate.cpp
│   │   ├── heart_rate.h
│   │   ├── spo2.cpp
│   │   └── spo2.h
│   │
│   ├── fall_detection/
│   │   ├── fall_detector.cpp
│   │   ├── fall_detector.h
│   │   └── fall_state.cpp
│   │
│   ├── connectivity/
│   │   ├── wifi_manager.cpp
│   │   ├── wifi_manager.h
│   │   ├── api_client.cpp
│   │   └── api_client.h
│   │
│   ├── battery/
│   │   ├── battery_monitor.cpp
│   │   └── battery_monitor.h
│   │
│   └── notification/
│       ├── alert_manager.cpp
│       └── alert_manager.h
│
├── assets/
│   ├── fonts/
│   └── icons/
│
└── docs/
    ├── ui-flow.md
    ├── state-machine.md
    └── development-notes.md
```

---

# 20. Layer Responsibilities

## UI

Chỉ xử lý:
```text
Render
Input
Navigation
Animation
```

Không đọc sensor trực tiếp.

## Sensors

```text
Read sensor
Filter raw data
Expose measurements
```

## Health

```text
Calculate BPM
Calculate SpO2
```

## Fall Detection

```text
Read motion
Extract features
Detect fall
Manage fall event
```

## Connectivity

```text
WiFi
HTTP/API
Connection status
```

## Notification

```text
Create alert
Send alert
Track status
Retry
```

---

# 21. Refresh Strategy

```text
Clock:
1 Hz

Battery:
5–30 seconds

WiFi:
On state change

Heart rate:
1–2 Hz

SpO2:
1–2 Hz

Fall:
Event driven

Notification:
Event driven
```

Không redraw toàn màn hình mỗi loop.

---

# 22. Performance Rules

- Không tạo LVGL object liên tục.
- Tạo screen một lần và update widget.
- Tái sử dụng label/image/button.
- Không animation dài.
- Không refresh full-screen nếu không cần.
- Hạn chế heap allocation trong loop.
- Có thể tách sensor task và UI task.
- Chỉ update widget khi giá trị thay đổi.

---

# 23. Definition of Done

```text
[ ] Home render đúng 240×240
[ ] Time chính xác
[ ] Date chính xác
[ ] WiFi icon đúng trạng thái
[ ] Battery icon đúng trạng thái
[ ] BPM hiển thị
[ ] SpO2 hiển thị
[ ] Fall monitor hiển thị
[ ] Tap BPM → Heart Rate
[ ] Tap SpO2 → SpO2
[ ] Swipe navigation
[ ] Fall alert countdown
[ ] Cancel hoạt động
[ ] Timeout → Sending
[ ] Sent/Failed hiển thị
[ ] Notification page
[ ] Không có temperature
[ ] Không có mechanical clock
[ ] Không có Roman numerals
[ ] Không có hardware page
[ ] Không overflow khỏi hình tròn
```

---

# 24. Yêu cầu thực thi cho Antigravity

1. Đọc `UI_SPEC.md` trước khi code.
2. Không tự ý thay đổi layout nếu không có lý do kỹ thuật.
3. Tách UI khỏi sensor drivers.
4. Dùng state model trung tâm.
5. Nếu project dùng LVGL, tiếp tục sử dụng LVGL.
6. Mỗi screen có file `.cpp` và `.h` riêng.
7. Không đặt sensor logic trong `screen_home`.
8. Không đặt network/notification logic trong UI.
9. Mọi coordinate phải phù hợp 240×240.
10. Ưu tiên performance và maintainability.
11. Build/test sau từng screen.
12. Không thêm chức năng ngoài scope nếu chưa được yêu cầu.

## Thứ tự implementation

```text
Phase 1
→ UI theme
→ HOME

Phase 2
→ HEART RATE
→ SpO2

Phase 3
→ FALL MONITOR
→ FALL ALERT

Phase 4
→ NOTIFICATION
→ QUICK MENU

Phase 5
→ Touch navigation
→ Animation
→ Performance tuning
```

---

# 25. Design Principle

Đây là **Health & Fall Detection Watch**, không phải smartwatch đa chức năng.

Ưu tiên:

```text
READABILITY
    >
SAFETY
    >
RELIABILITY
    >
PERFORMANCE
    >
VISUAL EFFECTS
```

Mục tiêu của màn hình 240×240 là: **ít thông tin nhưng đúng thông tin, đọc nhanh, thao tác dễ và khi có té ngã thì cảnh báo phải nổi bật tuyệt đối.**
