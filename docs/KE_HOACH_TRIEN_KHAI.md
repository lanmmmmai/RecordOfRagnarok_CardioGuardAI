# CardioGuardAI — Kế hoạch Triển khai Chi tiết

> **Trạng thái:** viết ngày 2026-08-15, sau đợt rà soát toàn bộ mã nguồn.
> Thay thế phần lộ trình trong plan cũ. Các Giai đoạn 0–2b, 3, 5a **đã xong**.

**Mục tiêu:** đưa CardioGuardAI từ "firmware chạy được nhưng chưa ai kiểm chứng"
thành thiết bị có số liệu đánh giá đầy đủ cho báo cáo, và có đường rõ ràng tới mức
dám đeo thật.

**Kiến trúc:** ESP32-S3R2 chạy Arduino trên PlatformIO, một vòng lặp hợp tác duy nhất
(không FreeRTOS task trừ `alertTask` gửi TLS). Cảm biến PPG MAX30102 trên bus I2C thứ hai,
IMU QMI8658 + cảm ứng CST816S trên bus thứ nhất. Giao diện TFT_eSPI + sprite 240×240.

**Công nghệ:** PlatformIO · Arduino ESP32 2.x · TFT_eSPI 2.5.43 · NimBLE-Arduino 1.4.x ·
SparkFun MAX3010x · Python + scikit-learn + emlearn (huấn luyện, chạy trên PC)

---

## Ràng buộc Toàn cục

Mọi giai đoạn dưới đây đều phải tuân thủ, không nhắc lại trong từng nhiệm vụ:

- **`include/secrets.h` KHÔNG BAO GIỜ được commit.** Đã git-ignore. Trước mỗi commit:
  `git status --short | grep -i secret` phải rỗng.
- **Không in giá trị token Telegram ra bất kỳ đâu** — kể cả log gỡ lỗi, kể cả thông báo lỗi.
- **Mỗi giai đoạn kết thúc bằng `pio run` sạch** trước khi commit. Cảnh báo `TOUCH_CS pin not
  defined` của TFT_eSPI là cảnh báo có sẵn, bỏ qua được; cảnh báo mới thì không.
- **Ngưỡng mới phải nằm trong `include/app_config.h`**, kèm comment giải thích *vì sao* là con
  số đó, không phải chỉ *nó là gì*. Không hardcode ngưỡng trong `.cpp`.
- **Vòng lặp chính không được chặn quá ~5 ms.** Chu kỳ IMU là 20 ms; chặn lâu hơn là mất mẫu
  té ngã. Đây là lý do `getLocalTime()` đã bị bỏ timeout 1000 ms.
- **Kiểm tra `git log --oneline -1` trước mỗi lần ghi file.** Lịch sử repo này đã từng bị
  13 commit song song chen vào giữa một nhiệm vụ.
- **Mọi số đo trên phần cứng phải ghi vào tài liệu ngay khi đo được**, không giữ trong đầu.
  SPEC §7 và §11 là nơi ghi.
- **Ngôn ngữ:** tài liệu tiếng Việt, comment trong code tiếng Anh (theo đúng code hiện có).

---

## Cấu trúc File — cái gì nằm ở đâu

| File | Trách nhiệm | Giai đoạn đụng vào |
|---|---|---|
| `include/app_config.h` | Toàn bộ hằng số + ngưỡng, kèm lý do | 5, 6 |
| `include/app_state.h` | `WatchState` — struct trạng thái dùng chung | 5, 6, 7 |
| `src/sensors/max30102_service.cpp` | Đọc PPG, phát hiện nhịp, SpO2, SQI | 5 |
| `src/sensors/battery_monitor.cpp` | Đo pin, đường cong LiPo, phát hiện sạc | 6 |
| `src/fall_detection/fall_detector.cpp` | Máy trạng thái 4 pha | 3b, 7 |
| `src/connectivity/alert_dispatcher.cpp` | Hàng đợi NVS + gửi Telegram | 6 |
| `src/main.cpp` | Vòng lặp hợp tác, lịch chạy các module | 6 |
| **Mới:** `src/dsp/rr_analysis.{h,cpp}` | Vòng đệm khoảng RR + đặc trưng HRV | 5, 7 |
| **Mới:** `src/ai/fall_model.h` | Cây quyết định té ngã (emlearn sinh) | 7 |
| **Mới:** `src/ai/rr_model.h` | Model sàng lọc nhịp bất thường | 7 |
| **Mới:** `ai_models/` | Script huấn luyện Python + notebook | 7, 8 |

**Nguyên tắc tách file:** phân tích khoảng RR tách riêng khỏi `max30102_service.cpp` vì file đó
đã 283 dòng và đang gánh 3 việc (đọc FIFO, phát hiện nhịp, SpO2). Thêm HRV vào nữa là quá tải.
Model AI để riêng vì chúng do máy sinh, không sửa tay.

---

# GIAI ĐOẠN 3b — Kiểm chứng phần cứng ⬅️ **LÀM TRƯỚC**

**Vì sao đứng đầu:** bốn đợt sửa đang treo lơ lửng — lỗi té ngã (`v0.2-fall-fix`), van thoát
nhịp tim, tự xoá màn cảnh báo, đường cong pin. Tất cả biên dịch sạch, logic đã đọc lại,
**nhưng chưa lần nào chạm phần cứng**. Xây tiếp lên trên nền chưa kiểm chứng là cách chắc chắn
nhất để sau này phải gỡ ngược.

**Người thực hiện:** bạn (cần thiết bị thật). Tôi đọc log cùng và ghi số vào tài liệu.

### Nhiệm vụ 3b.1 — Nạp firmware và xác nhận khởi động

- [ ] **Bước 1:** Nạp và mở serial monitor

```bash
cd /d/AIoT/RecordOfRagnarok_CardioGuardAI
pio run --target upload
pio device monitor -b 115200
```

- [ ] **Bước 2:** Xác nhận cả 3 cảm biến báo OK

Chờ dòng `[SENSOR]` trong log khởi động. Mong đợi:
```
-> SUCCESS: QMI8658 IMU Detected
-> SUCCESS: CST816S Touch Detected
-> SUCCESS: MAX30102 Detected
```
Nếu MAX30102 thiếu → kiểm tra dây module rời (đây là điểm hỏng cơ khí đã biết, SPEC §11 mục 12).

