import React, { useState, useEffect } from 'react';
import { Play, Pause, Tag, Download, Database, CheckCircle2, Clock, Activity, FileSpreadsheet, Zap } from 'lucide-react';
import confetti from 'canvas-confetti';

export default function DataCollectorModule({ simulatedBpm, onLogEvent }) {
  // AUTO-RECORDING ACTIVE BY DEFAULT (Không cần bấm bắt đầu)
  const [isRecording, setIsRecording] = useState(true);
  const [isPaused, setIsPaused] = useState(false);
  const [secondsElapsed, setSecondsElapsed] = useState(0);
  const [samplesCount, setSamplesCount] = useState(1250); // Initial live telemetry samples
  const [sessionTags, setSessionTags] = useState([]);
  const [sessionData, setSessionData] = useState([]);

  // Continuous Automatic Data Accumulator
  useEffect(() => {
    let timer;
    if (isRecording && !isPaused) {
      timer = setInterval(() => {
        setSecondsElapsed((prev) => prev + 1);
        setSamplesCount((prev) => prev + 50); // 50Hz continuous sampling rate

        const newRow = {
          timestamp: new Date().toLocaleTimeString('vi-VN'),
          bpm: simulatedBpm + (Math.floor(Math.random() * 5) - 2),
          spO2: 98,
          hrv: 52 + (Math.floor(Math.random() * 4) - 2),
          accX: (Math.random() * 0.2 + 0.1).toFixed(2),
          accY: (Math.random() * 0.2 + 0.9).toFixed(2),
          accZ: (Math.random() * 0.2 + 0.1).toFixed(2),
          aiRiskScore: 6,
          activeTag: sessionTags.length > 0 ? sessionTags[sessionTags.length - 1].tag : 'Tự động ghi nhận (Auto)'
        };

        setSessionData((prev) => [...prev, newRow]);
      }, 1000);
    }
    return () => clearInterval(timer);
  }, [isRecording, isPaused, simulatedBpm, sessionTags]);

  const handlePauseSession = () => {
    setIsPaused(!isPaused);
  };

  const handleAddTag = (tagName) => {
    const newTag = {
      time: formatTime(secondsElapsed),
      tag: tagName,
      bpmAtTag: simulatedBpm
    };
    setSessionTags((prev) => [...prev, newTag]);
  };

  const handleExportSessionCSV = () => {
    let csvContent = "data:text/csv;charset=utf-8,Timestamp,BPM,SpO2,HRV_ms,AccX_g,AccY_g,AccZ_g,AI_FallRiskScore,EventTag\n";
    sessionData.forEach((row) => {
      csvContent += `${row.timestamp},${row.bpm},${row.spO2},${row.hrv},${row.accX},${row.accY},${row.accZ},${row.aiRiskScore},${row.activeTag}\n`;
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `CardioGuardAI_AutoSessionData_${new Date().toISOString().slice(0, 10)}.csv`);
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
            <Zap className="w-5 h-5 text-emerald-400 animate-bounce" />
            <h3 className="text-lg font-bold text-white">TỰ ĐỘNG THU THẬP DỮ LIỆU SINH HỌC LIÊN TỤC (AUTO-COLLECTOR)</h3>
            <span className="px-2.5 py-0.5 rounded-full bg-emerald-500/20 text-emerald-400 font-mono text-xs font-bold border border-emerald-500/30">
              Tự Động Ghi Khi Đeo Đồng Hồ
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Không cần bấm bắt đầu — Hệ thống tự động lưu liên tục chuỗi dữ liệu Nhịp tim, SpO2, HRV & Gia tốc IMU để phục vụ báo cáo
          </p>
        </div>

        {/* Action Controls */}
        <div className="flex items-center space-x-2">
          <button
            onClick={handlePauseSession}
            className={`px-3.5 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-1.5 ${
              isPaused
                ? 'bg-emerald-500 text-slate-950'
                : 'glass-panel text-amber-300 border-amber-400/40 hover:text-white'
            }`}
          >
            {isPaused ? <Play className="w-4 h-4 fill-current" /> : <Pause className="w-4 h-4 fill-current" />}
            <span>{isPaused ? 'Tiếp tục ghi' : 'Tạm dừng ghi'}</span>
          </button>

          <button
            onClick={handleExportSessionCSV}
            className="px-4 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-xl shadow-lg flex items-center space-x-2 transition-all transform hover:scale-105"
          >
            <FileSpreadsheet className="w-4 h-4" />
            <span>Tải CSV Dữ Liệu Tự Động Ghi ({sessionData.length})</span>
          </button>
        </div>
      </div>

      {/* Recording Status Metrics Bar */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        
        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-emerald-400 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">TRẠNG THÁI GHI DỮ LIỆU</span>
          <div className="flex items-center space-x-2 mt-1">
            <span className={`w-3 h-3 rounded-full ${!isPaused ? 'bg-emerald-400 animate-ping' : 'bg-amber-400'}`} />
            <span className="text-sm font-bold text-emerald-400 font-mono">
              {!isPaused ? 'TỰ ĐỘNG GHI LIVE' : 'TẠM DỪNG'}
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
          <span className="text-[10px] font-mono text-slate-400 uppercase block">TỔNG MẪU ĐÃ LƯU TRỮ</span>
          <div className="flex items-center space-x-1.5 mt-1">
            <Activity className="w-4 h-4 text-rose-400" />
            <span className="text-lg font-black text-white font-mono">{samplesCount.toLocaleString()} samples</span>
          </div>
        </div>

        <div className="glass-panel p-4 rounded-2xl border-l-4 border-l-amber-400 bg-slate-950/80">
          <span className="text-[10px] font-mono text-slate-400 uppercase block">TẦN SỐ THỜI GIAN THỰC</span>
          <div className="flex items-center space-x-1.5 mt-1">
            <span className="text-lg font-black text-amber-300 font-mono">50Hz PPG • 100Hz IMU</span>
          </div>
        </div>

      </div>

      {/* Quick Event Tagging Section */}
      <div className="space-y-3 pt-1">
        <div className="flex items-center space-x-2">
          <Tag className="w-4 h-4 text-amber-400" />
          <h4 className="text-xs font-bold text-white uppercase tracking-wider">ĐÁNH DẤU NHÃN SỰ KIỆN TỨC THÌ (INSTANT EVENT MARKER)</h4>
        </div>

        <div className="flex flex-wrap gap-2">
          {['Vận động nhẹ', 'Vận động mạnh', 'Uống thuốc tim', 'Nghỉ ngơi', 'Nhịp tim bất thường', 'Giả lập Té ngã'].map((tag, idx) => (
            <button
              key={idx}
              onClick={() => handleAddTag(tag)}
              className="px-3 py-1.5 rounded-xl glass-panel border border-slate-700 hover:border-cyan-400 text-xs font-medium text-slate-300 hover:text-white transition-all flex items-center space-x-1"
            >
              <span>+ {tag}</span>
            </button>
          ))}
        </div>

        {/* Display Tagged List */}
        {sessionTags.length > 0 && (
          <div className="flex flex-wrap gap-2 pt-2">
            {sessionTags.map((t, i) => (
              <span key={i} className="px-2.5 py-1 rounded-lg bg-slate-800 text-cyan-300 text-[11px] font-mono border border-slate-700 flex items-center space-x-1">
                <span className="text-slate-500">[{t.time}]</span>
                <span className="font-bold">{t.tag}</span>
                <span className="text-rose-400 font-bold">({t.bpmAtTag} BPM)</span>
              </span>
            ))}
          </div>
        )}
      </div>

    </div>
  );
}
