import React from 'react';
import { Heart, Activity, ShieldCheck, TrendingUp } from 'lucide-react';
import ScrollReveal from '../ScrollReveal';

export default function VitalCard({ vitalSummary, simulatedBpm, isConnected, rawTelemetry }) {
  // Strict Real Data Mode: If connected to Watch via WebSocket
  const pulseVal = isConnected
    ? (rawTelemetry?.skinContact && rawTelemetry?.pulse > 0 ? rawTelemetry.pulse : "--")
    : (simulatedBpm || "--");

  const pulseStatus = isConnected
    ? (!rawTelemetry?.skinContact || rawTelemetry?.pulse === 0
        ? "CHƯA ĐEO DA"
        : rawTelemetry.pulse > 100 ? "NHỊP NHANH" : rawTelemetry.pulse < 55 ? "NHỊP CHẬM" : "BÌNH THƯỜNG (THỰC)")
    : (simulatedBpm > 100 ? "NHỊP NHANH" : simulatedBpm < 55 ? "NHỊP CHẬM" : "BÌNH THƯỜNG");

  const spo2Val = isConnected
    ? (rawTelemetry?.spo2Valid && rawTelemetry?.spo2 > 0 ? `${rawTelemetry.spo2}%` : "--")
    : `${vitalSummary.spO2}%`;

  const spo2Status = isConnected
    ? (!rawTelemetry?.spo2Valid || rawTelemetry?.spo2 === 0 ? "CHỜ TÍCH LŨY" : "AN TOÀN (THỰC)")
    : "AN TOÀN";

  const cards = [
    {
      title: "NHỊP TIM REALTIME (MAX30102)",
      value: `${pulseVal}`,
      unit: "BPM",
      status: pulseStatus,
      statusColor: pulseVal === "--" ? "text-slate-400 border-slate-700 bg-slate-800/40" : (rawTelemetry?.pulse > 100 || simulatedBpm > 100 ? "text-rose-400 border-rose-500/40 bg-rose-500/15" : "text-emerald-400 border-emerald-500/40 bg-emerald-500/15"),
      subInfo: isConnected ? `Chất lượng SQI: ${rawTelemetry?.quality || 0}%` : `Min: ${vitalSummary.minBpm24h} | Max: ${vitalSummary.maxBpm24h}`,
      icon: Heart,
      badgeBorder: "border-rose-500/30 hover:border-rose-400/60 shadow-rose-500/5",
      iconColor: "text-rose-400 bg-rose-500/10"
    },
    {
      title: "OXY MÁU (SPO2 REALTIME)",
      value: spo2Val,
      unit: "SpO2",
      status: spo2Status,
      statusColor: spo2Val === "--" ? "text-slate-400 border-slate-700 bg-slate-800/40" : "text-cyan-400 border-cyan-500/40 bg-cyan-500/15",
      subInfo: isConnected ? (rawTelemetry?.motionArtifact ? "Cảnh báo: Tay đang rung nhẹ" : "Lọc nhiễu DSP 5 tầng THỰC") : "Lọc nhiễu DSP 5 Tầng",
      icon: Activity,
      badgeBorder: "border-cyan-500/30 hover:border-cyan-400/60 shadow-cyan-500/5",
      iconColor: "text-cyan-400 bg-cyan-500/10"
    },
    {
      title: "CẢM BIẾN & ĐIỆN ÁP PIN",
      value: isConnected ? `${rawTelemetry?.battery || 100}%` : `${vitalSummary.hrv}`,
      unit: isConnected ? `${rawTelemetry?.voltage ? rawTelemetry.voltage.toFixed(2) + 'V' : 'Pin'}` : "ms",
      status: isConnected ? (rawTelemetry?.charging ? "ĐANG SẠC PIN" : "PIN KHỎE") : "TỐT",
      statusColor: "text-emerald-400 border-emerald-500/40 bg-emerald-500/15",
      subInfo: isConnected ? `Hardware IMU/PPG: ${rawTelemetry?.sensors?.imuOk ? 'OK' : 'ERR'}` : "Khoảng R-R Đạt chuẩn",
      icon: TrendingUp,
      badgeBorder: "border-emerald-500/30 hover:border-emerald-400/60 shadow-emerald-500/5",
      iconColor: "text-emerald-400 bg-emerald-500/10"
    },
    {
      title: "TINYML AI FALL RISK (QMI8658)",
      value: isConnected ? (rawTelemetry?.fallState === 0 ? "0/100" : "85/100") : `${vitalSummary.fallRiskScore}/100`,
      unit: "Risk Score",
      status: isConnected ? (rawTelemetry?.fallState === 0 ? "AN TOÀN (BÌNH THƯỜNG)" : "CẢNH BÁO TÉ NGÃ!") : "NGUY CƠ THẤP",
      statusColor: isConnected && rawTelemetry?.fallState > 0 ? "text-rose-400 border-rose-500/50 bg-rose-500/20" : "text-amber-300 border-amber-400/40 bg-amber-400/15",
      subInfo: isConnected ? `FPS Stream: 10Hz | RSSI: ${rawTelemetry?.rssi || 0}dBm` : "Suy luận INT8: 3.1ms",
      icon: ShieldCheck,
      badgeBorder: "border-amber-400/30 hover:border-amber-300/60 shadow-amber-400/5",
      iconColor: "text-amber-300 bg-amber-400/10"
    }
  ];

  return (
    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
      {cards.map((c, i) => {
        const Icon = c.icon;
        return (
          <ScrollReveal key={i} delay={i * 80}>
            <div
              tabIndex={0}
              aria-label={`${c.title}: ${c.value} ${c.unit}, trạng thái ${c.status}`}
              className={`glass-panel p-5 rounded-3xl border ${c.badgeBorder} shadow-xl hover:scale-[1.02] transition-all duration-300 bg-slate-900/80 backdrop-blur-xl group cursor-default relative overflow-hidden`}
            >
              <div className="flex items-center justify-between mb-3">
                <span className="text-[11px] font-mono font-bold text-slate-400 tracking-wider">
                  {c.title}
                </span>
                <div className={`w-8 h-8 rounded-xl ${c.iconColor} flex items-center justify-center transition-transform group-hover:scale-110`}>
                  <Icon className="w-4 h-4" />
                </div>
              </div>

              <div className="flex items-baseline space-x-2 my-1">
                <span className="text-3xl font-black text-white font-mono tracking-tight">{c.value}</span>
                <span className="text-xs text-slate-400 font-bold">{c.unit}</span>
              </div>

              <div className="mt-4 pt-3 border-t border-slate-800/80 flex items-center justify-between">
                <span className={`inline-flex items-center space-x-1 px-2.5 py-0.5 rounded-full text-[10px] font-bold border ${c.statusColor}`}>
                  <span className="w-1.5 h-1.5 rounded-full bg-current animate-pulse" />
                  <span>{c.status}</span>
                </span>
                <span className="text-[11px] text-slate-400 font-mono truncate">{c.subInfo}</span>
              </div>
            </div>
          </ScrollReveal>
        );
      })}
    </div>
  );
}
