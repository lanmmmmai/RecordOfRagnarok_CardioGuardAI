import React, { useState, useEffect } from 'react';
import { AlertTriangle, ShieldCheck, XCircle, Send, Volume2, CheckCircle2 } from 'lucide-react';
import confetti from 'canvas-confetti';

export default function AlertSimulator({ isOpen, onClose, onLogEvent }) {
  const [countdown, setCountdown] = useState(15);
  const [isCancelled, setIsCancelled] = useState(false);
  const [isSent, setIsSent] = useState(false);

  useEffect(() => {
    if (!isOpen) {
      setCountdown(15);
      setIsCancelled(false);
      setIsSent(false);
      return;
    }

    if (countdown > 0 && !isCancelled && !isSent) {
      const timer = setTimeout(() => {
        setCountdown((prev) => prev - 1);
      }, 1000);
      return () => clearTimeout(timer);
    } else if (countdown === 0 && !isCancelled && !isSent) {
      // Alarm triggered!
      setIsSent(true);
      if (onLogEvent) {
        onLogEvent({
          id: `LOG-${Math.floor(1000 + Math.random() * 9000)}`,
          timestamp: new Date().toLocaleString('vi-VN'),
          eventType: 'FALL_DETECTED_SIMULATED',
          severity: 'CRITICAL',
          aiModel: 'TinyML INT8 Fall AI (8KB)',
          confidence: '99.1%',
          actionTaken: 'Đã hết 15s đếm ngược -> Gửi Telegram Alert thành công',
          status: 'TELEGRAM_SENT',
          details: 'Giả lập cú ngã từ giao diện 3D Web App. Đã gửi Telegram khẩn cấp.'
        });
      }
      confetti({
        particleCount: 100,
        spread: 70,
        origin: { y: 0.6 }
      });
    }
  }, [isOpen, countdown, isCancelled, isSent, onLogEvent]);

  if (!isOpen) return null;

  const handleCancelSOS = () => {
    setIsCancelled(true);
    if (onLogEvent) {
      onLogEvent({
        id: `LOG-${Math.floor(1000 + Math.random() * 9000)}`,
        timestamp: new Date().toLocaleString('vi-VN'),
        eventType: 'FALL_ALERT_CANCELLED',
        severity: 'INFO',
        aiModel: 'User Interface Touch Button',
        confidence: '100%',
        actionTaken: 'Người dùng bấm nút HỦY trên giao diện trong thời gian 15s',
        status: 'CANCELLED_BY_USER',
        details: 'Đã bấm HỦY báo động thành công. Không gửi Telegram SOS.'
      });
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/85 backdrop-blur-md animate-fadeIn">
      
      <div className={`w-full max-w-lg rounded-3xl p-6 md:p-8 transition-all relative overflow-hidden ${
        isCancelled
          ? 'glass-panel border-emerald-500/50 shadow-2xl shadow-emerald-500/20'
          : isSent
          ? 'glass-panel border-blue-500/50 shadow-2xl shadow-blue-500/20'
          : 'glass-alert-glow border-red-500/80 animate-pulse-slow'
      }`}>

        {/* Close Modal Button */}
        <button
          onClick={onClose}
          className="absolute top-4 right-4 text-slate-400 hover:text-white p-2 rounded-full hover:bg-slate-800 transition-colors"
        >
          <XCircle className="w-6 h-6" />
        </button>

        {/* State 1: Active 15s Countdown */}
        {!isCancelled && !isSent && (
          <div className="text-center space-y-6">
            
            <div className="w-20 h-20 mx-auto rounded-full bg-rose-500/20 border-2 border-rose-500 flex items-center justify-center text-rose-500 animate-bounce">
              <AlertTriangle className="w-10 h-10" />
            </div>

            <div>
              <span className="px-3 py-1 bg-rose-500/30 text-rose-300 border border-rose-500/50 rounded-full text-xs font-bold font-mono">
                ⚠️ PHÁT HIỆN TÉ NGÃ • TINYML AI CONFIDENCE 99.4%
              </span>
              <h2 className="text-3xl font-black text-white mt-3">
                ĐANG ĐẾM NGƯỢC CẢNH BÁO SOS
              </h2>
              <p className="text-slate-300 text-xs mt-1">
                ESP32-S3 Watch đang rung & phát còi báo động khẩn cấp.
              </p>
            </div>

            {/* Huge Timer Circle */}
            <div className="relative w-36 h-36 mx-auto flex items-center justify-center">
              <svg className="w-full h-full transform -rotate-90">
                <circle
                  cx="72"
                  cy="72"
                  r="64"
                  stroke="rgba(255,51,102,0.2)"
                  strokeWidth="10"
                  fill="none"
                />
                <circle
                  cx="72"
                  cy="72"
                  r="64"
                  stroke="#ff3366"
                  strokeWidth="10"
                  fill="none"
                  strokeDasharray="402"
                  strokeDashoffset={402 - (402 * countdown) / 15}
                  strokeLinecap="round"
                  className="transition-all duration-1000"
                />
              </svg>
              <div className="absolute flex flex-col items-center">
                <span className="text-5xl font-black text-rose-500 font-mono tracking-tight">
                  {countdown}
                </span>
                <span className="text-[10px] text-slate-400 font-bold uppercase">Giây</span>
              </div>
            </div>

            {/* BIG CANCEL BUTTON */}
            <button
              onClick={handleCancelSOS}
              className="w-full py-4 bg-gradient-to-r from-emerald-500 to-teal-500 hover:from-emerald-400 hover:to-teal-400 text-slate-950 font-black text-base rounded-2xl shadow-xl shadow-emerald-500/30 transition-all transform hover:scale-105 active:scale-95 flex items-center justify-center space-x-2"
            >
              <ShieldCheck className="w-6 h-6" />
              <span>BẤM ĐỂ HỦY BÁO ĐỘNG (I'M OK)</span>
            </button>

            <p className="text-[11px] text-slate-400 font-mono flex items-center justify-center space-x-1">
              <Volume2 className="w-3.5 h-3.5 text-rose-400 animate-pulse" />
              <span>Còi báo động: 95dB • Telegram Bot API ready</span>
            </p>
          </div>
        )}

        {/* State 2: Cancelled by User */}
        {isCancelled && (
          <div className="text-center space-y-6 py-4">
            <div className="w-20 h-20 mx-auto rounded-full bg-emerald-500/20 border-2 border-emerald-500 flex items-center justify-center text-emerald-400">
              <CheckCircle2 className="w-10 h-10" />
            </div>

            <div>
              <h2 className="text-2xl font-black text-white">ĐÃ HỦY CẢNH BÁO TÉ NGÃ</h2>
              <p className="text-slate-300 text-xs mt-2">
                Hệ thống CardioGuardAI đã ghi nhận phản hồi từ người dùng. Không có tin nhắn SOS nào được gửi tới Telegram.
              </p>
            </div>

            <div className="p-4 bg-slate-900/80 rounded-2xl text-left border border-slate-800 text-xs font-mono space-y-1">
              <p className="text-emerald-400 font-bold">✓ Log status: CANCELLED_BY_USER</p>
              <p className="text-slate-400">Thời gian phản hồi: {15 - countdown}s</p>
              <p className="text-slate-400">ESP32-S3 State: Resumed Normal Monitoring</p>
            </div>

            <button
              onClick={onClose}
              className="w-full py-3 bg-slate-800 hover:bg-slate-700 text-white font-bold text-sm rounded-xl transition-all"
            >
              Đóng cửa sổ
            </button>
          </div>
        )}

        {/* State 3: Telegram Alert Sent */}
        {isSent && (
          <div className="text-center space-y-6 py-4">
            <div className="w-20 h-20 mx-auto rounded-full bg-blue-500/20 border-2 border-blue-500 flex items-center justify-center text-blue-400">
              <Send className="w-10 h-10 animate-bounce" />
            </div>

            <div>
              <h2 className="text-2xl font-black text-white">ĐÃ GỬI TELEGRAM SOS KHẨN CẤP!</h2>
              <p className="text-slate-300 text-xs mt-2">
                Thông báo vị trí GPS và chỉ số sinh học khẩn cấp đã được gửi tới Telegram Group gia đình & Bác sĩ trực ban.
              </p>
            </div>

            <div className="p-4 bg-slate-900/90 rounded-2xl text-left border border-blue-500/30 text-xs font-mono space-y-1 text-slate-300">
              <p className="text-blue-400 font-bold">📲 Payload Telegram Sent:</p>
              <p>• Bệnh nhân: Đại tá Nguyễn Văn An (64 tuổi)</p>
              <p>• Nhịp tim: 118 BPM | SpO2: 98%</p>
              <p>• Vị trí: Phòng khách (10.7769° N, 106.7009° E)</p>
              <p className="text-rose-400">✓ HTTPS API Response 200 OK</p>
            </div>

            <button
              onClick={onClose}
              className="w-full py-3 bg-blue-600 hover:bg-blue-500 text-white font-bold text-sm rounded-xl shadow-lg transition-all"
            >
              Đã hiểu & Đóng
            </button>
          </div>
        )}

      </div>

    </div>
  );
}
