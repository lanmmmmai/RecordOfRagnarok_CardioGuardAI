import React, { useState, useEffect } from 'react';
import { Wifi, CheckCircle2, ShieldAlert, Sparkles, Shield, Radio } from 'lucide-react';
import { websocketBridgeService } from '../../services/websocketBridgeService';

export default function RealtimeDeviceBridge({
  connectionState,
  setConnectionState
}) {
  const [watchIP, setWatchIP] = useState(() => {
    return localStorage.getItem('safewatch_ip') || '192.168.20.152';
  });
  const [wsStatus, setWsStatus] = useState({ isConnected: false, message: 'Đang tự động kết nối...' });

  useEffect(() => {
    const unsubStatus = websocketBridgeService.onStatusChange((info) => {
      setWsStatus(info);
      if (info.isConnected) {
        setConnectionState(prev => ({
          ...prev,
          connected: true,
          mode: 'WIFI_WEBSOCKET_LOCAL',
          deviceName: `Đồng Hồ SafeWatch (Wi-Fi: ${info.ip})`
        }));
      } else {
        setConnectionState(prev => ({
          ...prev,
          connected: false,
          mode: 'DISCONNECTED',
          deviceName: 'Chưa kết nối thiết bị'
        }));
      }
    });

    return () => {
      unsubStatus();
    };
  }, [setConnectionState]);

  const handleIPChange = (e) => {
    const newIP = e.target.value;
    setWatchIP(newIP);
    websocketBridgeService.setIP(newIP);
    websocketBridgeService.connect(newIP);
  };

  const handleSendCancelSOS = () => {
    websocketBridgeService.sendCancelSOS();
  };

  const handleSendTriggerSOS = () => {
    websocketBridgeService.sendTriggerSOS();
  };

  return (
    <div className="glass-panel rounded-3xl p-5 border border-cyan-500/30 space-y-4 bg-slate-900/80 shadow-xl relative">
      
      {/* Streamlined Header & Automatic Connection Notification */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 border-b border-slate-800/80 pb-3">
        <div className="flex items-center space-x-2">
          <Radio className="w-5 h-5 text-cyan-400 animate-pulse" />
          <h3 className="text-base font-bold text-white">TRẠNG THÁI KẾT NỐI TỰ ĐỘNG</h3>
        </div>

        {/* Live Auto Connection Status Badge */}
        <div className={`px-3.5 py-1.5 rounded-full border text-xs font-mono font-bold flex items-center space-x-2 transition-all ${
          wsStatus.isConnected
            ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 shadow-[0_0_12px_rgba(16,185,129,0.25)]'
            : 'bg-amber-500/20 text-amber-300 border-amber-500/40 animate-pulse'
        }`}>
          <span className={`w-2 h-2 rounded-full ${wsStatus.isConnected ? 'bg-emerald-400 shadow-[0_0_8px_#00e676]' : 'bg-amber-400 animate-ping'}`} />
          <span>
            {wsStatus.isConnected
              ? `🟢 ĐÃ TỰ ĐỘNG KẾT NỐI THÀNH CÔNG (${watchIP}:8080)`
              : `⚡ ĐANG TỰ ĐỘNG DÒ TÌM ĐỒNG HỒ (${watchIP})...`}
          </span>
        </div>
      </div>

      {/* Main Connection & Remote Actions Bar */}
      <div className="p-4 glass-panel rounded-2xl border border-cyan-500/30 bg-slate-950/70 flex flex-col md:flex-row items-center justify-between gap-4">
        
        {/* Left: Device IP Config */}
        <div className="flex items-center space-x-3 w-full md:w-auto">
          <Wifi className={`w-5 h-5 shrink-0 ${wsStatus.isConnected ? 'text-emerald-400' : 'text-slate-500'}`} />
          <div className="flex flex-col">
            <span className="text-[10px] font-mono text-slate-400 uppercase">ĐỊA CHỈ IP SAFEWATCH TRÊN MẠNG WI-FI:</span>
            <input
              type="text"
              value={watchIP}
              onChange={handleIPChange}
              placeholder="192.168.20.152"
              className="px-3 py-1.5 rounded-xl bg-slate-900 border border-slate-700 text-cyan-300 font-mono text-xs focus:outline-none focus:border-cyan-400 w-44 mt-1"
            />
          </div>
        </div>

        {/* Right: Quick Action Controls */}
        <div className="flex flex-wrap items-center gap-2 w-full md:w-auto justify-end">
          <button
            onClick={handleSendCancelSOS}
            disabled={!wsStatus.isConnected}
            className="px-3.5 py-2 rounded-xl bg-emerald-500/20 border border-emerald-500/40 text-emerald-300 font-bold text-xs hover:bg-emerald-500/30 disabled:opacity-40 disabled:cursor-not-allowed transition-all"
          >
            🛑 Gửi Lệnh Hủy Cảnh Báo
          </button>
          <button
            onClick={handleSendTriggerSOS}
            disabled={!wsStatus.isConnected}
            className="px-3.5 py-2 rounded-xl bg-rose-500/20 border border-rose-500/40 text-rose-300 font-bold text-xs hover:bg-rose-500/30 disabled:opacity-40 disabled:cursor-not-allowed transition-all"
          >
            🚨 Thử Còi Báo Động Đồng Hồ
          </button>
        </div>

      </div>

    </div>
  );
}
