import React, { useState, useEffect, useRef } from 'react';
import { TrendingUp, Activity, BarChart2, Zap } from 'lucide-react';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
} from 'chart.js';
import { Line } from 'react-chartjs-2';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
);

export default function TrendChart({ isConnected, rawTelemetry }) {
  const [history, setHistory] = useState(() => []);
  const lastSampleTimeRef = useRef(0);

  // Sample real data every 1 second to create a clean, scientifically accurate timeline
  useEffect(() => {
    if (!isConnected || !rawTelemetry) return;

    const now = Date.now();
    if (now - lastSampleTimeRef.current < 1000) return;
    lastSampleTimeRef.current = now;

    const hasSkin = Boolean(rawTelemetry.skinContact);
    const pulse = hasSkin && rawTelemetry.pulse > 0 ? rawTelemetry.pulse : 0;
    const spo2 = hasSkin && rawTelemetry.spo2Valid && rawTelemetry.spo2 > 0 ? rawTelemetry.spo2 : 0;
    const timeStr = new Date().toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    setHistory(prev => {
      const next = [...prev, { time: timeStr, pulse, spo2, hasSkin }];
      return next.slice(-25); // Keep rolling 25-second window
    });
  }, [isConnected, rawTelemetry]);

  // Accurate Statistical Formulas
  const activePulses = history.filter(h => h.hasSkin && h.pulse > 0).map(h => h.pulse);
  const activeSpo2 = history.filter(h => h.hasSkin && h.spo2 > 0).map(h => h.spo2);

  // 1. Mean Heart Rate: mu = sum(X) / N
  const avgPulse = activePulses.length > 0 
    ? Math.round(activePulses.reduce((a, b) => a + b, 0) / activePulses.length) 
    : 0;

  // 2. Min & Max Pulse in window
  const maxPulse = activePulses.length > 0 ? Math.max(...activePulses) : 0;
  const minPulse = activePulses.length > 0 ? Math.min(...activePulses) : 0;

  // 3. Heart Rate Standard Deviation (SDNN - Heart Rate Variability metric)
  // Formula: SD = sqrt( sum( (x - mu)^2 ) / N )
  const sdnn = activePulses.length > 1
    ? Math.round(
        Math.sqrt(
          activePulses.reduce((sum, val) => sum + Math.pow(val - avgPulse, 2), 0) / (activePulses.length - 1)
        ) * 10
      ) / 10
    : 0.0;

  // 4. Mean SpO2
  const avgSpo2 = activeSpo2.length > 0
    ? Math.round((activeSpo2.reduce((a, b) => a + b, 0) / activeSpo2.length) * 10) / 10
    : 0;

  const chartData = {
    labels: history.length > 0 ? history.map(h => h.time) : ['00:00:00'],
    datasets: [
      {
        label: 'Nhịp tim đo thật (BPM)',
        data: history.length > 0 ? history.map(h => h.pulse) : [0],
        borderColor: '#ff3366',
        backgroundColor: 'rgba(255, 51, 102, 0.12)',
        tension: 0.35,
        fill: true,
        pointBackgroundColor: '#ff3366',
        pointRadius: 4,
        pointHoverRadius: 6,
        yAxisID: 'yBpm',
      },
      {
        label: 'SpO2 đo thật (%)',
        data: history.length > 0 ? history.map(h => h.spo2) : [0],
        borderColor: '#00f2fe',
        backgroundColor: 'rgba(0, 242, 254, 0.05)',
        tension: 0.35,
        fill: false,
        pointBackgroundColor: '#00f2fe',
        pointRadius: 3,
        pointHoverRadius: 5,
        yAxisID: 'ySpO2',
      }
    ]
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    animation: { duration: 300 },
    plugins: {
      legend: {
        position: 'top',
        labels: {
          color: '#cbd5e1',
          font: { family: 'Inter', size: 11, weight: 'bold' },
          usePointStyle: true,
          boxWidth: 8
        }
      },
      tooltip: {
        backgroundColor: 'rgba(14, 21, 38, 0.95)',
        titleColor: '#00f2fe',
        bodyColor: '#ffffff',
        borderColor: 'rgba(0, 242, 254, 0.3)',
        borderWidth: 1,
        padding: 12,
        cornerRadius: 12,
        bodyFont: { family: 'Inter', size: 12 },
        callbacks: {
          label: function(context) {
            return ` ${context.dataset.label}: ${context.raw} ${context.dataset.yAxisID === 'yBpm' ? 'BPM' : '%'}`;
          }
        }
      }
    },
    scales: {
      x: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#94a3b8', font: { size: 10 }, maxRotation: 0 }
      },
      yBpm: {
        type: 'linear',
        position: 'left',
        min: 0,
        suggestedMax: 140,
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#ff3366', font: { size: 11 } },
        title: { display: true, text: 'BPM', color: '#ff3366' }
      },
      ySpO2: {
        type: 'linear',
        position: 'right',
        min: 0,
        max: 100,
        grid: { drawOnChartArea: false },
        ticks: { color: '#00f2fe', font: { size: 11 } },
        title: { display: true, text: 'SpO2 %', color: '#00f2fe' }
      }
    }
  };

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <TrendingUp className="w-5 h-5 text-cyan-400" />
            <h3 className="text-lg font-bold text-white">XU HƯỚNG ĐO THỰC TẾ THEO CÔNG THỨC TOÁN HỌC</h3>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Cửa sổ thống kê 25 giây gần nhất • Tự động tính trung bình & độ lệch chuẩn SDNN
          </p>
        </div>

        <div className="flex items-center space-x-2 px-3 py-1.5 rounded-xl bg-slate-950/80 border border-slate-800 text-xs font-mono text-cyan-400">
          <Activity className="w-4 h-4 animate-pulse" />
          <span>{history.length} Mẫu Thời Gian</span>
        </div>
      </div>

      {/* Chart Canvas Area */}
      <div className="h-72 w-full relative">
        <Line data={chartData} options={options} />
      </div>

      {/* Calculated Medical Metrics Formula Cards */}
      <div className="grid grid-cols-2 lg:grid-cols-4 gap-3 text-xs">
        
        {/* Metric 1: Mean HR */}
        <div className="p-3.5 glass-panel rounded-2xl border border-rose-500/30 bg-rose-500/5">
          <span className="text-slate-400 font-medium block">Nhịp tim trung bình (μ)</span>
          <span className="text-lg font-black text-rose-400 font-mono">
            {avgPulse > 0 ? `${avgPulse} BPM` : "0 BPM"}
          </span>
          <span className="text-[10px] text-slate-500 block mt-1 font-mono">
            Khoảng: {minPulse > 0 ? `${minPulse} - ${maxPulse}` : "0"} BPM
          </span>
        </div>

        {/* Metric 2: HRV SDNN */}
        <div className="p-3.5 glass-panel rounded-2xl border border-cyan-500/30 bg-cyan-500/5">
          <span className="text-slate-400 font-medium block">Độ biến thiên tim (SDNN)</span>
          <span className="text-lg font-black text-cyan-300 font-mono">
            {sdnn > 0 ? `±${sdnn} ms` : "0 ms"}
          </span>
          <span className="text-[10px] text-slate-500 block mt-1 font-mono">
            Độ lệch chuẩn σ nhịp
          </span>
        </div>

        {/* Metric 3: Mean SpO2 */}
        <div className="p-3.5 glass-panel rounded-2xl border border-emerald-500/30 bg-emerald-500/5">
          <span className="text-slate-400 font-medium block">SpO2 Trung bình</span>
          <span className="text-lg font-black text-emerald-400 font-mono">
            {avgSpo2 > 0 ? `${avgSpo2}%` : "0%"}
          </span>
          <span className="text-[10px] text-slate-500 block mt-1 font-mono">
            Bão hòa oxy máu
          </span>
        </div>

        {/* Metric 4: Signal Integrity */}
        <div className="p-3.5 glass-panel rounded-2xl border border-amber-500/30 bg-amber-500/5">
          <span className="text-slate-400 font-medium block">Chất lượng tín hiệu SQI</span>
          <span className="text-lg font-black text-amber-300 font-mono">
            {rawTelemetry?.skinContact ? `${rawTelemetry?.quality || 0}%` : "0%"}
          </span>
          <span className="text-[10px] text-slate-500 block mt-1 font-mono">
            {rawTelemetry?.skinContact ? "Tiếp xúc da tốt" : "Chưa đeo / Chạm da"}
          </span>
        </div>

      </div>

    </div>
  );
}
