import React, { useState } from 'react';
import { ShieldAlert, Compass, Layers, CheckCircle } from 'lucide-react';
import { mockIMUData } from '../../data/mockPersonalData';

export default function IMUMotionChart() {
  const [selectedPhase, setSelectedPhase] = useState(0);

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Compass className="w-5 h-5 text-amber-400" />
            <h3 className="text-lg font-bold text-white">VECTƠ GIA TỐC 3D (QMI8658 IMU SENSOR)</h3>
            <span className="px-2 py-0.5 rounded-full bg-amber-400/20 text-amber-300 font-mono text-xs font-bold border border-amber-400/30">
              100Hz IMU Sampling
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Dữ liệu gia tốc 3 chiều (X, Y, Z) phục vụ thuật toán đánh giá nguy cơ té ngã 4 giai đoạn
          </p>
        </div>

        <div className="text-xs font-mono text-cyan-400 font-bold px-3 py-1 bg-slate-900 rounded-xl border border-slate-800">
          ● MAGNITUDE |a| = √(X² + Y² + Z²)
        </div>
      </div>

      {/* 4-Phase Interactive Timeline Cards */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-3">
        {mockIMUData.fallPattern.map((p, idx) => (
          <button
            key={idx}
            onClick={() => setSelectedPhase(idx)}
            className={`p-4 rounded-2xl text-left transition-all border ${
              selectedPhase === idx
                ? 'bg-rose-500/20 border-rose-500 text-white shadow-lg shadow-rose-500/10'
                : 'glass-panel border-slate-800 text-slate-400 hover:text-white'
            }`}
          >
            <div className="flex items-center justify-between mb-2">
              <span className="text-[10px] font-mono font-bold px-2 py-0.5 rounded bg-slate-900 text-rose-400 border border-rose-500/30">
                {p.time}
              </span>
              {selectedPhase === idx && <CheckCircle className="w-4 h-4 text-rose-400" />}
            </div>

            <p className="text-xs font-bold text-white mb-1">{p.phase}</p>
            <p className="text-[11px] font-mono font-extrabold text-cyan-300 mb-2">
              Gia tốc: {p.accMag}g
            </p>
            <p className="text-[10px] text-slate-400 line-clamp-2 leading-relaxed">
              {p.desc}
            </p>
          </button>
        ))}
      </div>

      {/* Phase Details Card */}
      <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800 flex flex-col md:flex-row items-center justify-between gap-4 font-mono text-xs">
        <div className="space-y-1">
          <span className="text-slate-500 uppercase">Giai đoạn đang chọn:</span>
          <p className="text-rose-400 font-bold text-sm">
            {mockIMUData.fallPattern[selectedPhase].phase}
          </p>
          <p className="text-slate-300">
            {mockIMUData.fallPattern[selectedPhase].desc}
          </p>
        </div>

        <div className="glass-panel p-3 rounded-xl border border-amber-400/30 text-right space-y-1 min-w-[200px]">
          <p className="text-[10px] text-amber-300 font-bold">XÁC SUẤT AI INT8 MODEL</p>
          <p className="text-xl font-black text-white">99.4% CONFIDENCE</p>
        </div>
      </div>

    </div>
  );
}
