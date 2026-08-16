import React from 'react';
import { Heart, ShieldCheck, TrendingUp, Wind } from 'lucide-react';
import ScrollReveal from '../ScrollReveal';

export default function VitalCard({ isConnected, rawTelemetry }) {
  const hasSkinContact = Boolean(isConnected && rawTelemetry?.skinContact);

  // Pulse strictly 0 when not touching skin
  const pulseVal = hasSkinContact && rawTelemetry?.pulse > 0 ? rawTelemetry.pulse : 0;

  const pulseStatus = isConnected
    ? (!hasSkinContact || pulseVal === 0
        ? "CHƯA ĐEO DA (0 BPM)"
        : pulseVal > 100 ? "NHỊP NHANH" : pulseVal < 55 ? "NHỊP CHẬM" : "BÌNH THƯỜNG")
    : "CHƯA KẾT NỐI (0 BPM)";

  // SpO2 strictly 0% when not touching skin
  const rawSpo2 = hasSkinContact && rawTelemetry?.spo2Valid && rawTelemetry?.spo2 > 0 ? rawTelemetry.spo2 : 0;
  const spo2Val = `${rawSpo2}%`;

  const spo2Status = isConnected
    ? (!hasSkinContact || rawSpo2 === 0 ? "CHƯA CÓ TÍN HIỆU" : (rawSpo2 >= 96 ? "TỐI ƯU (96-100%)" : rawSpo2 >= 94 ? "CHẤP NHẬN (94-95%)" : "THIẾU OXY (<94%)"))
    : "CHƯA KẾT NỐI";

  // Perfusion Index & SQI strictly 0 when not touching skin
  const perfusionVal = hasSkinContact && rawTelemetry?.quality
    ? ((rawTelemetry.quality / 50.0) * 1.2).toFixed(2)
    : "0.00";
  const sqiVal = hasSkinContact ? (rawTelemetry?.quality || 0) : 0;

  // Battery and Voltage ALWAYS retain real measured hardware status
  const batteryVal = isConnected ? `${rawTelemetry?.battery || 0}%` : "0%";
  const voltageVal = isConnected && rawTelemetry?.voltage ? `${rawTelemetry.voltage.toFixed(2)}V` : "0.00V";

  const cards = [
    {
      title: "NHỊP TIM REALTIME (MAX30102)",
      value: `${pulseVal}`,
      unit: "BPM",
      status: pulseStatus,
      statusColor: pulseVal === 0 ? "text-slate-400 border-slate-700 bg-slate-800/40" : (pulseVal > 100 ? "text-rose-400 border-rose-500/40 bg-rose-500/15" : "text-emerald-400 border-emerald-500/40 bg-emerald-500/15"),
      subInfo: `Chất lượng SQI: ${sqiVal}%`,
      icon: Heart,
      badgeBorder: "border-rose-500/30 hover:border-rose-400/60 shadow-rose-500/5",
      iconColor: "text-rose-400 bg-rose-500/10"
    },
    {
      title: "NỒNG ĐỘ OXY MÁU (SPO2 REALTIME)",
      value: spo2Val,
      unit: "SpO2",
      status: spo2Status,
      statusColor: rawSpo2 === 0 ? "text-slate-400 border-slate-700 bg-slate-800/40" : (rawSpo2 >= 96 ? "text-cyan-400 border-cyan-500/40 bg-cyan-500/15" : "text-amber-400 border-amber-500/40 bg-amber-500/15"),
      subInfo: `Tưới máu PI: ${perfusionVal}%`,
      icon: Wind,
      badgeBorder: "border-cyan-500/30 hover:border-cyan-400/60 shadow-cyan-500/5",
      iconColor: "text-cyan-400 bg-cyan-500/10"
    },
    {
      title: "CẢM BIẾN & ĐIỆN ÁP PIN",
      value: batteryVal,
      unit: voltageVal,
      status: isConnected ? (rawTelemetry?.charging ? "ĐANG SẠC PIN" : "PIN HOẠT ĐỘNG") : "CHƯA KẾT NỐI",
      statusColor: isConnected ? "text-emerald-400 border-emerald-500/40 bg-emerald-500/15" : "text-slate-400 border-slate-700 bg-slate-800/40",
      subInfo: `Hardware: ${isConnected && rawTelemetry?.sensors?.imuOk ? 'IMU/PPG OK' : '0'}`,
      icon: TrendingUp,
      badgeBorder: "border-emerald-500/30 hover:border-emerald-400/60 shadow-emerald-500/5",
      iconColor: "text-emerald-400 bg-emerald-500/10"
    },
    {
      title: "PHÁT HIỆN TÉ NGÃ & VA ĐẬP (IMU 6-TRỤC)",
      value: !isConnected ? "CHƯA KẾT NỐI" : (rawTelemetry?.fallState >= 2 ? "🚨 TÉ NGÃ!" : "AN TOÀN"),
      unit: isConnected && rawTelemetry?.fallState > 1 ? `SOS ${rawTelemetry?.countdown || 0}s` : "GIÁM SÁT 235Hz",
      status: !isConnected ? "OFFLINE" : (rawTelemetry?.fallState >= 2 ? "KÍCH HOẠT CẤP CỨU" : "BÌNH THƯỜNG (1g)"),
      statusColor: !isConnected ? "text-slate-400 border-slate-700 bg-slate-800/40" : (rawTelemetry?.fallState >= 2 ? "text-rose-400 border-rose-500/50 bg-rose-500/25 animate-pulse" : "text-emerald-400 border-emerald-500/40 bg-emerald-500/15"),
      subInfo: isConnected ? `Gia tốc: ${rawTelemetry?.accel ? (rawTelemetry.accel.z / 4096.0).toFixed(2) : '1.00'}g | ${rawTelemetry?.rssi || -50} dBm` : "Chưa có tín hiệu",
      icon: ShieldCheck,
      badgeBorder: rawTelemetry?.fallState >= 2 ? "border-rose-500/50 hover:border-rose-400/80 shadow-rose-500/10" : "border-emerald-500/30 hover:border-emerald-400/60 shadow-emerald-500/5",
      iconColor: rawTelemetry?.fallState >= 2 ? "text-rose-400 bg-rose-500/15" : "text-emerald-400 bg-emerald-500/10"
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