- [ ] **Bước 3:** Xác nhận NTP không chặn vòng lặp

Mong đợi thấy `NTP request sent, waiting for the clock in the background.` **ngay lập tức**,
rồi `SUCCESS: Real-Time Clock Synchronized` xuất hiện ở một dòng log sau đó — không phải cùng lúc.
Nếu hai dòng liền nhau tức là bản sửa H1 không có tác dụng.

### Nhiệm vụ 3b.2 — Kiểm chứng phát hiện té ngã

- [ ] **Bước 1:** Thử đường tắt trước

Chạm nút SOS trong Quick Menu (gọi `triggerSimulatedFall()`). Mong đợi: màn cảnh báo + đếm ngược
15 giây. Bước này tách bạch "máy trạng thái cảnh báo hỏng" khỏi "phát hiện té ngã hỏng" — nếu
bước này đã sai thì không cần thả thiết bị.

- [ ] **Bước 2:** Thả thiết bị 10 lần lên đệm, 4 kiểu

| Kiểu | Cách mô phỏng | Đường vào mong đợi |
|---|---|---|
| Ngã trước | Thả từ ~1 m, tay duỗi | A (rơi tự do → va chạm) |
| Ngã ngang | Thả nghiêng | A |
| Ngã khuỵu | Hạ nhanh từ ~40 cm, không rơi tự do | **B (va chạm độc lập 3.2g)** |
| Trượt tường | Trượt dọc mặt phẳng nghiêng rồi dừng | B |

Kiểu 3 và 4 chính là lý do đường vào B tồn tại. Nếu chúng không kích hoạt, `FALL_IMPACT_STANDALONE_G`
đang quá cao.

- [ ] **Bước 3:** Ghi lại dòng log quyết định của **cả 10 lần**

Log in ra `totalG` đỉnh, `confirmMaxDev`, `angleDeg`, đường vào, kết quả. Chép nguyên văn vào
một file để đối chiếu — đây vừa là dữ liệu hiệu chuẩn, vừa là dữ liệu huấn luyện cho Giai đoạn 7.

- [ ] **Bước 4:** Đối chiếu và kết luận

| Quan sát | Nghĩa là | Xử lý |
|---|---|---|
| Bắn ≥ 8/10 | Ngưỡng hợp lý | Ghi số vào SPEC §11 mục 1, đánh dấu ĐÃ KIỂM CHỨNG |
| `confirmMaxDev` sát 0.35 | Còn dội sau 400 ms | Nới `FALL_SETTLE_MS` lên 500–600 |
| `angleDeg` < 30 mà vẫn là ngã thật | Cổ tay không đổi hướng đủ | Hạ `FALL_ORIENTATION_MIN_DEG` xuống 20–25 |
| Kiểu 3/4 không bắn | Ngưỡng B quá cao | Hạ `FALL_IMPACT_STANDALONE_G` xuống 2.8 |

### Nhiệm vụ 3b.3 — Kiểm chứng van thoát nhịp tim

- [ ] **Bước 1:** Đeo đồng hồ, ngồi yên 2 phút, quan sát log

- [ ] **Bước 2:** Đếm số dòng `Rate gate stuck`

| Số dòng | Nghĩa là | Xử lý |
|---|---|---|
| 0 | Bộ chặn không cản trở gì | Tốt, giữ nguyên |
| 1–2 | Bình thường lúc mới bắt nhịp | Giữ nguyên |
| > 5 | `PPG_MAX_BPM_STEP = 15` quá chặt cho cổ tay | Nới lên 20–25 |

- [ ] **Bước 3:** Đối chiếu số hiển thị

Bắt mạch cổ tay đếm tay trong 30 giây × 2, so với số trên màn hình. Lệch quá 10 BPM là dấu hiệu
tầng phát hiện nhịp có vấn đề chứ không phải bộ lọc.

- [ ] **Bước 4:** Đo giá trị IR khi **đang đeo** (dữ liệu cho Giai đoạn 5)

Ghi lại `irRaw` trong `[WATCH LOG]` khi đeo ổn định, và khi tháo ra để hở sáng.
`PPG_CONTACT_IR_THRESHOLD` nên đặt **giữa hai số đó**. Hiện là 25000 nhưng bằng chứng trong
comment lại đo ở mức LED `0x5F` cũ — đây là cơ hội sửa cho đúng.

### Nhiệm vụ 3b.4 — Kiểm chứng tự xoá màn cảnh báo

- [ ] **Bước 1:** Kích SOS, để cảnh báo gửi đi, **không chạm màn hình**

- [ ] **Bước 2:** Đợi 5 phút, xác nhận đồng hồ tự về màn Home

Mong đợi log: `[ALERT] Outcome screen timed out, resuming monitoring.`

- [ ] **Bước 3:** Kích SOS lần thứ hai ngay sau đó

Đây là bài kiểm tra thật sự: chứng minh **cú ngã thứ hai được phát hiện** sau khi cú thứ nhất
không có ai xác nhận. Đó chính là lỗ hổng B1 đã sửa.

### Nhiệm vụ 3b.5 — Kiểm chứng gauge pin

- [ ] **Bước 1:** Đo vôn kế qua 2 chân BAT, so với `raw ... mV` trong log

Nếu vôn kế đọc $V$ và log đọc $m$ mV thì tỉ số thật là $V / (m/1000)$. Hiện giả định là 3.0.
Sai tỉ số này thì mọi phần trăm đều sai theo.

- [ ] **Bước 2:** Rút sạc, để chạy pin, ghi `(V, %)` mỗi 30 phút trong ~3 giờ

- [ ] **Bước 3:** So với đường cong 6 điểm trong `battery_monitor.cpp`

Nếu lệch nhiều thì thay 6 điểm bằng số đo thật. Đường cong hiện tại lấy từ LiPo điển hình,
**chưa đo trên cell này**.

- [ ] **Bước 4:** Cắm sạc, xác nhận cờ `isCharging` bật sau ~2 phút

Nếu không bật, `BATTERY_CHARGE_RISE_V = 10 mV / 2 phút` quá cao với dòng sạc của bạn — hạ xuống.

- [ ] **Bước 5:** Commit mọi hiệu chỉnh + cập nhật SPEC

