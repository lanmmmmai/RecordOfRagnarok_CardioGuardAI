import React, { useState, useEffect } from 'react';
import { TrendingUp, Clock, Activity } from 'lucide-react';
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

  // Accumulate strictly real measurements from the watch stream
  useEffect(() => {
    if (!isConnected || !rawTelemetry) return;

    const pulse = rawTelemetry.skinContact && rawTelemetry.pulse > 0 ? rawTelemetry.pulse : 0;
    const spo2 = rawTelemetry.spo2Valid && rawTelemetry.spo2 > 0 ? rawTelemetry.spo2 : 0;
    const timeStr = rawTelemetry.timestamp || new Date().toLocaleTimeString('vi-VN');

    // Record only when receiving data
    if (pulse > 0 || spo2 > 0) {
      setHistory(prev => {
        const next = [...prev, { time: timeStr, pulse, spo2 }];
        return next.slice(-30);
      });
    }
  }, [isConnected, rawTelemetry]);

  // Compute actual metrics from real history
  const validPulses = history.map(h => h.pulse).filter(p => p > 0);
  const avgPulse = validPulses.length > 0 ? Math.round(validPulses.reduce((a, b) => a + b, 0) / validPulses.length) : 0;
  const maxPulse = validPulses.length > 0 ? Math.max(...validPulses) : 0;
  const minPulse = validPulses.length > 0 ? Math.min(...validPulses) : 0;

  const chartData = {
    labels: history.length > 0 ? history.map(h => h.time) : ['00:00:00'],
    datasets: [
      {
        label: 'Nhịp tim thực tế (BPM)',
        data: history.length > 0 ? history.map(h => h.pulse) : [0],
        borderColor: '#ff3366',
        backgroundColor: 'rgba(255, 51, 102, 0.12)',
        tension: 0.3,
        fill: true,
        pointBackgroundColor: '#ff3366',
        pointRadius: 4,
        yAxisID: 'yBpm',
      },
      {
        label: 'SpO2 thực tế (%)',
        data: history.length > 0 ? history.map(h => h.spo2) : [0],
        borderColor: '#00f2fe',
        backgroundColor: 'rgba(0, 242, 254, 0.05)',
        tension: 0.3,
        fill: false,
        pointBackgroundColor: '#00f2fe',
        pointRadius: 3,
        yAxisID: 'ySpO2',
      }
    ]
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
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
        bodyFont: { family: 'Inter', size: 12 }
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
        max: 180,
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
            <h3 className="text-lg font-bold text-white">XU HƯỚNG ĐO THỰC TẾ TỪ ĐỒNG HỒ</h3>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Dữ liệu ghi nhận trực tiếp theo thời gian thực (Zero Mock Data)
          </p>
        </div>

        <div className="flex items-center space-x-2 px-3 py-1.5 rounded-xl bg-slate-950/80 border border-slate-800 text-xs font-mono text-cyan-400">
          <Activity className="w-4 h-4 animate-pulse" />
          <span>{history.length} Mẫu Ghi Nhận</span>
        </div>
      </div>

      {/* Chart Canvas Area */}
      <div className="h-72 w-full relative">
        <Line data={chartData} options={options} />
      </div>

      {/* Real Live Calculations */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 text-xs">
        <div className="p-3 glass-panel rounded-2xl border border-cyan-500/30">
          <span className="text-slate-400 font-medium block">Nhịp tim trung bình thực đo</span>
          <span className="text-base font-bold text-white font-mono">
            {avgPulse > 0 ? `${avgPulse} BPM` : "0 BPM"}
          </span>
        </div>
        <div className="p-3 glass-panel rounded-2xl border border-rose-500/30">
          <span className="text-slate-400 font-medium block">Đỉnh nhịp cao nhất</span>
          <span className="text-base font-bold text-rose-400 font-mono">
            {maxPulse > 0 ? `${maxPulse} BPM` : "0 BPM"}
          </span>
        </div>
        <div className="p-3 glass-panel rounded-2xl border border-amber-400/30">
          <span className="text-slate-400 font-medium block">Nhịp tim thấp nhất</span>
          <span className="text-base font-bold text-amber-300 font-mono">
            {minPulse > 0 ? `${minPulse} BPM` : "0 BPM"}
          </span>
        </div>
      </div>

    </div>
  );
}
