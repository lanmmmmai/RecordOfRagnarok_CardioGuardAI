import React from 'react';
import { Shield, Sparkles, LayoutDashboard, Wifi } from 'lucide-react';
import { websocketBridgeService } from '../services/websocketBridgeService';

export default function Navbar({ activeTab, setActiveTab }) {
  const [isConnected, setIsConnected] = React.useState(false);

  React.useEffect(() => {
    const unsub = websocketBridgeService.onStatusChange((info) => {
      setIsConnected(info.isConnected);
    });
    return unsub;
  }, []);

  return (
    <nav className="sticky top-0 z-50 w-full glass-panel border-b border-slate-800 px-4 py-3 transition-all">
      <div className="max-w-7xl mx-auto flex items-center justify-between">
        
        {/* Brand Logo & Title */}
        <div 
          onClick={() => setActiveTab('LANDING')}
          className="flex items-center space-x-3 cursor-pointer group"
        >
          <div className="w-10 h-10 rounded-2xl bg-gradient-to-tr from-cyan-500 via-rose-500 to-amber-400 p-[2px] shadow-lg shadow-cyan-500/20 group-hover:scale-105 transition-transform">
            <div className="w-full h-full bg-slate-950 rounded-[14px] flex items-center justify-center">
              <Shield className="w-5 h-5 text-cyan-400 group-hover:text-rose-400 transition-colors" />
            </div>
          </div>

          <div>
            <div className="flex items-center space-x-2">
              <span className="font-extrabold text-lg text-white tracking-wider font-mono">
                CARDIO<span className="text-cyan-400">GUARD</span><span className="text-rose-500">AI</span>
              </span>
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-rose-500/20 text-rose-400 font-bold border border-rose-500/30">
                INT8 TinyML
              </span>
            </div>
            <p className="text-[11px] text-slate-400 font-medium">Record of Ragnarok Bio-Shield System</p>
          </div>
        </div>

        {/* Center Navigation Tabs */}
        <div className="hidden md:flex items-center space-x-1 bg-slate-950/80 p-1 rounded-2xl border border-slate-800/80 shadow-inner">
          <button
            onClick={() => setActiveTab('LANDING')}
            className={`px-4 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'LANDING'
                ? 'bg-gradient-to-r from-cyan-500 to-blue-600 text-slate-950 shadow-lg shadow-cyan-500/30'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            <Sparkles className="w-4 h-4" />
            <span>Trang 3D Landing Page</span>
          </button>

          <button
            onClick={() => setActiveTab('DASHBOARD')}
            className={`px-4 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'DASHBOARD'
                ? 'bg-gradient-to-r from-rose-500 to-pink-600 text-white shadow-lg shadow-rose-500/30'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            <LayoutDashboard className="w-4 h-4" />
            <span>Dashboard Cá nhân hóa</span>
          </button>
        </div>

        {/* Right Status Pill */}
        <div className="flex items-center space-x-3">
          <div className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-xl glass-panel text-xs font-mono border ${
            isConnected ? 'text-emerald-400 border-emerald-500/40 bg-emerald-500/10' : 'text-amber-300 border-amber-500/40 bg-amber-500/10 animate-pulse'
          }`}>
            <Wifi className="w-3.5 h-3.5 text-current" />
            <span className="font-bold">
              {isConnected ? "ĐỒNG HỒ SAFEWATCH ONLINE" : "TỰ ĐỘNG DÒ TÌM ĐỒNG HỒ..."}
            </span>
          </div>
        </div>

      </div>
    </nav>
  );
}