```bash
git add include/app_config.h src/ SYSTEM_ARCHITECTURE_SPEC.md
git commit -m "calib: hiệu chỉnh ngưỡng từ buổi kiểm chứng phần cứng đầu tiên"
git tag v0.3-hw-verified
```

**Nghiệm thu Giai đoạn 3b:** té ngã bắn ≥ 8/10 · van thoát không spam · màn cảnh báo tự xoá và
phát hiện được cú thứ hai · tỉ số phân áp đã xác nhận bằng vôn kế · SPEC §11 mục 1, 4b, 5 đổi từ
"chưa kiểm chứng" sang "đã kiểm chứng".

---

# GIAI ĐOẠN 5 (Phần B) — Nâng 200 Hz + DSP Tầng 5

**Vì sao cần:** cấu hình hiện tại `setup(brightness, 4, 2, 100, 411, 4096)` cho 100 Hz chia
trung bình phần cứng 4 = **25 Hz hiệu dụng = 40 ms/mẫu**. RMSSD của người bình thường là
**20–50 ms**. Sai số đo bằng đúng cỡ đại lượng cần đo → mọi đặc trưng HRV đều vô nghĩa.
Đây là **điều kiện chặn cứng** của Giai đoạn 7 nhánh loạn nhịp.

**Ràng buộc phải nhớ:** `FreqS = 25` và `BUFFER_SIZE = 100` là `#define` **bên trong thư viện
SparkFun** (`spo2_algorithm.h`), không sửa được. Thuật toán SpO2 vẫn phải được nuôi ở đúng 25 Hz.
Đây là điểm dễ hỏng nhất của giai đoạn này.

### Nhiệm vụ 5.1 — Nâng tốc độ lấy mẫu, giữ SpO2 nguyên vẹn

**File:** Sửa `src/sensors/max30102_service.cpp:83` và `:114`

**Giao diện tạo ra:** biến `sampleCounter` đếm mẫu thô; hằng `PPG_DECIMATE = 8`

- [ ] **Bước 1:** Đổi cấu hình cảm biến (2 chỗ — cả `init` lẫn `checkSensorAlive`)

```cpp
// 200 Hz thô, KHÔNG trung bình phần cứng (tham số thứ 2 = 1).
// 5 ms/mẫu -- đủ mịn để khoảng RR có nghĩa. Xem PPG_DECIMATE bên dưới
// cho cách thuật toán SpO2 vẫn nhận đúng 25 Hz như nó mong đợi.
particleSensor.setup(MAX30102_LED_BRIGHTNESS, 1, 2, 200, 411, 4096);
```

- [ ] **Bước 2:** Thêm bộ chia tần cho nhánh SpO2

```cpp
// Thuật toán SpO2 của SparkFun có FreqS = 25 nướng cứng trong thư viện.
// Nuôi nó bằng 200 Hz sẽ làm nó hiểu sai 8 lần về thời gian, ra chỉ số vô lý.
// Nên: nhánh phát hiện nhịp ăn toàn bộ 200 Hz, nhánh SpO2 chỉ ăn 1/8.
#define PPG_DECIMATE 8
static uint8_t sampleCounter = 0;
static uint32_t irAccum = 0, redAccum = 0;
```

- [ ] **Bước 3:** Tách hai nhánh trong vòng `while (particleSensor.available())`

Nhánh phát hiện nhịp (`checkForBeat`) giữ nguyên vị trí, ăn mọi mẫu.
Phần đổ vào `irBuffer`/`redBuffer` chuyển thành:

```cpp
// Trung bình 8 mẫu thay vì lấy mẫu thứ 8 -- lấy mẫu thưa sẽ gập nhiễu
// tần số cao xuống dải quan tâm (aliasing), trung bình thì lọc nó đi.
irAccum  += ir;
redAccum += red;
if (++sampleCounter >= PPG_DECIMATE) {
    irBuffer[bufferCount]  = irAccum  / PPG_DECIMATE;
    redBuffer[bufferCount] = redAccum / PPG_DECIMATE;
    bufferCount++;
    sampleCounter = 0;
    irAccum = redAccum = 0;

    if (bufferCount >= BUFFER_SIZE) {
        // ... khối tính SpO2 giữ nguyên không đổi
    }
}
```

- [ ] **Bước 4:** Đặt lại bộ đếm trong `resetMeasurement()`

```cpp
sampleCounter = 0;
irAccum = redAccum = 0;
```

- [ ] **Bước 5:** Build và nạp

```bash
pio run && pio run --target upload && pio device monitor -b 115200
```

- [ ] **Bước 6:** Nghiệm thu trên phần cứng

| Kiểm | Mong đợi | Nếu sai |
|---|---|---|
| SpO2 vẫn ra 95–100% khi đeo yên | Bộ chia tần đúng | Số vô lý → nhánh SpO2 đang ăn nhầm 200 Hz |
| Nhịp tim vẫn khớp bắt mạch tay | Phát hiện nhịp chịu được 200 Hz | Lệch → `checkForBeat` cần chỉnh ngưỡng |
| FIFO không tràn | Vòng lặp theo kịp | Tràn → FIFO 32 mẫu đầy sau 160 ms, vòng lặp 20 ms dư sức, nên tràn nghĩa là có chỗ khác chặn |

- [ ] **Bước 7:** Commit

```bash
git commit -am "feat(ppg): 200 Hz sampling with an 8x decimated SpO2 path"
```

### Nhiệm vụ 5.2 — Mốc thời gian nhịp từ chỉ số FIFO

**Vì sao:** dòng 153 hiện lấy `millis()` **lúc vòng lặp chạy tới**, không phải lúc mẫu được lấy.
Vòng lặp chạy mỗi 20 ms và mỗi lần rút nhiều mẫu từ FIFO, nên mọi mẫu trong cùng một lượt đều
mang **cùng một mốc thời gian**. Đó là ~20 ms jitter cộng thẳng vào khoảng RR — lớn hơn cả RMSSD
cần đo. Nâng 200 Hz mà không sửa chỗ này thì coi như không nâng.

**File:** Sửa `src/sensors/max30102_service.cpp:152-156`

- [ ] **Bước 1:** Thêm bộ đếm mẫu tuyệt đối

