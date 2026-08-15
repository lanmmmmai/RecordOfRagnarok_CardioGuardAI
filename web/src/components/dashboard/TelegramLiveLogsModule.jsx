import React, { useState } from 'react';
import { Send, BellRing, CheckCircle2, ShieldAlert, Download, RefreshCw, Smartphone, MapPin, Heart, Activity } from 'lucide-react';
import confetti from 'canvas-confetti';

export default function TelegramLiveLogsModule({ userProfile, simulatedBpm, onLogEvent }) {
  const [telegramLogs, setTelegramLogs] = useState([
    {
      id: "TELE-ALERT-001",
      timestamp: "2026-08-15 16:45:10",
      patientName: "Nguyễn Thị Mai Lan",
      alertType: "🚨 CẢNH BÁO TÉ NGÃ KHẨN CẤP",
      bpm: 118,
      spO2: 98,
      accMag: "3.85g",
      gpsLocation: "10.7769° N, 106.7009° E (Nhà riêng - TPHCM)",
      recipientGroup: "@CardioGuard_MaiLan_SOS",
      httpStatus: "200 OK (HTTPS Telegram API)"
    },
    {
      id: "TELE-ALERT-002",
      timestamp: "2026-08-14 09:15:00",
      patientName: "Nguyễn Thị Mai Lan",
      alertType: "⚠️ CẢNH BÁO LOẠN NHỊP TIM (PVC)",
      bpm: 112,
      spO2: 97,
      accMag: "1.08g",
      gpsLocation: "10.7769° N, 106.7009° E (Phòng đọc sách)",
      recipientGroup: "@CardioGuard_MaiLan_SOS",
      httpStatus: "200 OK (HTTPS Telegram API)"
    }
  ]);

  const [isSending, setIsSending] = useState(false);

  const handleSimulateTelegramSend = () => {
    setIsSending(true);
    setTimeout(() => {
      const newAlert = {
        id: `TELE-ALERT-${Math.floor(100 + Math.random() * 900)}`,
        timestamp: new Date().toLocaleString('vi-VN'),
        patientName: userProfile.name,
        alertType: "🚨 GIẢ LẬP CẢNH BÁO THỜI GIAN THỰC (REALTIME)",
        bpm: simulatedBpm > 95 ? simulatedBpm : 115,
        spO2: 98,
        accMag: "3.92g",
        gpsLocation: "10.7769° N, 106.7009° E (Đồng hồ ESP32-S3)",
        recipientGroup: "@CardioGuard_MaiLan_SOS",
        httpStatus: "200 OK (HTTPS Telegram API)"
      };

      setTelegramLogs((prev) => [newAlert, ...prev]);
      setIsSending(false);

      if (onLogEvent) {
        onLogEvent({
          id: newAlert.id,
          timestamp: newAlert.timestamp,
          eventType: 'TELEGRAM_ALERT_TRIGGERED',
          severity: 'CRITICAL',
          aiModel: 'TinyML INT8 Live Telegram Bot API',
          confidence: '99.4%',
          actionTaken: `Đã gửi thông báo Telegram SOS về Group ${newAlert.recipientGroup}`,
          status: 'TELEGRAM_SENT',
          details: `Bệnh nhân ${newAlert.patientName} • BPM: ${newAlert.bpm} | SpO2: ${newAlert.spO2}% | GPS: ${newAlert.gpsLocation}`
        });
      }

      confetti({
        particleCount: 80,
        spread: 70,
        origin: { y: 0.6 }
      });
    }, 800);
  };

  const handleExportTelegramLogsCSV = () => {
    let csvContent = "data:text/csv;charset=utf-8,AlertID,Timestamp,PatientName,AlertType,BPM,SpO2,IMU_AccForce,GPSLocation,RecipientGroup,HTTPStatus\n";
    telegramLogs.forEach((log) => {
      csvContent += `${log.id},${log.timestamp},"${log.patientName}","${log.alertType}",${log.bpm},${log.spO2},${log.accMag},"${log.gpsLocation}",${log.recipientGroup},"${log.httpStatus}"\n`;
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `CardioGuardAI_TelegramAlerts_${userProfile.name.replace(/\s+/g, '_')}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="glass-panel rounded-3xl p-6 border border-blue-500/30 space-y-6 bg-slate-900/85 shadow-2xl">
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Send className="w-5 h-5 text-blue-400 animate-bounce" />
            <h3 className="text-lg font-bold text-white">NHẬT KÝ THÔNG BÁO TELEGRAM REALTIME (USER: {userProfile.name.toUpperCase()})</h3>
            <span className="px-2.5 py-0.5 rounded-full bg-blue-500/20 text-blue-300 font-mono text-xs font-bold border border-blue-500/30">
              Auto-Save for Clinical Report
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Tự động lưu lại tất cả dữ liệu thông báo gửi về Telegram của người dùng Nguyễn Thị Mai Lan để phục vụ báo cáo
          </p>
        </div>

        {/* Controls */}
        <div className="flex items-center space-x-2">
          <button
            onClick={handleSimulateTelegramSend}
            disabled={isSending}
            className="px-4 py-2.5 bg-gradient-to-r from-blue-600 to-indigo-600 hover:from-blue-500 hover:to-indigo-500 text-white font-bold text-xs rounded-xl shadow-lg shadow-blue-500/20 flex items-center space-x-2 transition-all transform hover:scale-105"
          >
            {isSending ? <RefreshCw className="w-4 h-4 animate-spin" /> : <Send className="w-4 h-4" />}
            <span>{isSending ? 'Đang gửi HTTPS API...' : 'GỬI THỬ TELEGRAM REALTIME'}</span>
          </button>

          <button
            onClick={handleExportTelegramLogsCSV}
            className="px-3.5 py-2.5 glass-panel text-blue-300 hover:text-white font-bold text-xs rounded-xl border border-blue-500/40 flex items-center space-x-1.5 transition-all"
          >
            <Download className="w-4 h-4" />
            <span>Tải CSV Telegram Logs ({telegramLogs.length})</span>
          </button>
        </div>
      </div>

      {/* Telegram Alerts Cards List */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {telegramLogs.map((log) => (
          <div
            key={log.id}
            className="p-4 rounded-2xl bg-slate-950/90 border border-blue-500/30 font-mono text-xs space-y-2 relative overflow-hidden"
          >
            {/* Top Bar */}
            <div className="flex items-center justify-between border-b border-slate-800 pb-2">
              <span className="text-blue-400 font-bold flex items-center space-x-1">
                <Smartphone className="w-3.5 h-3.5" />
                <span>{log.id}</span>
              </span>
              <span className="text-[10px] text-slate-500">{log.timestamp}</span>
            </div>

            {/* Content Payload */}
            <p className="text-rose-400 font-bold text-xs">{log.alertType}</p>
            <p className="text-white">👤 Bệnh nhân: <strong>{log.patientName}</strong></p>

            <div className="flex items-center space-x-4 text-slate-300">
              <span className="flex items-center space-x-1 text-rose-400 font-bold">
                <Heart className="w-3.5 h-3.5" />
                <span>{log.bpm} BPM</span>
              </span>
              <span className="flex items-center space-x-1 text-cyan-400 font-bold">
                <Activity className="w-3.5 h-3.5" />
                <span>{log.spO2}% SpO2</span>
              </span>
              <span className="text-amber-300 font-bold">Gia tốc: {log.accMag}</span>
            </div>

            <p className="text-slate-400 flex items-center space-x-1 text-[11px] truncate">
              <MapPin className="w-3.5 h-3.5 text-rose-500" />
              <span>{log.gpsLocation}</span>
            </p>

            <div className="pt-1 flex items-center justify-between text-[10px] text-slate-500 border-t border-slate-800/80">
              <span>Nhận: <strong className="text-blue-300">{log.recipientGroup}</strong></span>
              <span className="text-emerald-400 font-bold">✓ {log.httpStatus}</span>
            </div>
          </div>
        ))}
      </div>

    </div>
  );
}
