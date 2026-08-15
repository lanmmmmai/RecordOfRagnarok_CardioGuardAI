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

import { Database, Compass, History } from 'lucide-react';

export default function Dashboard({
  userProfile,
  vitalSummary,
  simulatedBpm,
  isSimulating,
  setIsSimulating,
  onTriggerSOS,
  eventLogs,
  mock24hData,
  mock7DaysTrend,
  onExportReport
}) {
  const [isClinicalReportOpen, setIsClinicalReportOpen] = useState(false);
  const [activeSubTab, setActiveSubTab] = useState('COLLECTOR');
  const [liveBpm, setLiveBpm] = useState(simulatedBpm);
  const [rawTelemetry, setRawTelemetry] = useState(null);

  const [connectionState, setConnectionState] = useState({
    connected: false,
    mode: 'DISCONNECTED',
    deviceName: 'Chưa kết nối thiết bị',
    signal: -54,
    battery: 92
  });

  useEffect(() => {
    const unsubTelemetry = websocketBridgeService.onTelemetry((data) => {
      if (data) {
        setRawTelemetry(data);
        if (data.pulse !== undefined) {
          setLiveBpm(data.pulse);
        }
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

  const activeBpm = connectionState.connected ? liveBpm : simulatedBpm;

  const handleOpenReport = () => {
    setIsClinicalReportOpen(true);
    if (onExportReport) onExportReport();
  };

  return (
    <div className="max-w-7xl mx-auto px-4 py-6 space-y-6">
      
      {/* 1. Profile Header */}
      <ScrollReveal delay={0}>
        <DashboardHeader
          userProfile={userProfile}
          simulatedBpm={activeBpm}
          isSimulating={isSimulating}
          setIsSimulating={setIsSimulating}
          onTriggerSOS={onTriggerSOS}
          onExportReport={handleOpenReport}
        />
      </ScrollReveal>

      {/* 2. Core Vitals Grid */}
      <ScrollReveal delay={100}>
        <VitalCard
          vitalSummary={vitalSummary}
          simulatedBpm={activeBpm}
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

      {/* 4. Live ECG Waveform Oscilloscope & 24h/7d Trends (2 Columns) */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        <ScrollReveal delay={200} className="lg:col-span-6">
          <RealtimeECGChart
            simulatedBpm={activeBpm}
            isSimulating={isSimulating}
            isConnected={connectionState.connected}
            rawTelemetry={rawTelemetry}
          />
        </ScrollReveal>
        <ScrollReveal delay={250} className="lg:col-span-6">
          <TrendChart mock24hData={mock24hData} mock7DaysTrend={mock7DaysTrend} />
        </ScrollReveal>
      </div>

      {/* 5. Streamlined Tab Bar for Detail Features */}
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
              <DataCollectorModule simulatedBpm={activeBpm} />
            </ScrollReveal>
            <ScrollReveal delay={150}>
              <TelegramLiveLogsModule userProfile={userProfile} simulatedBpm={activeBpm} onLogEvent={eventLogs} />
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