```cpp
// Số mẫu đã đi qua kể từ lần reset. Ở 200 Hz mỗi mẫu là đúng 5 ms, nên
// chỉ số này là một đồng hồ mịn hơn millis() nhiều -- và quan trọng hơn,
// nó gắn với thời điểm CẢM BIẾN lấy mẫu, không phải thời điểm vòng lặp
// chạy tới. Đó là chỗ 20 ms jitter cũ sinh ra.
static uint32_t sampleIndex = 0;
static uint32_t lastBeatSample = 0;
#define PPG_SAMPLE_PERIOD_MS 5.0f
```

- [ ] **Bước 2:** Tăng bộ đếm ngay sau `nextSample()`

```cpp
particleSensor.nextSample();
sampleIndex++;
```

- [ ] **Bước 3:** Tính BPM từ chỉ số mẫu

```cpp
if (!g_watchState.motionArtifact && checkForBeat(ir)) {
    if (lastBeatSample > 0) {
        uint32_t deltaSamples = sampleIndex - lastBeatSample;
        float deltaMs = deltaSamples * PPG_SAMPLE_PERIOD_MS;
        float bpm = 60000.0f / deltaMs;
        // ... toàn bộ bộ chặn + van thoát + trung vị giữ nguyên
    }
    lastBeatSample = sampleIndex;
    lastBeatMs = millis();   // vẫn giữ: bộ canh số cũ ở dòng 278 dùng nó
}
```

> **Giữ `lastBeatMs`.** Nó không còn dùng để tính BPM nữa nhưng bộ canh "số đã cũ quá 6 giây"
> ở cuối hàm vẫn cần. Bỏ nó đi là làm hỏng một thứ không liên quan.

- [ ] **Bước 4:** Đặt lại cả hai trong `resetMeasurement()`

```cpp
sampleIndex = 0;
lastBeatSample = 0;
```

- [ ] **Bước 5:** Build, nạp, xác nhận nhịp tim không đổi hành vi

Số hiển thị phải **giống trước**. Giai đoạn này cải thiện *độ mịn* chứ không đổi *giá trị*.
Nếu số nhảy lung tung → `sampleIndex` đang chạy sai nhịp với thực tế.

- [ ] **Bước 6:** Commit

```bash
git commit -am "fix(ppg): derive beat timing from FIFO index, not loop arrival time"
```

### Nhiệm vụ 5.3 — Vòng đệm khoảng RR

**File mới:** `src/dsp/rr_analysis.h` và `src/dsp/rr_analysis.cpp`

**Phụ thuộc — bắt buộc:** Nhiệm vụ 5.2 phải xong trước. Bước 4 dưới đây dùng biến `deltaMs`,
biến này chỉ tồn tại sau khi 5.2 thay cách tính khoảng nhịp từ `millis()` sang chỉ số FIFO.
Làm 5.3 mà bỏ qua 5.2 thì vừa không biên dịch được, vừa vô nghĩa: khoảng RR tính từ `millis()`
mang sẵn ~20 ms jitter, lớn hơn cả RMSSD cần đo.

**Giao diện tạo ra** (Giai đoạn 7 sẽ gọi):
```cpp
void   initRRAnalysis();
void   pushRRInterval(float ms);        // gọi từ max30102_service mỗi nhịp nhận
bool   getRRFeatures(float* rmssd, float* pnn50, float* entropy);  // false nếu chưa đủ dữ liệu
void   resetRRAnalysis();
```

- [ ] **Bước 1:** Tạo header

```cpp
#ifndef RR_ANALYSIS_H
#define RR_ANALYSIS_H
#include <Arduino.h>

// Cửa sổ 60 giây ở nhịp tối đa 180 BPM = 180 khoảng. Lấy 200 cho dư.
#define RR_BUFFER_SIZE   200
#define RR_MIN_INTERVALS 30   // dưới mức này mọi thống kê HRV đều là nhiễu

void initRRAnalysis();
void pushRRInterval(float ms);
bool getRRFeatures(float* rmssd, float* pnn50, float* entropy);
void resetRRAnalysis();
#endif
```

- [ ] **Bước 2:** Khai báo trạng thái ở đầu `rr_analysis.cpp`

```cpp
#include "rr_analysis.h"
#include <math.h>

// Vòng đệm trượt: khi đầy thì dồn xuống một nửa thay vì quấn vòng. Thống kê
// HRV cần các khoảng theo ĐÚNG THỨ TỰ (RMSSD lấy hiệu hai khoảng liền kề),
// nên một ring buffer quấn vòng sẽ tạo ra một hiệu giả ở chỗ nối.
static float    buf[RR_BUFFER_SIZE];
static uint16_t count = 0;

void initRRAnalysis()  { count = 0; }
void resetRRAnalysis() { count = 0; }

void pushRRInterval(float ms) {
    // Khoảng RR ngoài dải sinh lý là nhịp bỏ sót hoặc nhịp thừa, không phải
    // dữ liệu. 333 ms = 180 BPM, 1333 ms = 45 BPM -- cùng dải với bộ lọc thô
    // ở tầng 3, giữ cho hai chỗ nhất quán.
    if (ms < 333.0f || ms > 1333.0f) return;

    if (count >= RR_BUFFER_SIZE) {
        // Giữ nửa sau, bỏ nửa đầu.
        memmove(buf, buf + RR_BUFFER_SIZE / 2,
                (RR_BUFFER_SIZE / 2) * sizeof(float));
        count = RR_BUFFER_SIZE / 2;
    }
    buf[count++] = ms;
}
```

- [ ] **Bước 3:** Cài đặt RMSSD và pNN50

```cpp
// RMSSD -- căn bậc hai trung bình bình phương hiệu hai khoảng liên tiếp.
// Đại lượng HRV chuẩn trong y văn, và là đại lượng nhạy nhất với rung nhĩ.
bool getRRFeatures(float* rmssd, float* pnn50, float* entropy) {
    if (count < RR_MIN_INTERVALS) return false;

    float sumSq = 0.0f;
    uint16_t nn50 = 0;
    for (uint16_t i = 1; i < count; i++) {
        float d = buf[i] - buf[i - 1];
        sumSq += d * d;
        if (fabsf(d) > 50.0f) nn50++;
    }
    *rmssd = sqrtf(sumSq / (float)(count - 1));
    *pnn50 = 100.0f * (float)nn50 / (float)(count - 1);
    *entropy = shannonEntropy();
    return true;
}
```

