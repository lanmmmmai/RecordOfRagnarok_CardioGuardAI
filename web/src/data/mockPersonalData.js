// Mock Data for CardioGuardAI Personalized Bio-Telemetry & Dashboard

export const initialUserProfile = {
  id: "CG-AI-9988-VN",
  name: "Nguyễn Thị Mai Lan",
  alias: "Patient Mai Lan",
  age: 62,
  gender: "Nữ",
  bloodType: "AB+",
  height: "160 cm",
  weight: "54 kg",
  avatar: "https://images.unsplash.com/photo-1544005313-94ddf0286df2?auto=format&fit=crop&w=300&q=80",
  deviceModel: "Đồng Hồ Enroll",
  firmwareVersion: "v2.4.1-TinyML-INT8",
  macAddress: "DC:54:75:A8:99:B2",
  connectedStatus: "ONLINE",
  wifiSignal: -56, // dBm
  batteryLevel: 92, // %
  lastSync: "Vừa xong",
  medicalHistory: [
    "Tiền sử Rối loạn Nhịp tim ngoài tâm thu (PVC)",
    "Huyết áp dao động nhẹ",
    "Đã thiết lập Telegram SOS Cấp cứu Gia đình 15s"
  ]
};

export const emergencyContacts = [
  {
    id: 1,
    name: "BS. Trịnh Hoàng Nam (Bác sĩ Chăm sóc)",
    role: "Bác sĩ Trực ban - Viện Tim mạch",
    phone: "+84 912 345 678",
    telegramUsername: "@Dr_HoangNam_Cardio",
    isPrimary: true
  },
  {
    id: 2,
    name: "Trần Mai Phương (Con gái)",
    role: "Người giám hộ chính",
    phone: "+84 987 123 456",
    telegramUsername: "@MaiPhuong_Family",
    isPrimary: false
  },
  {
    id: 3,
    name: "Group Telegram Gia Đình & Cấp Cứu",
    role: "Telegram Bot Alert Group",
    phone: "Telegram API",
    telegramUsername: "@CardioGuard_MaiLan_SOS",
    isPrimary: true
  }
];

export const mockVitalSummary = {
  currentBpm: 76,
  minBpm24h: 60,
  maxBpm24h: 122,
  avgBpm24h: 74,
  bpmStatus: "NORMAL",
  
  spO2: 98,
  spO2Status: "EXCELLENT",
  
  hrv: 52, // ms
  hrvStatus: "OPTIMAL",
  
  fallRiskScore: 6, // 0 - 100
  fallRiskStatus: "LOW_RISK",
  
  tinymlInferenceTime: "3.1 ms",
  dspFilterSNR: "25.2 dB",
  dspStages: "5-Stage Bandpass + Notch + Moving Avg",
};

export const mock24hData = [
  { time: "00:00", bpm: 64, spO2: 98, activity: "Ngủ sâu" },
  { time: "02:00", bpm: 61, spO2: 99, activity: "Ngủ sâu" },
  { time: "04:00", bpm: 60, spO2: 98, activity: "Ngủ nhẹ" },
  { time: "06:00", bpm: 70, spO2: 98, activity: "Thức dậy" },
  { time: "08:00", bpm: 86, spO2: 97, activity: "Tập yoga nhẹ" },
  { time: "10:00", bpm: 76, spO2: 98, activity: "Đọc sách" },
  { time: "12:00", bpm: 79, spO2: 98, activity: "Ăn trưa" },
  { time: "14:00", bpm: 72, spO2: 99, activity: "Nghỉ trưa" },
  { time: "16:00", bpm: 88, spO2: 97, activity: "Đi bộ công viên" },
  { time: "18:00", bpm: 81, spO2: 98, activity: "Ăn tối" },
  { time: "20:00", bpm: 75, spO2: 98, activity: "Xem TV" },
  { time: "22:00", bpm: 68, spO2: 98, activity: "Chuẩn bị ngủ" }
];

