import React, { useState, useEffect } from 'react';
import { Wifi, Radio } from 'lucide-react';
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
    <div className="glass-panel rounded-2xl px-5 py-3 border border-cyan-500/30 bg-slate-900/85 shadow-lg flex flex-col md:flex-row items-center justify-between gap-3 transition-all">
      
      {/* Left: Section Label & Live Auto-Connection Status Badge in ONE line */}
      <div className="flex items-center space-x-3 flex-wrap gap-y-2">
        <div className="flex items-center space-x-2">
          <Radio className="w-4 h-4 text-cyan-400 animate-pulse shrink-0" />
          <span className="text-xs font-bold text-white uppercase tracking-wider font-mono">
            TRẠNG THÁI KẾT NỐI TỰ ĐỘNG:
          </span>
        </div>

        <div className={`px-3 py-1 rounded-full border text-[11px] font-mono font-bold flex items-center space-x-1.5 transition-all ${
          wsStatus.isConnected
            ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 shadow-[0_0_10px_rgba(16,185,129,0.25)]'
            : 'bg-amber-500/20 text-amber-300 border-amber-500/40 animate-pulse'
        }`}>
          <span className={`w-2 h-2 rounded-full ${wsStatus.isConnected ? 'bg-emerald-400 shadow-[0_0_6px_#00e676]' : 'bg-amber-400 animate-ping'}`} />
          <span>
            {wsStatus.isConnected
              ? `🟢 ĐÃ TỰ ĐỘNG KẾT NỐI THÀNH CÔNG (${watchIP}:8080)`
              : `⚡ ĐANG TỰ ĐỘNG DÒ TÌM ĐỒNG HỒ (${watchIP})...`}
          </span>
        </div>
      </div>

      {/* Right: Quick IP Input & Remote SOS Actions in the same line */}
      <div className="flex items-center space-x-2.5 flex-wrap gap-y-2 justify-end w-full md:w-auto">
        <div className="flex items-center space-x-1.5 bg-slate-950/80 px-2.5 py-1 rounded-xl border border-slate-700">
          <Wifi className={`w-3.5 h-3.5 ${wsStatus.isConnected ? 'text-emerald-400' : 'text-slate-400'}`} />
          <input
            type="text"
            value={watchIP}
            onChange={handleIPChange}
            placeholder="192.168.20.152"
            className="bg-transparent text-cyan-300 font-mono text-xs focus:outline-none w-28 text-center"
          />
        </div>

        <button
          onClick={handleSendCancelSOS}
          disabled={!wsStatus.isConnected}
          className="px-3 py-1 rounded-xl bg-emerald-500/20 border border-emerald-500/40 text-emerald-300 font-bold text-xs hover:bg-emerald-500/30 disabled:opacity-40 disabled:cursor-not-allowed transition-all"
        >
          🛑 Hủy Cảnh Báo
        </button>
        <button
          onClick={handleSendTriggerSOS}
          disabled={!wsStatus.isConnected}
          className="px-3 py-1 rounded-xl bg-rose-500/20 border border-rose-500/40 text-rose-300 font-bold text-xs hover:bg-rose-500/30 disabled:opacity-40 disabled:cursor-not-allowed transition-all"
        >
          🚨 Thử Còi Báo Động
        </button>
      </div>

    </div>
  );
}
