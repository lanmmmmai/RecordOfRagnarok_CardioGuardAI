import React, { useState, useEffect, useRef, useMemo } from 'react';
import { 
  Tag, Download, Database, Clock, Activity, FileSpreadsheet, 
  Zap, Radio, AlertCircle, ShieldAlert, Heart, Wind, CheckCircle2 
} from 'lucide-react';
import confetti from 'canvas-confetti';

export default function DataCollectorModule({ isConnected, rawTelemetry }) {
  const [isRecording, setIsRecording] = useState(true);
  const [secondsElapsed, setSecondsElapsed] = useState(0);
  const [samplesCount, setSamplesCount] = useState(0);
  const [sessionTags, setSessionTags] = useState([]);
  const [sessionData, setSessionData] = useState([]);

  const isWorn = Boolean(isConnected && rawTelemetry?.skinContact && (!rawTelemetry?.raw || rawTelemetry?.raw?.ir > 20000));

  // Timer for active recording duration ONLY while wearing the watch
  useEffect(() => {
    let timer;
    if (isWorn && isRecording) {
      timer = setInterval(() => {
        setSecondsElapsed((prev) => prev + 1);
      }, 1000);
    }
    return () => clearInterval(timer);
  }, [isWorn, isRecording]);

  // Session boundary gap indicator when removing watch
  const wasWornRef = useRef(false);
  useEffect(() => {
    if (wasWornRef.current && !isWorn) {
      setSessionData(prev => {
        if (prev.length === 0 || prev[0].isGap) return prev;
        return [{
          isGap: true,
          timestamp: new Date().toLocaleTimeString('vi-VN'),
        }, ...prev].slice(0, 100);
      });
    }
    wasWornRef.current = isWorn;
  }, [isWorn]);

  // 5-Second Interval Rate Limiter
  const lastRecordedBucketRef = useRef(-1);

  useEffect(() => {
    if (!isConnected || !rawTelemetry || !isRecording) return;
    if (!rawTelemetry.skinContact || (rawTelemetry.raw && rawTelemetry.raw.ir < 20000)) return;

    setSamplesCount(prev => prev + 1);

    const pulse = rawTelemetry.pulse > 0 ? rawTelemetry.pulse : 0;
    const spo2 = rawTelemetry.spo2Valid && rawTelemetry.spo2 > 0 ? rawTelemetry.spo2 : 0;
    const pi = rawTelemetry.quality ? ((rawTelemetry.quality / 50.0) * 1.2).toFixed(2) : "0.00";
    const sqi = rawTelemetry.quality || 0;
    const accX = rawTelemetry.accel ? (rawTelemetry.accel.x / 4096.0).toFixed(2) : "0.00";
    const accY = rawTelemetry.accel ? (rawTelemetry.accel.y / 4096.0).toFixed(2) : "0.00";
    const accZ = rawTelemetry.accel ? (rawTelemetry.accel.z / 4096.0).toFixed(2) : "0.00";
    const gyroX = rawTelemetry.gyro?.x ?? 0;
    const gyroY = rawTelemetry.gyro?.y ?? 0;
    const gyroZ = rawTelemetry.gyro?.z ?? 0;

    // Detect clinical anomalies
    const isFall = rawTelemetry.fallState >= 2;
    const isTachycardia = pulse > 110;
    const isBradycardia = pulse > 0 && pulse < 50;
    const isHypoxia = spo2 > 0 && spo2 < 94;
    const isAnomaly = isFall || isTachycardia || isBradycardia || isHypoxia;

    let eventStatus = "BÌNH THƯỜNG";
    if (isFall) eventStatus = "🚨 TÉ NGÃ KHẨN CẤP";
    else if (isTachycardia) eventStatus = "⚠️ TIM ĐẬP NHANH";
    else if (isBradycardia) eventStatus = "⚠️ TIM ĐẬP CHẬM";
    else if (isHypoxia) eventStatus = "⚠️ THIẾU OXY (SpO2 THẤP)";

    const now = new Date();
    const current5sBucket = Math.floor(Date.now() / 5000); // 5-second interval bucket

    // RULE: Ghi đúng 5 giây / 1 dòng, HOẶC ghi ngay tức thì nếu có sự kiện cảnh báo khẩn cấp
    if (current5sBucket === lastRecordedBucketRef.current && !isAnomaly) {
      return; // Skip duplicate frames within the 5s window
    }
    lastRecordedBucketRef.current = current5sBucket;

    const newRow = {
      timestamp: now.toLocaleTimeString('vi-VN'),
      pulse,
      spo2,
      pi,
      sqi,
      accX,
      accY,
      accZ,
      gyroX,
      gyroY,
      gyroZ,
      battery: rawTelemetry.battery || 0,
      voltage: rawTelemetry.voltage || 0.0,
      fallState: rawTelemetry.fallState >= 2 ? "CẢNH BÁO" : "AN TOÀN",
      eventStatus,
      isAnomaly,
      activeTag: sessionTags.length > 0 ? sessionTags[sessionTags.length - 1].tag : '5s Chuẩn Y Tế'
    };

    setSessionData(prev => {
      const next = [newRow, ...prev];
      return next.slice(0, 100); // Keep last 100 records
    });
  }, [rawTelemetry, isConnected, isRecording, sessionTags]);

  const handleAddTag = (tagName) => {
    const newTag = {
      time: formatTime(secondsElapsed),
      tag: tagName,
      bpmAtTag: isWorn && rawTelemetry?.pulse > 0 ? rawTelemetry.pulse : 0
    };
    setSessionTags((prev) => [...prev, newTag]);
  };

  // Clinical Summary Calculations
  const stats = useMemo(() => {
    const validPulseRows = sessionData.filter(r => !r.isGap && r.pulse > 0);
    const validSpo2Rows = sessionData.filter(r => !r.isGap && r.spo2 > 0);
    const anomalyRows = sessionData.filter(r => !r.isGap && r.isAnomaly);

    const avgPulse = validPulseRows.length > 0
      ? Math.round(validPulseRows.reduce((a, b) => a + b.pulse, 0) / validPulseRows.length)
      : 0;
    const minPulse = validPulseRows.length > 0
      ? Math.min(...validPulseRows.map(r => r.pulse))
      : 0;
    const maxPulse = validPulseRows.length > 0
      ? Math.max(...validPulseRows.map(r => r.pulse))
      : 0;

    const avgSpo2 = validSpo2Rows.length > 0
      ? Math.round(validSpo2Rows.reduce((a, b) => a + b.spo2, 0) / validSpo2Rows.length)
      : 0;
    const minSpo2 = validSpo2Rows.length > 0
      ? Math.min(...validSpo2Rows.map(r => r.spo2))
      : 0;

    return {
      avgPulse,
      minPulse,
      maxPulse,
      avgSpo2,
      minSpo2,
      anomalyCount: anomalyRows.length,
      totalRows: sessionData.filter(r => !r.isGap).length
    };
  }, [sessionData]);

  const handleExportSessionCSV = () => {
    let csvContent = "Timestamp,Pulse_BPM,SpO2_Percent,PI_Percent,SQI,AccX_g,AccY_g,AccZ_g,GyroX_dps,GyroY_dps,GyroZ_dps,Battery_Percent,Voltage_V,FallState,EventStatus,EventTag\n";
    sessionData.filter(row => !row.isGap).forEach((row) => {
      csvContent += `${row.timestamp},${row.pulse},${row.spo2},${row.pi},${row.sqi},${row.accX},${row.accY},${row.accZ},${row.gyroX},${row.gyroY},${row.gyroZ},${row.battery},${row.voltage},${row.fallState},${row.eventStatus},${row.activeTag}\n`;
    });

    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.setAttribute("href", url);
    link.setAttribute("download", `SafeWatch_Clinical_5s_${new Date().toISOString().slice(0, 10)}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);

    confetti({
      particleCount: 60,
      spread: 60,
      origin: { y: 0.7 }
    });
  };

  const formatTime = (secs) => {
    const hrs = Math.floor(secs / 3600);
    const mins = Math.floor((secs % 3600) / 60);
    const s = secs % 60;
    return `${hrs.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  };

  return (
    <div className="glass-panel rounded-3xl p-6 border border-cyan-500/30 space-y-6 bg-slate-900/80 shadow-2xl">
      
      {/* 1. Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Radio className={`w-5 h-5 ${isWorn ? 'text-emerald-400 animate-pulse' : 'text-slate-500'}`} />
            <h3 className="text-lg font-bold text-white tracking-wide">THU THẬP DỮ LIỆU ĐỊNH KỲ (CHUẨN 5 GIÂY / DÒNG)</h3>
            <span className={`px-2.5 py-0.5 rounded-full font-mono text-xs font-bold border ${
              isWorn
                ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/30'
                : 'bg-amber-500/20 text-amber-300 border-amber-500/30'
            }`}>
              {isWorn ? "🟢 CHUẨN 5S/DÒNG (ĐANG GHI)" : "🟡 CHƯA ĐEO (TẠM DỪNG)"}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            {isWorn
              ? "Tự động ghi 1 bản ghi mỗi 5 giây — Tự động ghim nhãn cảnh báo đỏ tức thì khi có biến động bất thường"
              : "Đồng hồ chưa tiếp xúc da — Tự động tạm dừng thu thập dữ liệu bảng"}
          </p>
        </div>

        {/* Action Controls */}
        <div className="flex items-center space-x-2">
          <button
            onClick={handleExportSessionCSV}
            disabled={stats.totalRows === 0}
            className="px-4 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-xl shadow-lg flex items-center space-x-2 transition-all transform hover:scale-105 disabled:opacity-40"
          >
            <FileSpreadsheet className="w-4 h-4" />
            <span>Tải CSV Báo Cáo 5s ({stats.totalRows} dòng)</span>
          </button>
        </div>
      </div>

      {/* Unworn Status Banner */}
      {!isWorn && isConnected && (
        <div className="flex items-center space-x-2.5 p-3 rounded-2xl bg-amber-500/15 border border-amber-500/40 text-amber-300 text-xs font-mono font-bold">
          <AlertCircle className="w-4 h-4 text-amber-400 shrink-0" />
          <span>THÔNG BÁO: ĐỒNG HỒ CHƯA ĐƯỢC ĐEO VÀO TAY — HỆ THỐNG TỰ ĐỘNG TẠM DỪNG THU THẬP & TRUYỀN DỮ LIỆU BẢNG</span>
        </div>
      )}

      {/* 2. Clinical Session Summary Cards (Thanh thống kê phiên đo) */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        
        {/* Heart Rate Summary */}
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-rose-500 bg-slate-950/80 space-y-1">
          <div className="flex items-center justify-between">
            <span className="text-[10px] font-mono text-slate-400 uppercase">NHỊP TIM PHIÊN ĐO</span>
            <Heart className="w-4 h-4 text-rose-400" />
          </div>
          <div className="flex items-baseline space-x-1.5">
            <span className="text-xl font-black text-white font-mono">{stats.avgPulse > 0 ? stats.avgPulse : '--'}</span>
            <span className="text-xs text-slate-400 font-mono">BPM (Avg)</span>
          </div>
          <p className="text-[11px] text-slate-400 font-mono">
            Min: <span className="text-cyan-300 font-bold">{stats.minPulse || '--'}</span> | Max: <span className="text-rose-400 font-bold">{stats.maxPulse || '--'}</span>
          </p>
        </div>

        {/* SpO2 Summary */}
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-cyan-400 bg-slate-950/80 space-y-1">
          <div className="flex items-center justify-between">
            <span className="text-[10px] font-mono text-slate-400 uppercase">OXY MÁU (SpO2)</span>
            <Wind className="w-4 h-4 text-cyan-400" />
          </div>
          <div className="flex items-baseline space-x-1.5">
            <span className="text-xl font-black text-cyan-300 font-mono">{stats.avgSpo2 > 0 ? `${stats.avgSpo2}%` : '--%'}</span>
            <span className="text-xs text-slate-400 font-mono">(Avg)</span>
          </div>
          <p className="text-[11px] text-slate-400 font-mono">
            Thấp nhất: <span className="text-amber-300 font-bold">{stats.minSpo2 > 0 ? `${stats.minSpo2}%` : '--%'}</span>
          </p>
        </div>

        {/* Duration */}
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-emerald-400 bg-slate-950/80 space-y-1">
          <div className="flex items-center justify-between">
            <span className="text-[10px] font-mono text-slate-400 uppercase">THỜI GIAN THEO DÕI</span>
            <Clock className="w-4 h-4 text-emerald-400" />
          </div>
          <p className="text-xl font-black text-white font-mono">{formatTime(secondsElapsed)}</p>
          <p className="text-[11px] text-emerald-400 font-mono font-bold">
            {isWorn ? "● Đang ghi nhận 5s/dòng" : "○ Tạm dừng"}
          </p>
        </div>

        {/* Anomaly & Alert Summary */}
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-amber-400 bg-slate-950/80 space-y-1">
          <div className="flex items-center justify-between">
            <span className="text-[10px] font-mono text-slate-400 uppercase">SỰ KIỆN CẢNH BÁO</span>
            <ShieldAlert className="w-4 h-4 text-amber-400" />
          </div>
          <p className={`text-xl font-black font-mono ${stats.anomalyCount > 0 ? 'text-rose-400' : 'text-emerald-400'}`}>
            {stats.anomalyCount} sự kiện
          </p>
          <p className="text-[11px] text-slate-400 font-mono">
            {stats.anomalyCount === 0 ? "Chưa ghi nhận bất thường" : "Đã tự động gắn cờ"}
          </p>
        </div>

      </div>

      {/* 3. Live Structured Telemetry Stream Table (5s Per Row) */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <h4 className="text-xs font-bold text-white uppercase tracking-wider font-mono flex items-center space-x-2">
            <span className={`w-2 h-2 rounded-full ${isWorn ? 'bg-cyan-400 animate-pulse' : 'bg-slate-600'}`} />
            <span>NHẬT KÝ DỮ LIỆU ĐỊNH KỲ (CHUẨN 5 GIÂY / DÒNG)</span>
          </h4>
          <span className="text-[11px] font-mono text-slate-400">Lưu giữ {stats.totalRows} bản ghi trong phiên</span>
        </div>

        <div className="overflow-x-auto rounded-2xl border border-slate-800 bg-slate-950/90 max-h-80">
          <table className="w-full text-left text-xs font-mono">
            <thead className="sticky top-0 bg-slate-900 border-b border-slate-800 text-slate-400 uppercase text-[10px]">
              <tr>
                <th className="px-3 py-2.5">Thời gian</th>
                <th className="px-3 py-2.5 text-rose-400">Nhịp Tim (BPM)</th>
                <th className="px-3 py-2.5 text-cyan-300">SpO2 (%)</th>
                <th className="px-3 py-2.5 text-emerald-400">Tưới Máu (PI%)</th>
                <th className="px-3 py-2.5 text-slate-300">Chất lượng (SQI)</th>
                <th className="px-3 py-2.5 text-amber-300">Gia tốc (X, Y, Z)</th>
                <th className="px-3 py-2.5">Pin / V</th>
                <th className="px-3 py-2.5 text-right">Trạng Thái & Sự Kiện</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60">
              {sessionData.length > 0 ? (
                sessionData.map((row, idx) => (
                  row.isGap ? (
                    <tr key={idx} className="bg-amber-500/10">
                      <td colSpan="8" className="px-3 py-1.5 text-center text-[10px] font-bold text-amber-300 tracking-wider">
                        ── THÁO ĐỒNG HỒ LÚC {row.timestamp} • TẠM DỪNG GHI NHẬN ──
                      </td>
                    </tr>
                  ) : (
                    <tr 
                      key={idx} 
                      className={`transition-colors ${
                        row.isAnomaly 
                          ? 'bg-rose-500/15 hover:bg-rose-500/25 border-l-2 border-l-rose-500' 
                          : 'hover:bg-slate-800/40'
                      }`}
                    >
                      <td className="px-3 py-2 text-slate-400">{row.timestamp}</td>
                      <td className="px-3 py-2 font-bold text-rose-400">
                        {row.pulse > 0 ? `${row.pulse} BPM` : '--'}
                      </td>
                      <td className="px-3 py-2 font-bold text-cyan-300">
                        {row.spo2 > 0 ? `${row.spo2}%` : '--%'}
                      </td>
                      <td className="px-3 py-2 text-emerald-400">{row.pi}%</td>
                      <td className="px-3 py-2 text-slate-300">{row.sqi}%</td>
                      <td className="px-3 py-2 text-amber-300">{row.accX}, {row.accY}, {row.accZ} g</td>
                      <td className="px-3 py-2 text-cyan-400">{row.battery}% ({row.voltage.toFixed(2)}V)</td>
                      <td className="px-3 py-2 text-right">
                        <span className={`px-2 py-0.5 rounded-full text-[10px] font-bold inline-block ${
                          row.isAnomaly
                            ? 'bg-rose-500/30 text-rose-300 border border-rose-500/50 animate-pulse'
                            : 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30'
                        }`}>
                          {row.eventStatus}
                        </span>
                      </td>
                    </tr>
                  )
                ))
              ) : (
                <tr>
                  <td colSpan="8" className="text-center py-8 text-slate-400">
                    <div className="flex flex-col items-center justify-center space-y-1">
                      <span className="font-bold text-amber-400">
                        {isConnected ? "ĐỒNG HỒ ĐANG Ở TRẠNG THÁI NGHỈ / CHƯA ĐEO VÀO TAY" : "CHƯA KẾT NỐI ĐỒNG HỒ"}
                      </span>
                      <span className="text-[11px] text-slate-500">
                        {isConnected ? "Hãy đeo đồng hồ hoặc áp nhẹ ngón tay vào cảm biến để bắt đầu ghi nhận dữ liệu vào bảng" : "Hệ thống đang tự động dò tìm đồng hồ trên mạng Wi-Fi"}
                      </span>
                    </div>
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>

    </div>
  );
}