export const mock7DaysTrend = [
  { day: "Thứ 2", avgBpm: 73, restBpm: 61, peakBpm: 116, avgSpO2: 98, fallAlerts: 0 },
  { day: "Thứ 3", avgBpm: 75, restBpm: 62, peakBpm: 120, avgSpO2: 97, fallAlerts: 0 },
  { day: "Thứ 4", avgBpm: 77, restBpm: 63, peakBpm: 125, avgSpO2: 98, fallAlerts: 1 },
  { day: "Thứ 5", avgBpm: 72, restBpm: 60, peakBpm: 112, avgSpO2: 99, fallAlerts: 0 },
  { day: "Thứ 6", avgBpm: 74, restBpm: 61, peakBpm: 118, avgSpO2: 98, fallAlerts: 0 },
  { day: "Thứ 7", avgBpm: 79, restBpm: 64, peakBpm: 130, avgSpO2: 97, fallAlerts: 0 },
  { day: "Chủ Nhật", avgBpm: 73, restBpm: 59, peakBpm: 110, avgSpO2: 98, fallAlerts: 0 }
];

export const mockIMUData = {
  normalWalking: [
    { sample: 1, accX: 0.12, accY: 0.98, accZ: 0.15, mag: 1.01 },
    { sample: 2, accX: 0.25, accY: 1.15, accZ: 0.22, mag: 1.19 },
    { sample: 3, accX: -0.10, accY: 0.88, accZ: -0.05, mag: 0.89 },
    { sample: 4, accX: 0.30, accY: 1.28, accZ: 0.35, mag: 1.36 },
    { sample: 5, accX: 0.05, accY: 0.95, accZ: 0.10, mag: 0.96 }
  ],
  fallPattern: [
    { phase: "Giai đoạn 1: Rơi tự do (Free-Fall)", time: "0.0s - 0.3s", accMag: 0.15, desc: "Trọng lực giả = 0g (Mất thăng bằng)" },
    { phase: "Giai đoạn 2: Va chạm (Impact Peak)", time: "0.3s - 0.5s", accMag: 3.85, desc: "Gia tốc cực đại > 2.8g (Chạm sàn)" },
    { phase: "Giai đoạn 3: Nằm yên (Inaction/Stasis)", time: "0.5s - 4.0s", accMag: 1.02, desc: "Không có chuyển động góc (Độ lệch góc > 60°)" },
    { phase: "Giai đoạn 4: Cảnh báo 15s (SOS Countdown)", time: "4.0s - 19.0s", accMag: 1.00, desc: "Đang đếm ngược 15s + Rung còi + Màn hình đỏ" }
  ]
};

export const initialEventLogs = [
  {
    id: "TELE-LOG-9921",
    timestamp: "2026-08-15 16:45:10",
    eventType: "TELEGRAM_SOS_SENT",
    severity: "CRITICAL",
    aiModel: "TinyML INT8 Fall AI (8KB)",
    confidence: "99.2%",
    actionTaken: "Đã gửi thông báo SOS Telegram về Group @CardioGuard_MaiLan_SOS",
    status: "TELEGRAM_SENT",
    details: "Bệnh nhân Nguyễn Thị Mai Lan • Nhịp tim 118 BPM | SpO2 98% | GPS: 10.7769° N, 106.7009° E. Đã lưu vào báo cáo."
  },
  {
    id: "TELE-LOG-9810",
    timestamp: "2026-08-14 09:15:00",
    eventType: "TELEGRAM_ARRHYTHMIA_ALERT",
    severity: "WARNING",
    aiModel: "TinyML INT8 Arrhythmia AI (30KB)",
    confidence: "93.4%",
    actionTaken: "Gửi thông báo Telegram tự động: Phát hiện Nhịp ngoại tâm thu (PVC)",
    status: "TELEGRAM_SENT",
    details: "Phát hiện xung R-R bất thường khi đọc sách. Đã tự động gửi thông báo Telegram & sao lưu dữ liệu."
  },
  {
    id: "LOG-8812",
    timestamp: "2026-08-12 14:20:00",
    eventType: "FALL_ALERT_CANCELLED",
    severity: "INFO",
    aiModel: "Nút bấm HỦY trên mặt kính ESP32-S3",
    confidence: "100%",
    actionTaken: "Người dùng bấm HỦY báo động sau 3 giây (Báo động giả khi ngồi xuống sofa)",
    status: "CANCELLED_BY_USER",
    details: "Đã bấm HỦY thành công. Không phát tin nhắn SOS."
  }
];
