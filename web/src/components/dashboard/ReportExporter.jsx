import React from 'react';
import { Download, FileSpreadsheet, FileText, CheckCircle, X } from 'lucide-react';
import { initialUserProfile, mockVitalSummary } from '../../data/mockPersonalData';

export default function ReportExporter({ isOpen, onClose }) {
  if (!isOpen) return null;

  const handleDownloadCSV = () => {
    const csvContent =
      "data:text/csv;charset=utf-8," +
      "Timestamp,BPM,SpO2,HRV,FallRiskScore,Status\n" +
      "2026-08-15 14:00,76,98,48,8,NORMAL\n" +
      "2026-08-15 12:00,78,98,46,8,NORMAL\n" +
      "2026-08-15 10:00,75,98,50,8,NORMAL\n" +
      "2026-08-15 08:00,84,97,42,12,ELEVATED\n";

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `CardioGuardAI_Report_${initialUserProfile.id}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/85 backdrop-blur-md">
      
      <div className="glass-panel w-full max-w-2xl rounded-3xl p-6 md:p-8 border border-cyan-500/40 space-y-6 relative overflow-hidden">
        
        {/* Close Button */}
        <button
          onClick={onClose}
          className="absolute top-4 right-4 text-slate-400 hover:text-white"
        >
          <X className="w-6 h-6" />
        </button>

        {/* Header */}
        <div className="flex items-center space-x-3">
          <div className="w-10 h-10 rounded-xl bg-cyan-500/20 text-cyan-400 flex items-center justify-center">
            <FileText className="w-6 h-6" />
          </div>
          <div>
            <h2 className="text-xl font-bold text-white">XUẤT BÁO CÁO Y TẾ THỐNG KÊ CÁ NHÂN HÓA</h2>
            <p className="text-xs text-slate-400">CardioGuardAI • Record of Ragnarok Bio-Telemetry Report</p>
          </div>
        </div>

        {/* Report Preview Card */}
        <div className="p-4 bg-slate-950/90 rounded-2xl border border-slate-800 space-y-3 font-mono text-xs text-slate-300">
          <div className="flex items-center justify-between border-b border-slate-800 pb-2">
            <span className="text-cyan-400 font-bold">BÁO CÁO SINH HỌC 7 NGÀY QUA</span>
            <span>Mã BN: {initialUserProfile.id}</span>
          </div>

          <div className="grid grid-cols-2 gap-2 text-slate-300">
            <p>Họ tên: <strong>{initialUserProfile.name}</strong></p>
            <p>Tuổi / Giới: <strong>{initialUserProfile.age} / {initialUserProfile.gender}</strong></p>
            <p>Nhóm máu: <strong>{initialUserProfile.bloodType}</strong></p>
            <p>Thiết bị: <strong>{initialUserProfile.deviceModel}</strong></p>
          </div>

          <div className="p-3 glass-panel rounded-xl space-y-1">
            <p className="text-emerald-400 font-bold">✓ Chỉ số Nhịp tim Trung bình: 72 BPM</p>
            <p className="text-cyan-400 font-bold">✓ Nồng độ SpO2 Trung bình: 98%</p>
            <p className="text-amber-300 font-bold">✓ Đánh giá Nguy cơ Té ngã INT8: 8/100 (An toàn)</p>
            <p className="text-slate-400">✓ Lọc nhiễu 5 Tầng: SNR 24.8 dB • Lịch sử bấm HỦY SOS: 1 lần</p>
          </div>
        </div>

        {/* Export Buttons */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 pt-2">
          <button
            onClick={handleDownloadCSV}
            className="py-3 px-4 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-xl shadow-lg flex items-center justify-center space-x-2"
          >
            <FileSpreadsheet className="w-4 h-4" />
            <span>TẢI FILE DỮ LIỆU THÔ (CSV)</span>
          </button>

          <button
            onClick={() => window.print()}
            className="py-3 px-4 glass-panel text-white hover:text-cyan-300 font-bold text-xs rounded-xl border border-slate-700 flex items-center justify-center space-x-2"
          >
            <FileText className="w-4 h-4 text-cyan-400" />
            <span>IN / LƯU BÁO CÁO PDF</span>
          </button>
        </div>

      </div>

    </div>
  );
}
