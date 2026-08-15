import React from 'react';
import { FileText, Printer, Download, ShieldCheck, X, Activity, Heart, Cpu, Send, CheckCircle2 } from 'lucide-react';
import { initialUserProfile, mockVitalSummary } from '../../data/mockPersonalData';

export default function ClinicalReportModal({ isOpen, onClose }) {
  if (!isOpen) return null;

  const handlePrint = () => {
    window.print();
  };

  const handleDownloadCSV = () => {
    const csvContent =
      "data:text/csv;charset=utf-8," +
      "Timestamp,PatientID,PatientName,Age,BPM_Avg,SpO2_Avg,HRV_ms,FallRiskScore,DSP_SNR_dB,TinyML_Status\n" +
      `${new Date().toISOString()},${initialUserProfile.id},"${initialUserProfile.name}",${initialUserProfile.age},${mockVitalSummary.avgBpm24h},${mockVitalSummary.spO2},${mockVitalSummary.hrv},${mockVitalSummary.fallRiskScore},${mockVitalSummary.dspFilterSNR},"NORMAL_SINUS"\n`;

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `CardioGuardAI_ClinicalReport_${initialUserProfile.id}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/85 backdrop-blur-md overflow-y-auto">
      
      <div className="glass-panel w-full max-w-4xl rounded-3xl p-6 md:p-8 border border-cyan-500/40 space-y-6 relative max-h-[90vh] overflow-y-auto">
        
        {/* Top Control Bar */}
        <div className="flex items-center justify-between border-b border-slate-800 pb-4">
          <div className="flex items-center space-x-2">
            <FileText className="w-6 h-6 text-cyan-400" />
            <h2 className="text-xl font-bold text-white">BÁO CÁO Y TẾ & NGHIÊN CỨU SINH HỌC TỔNG HỢP</h2>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={handlePrint}
              className="px-3.5 py-2 glass-panel hover:bg-slate-800 text-cyan-300 font-bold text-xs rounded-xl flex items-center space-x-1.5 transition-all"
            >
              <Printer className="w-4 h-4" />
              <span>In Báo Cáo / Lưu PDF</span>
            </button>

            <button
              onClick={handleDownloadCSV}
              className="px-3.5 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-bold text-xs rounded-xl shadow-lg flex items-center space-x-1.5 transition-all"
            >
              <Download className="w-4 h-4" />
              <span>Tải Dataset CSV</span>
            </button>

            <button
              onClick={onClose}
              className="text-slate-400 hover:text-white p-2 rounded-full hover:bg-slate-800"
            >
              <X className="w-6 h-6" />
            </button>
          </div>
        </div>

        {/* Printable Medical Document Body */}
        <div className="bg-slate-950/90 rounded-2xl p-6 md:p-8 border border-slate-800 text-slate-200 font-sans space-y-6">
          
          {/* Header Document Branding */}
          <div className="flex flex-col sm:flex-row sm:items-center justify-between border-b border-slate-800 pb-6 gap-4">
            <div>
              <h1 className="text-2xl font-black text-white font-mono tracking-wider">
                CARDIO<span className="text-cyan-400">GUARD</span><span className="text-rose-500">AI</span> CLINICAL REPORT
              </h1>
              <p className="text-xs text-slate-400 mt-1">
                Hệ thống Cảnh báo Té ngã, Loạn nhịp tim & Lọc Nhiễu Tín hiệu tại Biên Đồng Hồ Enroll
              </p>
            </div>

            <div className="text-right font-mono text-xs text-slate-400">
              <p>Mã Báo cáo: <strong className="text-cyan-400">RPT-20260815-99</strong></p>
              <p>Ngày tạo: <strong className="text-slate-200">{new Date().toLocaleDateString('vi-VN')}</strong></p>
              <p>Firmware: <strong className="text-emerald-400">v2.4.1-TinyML-INT8</strong></p>
            </div>
          </div>

          {/* Section 1: Patient Administrative Info */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              I. THÔNG TIN HÀNH CHÍNH & THIẾT BỊ THEO DÕI
            </h3>
            
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4 p-4 glass-panel rounded-2xl text-xs font-mono">
              <div>
                <span className="text-slate-500 block text-[10px]">HỌ VÀ TÊN</span>
                <span className="font-bold text-white text-sm">{initialUserProfile.name}</span>
              </div>
              <div>
                <span className="text-slate-500 block text-[10px]">TUỔI / GIỚI TÍNH</span>
                <span className="font-bold text-white text-sm">{initialUserProfile.age} tuổi / {initialUserProfile.gender}</span>
              </div>
              <div>
                <span className="text-slate-500 block text-[10px]">NHÓM MÁU</span>
                <span className="font-bold text-rose-400 text-sm">{initialUserProfile.bloodType}</span>
              </div>
              <div>
                <span className="text-slate-500 block text-[10px]">MÃ THIẾT BỊ MAC</span>
                <span className="font-bold text-cyan-400 text-sm">{initialUserProfile.macAddress}</span>
              </div>
            </div>
          </div>

          {/* Section 2: Bio-Telemetry Metrics Summary */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              II. TỔNG HỢP CHỈ SỐ SINH HỌC (24 GIỜ QUA)
            </h3>

            <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-rose-500">
                <span className="text-slate-400 block text-[10px]">NHỊP TIM TRUNG BÌNH</span>
                <span className="text-xl font-bold text-white">{mockVitalSummary.avgBpm24h} BPM</span>
                <span className="text-[10px] text-slate-500 block">Min: {mockVitalSummary.minBpm24h} | Max: {mockVitalSummary.maxBpm24h}</span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-cyan-400">
                <span className="text-slate-400 block text-[10px]">NỒNG ĐỘ SPO2</span>
                <span className="text-xl font-bold text-cyan-400">{mockVitalSummary.spO2}%</span>
                <span className="text-[10px] text-slate-500 block">Trạng thái: An toàn</span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-emerald-400">
                <span className="text-slate-400 block text-[10px]">BIẾN THIÊN HRV</span>
                <span className="text-xl font-bold text-emerald-400">{mockVitalSummary.hrv} ms</span>
                <span className="text-[10px] text-slate-500 block">R-R Interval Optimal</span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-amber-400">
                <span className="text-slate-400 block text-[10px]">CHỈ SỐ RỦI RO TÉ NGÃ</span>
                <span className="text-xl font-bold text-amber-300">{mockVitalSummary.fallRiskScore}/100</span>
                <span className="text-[10px] text-slate-500 block">TinyML INT8: Low Risk</span>
              </div>
            </div>
          </div>

          {/* Section 3: TinyML AI Assessment & DSP Filter */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              III. ĐÁNH GIÁ MÔ HÌNH TINYML AI INT8 & TÍN HIỆU DSP
            </h3>

            <div className="p-4 glass-panel rounded-2xl space-y-2 text-xs font-mono text-slate-300">
              <p className="flex items-center space-x-2">
                <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                <span><strong>Mô hình Té Ngã INT8 (~8KB):</strong> Độ chính xác 99.4% • Không phát hiện sự cố té ngã chưa giải quyết trong 24h.</span>
              </p>
              <p className="flex items-center space-x-2">
                <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                <span><strong>Mô hình Loạn Nhịp Tim INT8 (~30KB):</strong> Tỷ lệ nhịp xoang bình thường 98.2% • Thời gian suy luận: 3.2ms.</span>
              </p>
              <p className="flex items-center space-x-2">
                <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                <span><strong>DSP Noise Filter 5 Tầng:</strong> Tỷ số tín hiệu trên nhiễu SNR = {mockVitalSummary.dspFilterSNR} (Đã lọc trôi đẳng điện 50Hz).</span>
              </p>
            </div>
          </div>

          {/* Section 4: Signature & Approval Block */}
          <div className="pt-6 border-t border-slate-800 grid grid-cols-2 text-center text-xs font-mono">
            <div>
              <p className="text-slate-400 uppercase font-bold">NGƯỜI LẬP BÁO CÁO / LẬP TRÌNH VIÊN</p>
              <p className="text-slate-500 text-[10px] mt-1">(Ký & ghi rõ họ tên)</p>
              <div className="h-16 flex items-center justify-center font-bold text-cyan-400">
                CardioGuardAI System Engine
              </div>
            </div>

            <div>
              <p className="text-slate-400 uppercase font-bold">BÁC SĨ CHUYÊN KHOA TIM MẠCH</p>
              <p className="text-slate-500 text-[10px] mt-1">(Ký & ghi rõ họ tên)</p>
              <div className="h-16 flex items-center justify-center font-bold text-emerald-400">
                BS. Trịnh Hoàng Nam
              </div>
            </div>
          </div>

        </div>

      </div>

    </div>
  );
}
