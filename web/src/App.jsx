import React, { useState, useEffect } from 'react';
import Navbar from './components/Navbar';
import Footer from './components/Footer';
import ParticleBackground from './components/3d/ParticleBackground';

// Landing Page Components
import HeroSection from './components/landing/HeroSection';
import Features3D from './components/landing/Features3D';
import DeviceExploded from './components/landing/DeviceExploded';
import AlertSimulator from './components/landing/AlertSimulator';

// Dashboard Components
import Dashboard from './components/dashboard/Dashboard';
import ReportExporter from './components/dashboard/ReportExporter';

// Mock Data
import {
  initialUserProfile,
  mockVitalSummary,
  initialEventLogs,
  mock24hData,
  mock7DaysTrend
} from './data/mockPersonalData';

export default function App() {
  const [activeTab, setActiveTab] = useState('LANDING'); // 'LANDING' or 'DASHBOARD'
  const [userProfile, setUserProfile] = useState(initialUserProfile);
  const [vitalSummary, setVitalSummary] = useState(mockVitalSummary);
  const [simulatedBpm, setSimulatedBpm] = useState(76);
  const [isSimulating, setIsSimulating] = useState(false);
  const [isSOSOpen, setIsSOSOpen] = useState(false);
  const [isReportOpen, setIsReportOpen] = useState(false);
  const [eventLogs, setEventLogs] = useState(initialEventLogs);

  // Dynamic Live Simulation Effect
  useEffect(() => {
    if (!isSimulating) return;

    const interval = setInterval(() => {
      setSimulatedBpm((prev) => {
        // Natural fluctuation between 68 and 92 BPM
        const delta = Math.floor(Math.random() * 7) - 3;
        const next = Math.max(62, Math.min(115, prev + delta));
        return next;
      });
    }, 2500);

    return () => clearInterval(interval);
  }, [isSimulating]);

  const handleLogEvent = (newLog) => {
    setEventLogs((prev) => [newLog, ...prev]);
  };

  return (
    <div className="min-h-screen bg-[#070a12] text-slate-100 flex flex-col relative overflow-x-hidden selection:bg-cyan-500 selection:text-slate-950">
      
      {/* 3D Particle Starfield Background */}
      <ParticleBackground />

      {/* Top Navbar */}
      <Navbar
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        onTriggerSOS={() => setIsSOSOpen(true)}
        simulatedBpm={simulatedBpm}
      />

      {/* Main View Router */}
      <main className="flex-1 z-10">
        {activeTab === 'LANDING' ? (
          <div className="space-y-12">
            <HeroSection
              onExploreDashboard={() => setActiveTab('DASHBOARD')}
              onTriggerSOS={() => setIsSOSOpen(true)}
              simulatedBpm={simulatedBpm}
            />
            <Features3D />
            <DeviceExploded />
          </div>
        ) : (
          <Dashboard
            userProfile={userProfile}
            vitalSummary={vitalSummary}
            simulatedBpm={simulatedBpm}
            isSimulating={isSimulating}
            setIsSimulating={setIsSimulating}
            onTriggerSOS={() => setIsSOSOpen(true)}
            eventLogs={eventLogs}
            mock24hData={mock24hData}
            mock7DaysTrend={mock7DaysTrend}
            onExportReport={() => setIsReportOpen(true)}
          />
        )}
      </main>

      {/* Modals */}
      <AlertSimulator
        isOpen={isSOSOpen}
        onClose={() => setIsSOSOpen(false)}
        onLogEvent={handleLogEvent}
      />

      <ReportExporter
        isOpen={isReportOpen}
        onClose={() => setIsReportOpen(false)}
      />

      {/* Footer */}
      <Footer
        onExplore3D={() => setActiveTab('LANDING')}
        onExploreDashboard={() => setActiveTab('DASHBOARD')}
      />

    </div>
  );
}
