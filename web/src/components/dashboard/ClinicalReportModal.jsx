import React from 'react';
import { FileText, Printer, Download, X, CheckCircle2, AlertTriangle } from 'lucide-react';
import { initialUserProfile } from '../../data/mockPersonalData';

// The report shows what the watch measured, and says so where it did not.
//
// It used to read every number out of mockVitalSummary -- a fixed object with
// avgBpm24h 74, HRV 52 ms, a fall-risk score of 6/100 and "TinyML INT8 accuracy
// 99.4%". None of it came from the device, none of it changed, and nothing on
// the page or in the exported CSV said so. The file downloads as
// CardioGuardAI_ClinicalReport_*.csv with PatientID and PatientName columns, so
// a reader had no way to tell invented figures from measured ones.
//
// Live telemetry now fills the fields the firmware actually produces. The rest
// is not filled in with something plausible: a report that is wrong in a way
// the reader cannot detect is worse than one that admits a gap, and this one
// carries a doctor's signature block.

// Fields the watch reports directly. Anything absent stays absent rather than
// falling back to a default that would read as a measurement.
function readVitals(t) {
  const contact = Boolean(t?.skinContact);
  return {
    // Guarded the same way VitalCard guards them: a pulse without skin contact
    // or a validity flag is a number the DSP has already withdrawn.
    pulse: contact && t?.hrValid && t?.pulse > 0 ? t.pulse : null,
    spo2: contact && t?.spo2Valid && t?.spo2 > 0 ? t.spo2 : null,
    quality: contact && typeof t?.quality === 'number' ? t.quality : null,
    battery: typeof t?.battery === 'number' ? t.battery : null,
    rssi: typeof t?.rssi === 'number' ? t.rssi : null,
    ip: t?.ip || null,
    imuOk: t?.sensors?.imuOk,
    hrOk: t?.sensors?.hrOk,
    fallState: typeof t?.fallState === 'number' ? t.fallState : null,
  };
}

const FALL_STATE_NAMES = [
  'BÌNH THƯỜNG',
  'NGHI NGỜ TÉ NGÃ',
  'CẢNH BÁO - ĐANG ĐẾM NGƯỢC',
  'ĐANG GỬI CẢNH BÁO',
  'ĐÃ GỬI CẢNH BÁO',
  'GỬI THẤT BẠI',
];

// One place decides how an absent measurement looks, so a missing value can
// never be mistaken for a reading of zero.
function Measured({ value, unit, className = 'text-white' }) {
  if (value === null || value === undefined) {
    return <span className="text-xl font-bold text-slate-600">--</span>;
  }
  return (
    <span className={`text-xl font-bold ${className}`}>
      {value}{unit ? <span className="text-sm ml-0.5">{unit}</span> : null}
    </span>
  );
}

