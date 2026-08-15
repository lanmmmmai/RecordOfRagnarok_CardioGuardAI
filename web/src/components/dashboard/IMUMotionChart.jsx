import React from 'react';
import { Compass, Activity } from 'lucide-react';

export default function IMUMotionChart({ isConnected, rawTelemetry }) {
  // Convert raw 16-bit IMU counts to physical units (QMI8658 ±8g range => ~4096 LSB/g)
  // If not connected or data missing => strictly 0.00g (No mock numbers)
  const accX_g = isConnected && rawTelemetry?.accel ? (rawTelemetry.accel.x / 4096.0).toFixed(2) : "0.00";
  const accY_g = isConnected && rawTelemetry?.accel ? (rawTelemetry.accel.y / 4096.0).toFixed(2) : "0.00";
  const accZ_g = isConnected && rawTelemetry?.accel ? (rawTelemetry.accel.z / 4096.0).toFixed(2) : "0.00";
  
  const mag_g = isConnected && rawTelemetry?.accel
    ? (Math.sqrt(rawTelemetry.accel.x ** 2 + rawTelemetry.accel.y ** 2 + rawTelemetry.accel.z ** 2) / 4096.0).toFixed(2)
    : "0.00";

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <Compass className="w-5 h-5 text-amber-400" />
            <h3 className="text-lg font-bold text-white">VECTƠ GIA TỐC 3D (QMI8658 SENSOR)</h3>
            <span className="px-2 py-0.5 rounded-full bg-amber-400/20 text-amber-300 font-mono text-xs font-bold border border-amber-400/30">
              {isConnected ? "10Hz Live Wi-Fi Stream" : "Chưa kết nối (0Hz)"}
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
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
          <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
            <span>GIA TỐC X (ACCEL X)</span>
            <Activity className="w-4 h-4 text-cyan-400" />
          </div>
          <p className="text-2xl font-black text-white font-mono">{accX_g}g</p>
          <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {isConnected ? (rawTelemetry?.accel?.x ?? 0) : 0}</p>
        </div>

        <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
          <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
            <span>GIA TỐC Y (ACCEL Y)</span>
            <Activity className="w-4 h-4 text-emerald-400" />
          </div>
          <p className="text-2xl font-black text-white font-mono">{accY_g}g</p>
          <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {isConnected ? (rawTelemetry?.accel?.y ?? 0) : 0}</p>
        </div>

        <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800/80 space-y-2">
          <div className="flex items-center justify-between text-xs text-slate-400 font-mono">
            <span>GIA TỐC Z (TRỌNG LỰC Z)</span>
            <Activity className="w-4 h-4 text-amber-400" />
          </div>
          <p className="text-2xl font-black text-white font-mono">{accZ_g}g</p>
          <p className="text-[11px] text-slate-400 font-mono">Raw LSB: {isConnected ? (rawTelemetry?.accel?.z ?? 0) : 0}</p>
        </div>
      </div>

      {/* Sensor Status Card */}
      <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800 flex flex-col md:flex-row items-center justify-between gap-4 font-mono text-xs">
        <div className="space-y-1">
          <span className="text-slate-500 uppercase">Trạng thái cảm biến IMU:</span>
          <p className={isConnected ? "text-emerald-400 font-bold text-sm" : "text-slate-500 font-bold text-sm"}>
            {isConnected ? "QMI8658 6-Axis IMU Active (Wi-Fi Telemetry Stream)" : "Chưa kết nối đồng hồ"}
          </p>
          <p className="text-slate-300">
            {isConnected ? `Góc quay Gyro: X=${rawTelemetry?.gyro?.x ?? 0}, Y=${rawTelemetry?.gyro?.y ?? 0}, Z=${rawTelemetry?.gyro?.z ?? 0} dps` : "Gyro: X=0, Y=0, Z=0 dps"}
          </p>
        </div>

        <div className="glass-panel p-3 rounded-xl border border-amber-400/30 text-right space-y-1 min-w-[200px]">
          <p className="text-[10px] text-amber-300 font-bold">TRẠNG THÁI TÉ NGÃ</p>
          <p className="text-xl font-black text-white">
            {isConnected ? (rawTelemetry?.fallState === 0 ? "NORMAL (0)" : "ALERT (1)") : "0"}
          </p>
        </div>
      </div>

    </div>
  );
}