- [ ] **Bước 4:** Cài đặt entropy Shannon (chia giỏ 16 mức)

```cpp
// Entropy Shannon trên histogram khoảng RR. Nhịp xoang đều cho phân bố hẹp
// (entropy thấp); rung nhĩ cho phân bố tán loạn (entropy cao). Đây là đặc
// trưng phân biệt mạnh thứ hai sau RMSSD.
static float shannonEntropy() {
    const uint8_t BINS = 16;
    uint16_t hist[BINS] = {0};
    float lo = 1e9f, hi = 0.0f;
    for (uint16_t i = 0; i < count; i++) {
        if (buf[i] < lo) lo = buf[i];
        if (buf[i] > hi) hi = buf[i];
    }
    if (hi - lo < 1.0f) return 0.0f;   // tất cả bằng nhau -> entropy 0

    for (uint16_t i = 0; i < count; i++) {
        uint8_t b = (uint8_t)((buf[i] - lo) / (hi - lo) * (BINS - 1));
        hist[b]++;
    }
    float h = 0.0f;
    for (uint8_t b = 0; b < BINS; b++) {
        if (hist[b] == 0) continue;
        float p = (float)hist[b] / (float)count;
        h -= p * log2f(p);
    }
    return h;
}
```

- [ ] **Bước 5:** Nối vào `max30102_service.cpp`

Ngay sau khi một nhịp **vượt qua** bộ chặn (nhánh `else` của `isSpike`, cùng chỗ ghi `rates[]`):

```cpp
// Chỉ đẩy khoảng RR đã qua bộ lọc. Một nhịp bỏ sót tạo khoảng RR gấp đôi,
// trông y hệt rung nhĩ -- và cảnh báo đó bắn thẳng vào nhóm gia đình.
pushRRInterval(deltaMs);
```

- [ ] **Bước 6:** In đặc trưng ra log mỗi 10 giây để quan sát

```cpp
float rmssd, pnn50, ent;
if (getRRFeatures(&rmssd, &pnn50, &ent)) {
    Serial.printf(" [HRV] RMSSD=%.1fms pNN50=%.1f%% H=%.2f\n", rmssd, pnn50, ent);
}
```

- [ ] **Bước 7:** Nghiệm thu

Ngồi yên 2 phút. Mong đợi RMSSD **20–60 ms** với người khoẻ. Nếu ra hàng trăm ms → khoảng RR
còn nhiễu, bộ lọc tầng 4 chưa đủ, **không được sang Giai đoạn 7 nhánh loạn nhịp**.

- [ ] **Bước 8:** Commit

```bash
git add src/dsp/ && git commit -m "feat(dsp): RR interval buffer with RMSSD, pNN50 and entropy"
```

### Nhiệm vụ 5.4 — DSP Tầng 5: Kalman 1D + chặn hiển thị theo SQI

- [ ] **Bước 1:** Thêm bộ lọc Kalman vô hướng

```cpp
// Kalman 1D trên nhịp tim. Trung vị đã bỏ được các gai; Kalman lo phần
// còn lại: nó tin số đo nhiều hơn khi số đo ổn định, và tin mô hình nhiều
// hơn khi số đo tán loạn. Mười dòng, không cần thư viện.
static float kalmanX = 0.0f;   // ước lượng
static float kalmanP = 1.0f;   // hiệp phương sai sai số

static float kalmanUpdate(float measurement) {
    kalmanP += PPG_KALMAN_Q;
    float k = kalmanP / (kalmanP + PPG_KALMAN_R);
    kalmanX += k * (measurement - kalmanX);
    kalmanP *= (1.0f - k);
    return kalmanX;
}
```

- [ ] **Bước 2:** Thêm hằng số vào `app_config.h`

```cpp
// Nhiễu quá trình: nhịp tim thật đổi bao nhanh. Nhỏ = tin mô hình, mượt
// nhưng chậm phản ứng.
#define PPG_KALMAN_Q  0.5f

// Nhiễu đo: cảm biến sai bao nhiều. Lớn = lọc mạnh hơn.
#define PPG_KALMAN_R  4.0f

// Ngưỡng SQI để hiển thị số. BẮT ĐẦU TỪ 20, KHÔNG PHẢI 75.
//
// Thang SQI là `perfusion * 50` -- một tỉ lệ chưa hiệu chuẩn, không phải
// phần trăm có nghĩa. Đặt 75 ngay là màn hình sẽ không bao giờ hiện số nào.
// Đọc SQI thật trong log ở Giai đoạn 3b.3, rồi mới siết dần.
#define PPG_MIN_SQI   20
```

- [ ] **Bước 3:** Áp Kalman sau trung vị

```cpp
if (valid >= 2) {
    // ... sắp xếp, lấy trung vị vào temp[valid/2]
    float smoothed = kalmanUpdate((float)temp[valid / 2]);
    g_watchState.heartRateBPM = (uint16_t)(smoothed + 0.5f);
    ...
}
```

Đặt lại `kalmanX = 0; kalmanP = 1.0f;` trong `resetMeasurement()`.

- [ ] **Bước 4:** Chặn hiển thị theo SQI

```cpp
// Số đúng mà tín hiệu rác vẫn là số sai. Dưới ngưỡng SQI thì thà không
// hiện gì còn hơn hiện một con số người ta sẽ tin.
if (g_watchState.signalQuality < PPG_MIN_SQI) {
    g_watchState.hrValid = false;
}
```

- [ ] **Bước 5:** Nghiệm thu

| Kiểm | Mong đợi |
|---|---|
| Ngồi yên | Số nhích mượt, không nhảy ±10 BPM giữa hai lần đọc |
| Vung tay mạnh | Số **biến mất** (SQI tụt), không nhảy lên 180 |
| Tháo khỏi cổ tay | Số biến mất trong vài giây |
| Đeo lại | Số quay lại trong ~10 giây |

- [ ] **Bước 6:** Commit + tag

```bash
git commit -am "feat(dsp): stage 5 -- 1D Kalman on heart rate plus SQI display gate"
git tag v0.4-dsp-complete
```

**Nghiệm thu Giai đoạn 5:** SpO2 vẫn hợp lý sau khi đổi tốc độ · nhịp tim ổn định khi ngồi yên ·
vung tay không đẩy số lên 180 · RMSSD nằm trong dải sinh lý · SPEC §3 đổi tầng 5 từ ❌ sang ✅.

