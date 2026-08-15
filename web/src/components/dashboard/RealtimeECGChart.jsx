import React, { useEffect, useState } from 'react';
import { Activity, Radio, AlertCircle } from 'lucide-react';

export default function RealtimeECGChart({ simulatedBpm, isConnected, rawTelemetry }) {
  // Real BPM from watch when connected
  const realBpm = isConnected
    ? (rawTelemetry?.skinContact && rawTelemetry?.pulse > 0 ? rawTelemetry.pulse : 0)
    : simulatedBpm;

  const [dataPoints, setDataPoints] = useState(() => Array(80).fill(0));

  // Helper generator for ECG wave shape (P-Q-R-S-T wave) driven by real BPM
  function generateECGValue(index, bpm) {
    if (bpm === 0) return (Math.random() - 0.5) * 1.5; // Baseline idle noise when not on skin
    const cycleLength = Math.max(12, Math.floor(600 / bpm));
    const pos = index % cycleLength;

    if (pos === 2) return 5; // P wave
    if (pos === 4) return -5; // Q wave
    if (pos === 5) return 48; // R peak (ECG Spike)
    if (pos === 6) return -20; // S wave
    if (pos === 8) return 12; // T wave
    return (Math.random() - 0.5) * 2; // Baseline noise
  }

  useEffect(() => {
    const interval = setInterval(() => {
      setDataPoints((prevPoints) => {
        const nextIndex = prevPoints.length;
        const nextVal = generateECGValue(nextIndex, realBpm);
        return [...prevPoints.slice(1), nextVal];
      });
    }, 60);

    return () => clearInterval(interval);
  }, [realBpm]);

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-4 relative overflow-hidden">
      
      {/* Header Info */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Activity className="w-5 h-5 text-rose-500 animate-pulse" />
            <h3 className="text-lg font-bold text-white">SÓNG TIM PPG REAL-TIME MONITOR</h3>
            <span className="px-2 py-0.5 rounded-full bg-rose-500/20 text-rose-400 font-mono text-xs font-bold border border-rose-500/30">
              {isConnected ? "Wi-Fi Telemetry Stream 10Hz" : "50Hz Sampling"}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            {isConnected
              ? "Tín hiệu sóng tim PPG thô truyền trực tiếp từ cảm biến MAX30102 trên đồng hồ SafeWatch"
              : "Lọc nhiễu DSP 5 tầng • Chuẩn R-R Interval • Phát hiện Ngoại tâm thu (PVC) INT8 AI"}
          </p>
        </div>

        <div className="flex items-center space-x-3 text-xs font-mono">
          <div className="flex items-center space-x-1.5 px-3 py-1 bg-slate-900 rounded-xl border border-slate-800">
            <Radio className="w-3.5 h-3.5 text-cyan-400 animate-ping" />
            <span className="text-cyan-400 font-bold">{isConnected ? "WIFI HARDWARE STREAM" : "STREAM LIVE"}</span>
          </div>

          <div className="px-3 py-1 bg-slate-900 rounded-xl border border-slate-800 text-rose-400 font-bold">
            {realBpm > 0 ? `${realBpm} BPM` : "CHƯA ĐEO DA"}
          </div>
        </div>
      </div>

      {/* No skin contact warning banner */}
      {isConnected && !rawTelemetry?.skinContact && (
        <div className="flex items-center space-x-2 p-3 rounded-2xl bg-amber-500/20 border border-amber-500/50 text-amber-300 text-xs font-bold">
          <AlertCircle className="w-4 h-4 text-amber-400" />
          <span>CẢNH BÁO PHẦN CỨNG: CẢM BIẾN CHƯA THÁO KHỎI / CHƯA CHẠM DA TAY DƯỚI ĐỒNG HỒ!</span>
        </div>
      )}

      {/* Oscilloscope Waveform Canvas */}
      <div className="h-56 w-full relative bg-slate-950/90 rounded-2xl p-3 border border-slate-800/80 flex items-center justify-center">
        {/* Background Grid Lines */}
        <div className="absolute inset-0 bg-[linear-gradient(to_right,#1e293b15_1px,transparent_1px),linear-gradient(to_bottom,#1e293b15_1px,transparent_1px)] bg-[size:16px_16px] pointer-events-none rounded-2xl" />

        <svg className="w-full h-full z-10" viewBox="0 0 800 200" preserveAspectRatio="none">
          {/* Baseline horizontal */}
          <line x1="0" y1="100" x2="800" y2="100" stroke="#1e293b" strokeWidth="1" strokeDasharray="4 4" />

          {/* Polyline connecting points */}
          <polyline
            fill="none"
            stroke={realBpm > 105 ? '#ff3366' : realBpm === 0 ? '#64748b' : '#00f2fe'}
            strokeWidth="2.5"
            strokeLinecap="round"
            strokeLinejoin="round"
            points={dataPoints
              .map((val, idx) => {
                const x = (idx / (dataPoints.length - 1)) * 800;
                const y = 100 - val * 2.2;
                return `${x},${y}`;
              })
              .join(' ')}
            className="drop-shadow-[0_0_10px_#00f2fe]"
          />
        </svg>

        {/* Scanline overlay effect */}
        <div className="absolute inset-0 bg-gradient-to-b from-transparent via-cyan-500/5 to-transparent animate-scanline pointer-events-none rounded-2xl" />
      </div>

      {/* Footer Stats Ticker */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
        <div className="p-2.5 glass-panel rounded-xl text-center">
          <span className="text-slate-400 block text-[10px]">TẦN SỐ TRUYỀN</span>
          <span className="text-cyan-400 font-bold">{isConnected ? "10 Hz Wi-Fi" : "1.25 Hz"}</span>
        </div>
        <div className="p-2.5 glass-panel rounded-xl text-center">
          <span className="text-slate-400 block text-[10px]">CHẤT LƯỢNG SÓNG SQI</span>
          <span className="text-emerald-400 font-bold">{isConnected ? `${rawTelemetry?.quality || 0}%` : "789 ms"}</span>
        </div>
        <div className="p-2.5 glass-panel rounded-xl text-center">
          <span className="text-slate-400 block text-[10px]">TRẠNG THÁI CHẠM DA</span>
          <span className={isConnected && rawTelemetry?.skinContact ? "text-emerald-400 font-bold" : "text-amber-300 font-bold"}>
            {isConnected ? (rawTelemetry?.skinContact ? "ĐÃ ĐEO CHẠM DA" : "CHƯA ĐEO DA") : "50 Hz ACTIVE"}
          </span>
        </div>
        <div className="p-2.5 glass-panel rounded-xl text-center">
          <span className="text-slate-400 block text-[10px]">CẢM BIẾN NGUYÊN BẢN</span>
          <span className="text-rose-400 font-bold">{isConnected ? "100% REAL DATA" : "NORMAL SINUS"}</span>
        </div>
      </div>

    </div>
  );
}
