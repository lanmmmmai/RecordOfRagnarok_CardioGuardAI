import React from 'react';
import { Heart, Activity, ShieldCheck, TrendingUp } from 'lucide-react';
import ScrollReveal from '../ScrollReveal';

export default function VitalCard({ vitalSummary, simulatedBpm }) {
  const cards = [
    {
      title: "NHỊP TIM REALTIME",
      value: `${simulatedBpm}`,
      unit: "BPM",
      status: simulatedBpm > 100 ? "NHỊP NHANH" : simulatedBpm < 55 ? "NHỊP CHẬM" : "BÌNH THƯỜNG",
      statusColor: simulatedBpm > 100 ? "text-rose-400 border-rose-500/30 bg-rose-500/10" : "text-emerald-400 border-emerald-500/30 bg-emerald-500/10",
      subInfo: `Min: ${vitalSummary.minBpm24h} | Max: ${vitalSummary.maxBpm24h}`,
      icon: Heart,
      accentColor: "border-l-rose-500 text-rose-500"
    },
    {
      title: "OXY MÁU (SPO2)",
      value: `${vitalSummary.spO2}%`,
      unit: "SpO2",
      status: "AN TOÀN",
      statusColor: "text-cyan-400 border-cyan-500/30 bg-cyan-500/10",
      subInfo: "Lọc nhiễu DSP 5 Tầng",
      icon: Activity,
      accentColor: "border-l-cyan-400 text-cyan-400"
    },
    {
      title: "BIẾN THIÊN NHỊP TIM (HRV)",
      value: `${vitalSummary.hrv}`,
      unit: "ms",
      status: "TỐT",
      statusColor: "text-emerald-400 border-emerald-500/30 bg-emerald-500/10",
      subInfo: "Khoảng R-R Đạt chuẩn",
      icon: TrendingUp,
      accentColor: "border-l-emerald-400 text-emerald-400"
    },
    {
      title: "TINYML AI FALL RISK",
      value: `${vitalSummary.fallRiskScore}/100`,
      unit: "Risk Score",
      status: "NGUY CƠ THẤP",
      statusColor: "text-amber-300 border-amber-400/30 bg-amber-400/10",
      subInfo: "Suy luận INT8: 3.1ms",
      icon: ShieldCheck,
      accentColor: "border-l-amber-400 text-amber-400"
    }
  ];

  return (
    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
      {cards.map((c, i) => {
        const Icon = c.icon;
        return (
          <ScrollReveal key={i} delay={i * 80}>
            <div
              className={`glass-panel p-5 rounded-3xl border border-slate-800 ${c.accentColor} border-l-4 shadow-lg hover:scale-[1.02] transition-transform bg-slate-900/70`}
            >
              <div className="flex items-center justify-between mb-2">
                <span className="text-[11px] font-mono font-bold text-slate-400 tracking-wider">
                  {c.title}
                </span>
                <Icon className="w-5 h-5 opacity-80" />
              </div>

              <div className="flex items-baseline space-x-2 my-1">
                <span className="text-3xl font-black text-white font-mono">{c.value}</span>
                <span className="text-xs text-slate-400 font-bold">{c.unit}</span>
              </div>

              <div className="mt-3 flex items-center justify-between">
                <span className={`inline-block px-2.5 py-0.5 rounded-full text-[10px] font-bold border ${c.statusColor}`}>
                  ● {c.status}
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
