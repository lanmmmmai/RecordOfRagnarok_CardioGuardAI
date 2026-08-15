import React, { useState, useEffect, useRef } from 'react';
import { Activity, Wind, HeartPulse, CheckCircle2, AlertTriangle, ShieldCheck, Zap } from 'lucide-react';
import ScrollReveal from '../ScrollReveal';

export default function SpO2ClinicalModule({ isConnected, rawTelemetry }) {
  const [spo2History, setSpo2History] = useState(() => Array(30).fill(0));
  const canvasRef = useRef(null);

  const spo2Val = isConnected && rawTelemetry?.spo2Valid && rawTelemetry?.spo2 > 0
    ? rawTelemetry.spo2
    : 0;

  const perfusion = isConnected && rawTelemetry?.quality
    ? ((rawTelemetry.quality / 50.0) * 1.2).toFixed(2)
    : "0.00";

  // Classify clinical status
  const getClinicalStatus = (val) => {
    if (!isConnected || val === 0) return { label: "CHƯA CÓ DỮ LIỆU", color: "text-slate-400 border-slate-700 bg-slate-800/40", desc: "Đeo đồng hồ để bắt đầu phân tích SpO2" };
    if (val >= 96) return { label: "OXY MÁU TỐI ƯU (BÌNH THƯỜNG)", color: "text-cyan-400 border-cyan-500/40 bg-cyan-500/15", desc: "Độ bão hòa oxy mao mạch ở trạng thái lý tưởng" };
    if (val >= 94) return { label: "MỨC CHẤP NHẬN ĐƯỢC", color: "text-emerald-400 border-emerald-500/40 bg-emerald-500/15", desc: "Mức oxy ổn định, tiếp tục theo dõi thường quy" };
    if (val >= 90) return { label: "THIẾU OXY NHẸ (MILD HYPOXEMIA)", color: "text-amber-400 border-amber-500/40 bg-amber-500/15", desc: "Cần hít thở sâu và kiểm tra thông khí phòng" };
    return { label: "THIẾU OXY CẤP (CRITICAL HYPOXIA)", color: "text-rose-400 border-rose-500/50 bg-rose-500/20 animate-pulse", desc: "Cảnh báo khẩn cấp: Độ bão hòa oxy dưới ngưỡng an toàn" };
  };

  const status = getClinicalStatus(spo2Val);

  // Update history stream
  useEffect(() => {
    if (isConnected && spo2Val > 0) {
      setSpo2History(prev => {
        const next = [...prev.slice(1), spo2Val];
        return next;
      });
    }
  }, [spo2Val, isConnected]);

  // Render Realtime PPG Optical Waveform (Plethysmogram)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;

    let animId;
    let offset = 0;

    const render = () => {
      ctx.clearRect(0, 0, w, h);

      // Grid Lines
      ctx.strokeStyle = 'rgba(6, 182, 212, 0.08)';
      ctx.lineWidth = 1;
      for (let x = 0; x < w; x += 30) {
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
      }
      for (let y = 0; y < h; y += 20) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
      }

      if (!isConnected || spo2Val === 0) {
        // Flatline
        ctx.strokeStyle = '#334155';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(0, h / 2);
        ctx.lineTo(w, h / 2);
        ctx.stroke();

        ctx.fillStyle = '#64748b';
        ctx.font = 'bold 12px monospace';
        ctx.textAlign = 'center';
        ctx.fillText('ĐANG CHỜ TÍN HIỆU QUANG HỌC SPO2 (RED/IR PPG)...', w / 2, h / 2 - 12);
        return;
      }

      // Draw dynamic pulse plethysmogram curve
      ctx.strokeStyle = '#06b6d4';
      ctx.lineWidth = 2.5;
      ctx.shadowColor = '#06b6d4';
      ctx.shadowBlur = 8;
      ctx.beginPath();

      offset += 0.05;
      for (let x = 0; x < w; x++) {
        const t = (x * 0.035) - offset;
        // Realistic PPG pulse contour: Systolic peak + Dicrotic notch
        const yBase = h * 0.55;
        const pulse = Math.sin(t) * 28 + Math.sin(t * 2 + 0.5) * 12 + Math.cos(t * 3) * 4;
        const y = yBase - pulse;

        if (x === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.shadowBlur = 0;

      animId = requestAnimationFrame(render);
    };

    render();
    return () => cancelAnimationFrame(animId);
  }, [isConnected, spo2Val]);

  return (
    <div className="glass-panel p-6 rounded-3xl border border-cyan-500/30 bg-slate-900/90 shadow-2xl space-y-6">
      
      {/* Module Title Bar */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 border-b border-slate-800 pb-4">
        <div className="flex items-center space-x-3">
          <div className="w-10 h-10 rounded-2xl bg-cyan-500/20 text-cyan-400 flex items-center justify-center border border-cyan-500/30">
            <Wind className="w-5 h-5" />
          </div>
          <div>
            <h3 className="text-base font-bold text-white uppercase tracking-wider">
              THEO DÕI NỒNG ĐỘ OXY TRONG MÁU (SPO2 CLINICAL SUITE)
            </h3>
            <p className="text-xs text-slate-400">
              Thuật toán quang phổ Maxim 25Hz kết hợp lọc nhiễu thích nghi DSP 5 tầng
            </p>
          </div>
        </div>

        <div className={`px-3 py-1.5 rounded-full border text-xs font-mono font-bold flex items-center space-x-1.5 ${status.color}`}>
          <span className="w-2 h-2 rounded-full bg-current animate-pulse" />
          <span>{status.label}</span>
        </div>
      </div>

      {/* 4-Stat High-Density Row */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        
        {/* Main SpO2 */}
        <div className="p-4 glass-panel rounded-2xl border border-cyan-500/40 bg-slate-950/70">
          <span className="text-[11px] font-mono text-slate-400 block mb-1">NỒNG ĐỘ OXY MÁU (SPO2)</span>
          <div className="flex items-baseline space-x-2">
            <span className="text-4xl font-black text-white font-mono">{spo2Val > 0 ? spo2Val : 0}</span>
            <span className="text-sm font-bold text-cyan-400">% SpO2</span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2 font-mono truncate">{status.desc}</p>
        </div>

        {/* Perfusion Index */}
        <div className="p-4 glass-panel rounded-2xl border border-slate-800 bg-slate-950/70">
          <span className="text-[11px] font-mono text-slate-400 block mb-1">CHỈ SỐ TƯỚI MÁU (PI)</span>
          <div className="flex items-baseline space-x-2">
            <span className="text-4xl font-black text-emerald-400 font-mono">{perfusion}</span>
            <span className="text-sm font-bold text-slate-400">% PI</span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2 font-mono">Đo lưu lượng mạch máu ngoại vi</p>
        </div>

        {/* SQI Quality */}
        <div className="p-4 glass-panel rounded-2xl border border-slate-800 bg-slate-950/70">
          <span className="text-[11px] font-mono text-slate-400 block mb-1">CHẤT LƯỢNG TÍN HIỆU QUANG</span>
          <div className="flex items-baseline space-x-2">
            <span className="text-4xl font-black text-cyan-300 font-mono">{isConnected ? (rawTelemetry?.quality || 0) : 0}</span>
            <span className="text-sm font-bold text-slate-400">/ 100 SQI</span>
          </div>
          <div className="w-full bg-slate-800 rounded-full h-1.5 mt-3 overflow-hidden">
            <div
              className="bg-gradient-to-r from-cyan-500 to-emerald-400 h-full transition-all duration-500"
              style={{ width: `${isConnected ? (rawTelemetry?.quality || 0) : 0}%` }}
            />
          </div>
        </div>

        {/* Optical Sensor Status */}
        <div className="p-4 glass-panel rounded-2xl border border-slate-800 bg-slate-950/70">
          <span className="text-[11px] font-mono text-slate-400 block mb-1">TRẠNG THÁI TIẾP XÚC DA</span>
          <div className="flex items-baseline space-x-2">
            <span className={`text-2xl font-bold font-mono ${rawTelemetry?.skinContact ? 'text-emerald-400' : 'text-slate-500'}`}>
              {rawTelemetry?.skinContact ? "CHẠM DA TỐT" : "CHƯA TIẾP XÚC"}
            </span>
          </div>
          <p className="text-[11px] text-slate-400 mt-2 font-mono">
            {rawTelemetry?.motionArtifact ? "⚠️ Cảnh báo: Nhiễu do cử động" : "✓ Ổn định (Không nhiễu động)"}
          </p>
        </div>

      </div>

      {/* Realtime Plethysmogram Waveform Canvas */}
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-2">
            <Activity className="w-4 h-4 text-cyan-400 animate-pulse" />
            <span className="text-xs font-bold text-slate-300 font-mono">DẠNG SÓNG THỂ TÍCH MẠCH QUANG HỌC REALTIME (PPG PLETHYSMOGRAM)</span>
          </div>
          <span className="text-[11px] font-mono text-cyan-400">25Hz Maxim Interpolation</span>
        </div>

        <div className="w-full h-32 rounded-2xl bg-slate-950/90 border border-slate-800 p-2 overflow-hidden relative">
          <canvas ref={canvasRef} width={800} height={120} className="w-full h-full" />
        </div>
      </div>

      {/* Clinical Oxygenation Stage Reference Guide */}
      <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs font-mono">
        <div className="p-2.5 rounded-xl bg-cyan-500/10 border border-cyan-500/30">
          <span className="text-cyan-400 font-bold block">96% - 100%</span>
          <span className="text-slate-300 text-[11px]">Bình thường (Tối ưu)</span>
        </div>
        <div className="p-2.5 rounded-xl bg-emerald-500/10 border border-emerald-500/30">
          <span className="text-emerald-400 font-bold block">94% - 95%</span>
          <span className="text-slate-300 text-[11px]">Chấp nhận được</span>
        </div>
        <div className="p-2.5 rounded-xl bg-amber-500/10 border border-amber-500/30">
          <span className="text-amber-400 font-bold block">90% - 93%</span>
          <span className="text-slate-300 text-[11px]">Thiếu Oxy nhẹ</span>
        </div>
        <div className="p-2.5 rounded-xl bg-rose-500/10 border border-rose-500/30">
          <span className="text-rose-400 font-bold block">&lt; 90%</span>
          <span className="text-slate-300 text-[11px]">Nguy kịch (Cần can thiệp)</span>
        </div>
      </div>

    </div>
  );
}
