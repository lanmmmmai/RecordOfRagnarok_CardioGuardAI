import React, { useState, useEffect, useRef } from 'react';
import { Tag, Download, Database, Clock, Activity, FileSpreadsheet, Zap, Radio } from 'lucide-react';
import confetti from 'canvas-confetti';

export default function DataCollectorModule({ isConnected, rawTelemetry }) {
  const [isRecording, setIsRecording] = useState(true);
  const [secondsElapsed, setSecondsElapsed] = useState(0);
  const [samplesCount, setSamplesCount] = useState(0);
  const [sessionTags, setSessionTags] = useState([]);
  const [sessionData, setSessionData] = useState([]);

  // Timer for duration
  useEffect(() => {
    let timer;
    if (isConnected && isRecording) {
      timer = setInterval(() => {
        setSecondsElapsed((prev) => prev + 1);
      }, 1000);
    }
    return () => clearInterval(timer);
  }, [isConnected, isRecording]);

  // Continuous real-time packet ingestion (10Hz from WebSocket)
  useEffect(() => {
    if (!isConnected || !rawTelemetry || !isRecording) return;

    setSamplesCount(prev => prev + 1);

    const hasSkin = Boolean(rawTelemetry.skinContact);
    const pulse = hasSkin && rawTelemetry.pulse > 0 ? rawTelemetry.pulse : 0;
    const spo2 = hasSkin && rawTelemetry.spo2Valid && rawTelemetry.spo2 > 0 ? rawTelemetry.spo2 : 0;
    const pi = hasSkin && rawTelemetry.quality ? ((rawTelemetry.quality / 50.0) * 1.2).toFixed(2) : "0.00";
    const sqi = hasSkin ? (rawTelemetry.quality || 0) : 0;
    const accX = rawTelemetry.accel ? (rawTelemetry.accel.x / 4096.0).toFixed(2) : "0.00";
    const accY = rawTelemetry.accel ? (rawTelemetry.accel.y / 4096.0).toFixed(2) : "0.00";
    const accZ = rawTelemetry.accel ? (rawTelemetry.accel.z / 4096.0).toFixed(2) : "0.00";
    const gyroX = rawTelemetry.gyro?.x ?? 0;
    const gyroY = rawTelemetry.gyro?.y ?? 0;
    const gyroZ = rawTelemetry.gyro?.z ?? 0;

    const newRow = {
      timestamp: rawTelemetry.timestamp || new Date().toLocaleTimeString('vi-VN'),
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
      fallState: rawTelemetry.fallState === 0 ? "AN TOÀN" : "CẢNH BÁO",
      activeTag: sessionTags.length > 0 ? sessionTags[sessionTags.length - 1].tag : 'Realtime Stream'
    };

    setSessionData(prev => {
      const next = [newRow, ...prev];
      return next.slice(0, 50); // Keep last 50 live frames in memory
    });
  }, [rawTelemetry, isConnected, isRecording, sessionTags]);

  const handleAddTag = (tagName) => {
    const newTag = {
      time: formatTime(secondsElapsed),
      tag: tagName,
      bpmAtTag: rawTelemetry?.skinContact && rawTelemetry?.pulse > 0 ? rawTelemetry.pulse : 0
    };
    setSessionTags((prev) => [...prev, newTag]);
  };

  const handleExportSessionCSV = () => {
    let csvContent = "data:text/csv;charset=utf-8,Timestamp,Pulse_BPM,SpO2_Percent,PI_Percent,SQI,AccX_g,AccY_g,AccZ_g,GyroX_dps,GyroY_dps,GyroZ_dps,Battery_Percent,Voltage_V,FallState,EventTag\n";
    sessionData.forEach((row) => {
      csvContent += `${row.timestamp},${row.pulse},${row.spo2},${row.pi},${row.sqi},${row.accX},${row.accY},${row.accZ},${row.gyroX},${row.gyroY},${row.gyroZ},${row.battery},${row.voltage},${row.fallState},${row.activeTag}\n`;
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `SafeWatch_Realtime_Stream_${new Date().toISOString().slice(0, 10)}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);

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
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Radio className="w-5 h-5 text-emerald-400 animate-pulse" />
            <h3 className="text-lg font-bold text-white">TRUYỀN VÀ THU THẬP THÔNG SỐ LIÊN TỤC (10Hz REALTIME STREAM)</h3>
            <span className={`px-2.5 py-0.5 rounded-full font-mono text-xs font-bold border ${
              isConnected ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/30' : 'bg-slate-800 text-slate-400 border-slate-700'
            }`}>
              {isConnected ? "🟢 10Hz Đang Truyền Dữ Liệu Thật" : "OFFLINE"}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Dữ liệu sinh hiệu và cảm biến từ đồng hồ được truyền về liên tục 10 lần/giây qua WebSocket & lưu trữ PostgreSQL
          </p>
        </div>

        {/* Action Controls */}
        <div className="flex items-center space-x-2">
          <button
            onClick={handleExportSessionCSV}
            disabled={sessionData.length === 0}
            className="px-4 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-xl shadow-lg flex items-center space-x-2 transition-all transform hover:scale-105 disabled:opacity-40"
          >
            <FileSpreadsheet className="w-4 h-4" />
            <span>Tải CSV Dữ Liệu Thật ({sessionData.length} dòng)</span>
          </button>
        </div>
      </div>

      {/* Recording Status Metrics Bar */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-emerald-400 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">TRẠNG THÁI STREAM</span>
          <div className="flex items-center space-x-2 mt-1">
            <span className={`w-3 h-3 rounded-full ${isConnected ? 'bg-emerald-400 animate-ping' : 'bg-slate-600'}`} />
            <span className="text-sm font-bold text-emerald-400 font-mono">
              {isConnected ? 'LIÊN TỤC 10Hz' : 'CHƯA KẾT NỐI'}
            </span>
          </div>
        </div>

        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-cyan-400 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">THỜI GIAN THEO DÕI</span>
          <div className="flex items-center space-x-1.5 mt-1">
            <Clock className="w-4 h-4 text-cyan-400" />
            <span className="text-lg font-black text-white font-mono">{formatTime(secondsElapsed)}</span>
          </div>
        </div>

        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-rose-500 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">GÓI TIN ĐÃ TRUYỀN VỀ</span>
          <div className="flex items-center space-x-1.5 mt-1">
            <Activity className="w-4 h-4 text-rose-400" />
            <span className="text-lg font-black text-white font-mono">{samplesCount.toLocaleString()} frames</span>
          </div>
        </div>

        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-amber-400 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">BĂNG THÔNG DỮ LIỆU</span>
          <div className="flex items-center space-x-1.5 mt-1">
            <span className="text-lg font-black text-amber-300 font-mono">10Hz Telemetry • 12 Fields</span>
          </div>
        </div>

      </div>

      {/* Live Continuous Telemetry Stream Table */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <h4 className="text-xs font-bold text-white uppercase tracking-wider font-mono flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" />
            <span>NHẬT KÝ GÓI TIN TRUYỀN VỀ LIÊN TỤC (LIVE STREAMING FRAMES)</span>
          </h4>
          <span className="text-[11px] font-mono text-slate-400">Hiển thị 50 gói tin gần nhất</span>
        </div>

        <div className="overflow-x-auto rounded-2xl border border-slate-800 bg-slate-950/90 max-h-72">
          <table className="w-full text-left text-xs font-mono">
            <thead className="sticky top-0 bg-slate-900 border-b border-slate-800 text-slate-400 uppercase text-[10px]">
              <tr>
                <th className="px-3 py-2.5">Thời gian</th>
                <th className="px-3 py-2.5 text-rose-400">Nhịp Tim (BPM)</th>
                <th className="px-3 py-2.5 text-cyan-300">SpO2 (%)</th>
                <th className="px-3 py-2.5 text-emerald-400">Tưới Máu (PI%)</th>
                <th className="px-3 py-2.5">SQI</th>
                <th className="px-3 py-2.5 text-amber-300">Gia tốc (X, Y, Z)</th>
                <th className="px-3 py-2.5">Pin / V</th>
                <th className="px-3 py-2.5">Trạng Thái</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60">
              {sessionData.length > 0 ? (
                sessionData.map((row, idx) => (
                  <tr key={idx} className="hover:bg-slate-800/40 transition-colors">
                    <td className="px-3 py-2 text-slate-400">{row.timestamp}</td>
                    <td className="px-3 py-2 font-bold text-rose-400">{row.pulse > 0 ? `${row.pulse} BPM` : '0'}</td>
                    <td className="px-3 py-2 font-bold text-cyan-300">{row.spo2 > 0 ? `${row.spo2}%` : '0%'}</td>
                    <td className="px-3 py-2 text-emerald-400">{row.pi}%</td>
                    <td className="px-3 py-2 text-slate-300">{row.sqi}%</td>
                    <td className="px-3 py-2 text-amber-300">{row.accX}, {row.accY}, {row.accZ} g</td>
                    <td className="px-3 py-2 text-cyan-400">{row.battery}% ({row.voltage.toFixed(2)}V)</td>
                    <td className="px-3 py-2">
                      <span className="px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 text-[10px] font-bold">
                        {row.fallState}
                      </span>
                    </td>
                  </tr>
                ))
              ) : (
                <tr>
                  <td colSpan="8" className="text-center py-6 text-slate-500">
                    {isConnected ? "Đang chờ nhận gói tin telemetry từ đồng hồ..." : "Đồng hồ chưa kết nối"}
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
