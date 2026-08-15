import React, { useState } from 'react';
import { TrendingUp, Calendar, Clock, BarChart3, Filter } from 'lucide-react';
import { Line } from 'react-chartjs-2';

export default function TrendChart({ mock24hData, mock7DaysTrend }) {
  const [timeRange, setTimeRange] = useState('24H'); // 24H, 7D

  // Prepare chart dataset for 24H
  const chartData24H = {
    labels: mock24hData.map((d) => d.time),
    datasets: [
      {
        label: 'Nhịp tim (BPM)',
        data: mock24hData.map((d) => d.bpm),
        borderColor: '#ff3366',
        backgroundColor: 'rgba(255, 51, 102, 0.12)',
        tension: 0.4,
        fill: true,
        pointBackgroundColor: '#ff3366',
        pointRadius: 4,
        yAxisID: 'yBpm',
      },
      {
        label: 'Nồng độ SpO2 (%)',
        data: mock24hData.map((d) => d.spO2),
        borderColor: '#00f2fe',
        backgroundColor: 'rgba(0, 242, 254, 0.05)',
        tension: 0.4,
        fill: false,
        pointBackgroundColor: '#00f2fe',
        pointRadius: 3,
        yAxisID: 'ySpO2',
      }
    ]
  };

  // Prepare chart dataset for 7D
  const chartData7D = {
    labels: mock7DaysTrend.map((d) => d.day),
    datasets: [
      {
        label: 'Nhịp tim trung bình (BPM)',
        data: mock7DaysTrend.map((d) => d.avgBpm),
        borderColor: '#ff3366',
        backgroundColor: 'rgba(255, 51, 102, 0.15)',
        tension: 0.4,
        fill: true,
        pointBackgroundColor: '#ff3366',
        pointRadius: 5,
        yAxisID: 'yBpm',
      },
      {
        label: 'Nhịp tim nghỉ ngơi (Resting BPM)',
        data: mock7DaysTrend.map((d) => d.restBpm),
        borderColor: '#ffd700',
        backgroundColor: 'transparent',
        borderDash: [5, 5],
        tension: 0.4,
        fill: false,
        pointRadius: 4,
        yAxisID: 'yBpm',
      },
      {
        label: 'SpO2 trung bình (%)',
        data: mock7DaysTrend.map((d) => d.avgSpO2),
        borderColor: '#00f2fe',
        backgroundColor: 'transparent',
        tension: 0.4,
        fill: false,
        pointRadius: 4,
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
        ticks: { color: '#94a3b8', font: { size: 11 } }
      },
      yBpm: {
        type: 'linear',
        position: 'left',
        min: 40,
        max: 150,
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#ff3366', font: { size: 11 } },
        title: { display: true, text: 'Heart Rate (BPM)', color: '#ff3366' }
      },
      ySpO2: {
        type: 'linear',
        position: 'right',
        min: 90,
        max: 100,
        grid: { drawOnChartArea: false },
        ticks: { color: '#00f2fe', font: { size: 11 } },
        title: { display: true, text: 'SpO2 (%)', color: '#00f2fe' }
      }
    }
  };

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header with Switcher */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <TrendingUp className="w-5 h-5 text-cyan-400" />
            <h3 className="text-lg font-bold text-white">THỐNG KÊ XU HƯỚNG CÁ NHÂN HÓA</h3>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Phân tích biến động Nhịp tim, Oxy trong máu & Tình trạng vận động sinh học
          </p>
        </div>

        {/* Range Selector */}
        <div className="flex items-center bg-slate-950/90 p-1 rounded-2xl border border-slate-800">
          <button
            onClick={() => setTimeRange('24H')}
            className={`px-3 py-1.5 rounded-xl text-xs font-bold transition-all flex items-center space-x-1 ${
              timeRange === '24H'
                ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            <Clock className="w-3.5 h-3.5" />
            <span>24 Giờ qua</span>
          </button>

          <button
            onClick={() => setTimeRange('7D')}
            className={`px-3 py-1.5 rounded-xl text-xs font-bold transition-all flex items-center space-x-1 ${
              timeRange === '7D'
                ? 'bg-rose-500 text-white shadow-md shadow-rose-500/20'
                : 'text-slate-400 hover:text-white'
            }`}
          >
            <Calendar className="w-3.5 h-3.5" />
            <span>7 Ngày qua</span>
          </button>
        </div>
      </div>

      {/* Chart Canvas Area */}
      <div className="h-72 w-full relative">
        <Line data={timeRange === '24H' ? chartData24H : chartData7D} options={options} />
      </div>

      {/* Key Insights Ticker */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 text-xs">
        <div className="p-3 glass-panel rounded-2xl border-l-4 border-l-cyan-400">
          <span className="text-slate-400 font-medium block">Nhịp tim trung bình</span>
          <span className="text-base font-bold text-white font-mono">72 BPM (Ổn định)</span>
        </div>
        <div className="p-3 glass-panel rounded-2xl border-l-4 border-l-rose-500">
          <span className="text-slate-400 font-medium block">Đỉnh nhịp cao nhất</span>
          <span className="text-base font-bold text-rose-400 font-mono">124 BPM (Khi vận động)</span>
        </div>
        <div className="p-3 glass-panel rounded-2xl border-l-4 border-l-amber-400">
          <span className="text-slate-400 font-medium block">Nhịp tim nghỉ ngơi (Resting)</span>
          <span className="text-base font-bold text-amber-300 font-mono">58 BPM (Lúc 04:00 sáng)</span>
        </div>
      </div>

    </div>
  );
}