---

# GIAI ĐOẠN 6 (Phần C) — Ba tính năng còn thiếu

**Thứ tự trong giai đoạn có ràng buộc:** C1 (ngưỡng sinh lý) **phải sau Giai đoạn 5**, không thì
cảnh báo bắn ra từ số rác. C2 và C3 độc lập, làm trước được.

### Nhiệm vụ 6.1 — Nút SOS vật lý (BOOT / GPIO 0)

**⚠️ GPIO 0 là chân strapping.** Mức của nó lúc khởi động quyết định ESP32 vào chế độ nạp hay
chạy bình thường. **Chỉ được `pinMode()` và đọc SAU KHI `setup()` xong.** Đọc sớm là treo máy.

- [ ] **Bước 1:** Thêm hằng số

```cpp
// Nút BOOT dùng lại làm SOS. GPIO 0 là chân strapping -- chỉ chạm vào nó
// sau khi setup() đã xong, nếu không sẽ can thiệp vào quyết định boot mode.
#define SOS_BUTTON_PIN     0
#define SOS_HOLD_MS        1500UL
```

- [ ] **Bước 2:** Khởi tạo ở **cuối** `setup()`, không phải đầu

```cpp
// Đặt ở dòng cuối cùng của setup() một cách có chủ ý. Xem SOS_BUTTON_PIN.
pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
```

- [ ] **Bước 3:** Xử lý giữ nút trong `loop()`

```cpp
static unsigned long sosPressStart = 0;
static bool sosFired = false;

void updateSOSButton() {
    bool pressed = (digitalRead(SOS_BUTTON_PIN) == LOW);
    if (!pressed) { sosPressStart = 0; sosFired = false; return; }
    if (sosPressStart == 0) { sosPressStart = millis(); return; }
    // Bắn đúng một lần cho mỗi lần giữ -- không lặp lại khi vẫn đang giữ.
    if (!sosFired && millis() - sosPressStart >= SOS_HOLD_MS) {
        sosFired = true;
        Serial.println(" [SOS] Physical button held, raising alert.");
        triggerSimulatedFall();
    }
}
```

- [ ] **Bước 4:** Nghiệm thu — giữ 1.5 s bắn cảnh báo; nhấn nhanh **không** bắn; giữ 10 s chỉ bắn một lần

- [ ] **Bước 5:** Commit `feat(ui): physical SOS on the BOOT button`

### Nhiệm vụ 6.2 — Cảnh báo pin yếu

**Phụ thuộc:** cần đường cong pin của Giai đoạn 3b.5 đã hiệu chỉnh, nếu không ngưỡng 15% không
tương ứng với 15% thật.

- [ ] **Bước 1:** Thêm ngưỡng có trễ (hysteresis)

```cpp
#define BATTERY_LOW_PCT      15
// Ngưỡng nhả cao hơn ngưỡng bắt: pin dao động quanh 15% sẽ spam nhóm gia
// đình hàng chục tin nếu hai ngưỡng bằng nhau.
#define BATTERY_CLEAR_PCT    20
```

- [ ] **Bước 2:** Gửi đúng một tin

```cpp
static bool lowBatteryNotified = false;

void checkLowBattery() {
    if (g_watchState.isCharging) { lowBatteryNotified = false; return; }

    if (!lowBatteryNotified && g_watchState.batteryPercent <= BATTERY_LOW_PCT) {
        lowBatteryNotified = true;
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "⚠️ BÁO PIN YẾU: Đồng hồ chỉ còn %d%% pin. Vui lòng cắm sạc!",
                 g_watchState.batteryPercent);
        queueAlertMessage(msg);
    } else if (g_watchState.batteryPercent >= BATTERY_CLEAR_PCT) {
        lowBatteryNotified = false;
    }
}
```

- [ ] **Bước 3:** Nghiệm thu — xả pin xuống dưới 15%, xác nhận **đúng một** tin Telegram

> Đây chính là kịch bản mà lỗi H2 từng chặn hoàn toàn: khi "đang sạc" suy ra từ mức điện áp,
> cắm USB không pin luôn đọc 100% nên nhánh này không bao giờ chạy tới.

- [ ] **Bước 4:** Commit `feat(alert): low battery warning with hysteresis`

### Nhiệm vụ 6.3 — Cảnh báo ngưỡng sinh lý

**⚠️ Chỉ làm SAU Giai đoạn 5.** Không có tầng 5 thì cảnh báo bắn từ số rác, và một cảnh báo y tế
sai còn tệ hơn không có cảnh báo.

- [ ] **Bước 1:** Thêm ngưỡng + thời gian duy trì

```cpp
#define VITAL_HR_HIGH        130
#define VITAL_HR_LOW          45
#define VITAL_SPO2_LOW        90
// Phải duy trì liên tục ngần này mới báo. Một lần đọc lệch không phải là
// một sự kiện y tế.
#define VITAL_SUSTAIN_MS    5000UL
// Không nhắc lại cùng một cảnh báo trong vòng 10 phút.
#define VITAL_REPEAT_MS   600000UL
```

- [ ] **Bước 2:** Cài đặt, **chỉ chạy khi số đáng tin**

```cpp
void checkVitalThresholds() {
    // Không có tiếp xúc da, hoặc SQI thấp, thì không có gì để nói.
    if (!g_watchState.hrValid || g_watchState.signalQuality < PPG_MIN_SQI) {
        hrBreachSince = 0;
        return;
    }
    // ... đếm thời gian duy trì, gửi khi vượt VITAL_SUSTAIN_MS
}
```

- [ ] **Bước 3:** Nghiệm thu — chạy tại chỗ cho nhịp lên >130, xác nhận cảnh báo sau 5 giây liên tục

- [ ] **Bước 4:** Commit + tag `v0.5-features-complete`

---

# GIAI ĐOẠN 7 (Phần D) — TinyML

**Hai nhánh độc lập nhau.** Nhánh té ngã chỉ cần Giai đoạn 3b. Nhánh loạn nhịp **bị chặn cứng bởi
Giai đoạn 5** — không có 200 Hz thì không có khoảng RR đủ mịn, không có khoảng RR thì không có
đặc trưng, không có đặc trưng thì không có model.

