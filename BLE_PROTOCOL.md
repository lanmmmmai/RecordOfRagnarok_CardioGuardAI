# BLE_PROTOCOL.md — Giao thức BLE giữa đồng hồ và điện thoại

> **Nguồn sự thật:** [src/connectivity/ble_service.cpp](src/connectivity/ble_service.cpp)
> và khối UUID trong [include/app_config.h](include/app_config.h).
> Ứng dụng điện thoại đọc thẳng các byte offset dưới đây, nên **khi đổi code phải đổi file này cùng lúc**.
> Nếu bạn thấy một dòng ở đây không khớp với code, code là bên đúng — hãy báo để sửa tài liệu.

---

## 1. Vì sao BLE tồn tại song song với Telegram

Đồng hồ có **hai đường báo động độc lập**, và đó là chủ ý:

| | BLE | Telegram |
|---|---|---|
| Bắn lúc nào | **Ngay khi bắt đầu đếm ngược** ([fall_detector.cpp:45](src/fall_detection/fall_detector.cpp#L45)) | Sau khi hết 15 giây |
| Cần gì | Điện thoại trong tầm ~10 m | Wi-Fi còn sống |
| Hỏng khi nào | Người đeo đi xa điện thoại | Mất điện router, ra khỏi nhà, Wi-Fi rớt |

Wi-Fi là điểm hỏng đơn lẻ dễ xảy ra nhất của thiết bị này. BLE bắn **trước** Telegram
15 giây, nên nếu Wi-Fi chết thì điện thoại vẫn là bên biết chuyện. Đây là lý do
`bleNotifyFallEvent()` được gọi trong `enterAlert()` chứ không phải sau khi hết đếm ngược.

---

## 2. Quảng bá & kết nối

| | Giá trị |
|---|---|
| Tên thiết bị | `HealthWatch` |
| Công suất phát | `ESP_PWR_LVL_P9` (+9 dBm, mức cao nhất) |
| Scan response | Bật |
| Ghép đôi / mã hoá | **Không có** — xem [§6 Bảo mật](#6-bảo-mật--hạn-chế-đã-biết) |
| Số kết nối cùng lúc | 1 (mặc định NimBLE) |

Sau khi điện thoại ngắt kết nối, đồng hồ **tự quảng bá lại ngay** (`onDisconnect` →
`startAdvertising()`), không cần khởi động lại.

---

## 3. Bảng GATT

**Service UUID:** `6e400001-b5a3-f393-e0a9-e50e24dcca9e`

| Characteristic | UUID | Thuộc tính | Độ dài | Chu kỳ |
|---|---|---|---|---|
| [Vitals](#31-vitals--chỉ-số-sức-khoẻ) | `6e400002-…` | `NOTIFY` | 8 byte | 2 giây |
| [Fall](#32-fall--sự-kiện-té-ngã) | `6e400003-…` | `INDICATE` | 12 byte | Theo sự kiện |
| [Status](#33-status--tình-trạng-hệ-thống) | `6e400004-…` | `READ` + `NOTIFY` | 4 byte | 2 giây |
| [Command](#34-command--lệnh-từ-điện-thoại) | `6e400005-…` | `WRITE` | 1 byte | Điện thoại chủ động |

Bốn UUID sau dùng chung 4 nhóm cuối `-b5a3-f393-e0a9-e50e24dcca9e` với service.

> ⚠️ Dải UUID này là dải **Nordic UART Service (NUS)**. Ứng dụng dò tìm theo tên
> service có thể nhầm đồng hồ là một thiết bị UART. Đây là hạn chế đã biết —
> xem [§6](#6-bảo-mật--hạn-chế-đã-biết).

**Vì sao Fall dùng `INDICATE` còn Vitals dùng `NOTIFY`:** `INDICATE` yêu cầu điện thoại
xác nhận ở tầng liên kết, `NOTIFY` thì không. Một chỉ số nhịp tim rơi mất thì 2 giây sau
có cái mới; một sự kiện té ngã rơi mất là mất luôn.

---

### 3.1 Vitals — chỉ số sức khoẻ

`NOTIFY`, 8 byte, gửi mỗi 2 giây **và chỉ khi đang có kết nối**.
Little-endian cho các trường 16-bit.

| Byte | Kiểu | Trường | Ý nghĩa |
|---|---|---|---|
| 0 | bitfield | Cờ | xem bảng bit bên dưới |
| 1–2 | `uint16` LE | `heartRateBPM` | Nhịp tim. **Chỉ hợp lệ khi bit 1 = 1** |
| 3 | `uint8` | `spo2Percent` | SpO₂ %. **Chỉ hợp lệ khi bit 2 = 1** |
| 4 | `uint8` | `signalQuality` | SQI 0–100 (**thang chưa hiệu chuẩn**) |
| 5 | `uint8` | `batteryPercent` | Pin % |
| 6 | `uint8` | `isCharging` | 1 = đang sạc |
| 7 | — | *dự phòng* | Luôn bằng 0 |

**Byte 0 — các bit cờ:**

| Bit | Mặt nạ | Tên | Ý nghĩa |
|---|---|---|---|
| 0 | `0x01` | `skinContact` | Cảm biến đang áp vào da |
| 1 | `0x02` | `hrValid` | Nhịp tim ở byte 1–2 đáng tin |
| 2 | `0x04` | `spo2Valid` | SpO₂ ở byte 3 đáng tin |
| 3 | `0x08` | `motionArtifact` | IMU báo tay đang cử động quá nhiều |

**Quy tắc bắt buộc cho ứng dụng:** khi `hrValid = 0` thì **hiển thị `--`**, tuyệt đối
không hiển thị byte 1–2 (nó bằng 0) và không hiển thị giá trị cũ như thể là giá trị mới.
Firmware cố ý khởi tạo nhịp tim bằng `0` chứ không phải một con số trông-có-vẻ-khoẻ-mạnh
([app_state.h:116](include/app_state.h#L116)); ứng dụng đừng phá vỡ nguyên tắc đó.

Khi `motionArtifact = 1`, nên làm mờ chỉ số kèm chú thích "đang cử động" thay vì giấu đi —
người dùng cần biết vì sao số không cập nhật.

---

### 3.2 Fall — sự kiện té ngã

`INDICATE`, 12 byte. Gửi **một lần** tại thời điểm bắt đầu đếm ngược 15 giây.

| Byte | Kiểu | Trường | Ý nghĩa |
|---|---|---|---|
| 0 | `uint8` | `eventType` | `1` = té ngã đã xác nhận · `2` = SOS thủ công |
| 1 | `uint8` | `fallState` | Trạng thái máy trạng thái, xem bảng dưới |
| 2 | `uint8` | Giờ | 0–23 |
| 3 | `uint8` | Phút | 0–59 |
| 4 | `uint8` | Giây | 0–59 |
| 5 | `uint8` | Ngày | 1–31 |
| 6 | `uint8` | Tháng | **1–12** (firmware đã cộng 1 vào `tm_mon`) |
| 7–8 | `uint16` LE | Năm | ví dụ `2026` |
| 9–10 | `uint16` LE | `heartRateBPM` | Nhịp tim lúc xảy ra sự kiện |
| 11 | `uint8` | `batteryPercent` | Pin % lúc xảy ra sự kiện |

**Giá trị `fallState` (byte 1):**

| Giá trị | Tên | Ý nghĩa |
|---|---|---|
| 0 | `NORMAL` | Bình thường |
| 1 | `SUSPECTED` | Đang xác nhận va chạm |
| 2 | `ALERT` | **Đang đếm ngược** — đây là giá trị bạn sẽ thấy trong gói này |
| 3 | `SENDING` | Đang gửi Telegram |
| 4 | `SENT` | Đã gửi xong |
| 5 | `FAILED` | Gửi thất bại |

> 🕐 **Cảnh báo về mốc thời gian.** Byte 2–8 lấy từ RTC của đồng hồ. Nếu Wi-Fi chưa
> kết nối được thì NTP chưa chạy, và firmware rơi về đếm giờ theo uptime
> ([main.cpp:153-158](src/main.cpp#L153-L158)) — lúc đó giờ/phút/giây là **thời gian
> chạy máy**, không phải giờ thật, còn ngày/tháng/năm đứng ở giá trị mặc định.
> Ứng dụng nên **ưu tiên dùng đồng hồ của điện thoại** làm mốc thời gian sự kiện,
> chỉ dùng byte 2–8 để đối chiếu.

**Ứng dụng nên làm gì khi nhận gói này:**

1. Báo động ngay — âm thanh + rung, kể cả khi máy đang im lặng. Đây là 15 giây duy nhất
   để người thân biết trước khi Telegram bắn.
2. Hiển thị `eventType`: "Phát hiện té ngã" khác hẳn "Người đeo tự bấm SOS".
3. Cho phép gọi điện ngay cho người đeo.
4. **Không tự động huỷ báo động.** Chỉ người dùng mới được huỷ, và huỷ bằng
   [lệnh `0x01`](#34-command--lệnh-từ-điện-thoại).

---

### 3.3 Status — tình trạng hệ thống

`READ` + `NOTIFY`, 4 byte, gửi mỗi 2 giây. Dùng cho màn hình chẩn đoán.

| Byte | Trường | Ý nghĩa |
|---|---|---|
| 0 | Cờ cảm biến | bit 0 `0x01` = IMU OK · bit 1 `0x02` = PPG OK · bit 2 `0x04` = cảm ứng OK |
| 1 | Wi-Fi | 1 = đã kết nối |
| 2 | `queuedAlerts` | Số cảnh báo đang xếp hàng trong NVS (0–8) |
| 3 | `batteryPercent` | Pin % |

**Hai giá trị đáng cảnh báo cho người dùng:**

- **Byte 0, bit 0 = 0 (IMU FAIL)** → **phát hiện té ngã đã ngừng hoạt động**.
  Firmware thoát sớm khỏi `updateFallDetector()` khi `imuOk = false`
  ([fall_detector.cpp:75](src/fall_detection/fall_detector.cpp#L75)).
  Đây là hỏng hóc nghiêm trọng nhất mà thiết bị có thể gặp — ứng dụng phải báo rõ,
  không được để lẫn trong danh sách chỉ số.
- **Byte 2 > 0** → có cảnh báo chưa gửi được vì mất mạng. Nếu con số này không giảm
  sau vài phút, mạng đang có vấn đề.

---

### 3.4 Command — lệnh từ điện thoại

`WRITE`, 1 byte.

| Giá trị | Lệnh | Hiệu lực |
|---|---|---|
| `0x01` | Huỷ cảnh báo | **Chỉ có tác dụng khi `fallState != NORMAL`.** Gọi `cancelFallAlert()` |
| `0x02` | Kích hoạt SOS | Gọi `triggerSimulatedFall()` — giống hệt bấm SOS trên đồng hồ |
| khác | *bỏ qua* | Ghi ra Serial `Unknown command 0x%02X ignored` |

Byte ghi vào rỗng (`length 0`) bị bỏ qua.

**Chi tiết triển khai:** callback BLE **không** tự thay đổi trạng thái. Nó chỉ đặt biến
`pendingCommand`, còn `updateBLEService()` trong `loop()` mới thực thi
([ble_service.cpp:38-41](src/connectivity/ble_service.cpp#L38-L41)). Lý do: callback chạy
trên task host của NimBLE, còn `g_watchState` được toàn bộ phần còn lại của firmware
đọc/ghi mà không có khoá. Sửa trạng thái từ callback là chạy đua dữ liệu.

**Hệ quả cho ứng dụng:** lệnh có độ trễ tới **một vòng `loop()`** (dưới 33 ms trong thực tế)
và **không có phản hồi ACK ở tầng ứng dụng**. Muốn biết lệnh đã ăn chưa thì theo dõi
`fallState` ở gói Fall / màn hình đồng hồ, đừng giả định thành công ngay sau khi ghi.

**Lệnh `0x02` là lệnh nguy hiểm** — nó khởi động một chuỗi sẽ nhắn tin cho cả gia đình.
Ứng dụng phải có bước xác nhận, đừng đặt nút này chỗ dễ chạm nhầm.

---

## 4. Cảnh báo mất kết nối (BLE leash)

Nếu điện thoại rời khỏi tầm quá `BLE_LEASH_TIMEOUT_MS` = **60 giây**, đồng hồ tự gửi
một tin Telegram: *"Đồng hồ đã mất kết nối Bluetooth với điện thoại quá lâu.
Người đeo có thể đã đi ra xa."*

Chỉ gửi **một lần** cho mỗi lần mất kết nối; cờ `leashAlertSent` được reset khi
điện thoại kết nối lại.

Đây là cảnh báo một chiều: khi cả BLE lẫn Wi-Fi cùng chết, đồng hồ không còn cách nào
liên lạc với ai, nên **phía ứng dụng cũng nên tự phát hiện mất kết nối và báo động** —
đừng chờ đồng hồ nói.

---

## 5. Mẫu tích hoạt cho ứng dụng

```text
1. Quét tìm thiết bị tên "HealthWatch" hoặc service 6e400001-…
2. Kết nối
3. Bật INDICATE cho Fall      (6e400003)  ← làm ĐẦU TIÊN, đây là thứ quan trọng nhất
4. Bật NOTIFY cho Status      (6e400004)
5. Bật NOTIFY cho Vitals      (6e400002)
6. Vòng lặp:
     - nhận Vitals mỗi 2s  → cập nhật màn hình, tôn trọng các bit valid
     - nhận Status mỗi 2s  → cảnh báo nếu IMU FAIL hoặc queuedAlerts tăng
     - nhận Fall           → BÁO ĐỘNG NGAY, kèm âm thanh + rung
     - ghi 0x01            → khi người dùng bấm huỷ
7. Mất kết nối → tự báo động sau ngưỡng của riêng ứng dụng, rồi quét lại
```

---

## 6. Bảo mật & hạn chế đã biết

| # | Vấn đề | Ảnh hưởng |
|---|---|---|
| 1 | 🔴 **Không ghép đôi, không mã hoá, không xác thực** | Bất kỳ thiết bị BLE nào trong tầm đều ghi được characteristic Command → **ai cũng có thể huỷ cảnh báo té ngã thật, hoặc bắn SOS giả**. Chấp nhận được cho đồ án; **phải sửa trước khi dùng thật** bằng NimBLE bonding + `NIMBLE_PROPERTY::WRITE_ENC` |
| 2 | 🟠 **Dữ liệu sức khoẻ phát quảng bá không mã hoá** | Nhịp tim và SpO₂ đi qua sóng dạng rõ; ai bắt gói cũng đọc được |
| 3 | 🟠 **Dùng nhầm dải UUID của Nordic UART Service** | Ứng dụng khác có thể nhận nhầm thiết bị. Nên đổi sang UUID 128-bit tự sinh |
| 4 | 🟡 **Mốc thời gian không đáng tin khi chưa có NTP** | Xem cảnh báo ở [§3.2](#32-fall--sự-kiện-té-ngã) |
| 5 | 🟡 **Không có ACK tầng ứng dụng cho Command** | Ứng dụng phải xác nhận gián tiếp qua `fallState` |
| 6 | ⚪ **Byte 7 của Vitals bỏ trống** | Còn chỗ cho một trường nữa mà không phá vỡ tương thích |

Hạng mục 1 và 3 nằm trong lộ trình; xem
[SYSTEM_ARCHITECTURE_SPEC.md §11](SYSTEM_ARCHITECTURE_SPEC.md#11-hạn-chế-đã-biết--lộ-trình).

---

## 7. Cách kiểm tra bằng tay

Dùng **nRF Connect** (Android/iOS) hoặc `bluetoothctl` (Linux):

1. Quét → tìm `HealthWatch` → Connect
2. Mở service `6e400001-…`, bật notify cho cả ba characteristic đọc được
3. Vitals và Status phải nhảy số **mỗi 2 giây**
4. Trên đồng hồ: vào Quick Menu → bấm SOS → phải nhận được một gói **12 byte**
   trên characteristic Fall, byte 0 = `0x02`
5. Ghi `0x01` vào characteristic Command → đồng hồ phải huỷ đếm ngược,
   Serial in ` [BLE] Cancel command from phone.`

Nếu bước 4 không ra gói nào: kiểm tra đã bật **indication** (không phải notification)
cho characteristic Fall.
