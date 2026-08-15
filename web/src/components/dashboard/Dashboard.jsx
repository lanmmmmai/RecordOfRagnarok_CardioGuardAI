import React, { useState, useEffect } from 'react';
import DashboardHeader from './DashboardHeader';
import VitalCard from './VitalCard';
import RealtimeDeviceBridge from './RealtimeDeviceBridge';
import RealtimeECGChart from './RealtimeECGChart';
import TrendChart from './TrendChart';
import IMUMotionChart from './IMUMotionChart';
import FallLogsTable from './FallLogsTable';
import EmergencyConfig from './EmergencyConfig';
import DataCollectorModule from './DataCollectorModule';
import TelegramLiveLogsModule from './TelegramLiveLogsModule';
import ClinicalReportModal from './ClinicalReportModal';
import ScrollReveal from '../ScrollReveal';
import { websocketBridgeService } from '../../services/websocketBridgeService';

import { Database, Compass, History, Fingerprint, Monitor } from 'lucide-react';

export default function Dashboard({
  userProfile,
  mock24hData,
  mock7DaysTrend,
  eventLogs,
  onTriggerSOS,
  onExportReport
}) {
  const [isClinicalReportOpen, setIsClinicalReportOpen] = useState(false);
  const [activeSubTab, setActiveSubTab] = useState('COLLECTOR');
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

      {/* 3. Realtime Hardware Bridge & Diagnostics */}
      <ScrollReveal delay={150}>
        <RealtimeDeviceBridge
          connectionState={connectionState}
          setConnectionState={setConnectionState}
        />
      </ScrollReveal>

      {/* 4. Live ECG Waveform Oscilloscope & Trends */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        <ScrollReveal delay={200} className="lg:col-span-6">
          <RealtimeECGChart
            isConnected={connectionState.connected}
            rawTelemetry={rawTelemetry}
          />
        </ScrollReveal>
        <ScrollReveal delay={250} className="lg:col-span-6">
          <TrendChart mock24hData={mock24hData} mock7DaysTrend={mock7DaysTrend} />
        </ScrollReveal>
      </div>

      {/* 5. Streamlined Tab Bar */}
      <ScrollReveal delay={100}>
        <div className="glass-panel p-2 rounded-2xl border border-slate-800 flex flex-wrap items-center justify-between gap-2">
          <div className="flex items-center space-x-2">
            <button
              onClick={() => setActiveSubTab('COLLECTOR')}
              className={`px-4 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-2 ${
                activeSubTab === 'COLLECTOR'
                  ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              <Database className="w-4 h-4" />
              <span>Thu Thập Dữ Liệu & Telegram</span>
            </button>

            <button
              onClick={() => setActiveSubTab('IMU')}
              className={`px-4 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-2 ${
                activeSubTab === 'IMU'
                  ? 'bg-amber-400 text-slate-950 shadow-md shadow-amber-400/20'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              <Compass className="w-4 h-4" />
              <span>Vectơ Gia Tốc 3D IMU</span>
            </button>

            <button
              onClick={() => setActiveSubTab('LOGS')}
              className={`px-4 py-2 rounded-xl text-xs font-bold transition-all flex items-center space-x-2 ${
                activeSubTab === 'LOGS'
                  ? 'bg-rose-500 text-white shadow-md shadow-rose-500/20'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              <History className="w-4 h-4" />
              <span>Nhật Ký Sự Kiện & Cấu Hình SOS</span>
            </button>
          </div>

          <span className="text-[11px] font-mono text-slate-500 pr-2 hidden sm:inline">
            Báo cáo: Nguyễn Thị Mai Lan
          </span>
        </div>
      </ScrollReveal>

      {/* 6. Active Tab Content Area */}
      <div className="space-y-6">
        {activeSubTab === 'COLLECTOR' && (
          <>
            <ScrollReveal delay={100}>
              <DataCollectorModule simulatedBpm={rawTelemetry?.pulse || 0} />
            </ScrollReveal>
            <ScrollReveal delay={150}>
              <TelegramLiveLogsModule userProfile={userProfile} simulatedBpm={rawTelemetry?.pulse || 0} onLogEvent={eventLogs} />
            </ScrollReveal>
          </>
        )}

        {activeSubTab === 'IMU' && (
          <ScrollReveal delay={100}>
            <IMUMotionChart
              isConnected={connectionState.connected}
              rawTelemetry={rawTelemetry}
            />
          </ScrollReveal>
        )}

        {activeSubTab === 'LOGS' && (
          <>
            <ScrollReveal delay={100}>
              <FallLogsTable eventLogs={eventLogs} />
            </ScrollReveal>
            <ScrollReveal delay={150}>
              <EmergencyConfig />
            </ScrollReveal>
          </>
        )}
      </div>

      {/* Clinical PDF / CSV Report Modal */}
      <ClinicalReportModal
        isOpen={isClinicalReportOpen}
        onClose={() => setIsClinicalReportOpen(false)}
      />

    </div>
  );
}
