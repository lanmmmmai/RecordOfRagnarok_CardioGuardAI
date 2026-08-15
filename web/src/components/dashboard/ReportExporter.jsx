import React from 'react';
import { FileSpreadsheet, FileText, X } from 'lucide-react';
import { initialUserProfile } from '../../data/mockPersonalData';
import { websocketBridgeService } from '../../services/websocketBridgeService';

export default function ReportExporter({ isOpen, onClose }) {
  if (!isOpen) return null;

  const handleDownloadCSV = () => {
    // Exports the watch's current reading, or an empty row saying no device was
    // connected.
    //
    // This used to write four hardcoded rows -- "2026-08-15 14:00,76,98,48,8,
    // NORMAL" and three more like it -- under a header naming BPM, SpO2, HRV
    // and a fall-risk score. They were invented, dated, and downloaded under
    // the same filename this still uses, so the file was indistinguishable from
    // a genuine export. HRV and the risk score are gone from the columns
    // entirely: the firmware computes neither, so there was nothing they could
    // ever have been filled from.
    //
    // Telemetry is read from the service singleton rather than passed down as a
    // prop, because this modal is mounted by App while the WebSocket
    // subscription lives in Dashboard. Reading the latest value at click time
    // is enough here -- nothing needs to re-render when it changes.
    const t = websocketBridgeService.latestTelemetry;
    const contact = Boolean(t?.skinContact);

    const pulse = contact && t?.hrValid && t?.pulse > 0 ? t.pulse : '';
    const spo2 = contact && t?.spo2Valid && t?.spo2 > 0 ? t.spo2 : '';
    const quality = contact && typeof t?.quality === 'number' ? t.quality : '';

    const header = 'Timestamp,Pulse_BPM,SpO2_Percent,SignalQuality_SQI,SkinContact,Battery_Percent,DataSource\n';
    const row = [
      new Date().toISOString(),
      pulse,
      spo2,
      quality,
      t ? (contact ? 'YES' : 'NO') : '',
      typeof t?.battery === 'number' ? t.battery : '',
      t ? 'LIVE_DEVICE' : 'NO_DEVICE_CONNECTED',
    ].join(',');

    // Empty cells mean the watch did not report that field, not a zero reading.
    const blob = new Blob([header + row + '\n'], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.setAttribute("href", url);
    link.setAttribute("download", `CardioGuardAI_Export_${initialUserProfile.id}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
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

          {/* Preview of what the export will contain, read live at render time.
              This block used to show four fixed lines each prefixed with a tick
              -- "72 BPM", "SpO2 98%", "Fall risk INT8 8/100", "SNR 24.8 dB" --
              which read as verified results while being constants. The risk
              score and SNR are dropped rather than restated: no part of the
              firmware produces either. */}
          <div className="p-3 glass-panel rounded-xl space-y-1">
            {(() => {
              const t = websocketBridgeService.latestTelemetry;
              const contact = Boolean(t?.skinContact);
              const pulse = contact && t?.hrValid && t?.pulse > 0 ? t.pulse : null;
              const spo2 = contact && t?.spo2Valid && t?.spo2 > 0 ? t.spo2 : null;

              if (!t) {
                return (
                  <p className="text-amber-300 font-bold">
                    Chưa kết nối đồng hồ — file xuất ra sẽ không có chỉ số đo nào.
                  </p>
                );
              }

              return (
                <>
                  <p className="text-emerald-400 font-bold">
                    Nhịp tim hiện tại: {pulse === null ? '-- (chưa đo được)' : `${pulse} BPM`}
                  </p>
                  <p className="text-cyan-400 font-bold">
                    Nồng độ SpO2: {spo2 === null ? '-- (chưa đo được)' : `${spo2}%`}
                  </p>
                  <p className="text-slate-400">
                    Số đo tức thời tại thời điểm xuất file, không phải trung bình —
                    firmware không lưu lịch sử.
                  </p>
                </>
              );
            })()}
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
