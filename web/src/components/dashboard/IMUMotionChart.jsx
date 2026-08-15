import React, { useState } from 'react';
import { Compass, CheckCircle, Activity, Gauge } from 'lucide-react';
import { mockIMUData } from '../../data/mockPersonalData';

export default function IMUMotionChart({ isConnected, rawTelemetry }) {
  const [selectedPhase, setSelectedPhase] = useState(0);

  // Convert raw 16-bit IMU counts to physical units (QMI8658 ±8g range => ~4096 LSB/g)
  const accX_g = rawTelemetry?.accel ? (rawTelemetry.accel.x / 4096.0).toFixed(2) : "0.00";
  const accY_g = rawTelemetry?.accel ? (rawTelemetry.accel.y / 4096.0).toFixed(2) : "0.00";
  const accZ_g = rawTelemetry?.accel ? (rawTelemetry.accel.z / 4096.0).toFixed(2) : "-1.00";
  
  const mag_g = rawTelemetry?.accel
    ? (Math.sqrt(rawTelemetry.accel.x ** 2 + rawTelemetry.accel.y ** 2 + rawTelemetry.accel.z ** 2) / 4096.0).toFixed(2)
    : "1.00";

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Compass className="w-5 h-5 text-amber-400" />
            <h3 className="text-lg font-bold text-white">VECTƠ GIA TỐC 3D (QMI8658 SENSOR)</h3>
            <span className="px-2 py-0.5 rounded-full bg-amber-400/20 text-amber-300 font-mono text-xs font-bold border border-amber-400/30">
              {isConnected ? "10Hz Live Wi-Fi Stream" : "100Hz IMU Sampling"}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Dữ liệu gia tốc 3 chiều (X, Y, Z) và con quay hồi chuyển từ đồng hồ SafeWatch
          </p>
        </div>

        <div className="text-xs font-mono text-cyan-400 font-bold px-3 py-1 bg-slate-900 rounded-xl border border-slate-800">
          ● MAGNITUDE |a| = {mag_g}g
        </div>
      </div>

      {/* Real-time Hardware Telemetry Display Panel */}
      {isConnected ? (
        <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
          <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
              <span>GIA TỐC X (ACCEL X)</span>
              <Activity className="w-4 h-4 text-cyan-400" />
            </div>
            <p className="text-2xl font-black text-white font-mono">{accX_g}g</p>
            <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {rawTelemetry?.accel?.x ?? 0}</p>
          </div>

          <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
              <span>GIA TỐC Y (ACCEL Y)</span>
              <Activity className="w-4 h-4 text-emerald-400" />
            </div>
            <p className="text-2xl font-black text-white font-mono">{accY_g}g</p>
            <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {rawTelemetry?.accel?.y ?? 0}</p>
          </div>

          <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
            <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
              <span>GIA TỐC Z (TRỌNG LỰC Z)</span>
              <Activity className="w-4 h-4 text-amber-400" />
            </div>
            <p className="text-2xl font-black text-white font-mono">{accZ_g}g</p>
            <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {rawTelemetry?.accel?.z ?? 0}</p>
          </div>
        </div>
      ) : (
        /* 4-Phase Fall Detection Pattern Cards (Offline Reference Mode) */
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
      )}

      {/* Phase Details Card */}
      <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800 flex flex-col md:flex-row items-center justify-between gap-4 font-mono text-xs">
        <div className="space-y-1">
          <span className="text-slate-500 uppercase">Trạng thái cảm biến:</span>
          <p className="text-emerald-400 font-bold text-sm">
            {isConnected ? "QMI8658 6-Axis IMU Active (Wi-Fi Telemetry Stream)" : mockIMUData.fallPattern[selectedPhase].phase}
          </p>
          <p className="text-slate-300">
            {isConnected ? `Góc quay Gyro: X=${rawTelemetry?.gyro?.x ?? 0}, Y=${rawTelemetry?.gyro?.y ?? 0}, Z=${rawTelemetry?.gyro?.z ?? 0} dps` : mockIMUData.fallPattern[selectedPhase].desc}
          </p>
        </div>

        <div className="glass-panel p-3 rounded-xl border border-amber-400/30 text-right space-y-1 min-w-[200px]">
          <p className="text-[10px] text-amber-300 font-bold">THUẬT TOÁN TÉ NGÃ INT8</p>
          <p className="text-xl font-black text-white">
            {isConnected ? (rawTelemetry?.fallState === 0 ? "NORMAL (AN TOÀN)" : "ALERT (CẢNH BÁO)") : "99.4% CONFIDENCE"}
          </p>
        </div>
      </div>

    </div>
  );
}
