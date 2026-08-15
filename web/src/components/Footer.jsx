import React from 'react';
import { Shield, Heart, Cpu, Send, GitBranch } from 'lucide-react';

export default function Footer({ onExplore3D, onExploreDashboard }) {
  return (
    <footer className="w-full glass-panel border-t border-slate-800 mt-20 py-12 px-4 text-slate-400 text-xs">
      <div className="max-w-7xl mx-auto grid grid-cols-1 md:grid-cols-4 gap-8">
        
        {/* Col 1: Brand Info */}
        <div className="space-y-3">
          <div className="flex items-center space-x-2">
            <Shield className="w-5 h-5 text-cyan-400" />
            <span className="font-extrabold text-base text-white tracking-wider font-mono">
              CARDIO<span className="text-cyan-400">GUARD</span><span className="text-rose-500">AI</span>
            </span>
          </div>
          <p className="text-slate-400 leading-relaxed text-[11px]">
            Hệ thống Cảnh báo Té ngã, Loạn nhịp tim & Lọc Nhiễu Tín hiệu tại Biên (ESP32-S3 Watch + TinyML AI INT8 + Direct Telegram Alert).
          </p>
          <p className="text-[10px] text-slate-500 font-mono">
            Record of Ragnarok Styled Bio-Shield Platform • Version 2.4.1
          </p>
        </div>

        {/* Col 2: Navigation */}
        <div className="space-y-2">
          <h4 className="font-bold text-white uppercase text-[11px] font-mono tracking-wider">ĐIỀU HƯỚNG BẢN ĐỒ WEB</h4>
          <ul className="space-y-1.5 font-medium">
            <li>
              <button onClick={onExplore3D} className="hover:text-cyan-400 transition-colors">
                • Trang 3D Landing Page & Smartwatch 3D
              </button>
            </li>
            <li>
              <button onClick={onExploreDashboard} className="hover:text-rose-400 transition-colors">
                • Trang Dashboard Thống kê Cá nhân hóa
              </button>
            </li>
            <li>
              <a href="#features" className="hover:text-amber-300 transition-colors">
                • Lọc nhiễu DSP 5 Tầng & Thuật toán 4 Giai đoạn
              </a>
            </li>
            <li>
              <a href="#specs" className="hover:text-emerald-400 transition-colors">
                • Cấu hình Phần cứng ESP32-S3 Watch
              </a>
            </li>
          </ul>
        </div>

        {/* Col 3: Tech Stack */}
        <div className="space-y-2">
          <h4 className="font-bold text-white uppercase text-[11px] font-mono tracking-wider">CÔNG NGHỆ TÍCH HỢP</h4>
          <ul className="space-y-1.5 font-mono text-[11px] text-slate-400">
            <li>• ESP32-S3 Dual Core 240MHz</li>
            <li>• TensorFlow Lite Micro INT8 Quantized</li>
            <li>• MAX30102 PPG + QMI8658 3D IMU</li>
            <li>• Three.js WebGL 3D Canvas</li>
            <li>• React + Vite + Tailwind CSS</li>
          </ul>
        </div>

        {/* Col 4: Project Info */}
        <div className="space-y-3">
          <h4 className="font-bold text-white uppercase text-[11px] font-mono tracking-wider">PROJECT REPOSITORY</h4>
          <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 space-y-1 text-[11px] font-mono">
            <p className="text-cyan-400 font-bold flex items-center space-x-1">
              <GitBranch className="w-3.5 h-3.5" />
              <span>lanmmmmai/RecordOfRagnarok_CardioGuardAI</span>
            </p>
            <p className="text-slate-400">Firmware: PlatformIO C++</p>
            <p className="text-slate-400">AI Models: Python Trainer INT8</p>
          </div>
        </div>

      </div>

      <div className="max-w-7xl mx-auto border-t border-slate-800/80 mt-8 pt-6 flex flex-col sm:flex-row items-center justify-between text-[11px] text-slate-500">
        <p>© 2026 CardioGuardAI System. Được phát triển với công nghệ Edge AI & 3D WebGL.</p>
        <div className="flex items-center space-x-2 mt-2 sm:mt-0 text-slate-400 font-mono">
          <span>CardioGuardAI 🛡️❤️ — Protect Every Heartbeat</span>
        </div>
      </div>
    </footer>
  );
}