### Nhiệm vụ 7.1 — Thu dữ liệu té ngã

- [ ] **Bước 1:** Bật ghi log thô

```cpp
#define FALL_LOG_RAW_SAMPLES  1   // tạm bật cho buổi thu dữ liệu
```

- [ ] **Bước 2:** Thu **40–50 cú ngã mô phỏng**, 4 kiểu, ghi ra file

```bash
pio device monitor -b 115200 | tee ai_models/data/falls_raw.csv
```

- [ ] **Bước 3:** Thu **2–3 giờ sinh hoạt bình thường** làm mẫu âm

Đi bộ, ngồi xuống mạnh, vỗ tay, đánh răng, lên xuống cầu thang. **Mẫu âm quan trọng ngang mẫu
dương** — model chỉ thấy cú ngã sẽ báo động với mọi thứ.

- [ ] **Bước 4:** Tắt lại `FALL_LOG_RAW_SAMPLES` (nó chặn ~5 ms mỗi tick 20 ms)

### Nhiệm vụ 7.2 — Cây quyết định té ngã

**File mới:** `ai_models/train_fall.py`, sinh ra `src/ai/fall_model.h`

- [ ] **Bước 1:** Tải UMAFall / FallAllD (có node **cổ tay** — đây là lý do chọn chúng thay vì
  các bộ ghi ở túi quần / thắt lưng)

- [ ] **Bước 2:** Tiền xử lý cho khớp phần cứng thật

```python
# IMU chạy ±8g @ 4096 LSB/g, đọc 50 Hz. Dataset thường ±16g @ 200 Hz.
# Phải mô phỏng CẢ BA khác biệt, nếu không model học trên tín hiệu mà thiết
# bị không bao giờ thấy:
df = resample(df, from_hz=200, to_hz=50)
df = df.clip(-8.0, 8.0)                    # bão hoà cắt ngọn ở 8g
df = np.round(df * 4096) / 4096            # lượng tử hoá về đúng LSB
```

> Bước cắt ngọn là bước dễ quên nhất và cũng gây hại nhất: cú ngã mạnh **bão hoà** ở 8g trên
> thiết bị này, nhưng trong dataset ±16g thì không. Model học trên đỉnh 12g sẽ không nhận ra
> cùng cú ngã đó khi nó bị cắt phẳng ở 8g.

- [ ] **Bước 3:** Trích đặc trưng — cùng công thức với firmware, nếu không train và suy luận lệch nhau

- [ ] **Bước 4:** Huấn luyện `DecisionTreeClassifier(max_depth=8)`, kiểm định trên dữ liệu 7.1

- [ ] **Bước 5:** Xuất C thuần bằng `emlearn`

```python
import emlearn
emlearn.convert(model).save(file='src/ai/fall_model.h', name='fall_model')
```

> **Không gọi đây là "lượng tử hoá INT8".** Cây quyết định là chuỗi `if (feature < threshold)` —
> không có phép nhân ma trận nào để lượng tử hoá. Con số ~8 KB và 0.02 ms thì đúng, chỉ cái nhãn
> là sai. Đây là lỗi đã có trong tài liệu cũ.

- [ ] **Bước 6:** Chạy **song song** thuật toán 4 pha, ngưỡng `P(Fall) ≥ 0.80`

> Ngưỡng cũ `80% ≤ P ≤ 85%` là lỗi logic: cú ngã mà model chắc 90% sẽ **không** kích hoạt.

- [ ] **Bước 7:** Ghi log cả hai quyết định để so sánh, chưa cho model quyền tự bắn cảnh báo

- [ ] **Bước 8:** Commit `feat(ai): fall decision tree running alongside the 4-phase detector`

### Nhiệm vụ 7.3 — Sàng lọc nhịp bất thường

**⚠️ Kiểm tra điều kiện trước:** RMSSD ở Giai đoạn 5.3 phải nằm trong dải sinh lý 20–60 ms.
Nếu chưa, dừng lại và sửa DSP — model huấn luyện trên khoảng RR nhiễu là model vô dụng.

- [ ] **Bước 1:** Tải MIT-BIH **`afdb`** (Atrial Fibrillation Database — 23 bản ghi 10 giờ,
  có nhãn từng cơn)

> **Dùng `afdb`, không dùng `mitdb`.** Và **không dùng 1D-CNN trên dạng sóng**: MIT-BIH là ECG
> ghi hoạt động *điện* (P-QRS-T), MAX30102 là quang học ghi *thể tích máu*. Sóng PPG không có
> sóng P, không có phức bộ QRS. CNN học hình dạng → thứ nó cần không tồn tại trong PPG.
>
> **Khoảng RR thì khác:** nó là *cùng một đại lượng sinh lý* dù đo bằng điện hay ánh sáng. Nên
> huấn luyện trên `afdb` rồi suy luận trên PPG là **hợp lệ** — đây là cách chuẩn trong y văn và
> là cách Apple Watch / Fitbit làm.

- [ ] **Bước 2:** Trích khoảng RR từ nhãn nhịp, cắt cửa sổ 30–60 giây

- [ ] **Bước 3:** Tính RMSSD / pNN50 / entropy — **dùng đúng công thức trong `rr_analysis.cpp`**

- [ ] **Bước 4:** Huấn luyện cây nhỏ hoặc hồi quy logistic (~2 KB), xuất `src/ai/rr_model.h`

- [ ] **Bước 5:** Chỉ chạy khi `signalQuality >= PPG_MIN_SQI` **và** có đủ `RR_MIN_INTERVALS`

- [ ] **Bước 6:** Diễn đạt kết quả cho đúng

```cpp
// "gợi ý dấu hiệu nhịp không đều", KHÔNG PHẢI "phát hiện rung nhĩ".
// Đây không phải thiết bị chẩn đoán y tế và không được nói như thể nó là.
"⚠️ Phát hiện dấu hiệu nhịp tim không đều. Đây KHÔNG phải chẩn đoán y tế — "
"vui lòng đến cơ sở y tế để kiểm tra."
```

- [ ] **Bước 7:** Commit + tạo `ai_models/` với script và notebook (**không commit dataset**)

---

# GIAI ĐOẠN 8 — Đánh giá cho báo cáo

