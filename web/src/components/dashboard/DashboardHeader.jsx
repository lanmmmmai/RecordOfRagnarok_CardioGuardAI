import React from 'react';
import { User, Activity, Wifi, Battery, AlertTriangle, Download, Settings, HeartPulse } from 'lucide-react';

export default function DashboardHeader({
  userProfile,
  simulatedBpm,
  isSimulating,
  setIsSimulating,
  onTriggerSOS,
  onExportReport
}) {
  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-6">
        
        {/* User Info Avatar & Details */}
        <div className="flex items-center space-x-4">
          <div className="relative">
            <img
              src={userProfile.avatar}
              alt={userProfile.name}
              className="w-16 h-16 rounded-2xl object-cover border-2 border-cyan-400/50 shadow-lg shadow-cyan-500/20"
            />
            <span className="absolute -bottom-1 -right-1 w-4 h-4 bg-emerald-500 border-2 border-slate-950 rounded-full animate-pulse" />
          </div>

          <div>
            <div className="flex items-center space-x-2">
              <h1 className="text-xl md:text-2xl font-black text-white">{userProfile.name}</h1>
              <span className="px-2.5 py-0.5 rounded-full bg-cyan-500/20 text-cyan-400 text-xs font-bold font-mono border border-cyan-500/30">
                ID: {userProfile.id}
              </span>
            </div>

            <div className="flex flex-wrap items-center gap-3 text-xs text-slate-400 mt-1 font-medium">
              <span>Tuổi: <strong className="text-slate-200">{userProfile.age}</strong></span>
              <span>•</span>
              <span>Nhóm máu: <strong className="text-rose-400 font-bold">{userProfile.bloodType}</strong></span>
              <span>•</span>
              <span>Bệnh lý: <strong className="text-amber-300">Thiếu máu tim nhẹ</strong></span>
              <span>•</span>
              <span className="text-emerald-400 font-mono">Đồng Hồ Enroll Online</span>
            </div>
          </div>
        </div>

        {/* Live Device Status & Actions */}
        <div className="flex flex-wrap items-center gap-3">
          
          {/* Watch Status Pill */}
          <div className="glass-panel px-4 py-2 rounded-2xl flex items-center space-x-3 text-xs border border-slate-700/80">
            <div className="flex items-center space-x-1 text-emerald-400">
              <Wifi className="w-4 h-4" />
              <span className="font-mono">{userProfile.wifiSignal} dBm</span>
            </div>

            <span className="text-slate-700">|</span>

            <div className="flex items-center space-x-1 text-cyan-400">
              <Battery className="w-4 h-4" />
              <span className="font-mono">{userProfile.batteryLevel}% Pin</span>
            </div>

            <span className="text-slate-700">|</span>

            <div className="flex items-center space-x-1 text-rose-400">
              <HeartPulse className="w-4 h-4 animate-bounce" />
              <span className="font-mono font-bold">{simulatedBpm} BPM</span>
            </div>
          </div>

          {/* Real-time Data Stream Simulation Switch */}
          <button
            onClick={() => setIsSimulating(!isSimulating)}
            className={`px-4 py-2.5 rounded-2xl text-xs font-bold transition-all flex items-center space-x-2 border ${
              isSimulating
                ? 'bg-amber-400/20 text-amber-300 border-amber-400/40 shadow-lg shadow-amber-400/10'
                : 'glass-panel text-slate-400 hover:text-white border-slate-700'
            }`}
          >
            <Activity className={`w-4 h-4 ${isSimulating ? 'animate-spin text-amber-400' : ''}`} />
            <span>{isSimulating ? 'Đang phát Giả lập Live...' : 'Bật Giả lập Live'}</span>
          </button>

          {/* Export Report CTA */}
          <button
            onClick={onExportReport}
            className="px-4 py-2.5 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-2xl shadow-lg shadow-cyan-500/20 flex items-center space-x-2 transition-all"
          >
            <Download className="w-4 h-4" />
            <span>Xuất Báo Cáo PDF/CSV</span>
          </button>

          {/* Trigger Emergency */}
          <button
            onClick={onTriggerSOS}
            className="px-4 py-2.5 bg-rose-600 hover:bg-rose-500 text-white font-bold text-xs rounded-2xl shadow-lg shadow-rose-600/30 flex items-center space-x-1.5 transition-all"
          >
            <AlertTriangle className="w-4 h-4 animate-pulse" />
            <span>Kích Hoạt SOS</span>
          </button>

        </div>

      </div>

    </div>
  );
}
