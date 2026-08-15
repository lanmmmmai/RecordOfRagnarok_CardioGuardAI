import React from 'react';
import Watch3DCanvas from '../3d/Watch3DCanvas';
import { Activity, BellRing, ArrowRight, Zap, ChevronRight } from 'lucide-react';

export default function HeroSection({ onExploreDashboard, onTriggerSOS, simulatedBpm }) {
  return (
    <section className="relative pt-6 md:pt-10 pb-12 px-4 max-w-7xl mx-auto overflow-hidden">
      
      {/* Background Radial Glow */}
      <div className="absolute top-1/2 right-0 -translate-y-1/2 w-[450px] h-[450px] bg-cyan-500/5 blur-[140px] rounded-full pointer-events-none" />

      {/* Top Badge Tag */}
      <div className="flex justify-center md:justify-start mb-6">
        <div className="inline-flex items-center space-x-2 px-3.5 py-1.5 rounded-full bg-slate-900/90 border border-cyan-500/30 text-xs font-semibold text-cyan-300 shadow-sm">
          <Zap className="w-3.5 h-3.5 text-amber-400 animate-pulse" />
          <span>TinyML AI INT8 • Cảm Biến Sinh Học &amp; Bảo Vệ Cấp Cứu</span>
          <ChevronRight className="w-3.5 h-3.5 text-slate-500 hidden sm:inline" />
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-center">
        
        {/* Left Column: Headline, Subtitle, Stats & Actions */}
        <div className="lg:col-span-7 space-y-5 text-center lg:text-left">
          
          {/* NỘI DUNG THẺ H1 CHÍNH THỨC */}
          <h1 className="text-4xl sm:text-5xl md:text-6xl lg:text-7xl font-extrabold tracking-tight leading-[1.1]">
            <span className="block gradient-text-cyan font-black filter drop-shadow-[0_0_25px_rgba(0,242,254,0.4)]">
              Đồng Hồ Enroll
            </span>
          </h1>

          {/* DÒNG PHỤ TIÊU ĐỀ */}
          <p className="text-lg sm:text-xl font-extrabold text-rose-400 tracking-wide">
            CẢNH BÁO TÉ NGÃ &amp; LOẠN NHỊP TIM BẰNG TINYML AI INT8
          </p>

          {/* PHẦN MÔ TẢ */}
          <p className="text-slate-300 text-sm sm:text-base leading-[1.6] font-normal max-w-xl mx-auto lg:mx-0">
            CardioGuardAI giải pháp bảo vệ sức khỏe sinh học tại biên, tích hợp trực tiếp 
            trên <strong>Đồng Hồ Enroll</strong>. Lọc nhiễu 5 tầng, tự động phát hiện té ngã 3D &amp; gửi thông báo SOS Telegram chỉ sau 15 giây.
          </p>

          {/* CÁC THẺ THÔNG SỐ (3 cards) */}
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 pt-1 max-w-xl mx-auto lg:mx-0">
            
            <div className="glass-panel p-4 rounded-2xl text-center border-l-4 border-l-cyan-400 bg-slate-900/60 hover:bg-slate-900/90 transition-all flex flex-col justify-center">
              <p className="text-3xl font-black text-white font-mono tracking-tight">99.4%</p>
              <p className="text-xs text-slate-400 font-medium mt-1">Độ chính xác AI</p>
            </div>

            <div className="glass-panel p-4 rounded-2xl text-center border-l-4 border-l-rose-500 bg-slate-900/60 hover:bg-slate-900/90 transition-all flex flex-col justify-center">
              <p className="text-3xl font-black text-white font-mono tracking-tight">~8 KB</p>
              <p className="text-xs text-slate-400 font-medium mt-1">Model INT8 AI</p>
            </div>

            <div className="glass-panel p-4 rounded-2xl text-center border-l-4 border-l-amber-400 bg-slate-900/60 hover:bg-slate-900/90 transition-all flex flex-col justify-center">
              <p className="text-3xl font-black text-white font-mono tracking-tight">15 giây</p>
              <p className="text-xs text-slate-400 font-medium mt-1">Đếm ngược HỦY SOS</p>
            </div>

          </div>

          {/* BUTTONS (CTA Primary & Secondary) */}
          <div className="flex flex-col sm:flex-row items-center justify-center lg:justify-start gap-3.5 pt-2">
            <button
              onClick={onExploreDashboard}
              className="w-full sm:w-auto px-7 py-4 bg-gradient-to-r from-cyan-500 via-blue-600 to-indigo-600 hover:from-cyan-400 hover:to-indigo-500 text-slate-950 font-black text-sm rounded-2xl shadow-xl shadow-cyan-500/20 flex items-center justify-center space-x-2.5 transition-all transform hover:-translate-y-0.5 active:translate-y-0"
            >
              <Activity className="w-5 h-5" />
              <span>VÀO DASHBOARD CÁ NHÂN HÓA</span>
              <ArrowRight className="w-4 h-4" />
            </button>

            <button
              onClick={onTriggerSOS}
              className="w-full sm:w-auto px-6 py-4 rounded-2xl font-bold text-sm border border-rose-500/60 bg-rose-500/10 hover:bg-rose-500/20 text-rose-300 hover:text-white flex items-center justify-center space-x-2 transition-all transform hover:-translate-y-0.5 active:translate-y-0"
            >
              <BellRing className="w-4.5 h-4.5 text-rose-400 animate-pulse" />
              <span>Mô phỏng Sự cố Té ngã</span>
            </button>
          </div>

        </div>

        {/* Right Column: Đồng Hồ Enroll 3D Canvas */}
        <div className="lg:col-span-5 relative flex flex-col items-center mt-2 lg:mt-0">
          <Watch3DCanvas onTriggerSOS={onTriggerSOS} simulatedBpm={simulatedBpm} />
        </div>

      </div>

    </section>
  );
}