**Vì sao tách riêng:** "model chạy được" và "model tốt" là hai chuyện. Giai đoạn này sinh ra
những con số mà báo cáo cần và mà người đọc có quyền đòi hỏi.

- [ ] **Bước 1:** Tách tập kiểm tra **giữ riêng** — không dùng lúc huấn luyện, không dùng lúc
  chỉnh siêu tham số. Chia theo **người**, không theo mẫu: cùng một người ở cả hai tập là rò rỉ dữ liệu.

- [ ] **Bước 2:** Ma trận nhầm lẫn cho cả hai model

- [ ] **Bước 3:** Độ nhạy và độ đặc hiệu, kèm khoảng tin cậy

> Trên thiết bị báo té ngã, **độ nhạy quan trọng hơn độ chính xác**: bỏ sót một cú ngã là hỏng
> mục đích tồn tại của thiết bị, còn một báo động giả chỉ gây phiền.

- [ ] **Bước 4:** Đường ROC + AUC

- [ ] **Bước 5:** **So sánh với baseline 4 pha** — đây là phần quan trọng nhất của báo cáo.
  Nếu model không thắng được thuật toán ngưỡng thủ công, đó vẫn là một kết quả trung thực và
  phải viết ra như vậy.

- [ ] **Bước 6:** Đo chi phí thực tế trên thiết bị: thời gian suy luận (µs), RAM thêm, flash thêm

- [ ] **Bước 7:** Viết `docs/DANH_GIA.md` với toàn bộ số liệu + notebook tái lập được

---

# GIAI ĐOẠN 4 — Chuyển sang ESP-IDF ⬅️ **LÀM SAU CÙNG**

**Làm trên nhánh riêng `feat/espidf-migration`. Không merge cho tới khi đạt đủ 6 tiêu chí.**

> **Ghi chú thẳng thắn:** tôi đã khuyến nghị giữ Arduino và bạn đã cân nhắc rồi chọn đổi. Kế hoạch
> này làm theo quyết định đó, nhưng đặt nó **sau cùng** để nếu thất bại thì không mất gì — mọi
> tính năng đã hoàn thành và đã kiểm chứng đều nằm an toàn trên `main`.

- [ ] **Bước 1:** Đổi platform sang fork `pioarduino` (IDF 5.x + Arduino core 3.x).
  `platform = espressif32` chính thống dừng ở IDF 4.4, trong khi `esp-tflite-micro` nhắm IDF 5.x.

- [ ] **Bước 2:** `framework = arduino, espidf`. `setup()`/`loop()` **giữ nguyên** — Arduino core
  chạy như một component và tự tạo task gọi chúng.

- [ ] **Bước 3:** `sdkconfig.defaults`

```
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y          # R2 dùng QUAD, KHÔNG phải octal
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_AUTOSTART_ARDUINO=y
CONFIG_FREERTOS_HZ=1000
```

> ⚠️ Đây **đúng là loại lỗi đã giết `IMPLEMENTATION_PLAN.md`** (`psram_type = opi` trên board dùng
> PSRAM quad). Sai dòng `CONFIG_SPIRAM_MODE_QUAD` thì board không boot.

- [ ] **Bước 4:** Chuyển 20 dòng `build_flags` TFT_eSPI. **Đây là rủi ro cao nhất** — TFT_eSPI dò
  cấu hình qua `User_Setup_Select.h`, cơ chế này hay hỏng ở chế độ IDF, và nó đang vẽ toàn bộ giao diện.

- [ ] **Bước 5:** Rà breaking change Arduino 2.x → 3.x

| Chỗ | Rủi ro |
|---|---|
| `battery_monitor.cpp` | API `analogReadMilliVolts` đổi — ảnh hưởng trực tiếp gauge pin vừa hiệu chỉnh |
| `Wire.begin()` | Chữ ký hàm đổi |
| NimBLE-Arduino 1.4.x | Có thể phải nâng 2.x |
| TFT_eSPI 2.5.43 | Kiểm tương thích |

- [ ] **Bước 6:** Thêm component IDF qua `idf_component.yml` (không cài qua `lib_deps` được)

**Tiêu chí nghiệm thu — đủ cả 6 mới merge:**
1. Build sạch, không cảnh báo mới nghiêm trọng
2. Boot được, không panic PSRAM
3. Màn hình vẽ đúng, **font tiếng Việt có dấu đúng**
4. Cả 3 cảm biến báo OK
5. Wi-Fi kết nối + gửi được tin Telegram thật
6. BLE kết nối được từ điện thoại

**Phương án lùi:** quá **5 ngày** chưa đạt → `git checkout v0.5-features-complete`, giữ nhánh spike
lại. Cả hai model đã chốt đều là C thuần nên **không mất tính năng nào**.

---

## Việc của bạn, không phải của tôi

- [ ] **🔴 Thu hồi token Telegram** qua `@BotFather` → `/revoke` → `@Gia_Dinh_bot`.
  Token **vẫn còn sống**. Viết lại lịch sử git đã gỡ nó khỏi nhánh, nhưng GitHub vẫn giữ các commit
  cũ dạng dangling truy cập được qua SHA. Chỉ `/revoke` mới thật sự đóng lỗ hổng này.

- [ ] Quyết định có xoá bản MD trùng lặp ở `d:\AIoT\Test` không (chúng đã lệch so với repo).

- [ ] Quyết định có xoá hẳn thư mục `d:\AIoT\Test` không, khi đã chắc repo là nơi làm việc chính.

---

## Hai mức nghiệm thu — phát biểu cho đúng

| | Đủ để báo cáo | Dám đeo thật |
|---|---|---|
| Té ngã | Ma trận nhầm lẫn trên dữ liệu tự thu | Thử nghiệm nhiều ngày trên người thật |
| Nhịp tim | So với bắt mạch tay | So với thiết bị đo tham chiếu được chứng nhận |
| BLE | Just Works chấp nhận được | **Phải có bonding + mã hoá** |
| Cảnh báo | Gửi được tới Telegram | Có đường dự phòng khi Wi-Fi chết |
| Diễn đạt | "đồ án nghiên cứu" | Có câu miễn trừ y tế rõ ràng |

Dự án hiện đang ở giữa hai cột. Giai đoạn 3b–8 đưa cột trái về đích. Cột phải cần thêm thời gian
thực địa mà không giai đoạn nào ở trên thay thế được.
