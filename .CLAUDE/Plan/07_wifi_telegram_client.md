# Task 7: Wi-Fi & Direct Telegram Bot API HTTPS Client

## Mục tiêu
Lập trình module kết nối Wi-Fi và phát tin nhắn báo động trực tiếp qua Telegram Bot API (HTTPS POST) tới Group Chat Gia đình (Chat ID cấu hình trong `include/secrets.h`).

## Các File Cần Tạo / Sửa
- `firmware/src/connectivity/telegram_bot.h`
- `firmware/src/connectivity/telegram_bot.cpp`

## Các Bước Triển Khai
- [ ] **Bước 1**: Tạo `telegram_bot.h` khai báo class `TelegramBot`.
- [ ] **Bước 2**: Viết `telegram_bot.cpp` kết nối HTTPS `https://api.telegram.org/bot<TOKEN>/sendMessage`.
- [ ] **Bước 3**: Định dạng tin nhắn khẩn cấp (Tên người đeo, Thời gian, BPM, SpO2, Pin %).
- [ ] **Bước 4**: Kiểm thử gửi tin nhắn Telegram thực tế từ ESP32-S3.