export default function ClinicalReportModal({ isOpen, onClose, rawTelemetry, isConnected }) {
  if (!isOpen) return null;

  const v = readVitals(rawTelemetry);
  const live = Boolean(isConnected && rawTelemetry);

  const handlePrint = () => {
    window.print();
  };

  const handleDownloadCSV = () => {
    // Columns match what the device reports. The 24 h aggregates, HRV and
    // fall-risk score that used to be here are gone rather than exported as
    // constants: the firmware keeps no history to average, computes no HRV and
    // has no risk model, so every one of those columns was a fabricated value
    // sitting under a real-looking header.
    const esc = (s) => `"${String(s ?? '').replace(/"/g, '""')}"`;
    const n = (x) => (x === null || x === undefined ? '' : x);

    const header =
      'Timestamp,PatientID,PatientName,Age,Pulse_BPM,SpO2_Percent,SignalQuality_SQI,' +
      'SkinContact,FallState,Battery_Percent,WiFi_RSSI_dBm,DeviceIP,DataSource\n';

    const row = [
      new Date().toISOString(),
      initialUserProfile.id,
      esc(initialUserProfile.name),
      initialUserProfile.age,
      n(v.pulse),
      n(v.spo2),
      n(v.quality),
      rawTelemetry?.skinContact ? 'YES' : 'NO',
      v.fallState !== null ? FALL_STATE_NAMES[v.fallState] || v.fallState : '',
      n(v.battery),
      n(v.rssi),
      esc(v.ip),
      live ? 'LIVE_DEVICE' : 'NO_DEVICE_CONNECTED',
    ].join(',');

    // Empty cells mean the watch did not report that field at export time.
    const blob = new Blob([header + row + '\n'], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.setAttribute('href', url);
    link.setAttribute('download', `CardioGuardAI_Report_${initialUserProfile.id}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/85 backdrop-blur-md overflow-y-auto">

      <div className="glass-panel w-full max-w-4xl rounded-3xl p-6 md:p-8 border border-cyan-500/40 space-y-6 relative max-h-[90vh] overflow-y-auto">

        {/* Top Control Bar */}
        <div className="flex items-center justify-between border-b border-slate-800 pb-4">
          <div className="flex items-center space-x-2">
            <FileText className="w-6 h-6 text-cyan-400" />
            <h2 className="text-xl font-bold text-white">BÁO CÁO CHỈ SỐ ĐO TỪ THIẾT BỊ</h2>
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
              <span>Tải CSV</span>
            </button>

            <button
              onClick={onClose}
              className="text-slate-400 hover:text-white p-2 rounded-full hover:bg-slate-800"
            >
              <X className="w-6 h-6" />
            </button>
          </div>
        </div>

        {/* Printable Document Body */}
        <div className="bg-slate-950/90 rounded-2xl p-6 md:p-8 border border-slate-800 text-slate-200 font-sans space-y-6">

          {/* Header Document Branding */}
          <div className="flex flex-col sm:flex-row sm:items-center justify-between border-b border-slate-800 pb-6 gap-4">
            <div>
              <h1 className="text-2xl font-black text-white font-mono tracking-wider">
                CARDIO<span className="text-cyan-400">GUARD</span><span className="text-rose-500">AI</span> DEVICE REPORT
              </h1>
              <p className="text-xs text-slate-400 mt-1">
                Chỉ số đo tức thời từ đồng hồ SafeWatch (ESP32-S3)
              </p>
            </div>

            <div className="text-right font-mono text-xs text-slate-400">
              <p>Ngày tạo: <strong className="text-slate-200">{new Date().toLocaleString('vi-VN')}</strong></p>
              <p>Nguồn dữ liệu: {live
                ? <strong className="text-emerald-400">THIẾT BỊ THẬT (WebSocket)</strong>
                : <strong className="text-amber-400">CHƯA KẾT NỐI THIẾT BỊ</strong>}</p>
            </div>
          </div>

          {/* Not connected: say so at the top, before any field is read. */}
          {!live && (
            <div className="flex items-start space-x-3 p-4 rounded-2xl border border-amber-500/40 bg-amber-500/10">
              <AlertTriangle className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
              <div className="text-xs text-amber-200/90">
                <strong className="block text-amber-300 mb-1">Không có dữ liệu đo</strong>
                Đồng hồ chưa kết nối, nên báo cáo này không chứa chỉ số sinh học nào.
                Các ô hiển thị “--” là chưa đo được, không phải giá trị bằng 0.
              </div>
            </div>
          )}

          {/* Section 1: Administrative */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              I. THÔNG TIN HÀNH CHÍNH
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
                <span className="text-slate-500 block text-[10px]">ĐỊA CHỈ IP THIẾT BỊ</span>
                <span className="font-bold text-cyan-400 text-sm">{v.ip || '--'}</span>
              </div>
            </div>
            <p className="text-[10px] text-slate-500 font-mono px-1">
              Hồ sơ hành chính là dữ liệu mẫu cấu hình sẵn trong ứng dụng, không đến từ thiết bị.
            </p>
          </div>

          {/* Section 2: Measured vitals -- instantaneous, and labelled as such */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              II. CHỈ SỐ ĐO TẠI THỜI ĐIỂM TẠO BÁO CÁO
            </h3>

            <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-rose-500">
                <span className="text-slate-400 block text-[10px]">NHỊP TIM</span>
                <Measured value={v.pulse} unit=" BPM" />
                <span className="text-[10px] text-slate-500 block">
                  {v.pulse === null ? 'Chưa đo được' : v.pulse > 100 ? 'Nhịp nhanh' : v.pulse < 55 ? 'Nhịp chậm' : 'Trong ngưỡng bình thường'}
                </span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-cyan-400">
                <span className="text-slate-400 block text-[10px]">NỒNG ĐỘ SPO2</span>
                <Measured value={v.spo2} unit="%" className="text-cyan-400" />
                <span className="text-[10px] text-slate-500 block">
                  {v.spo2 === null ? 'Chưa đo được' : v.spo2 >= 95 ? 'Trong ngưỡng bình thường' : 'Thấp hơn ngưỡng 95%'}
                </span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-emerald-400">
                <span className="text-slate-400 block text-[10px]">CHẤT LƯỢNG TÍN HIỆU (SQI)</span>
                <Measured value={v.quality} unit="/100" className="text-emerald-400" />
                <span className="text-[10px] text-slate-500 block">
                  {v.quality === null ? 'Không có tiếp xúc da' : 'Thang đo chưa hiệu chuẩn'}
                </span>
              </div>

              <div className="p-3 glass-panel rounded-xl border-l-4 border-l-amber-400">
                <span className="text-slate-400 block text-[10px]">TRẠNG THÁI TÉ NGÃ</span>
                <span className={`text-sm font-bold ${v.fallState ? 'text-rose-400' : 'text-emerald-400'}`}>
                  {v.fallState !== null ? (FALL_STATE_NAMES[v.fallState] || v.fallState) : '--'}
                </span>
                <span className="text-[10px] text-slate-500 block">Thuật toán ngưỡng, chưa dùng học máy</span>
              </div>
            </div>

            <p className="text-[10px] text-slate-500 font-mono px-1">
              Đây là số đo tức thời, không phải trung bình 24 giờ: firmware không lưu lịch sử chỉ số sinh học.
            </p>
          </div>

          {/* Section 3: Device state -- reported, not inferred */}
          <div className="space-y-3">
            <h3 className="text-xs font-bold font-mono text-cyan-400 uppercase tracking-widest">
              III. TRẠNG THÁI THIẾT BỊ
            </h3>

            <div className="p-4 glass-panel rounded-2xl space-y-2 text-xs font-mono text-slate-300">
              <p className="flex items-center space-x-2">
                {v.imuOk ? <CheckCircle2 className="w-4 h-4 text-emerald-400" /> : <AlertTriangle className="w-4 h-4 text-amber-400" />}
                <span><strong>Cảm biến gia tốc (QMI8658):</strong> {v.imuOk === undefined ? '--' : v.imuOk ? 'Hoạt động' : 'Không phản hồi'}</span>
              </p>
              <p className="flex items-center space-x-2">
                {v.hrOk ? <CheckCircle2 className="w-4 h-4 text-emerald-400" /> : <AlertTriangle className="w-4 h-4 text-amber-400" />}
                <span><strong>Cảm biến nhịp tim (MAX30102):</strong> {v.hrOk === undefined ? '--' : v.hrOk ? 'Hoạt động' : 'Không phản hồi'}</span>
              </p>
              <p className="flex items-center space-x-2">
                <CheckCircle2 className="w-4 h-4 text-slate-500" />
                <span><strong>Pin:</strong> {v.battery === null ? '--' : `${v.battery}%`} • <strong>Wi-Fi:</strong> {v.rssi === null ? '--' : `${v.rssi} dBm`}</span>
              </p>
            </div>

            {/* What this device does not do. Stated because the previous version
                of this report claimed all three as completed features. */}
            <div className="p-4 rounded-2xl border border-slate-700 bg-slate-900/60 space-y-1.5 text-[11px] font-mono text-slate-400">
              <strong className="block text-slate-300 text-xs mb-1">Chưa có trong phiên bản hiện tại</strong>
              <p>• Mô hình TinyML: chưa được huấn luyện. Phát hiện té ngã hiện dùng ngưỡng gia tốc cố định.</p>
              <p>• Phát hiện loạn nhịp tim: chưa triển khai.</p>
              <p>• HRV, chỉ số rủi ro té ngã, thống kê 24 giờ: firmware không tính và không lưu.</p>
            </div>
          </div>

          {/* Section 4: Signature block */}
          <div className="pt-6 border-t border-slate-800 grid grid-cols-2 text-center text-xs font-mono">
            <div>
              <p className="text-slate-400 uppercase font-bold">NGƯỜI LẬP BÁO CÁO</p>
              <p className="text-slate-500 text-[10px] mt-1">(Ký & ghi rõ họ tên)</p>
              <div className="h-16" />
            </div>

            <div>
              <p className="text-slate-400 uppercase font-bold">BÁC SĨ CHUYÊN KHOA</p>
              <p className="text-slate-500 text-[10px] mt-1">(Ký & ghi rõ họ tên)</p>
              <div className="h-16" />
            </div>
          </div>

          <p className="text-[10px] text-slate-500 text-center font-mono border-t border-slate-800 pt-4">
            Thiết bị nghiên cứu, chưa được kiểm định y tế. Không dùng để chẩn đoán hoặc điều trị.
          </p>

        </div>

      </div>

    </div>
  );
}
