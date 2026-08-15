import React, { useState, useEffect } from 'react';
import DashboardHeader from './DashboardHeader';
import VitalCard from './VitalCard';
import RealtimeDeviceBridge from './RealtimeDeviceBridge';
import DataCollectorModule from './DataCollectorModule';
import TelegramLiveLogsModule from './TelegramLiveLogsModule';
import ClinicalReportModal from './ClinicalReportModal';
import ScrollReveal from '../ScrollReveal';
import { websocketBridgeService } from '../../services/websocketBridgeService';
import { Fingerprint, Monitor } from 'lucide-react';

export default function Dashboard({
  userProfile,
  eventLogs,
  onTriggerSOS,
  onExportReport
}) {
  const [isClinicalReportOpen, setIsClinicalReportOpen] = useState(false);
  const [rawTelemetry, setRawTelemetry] = useState(null);

  const [connectionState, setConnectionState] = useState({
    connected: false,
    mode: 'DISCONNECTED',
    deviceName: 'Chưa kết nối thiết bị',
    signal: 0,
    battery: 0
  });

  useEffect(() => {
    const unsubTelemetry = websocketBridgeService.onTelemetry((data) => {
      if (data) {
        setRawTelemetry(data);
      }
    });

    const unsubFall = websocketBridgeService.onFallAlert((data) => {
      if (onTriggerSOS) onTriggerSOS();
    });

    return () => {
      unsubTelemetry();
      unsubFall();
    };
  }, [onTriggerSOS]);

  const handleOpenReport = () => {
    setIsClinicalReportOpen(true);
    if (onExportReport) onExportReport();
  };

  const getScreenName = (screenId) => {
    switch (screenId) {
      case 0: return "MÀN HÌNH CHÍNH (HOME)";
      case 1: return "ĐO NHỊP TIM (HEART RATE)";
      case 2: return "ĐO NỒNG ĐỘ OXY (SPO2)";
      case 3: return "GIÁM SÁT TÉ NGÃ (FALL MONITOR)";
      case 4: return "CẢNH BÁO TÉ NGÃ KHẨN CẤP (FALL ALERT)";
      case 5: return "THÔNG BÁO HỆ THỐNG (NOTIFICATION)";
      case 6: return "MENU CÀI ĐẶT NHANH (QUICK MENU)";
      default: return "MÀN HÌNH CHÍNH (HOME)";
    }
  };

  return (
    <div className="max-w-7xl mx-auto px-4 py-6 space-y-6">
      
      {/* 1. Profile Header */}
      <ScrollReveal delay={0}>
        <DashboardHeader
          userProfile={userProfile}
          isConnected={connectionState.connected}
          rawTelemetry={rawTelemetry}
          onTriggerSOS={onTriggerSOS}
          onExportReport={handleOpenReport}
        />
      </ScrollReveal>

      {/* Real-time Physical Touch & Active Screen Interaction HUD */}
      {connectionState.connected && (
        <ScrollReveal delay={50}>
          <div className="glass-panel p-4 rounded-2xl border border-cyan-500/40 bg-slate-900/90 shadow-xl flex flex-col md:flex-row items-center justify-between gap-4 font-mono text-xs">
            <div className="flex items-center space-x-3">
              <div className={`w-10 h-10 rounded-xl flex items-center justify-center ${rawTelemetry?.touch?.touched ? 'bg-rose-500/30 text-rose-400 border border-rose-500 animate-pulse' : 'bg-slate-800 text-cyan-400'}`}>
                <Fingerprint className="w-5 h-5" />
              </div>
              <div>
                <span className="text-[10px] text-slate-400 uppercase tracking-wider block">TƯƠNG TÁC MẶT KÍNH ĐỒNG HỒ</span>
                <p className="text-white font-bold text-sm">
                  {rawTelemetry?.touch?.touched
                    ? `👉 ĐANG CHẠM (X: ${rawTelemetry.touch.x}, Y: ${rawTelemetry.touch.y}) • ${rawTelemetry.touch.gesture}`
                    : "Chờ người dùng chạm vào mặt kính"}
                </p>
              </div>
            </div>

            <div className="flex items-center space-x-3 bg-slate-950/80 px-4 py-2 rounded-xl border border-slate-800">
              <Monitor className="w-4 h-4 text-emerald-400 animate-pulse" />
              <div>
                <span className="text-[10px] text-slate-400 block">MÀN HÌNH ĐỒNG HỒ HIỆN TẠI</span>
                <span className="text-emerald-400 font-bold text-xs">
                  {getScreenName(rawTelemetry?.screen)}
                </span>
              </div>
            </div>
          </div>
        </ScrollReveal>
      )}

      {/* 2. Core Vitals Grid */}
      <ScrollReveal delay={100}>
        <VitalCard
          isConnected={connectionState.connected}
          rawTelemetry={rawTelemetry}
        />
      </ScrollReveal>

      {/* 3. Realtime Hardware Bridge & Diagnostics (1 Single Line) */}
      <ScrollReveal delay={150}>
        <RealtimeDeviceBridge
          connectionState={connectionState}
          setConnectionState={setConnectionState}
        />
      </ScrollReveal>

      {/* 4. Streamlined Core Modules: Thu Thập Dữ Liệu & Telegram (Bỏ hết các phần khác) */}
      <div className="space-y-6">
        <ScrollReveal delay={200}>
          <DataCollectorModule
            isConnected={connectionState.connected}
            rawTelemetry={rawTelemetry}
          />
        </ScrollReveal>

        <ScrollReveal delay={250}>
          <TelegramLiveLogsModule
            userProfile={userProfile}
            isConnected={connectionState.connected}
            rawTelemetry={rawTelemetry}
            onLogEvent={eventLogs}
          />
        </ScrollReveal>
      </div>

      {/* Clinical PDF / CSV Report Modal */}
      <ClinicalReportModal
        isOpen={isClinicalReportOpen}
        onClose={() => setIsClinicalReportOpen(false)}
      />

    </div>
  );
}
