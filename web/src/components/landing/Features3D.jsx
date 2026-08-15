import React, { useState } from 'react';
import { Filter, AlertOctagon, BrainCircuit, Send, CheckCircle2, ChevronRight } from 'lucide-react';
import ScrollReveal from '../ScrollReveal';

export default function Features3D() {
  const [activeTab, setActiveTab] = useState('DSP');

  return (
    <section className="py-16 px-4 max-w-7xl mx-auto">
      
      {/* Section Header */}
      <ScrollReveal delay={0}>
        <div className="text-center space-y-3 mb-12">
          <h2 className="text-xs font-extrabold uppercase tracking-widest text-cyan-400">
            CÔNG NGHỆ CỐT LÕI TẠI BIÊN (EDGE COMPUTING)
          </h2>
          <p className="text-3xl md:text-4xl font-black text-white">
            Kiến trúc 4 Trụ cột An toàn Bio-Tech 🛡️
          </p>
          <p className="text-slate-400 text-sm max-w-2xl mx-auto">
            CardioGuardAI kết hợp xử lý tín hiệu số DSP 5 tầng, thuật toán té ngã 4 giai đoạn, mô hình TinyML lượng tử hóa INT8 và Telegram Alert khẩn cấp.
          </p>
        </div>
      </ScrollReveal>

      {/* Feature Tabs Selector */}
      <ScrollReveal delay={100}>
        <div className="flex flex-wrap items-center justify-center gap-3 mb-8">
          <button
            onClick={() => setActiveTab('DSP')}
            className={`px-5 py-3 rounded-2xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'DSP'
                ? 'bg-cyan-500 text-slate-950 shadow-lg shadow-cyan-500/30'
                : 'glass-panel text-slate-400 hover:text-white'
            }`}
          >
            <Filter className="w-4 h-4" />
            <span>1. Lọc Nhiễu DSP 5 Tầng</span>
          </button>

          <button
            onClick={() => setActiveTab('FALL')}
            className={`px-5 py-3 rounded-2xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'FALL'
                ? 'bg-rose-500 text-white shadow-lg shadow-rose-500/30'
                : 'glass-panel text-slate-400 hover:text-white'
            }`}
          >
            <AlertOctagon className="w-4 h-4" />
            <span>2. Thuật toán Té ngã 4 Giai đoạn</span>
          </button>

          <button
            onClick={() => setActiveTab('AI')}
            className={`px-5 py-3 rounded-2xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'AI'
                ? 'bg-amber-400 text-slate-950 shadow-lg shadow-amber-400/30'
                : 'glass-panel text-slate-400 hover:text-white'
            }`}
          >
            <BrainCircuit className="w-4 h-4" />
            <span>3. TinyML AI INT8</span>
          </button>

          <button
            onClick={() => setActiveTab('TELEGRAM')}
            className={`px-5 py-3 rounded-2xl text-xs font-bold transition-all flex items-center space-x-2 ${
              activeTab === 'TELEGRAM'
                ? 'bg-blue-500 text-white shadow-lg shadow-blue-500/30'
                : 'glass-panel text-slate-400 hover:text-white'
            }`}
          >
            <Send className="w-4 h-4" />
            <span>4. Telegram Alert Trực tiếp</span>
          </button>
        </div>
      </ScrollReveal>

      {/* Dynamic Interactive Card Content */}
      <ScrollReveal delay={150}>
        <div className="glass-panel rounded-3xl p-6 md:p-10 border border-slate-800 relative overflow-hidden">
          
          {activeTab === 'DSP' && (
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-center">
              <div className="lg:col-span-6 space-y-4">
                <span className="px-3 py-1 bg-cyan-500/20 text-cyan-400 border border-cyan-500/30 rounded-lg text-xs font-bold">
                  MAX30102 PPG SENSOR ENGINE
                </span>
                <h3 className="text-2xl font-bold text-white">5-Stage DSP Noise Cancellation Pipeline</h3>
                <p className="text-slate-300 text-sm leading-relaxed">
                  Tín hiệu quang học PPG từ cổ tay thường bị nhiễu rung cơ (Motion Artifacts) và tần số điện lưới. 
                  CardioGuardAI áp dụng chuỗi lọc 5 tầng trực tiếp trên ESP32-S3 trước khi đưa vào mô hình AI:
                </p>
                
                <ul className="space-y-2 text-xs text-slate-300">
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                    <span><strong>Tầng 1: High-Pass Filter (0.5 Hz)</strong> — Loại bỏ trôi đường đẳng điện (DC Offset).</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                    <span><strong>Tầng 2: Low-Pass Filter (5.0 Hz)</strong> — Loại bỏ nhiễu tần số cao &amp; tần số cơ bắp.</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                    <span><strong>Tầng 3: 50Hz Notch Filter</strong> — Cắt bỏ triệt để nhiễu điện lưới thoại 50Hz/60Hz.</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                    <span><strong>Tầng 4: Moving Average Smooth</strong> — Làm mịn dạng sóng đỉnh R-R pulse.</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                    <span><strong>Tầng 5: Wavelet Baseline Subtraction</strong> — Tái tạo xung PPG sạch với SNR {'>'} 24dB.</span>
                  </li>
                </ul>
              </div>

              {/* Visualizer comparison box */}
              <div className="lg:col-span-6 glass-panel bg-slate-950/90 p-5 rounded-2xl border border-cyan-500/20">
                <div className="flex items-center justify-between mb-4">
                  <span className="text-xs font-bold text-rose-400 font-mono">🔴 Tín hiệu thô (Raw Noise)</span>
                  <span className="text-xs font-bold text-cyan-400 font-mono">🟢 Sau khi lọc DSP 5 Tầng</span>
                </div>

                {/* Simulated Waveform Canvas */}
                <div className="h-44 w-full relative flex items-center justify-center border-y border-slate-800">
                  <svg className="w-full h-full" viewBox="0 0 400 150">
                    {/* Raw Noisy Wave (Red) */}
                    <path
                      d="M0,75 Q20,30 40,110 T80,40 T120,130 T160,20 T200,90 T240,10 T280,140 T320,50 T360,100 T400,75"
                      fill="none"
                      stroke="#ff3366"
                      strokeWidth="1.5"
                      opacity="0.5"
                    />
                    {/* Filtered Clean PPG Wave (Cyan Glowing) */}
                    <path
                      d="M0,75 Q20,75 35,75 Q40,65 45,75 L50,75 L55,40 L60,110 L65,20 L70,120 L75,75 Q90,75 110,75 M110,75 Q130,75 145,75 Q150,65 155,75 L160,75 L165,40 L170,110 L175,20 L180,120 L185,75 Q200,75 220,75 M220,75 Q240,75 255,75 Q260,65 265,75 L270,75 L275,40 L280,110 L285,20 L290,120 L295,75 Q310,75 330,75"
                      fill="none"
                      stroke="#00f2fe"
                      strokeWidth="2.5"
                      className="drop-shadow-[0_0_8px_#00f2fe]"
                    />
                  </svg>
                </div>
                <p className="text-[11px] text-center text-slate-400 mt-3 font-mono">
                  Tỷ lệ lọc nhiễu SNR tăng từ 6.2 dB lên 24.8 dB • Tốc độ xử lý: 0.8ms / sample
                </p>
              </div>
            </div>
          )}

          {activeTab === 'FALL' && (
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-center">
              <div className="lg:col-span-6 space-y-4">
                <span className="px-3 py-1 bg-rose-500/20 text-rose-400 border border-rose-500/30 rounded-lg text-xs font-bold">
                  QMI8658 3D IMU SENSOR
                </span>
                <h3 className="text-2xl font-bold text-white">Thuật toán Dự đoán Té ngã 4 Giai đoạn</h3>
                <p className="text-slate-300 text-sm leading-relaxed">
                  Để tránh báo động giả khi người dùng vận động mạnh hoặc đặt mạnh tay xuống bàn, CardioGuardAI yêu cầu thỏa mãn đồng thời 4 giai đoạn logic sinh học:
                </p>

                <div className="grid grid-cols-2 gap-3">
                  <div className="glass-panel p-3 rounded-xl border-l-2 border-l-cyan-400">
                    <p className="text-xs font-bold text-cyan-400">Giai đoạn 1: Rơi tự do</p>
                    <p className="text-[11px] text-slate-400">Gia tốc tổng hợp |a| {'<'} 0.4g trong hơn 100ms.</p>
                  </div>
                  <div className="glass-panel p-3 rounded-xl border-l-2 border-l-rose-500">
                    <p className="text-xs font-bold text-rose-400">Giai đoạn 2: Va chạm</p>
                    <p className="text-[11px] text-slate-400">Đỉnh va chạm cực đại |a| {'>'} 2.8g chạm sàn.</p>
                  </div>
                  <div className="glass-panel p-3 rounded-xl border-l-2 border-l-amber-400">
                    <p className="text-xs font-bold text-amber-400">Giai đoạn 3: Nằm yên</p>
                    <p className="text-[11px] text-slate-400">Độ lệch góc tilt {'>'} 60° và không di chuyển trong 3 giây.</p>
                  </div>
                  <div className="glass-panel p-3 rounded-xl border-l-2 border-l-emerald-400">
                    <p className="text-xs font-bold text-emerald-400">Giai đoạn 4: Đếm ngược 15s</p>
                    <p className="text-[11px] text-slate-400">Màn hình nhấp nháy đỏ + còi + Nút HỦY trước khi gửi SOS Telegram.</p>
                  </div>
                </div>
              </div>

              <div className="lg:col-span-6 glass-panel bg-slate-950/90 p-5 rounded-2xl border border-rose-500/20 text-center space-y-4">
                <h4 className="text-xs font-mono font-bold text-rose-400 uppercase tracking-widest">
                  ĐỒ THỊ VECTƠ GIA TỐC IMU 3D (X, Y, Z)
                </h4>
                <div className="h-44 w-full flex items-center justify-center border-y border-slate-800">
                  <svg className="w-full h-full" viewBox="0 0 400 150">
                    {/* Normal movement */}
                    <path d="M0,75 L80,75 L90,65 L100,85 L110,75 L160,75" stroke="#00f2fe" strokeWidth="2" fill="none" />
                    {/* Free fall dip */}
                    <path d="M160,75 L170,120 L180,120 L190,75" stroke="#ffd700" strokeWidth="2" fill="none" />
                    {/* Impact Spike */}
                    <path d="M190,75 L200,10 L210,140 L220,75" stroke="#ff3366" strokeWidth="3" fill="none" className="drop-shadow-[0_0_8px_#ff3366]" />
                    {/* Stasis flat horizontal */}
                    <path d="M220,75 L400,75" stroke="#00e676" strokeWidth="2" strokeDasharray="4 4" fill="none" />
                  </svg>
                </div>
                <p className="text-xs text-slate-300 font-mono">
                  [Biểu diễn: Dip rơi tự do → Spike va chạm 3.8g → Đường thẳng nằm yên 3s]
                </p>
              </div>
            </div>
          )}

          {activeTab === 'AI' && (
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-center">
              <div className="lg:col-span-6 space-y-4">
                <span className="px-3 py-1 bg-amber-400/20 text-amber-300 border border-amber-400/30 rounded-lg text-xs font-bold">
                  QUANTIZED TENSORFLOW LITE INT8
                </span>
                <h3 className="text-2xl font-bold text-white">Mô hình AI Siêu nhẹ ~8KB &amp; ~30KB</h3>
                <p className="text-slate-300 text-sm leading-relaxed">
                  Các mô hình Deep Learning được huấn luyện trên Python, nén lượng tử hóa INT8 (Quantization) 
                  và chuyển đổi thành file header C++ byte array nạp trực tiếp vào bộ nhớ Flash của ESP32-S3:
                </p>

                <div className="space-y-3">
                  <div className="p-3 glass-panel rounded-xl flex items-center justify-between border-l-4 border-l-cyan-400">
                    <div>
                      <p className="text-sm font-bold text-white">Mô hình Té Ngã INT8 (fall_model.h)</p>
                      <p className="text-xs text-slate-400">Độ rộng cửa sổ 50 mẫu IMU • Kích thước: 8.2 KB</p>
                    </div>
                    <span className="px-2.5 py-1 bg-cyan-500/20 text-cyan-400 text-xs font-mono font-bold rounded-lg">
                      Inference: 3.2 ms
                    </span>
                  </div>

                  <div className="p-3 glass-panel rounded-xl flex items-center justify-between border-l-4 border-l-rose-500">
                    <div>
                      <p className="text-sm font-bold text-white">Mô hình Loạn Nhịp Tim INT8 (arrhythmia_model.h)</p>
                      <p className="text-xs text-slate-400">Phân loại PVC / PAC / Normal • Kích thước: 30.5 KB</p>
                    </div>
                    <span className="px-2.5 py-1 bg-rose-500/20 text-rose-400 text-xs font-mono font-bold rounded-lg">
                      Inference: 7.8 ms
                    </span>
                  </div>
                </div>
              </div>

              <div className="lg:col-span-6 glass-panel bg-slate-950/90 p-6 rounded-2xl border border-amber-400/20 text-center space-y-4">
                <h4 className="text-xs font-mono font-bold text-amber-400 uppercase tracking-widest">
                  KIẾN TRÚC MẠNG NƠ-RON INT8 TRÊN LÕI KÉP ESP32-S3
                </h4>
                <div className="flex items-center justify-around py-4">
                  <div className="p-3 glass-panel rounded-xl text-center">
                    <p className="text-xs font-bold text-slate-400">Input Layer</p>
                    <p className="text-sm font-mono text-cyan-400">50x3 IMU Array</p>
                  </div>
                  <ChevronRight className="w-5 h-5 text-amber-400" />
                  <div className="p-3 glass-panel rounded-xl text-center border border-amber-400/40">
                    <p className="text-xs font-bold text-amber-400">Dense INT8 Layers</p>
                    <p className="text-sm font-mono text-amber-300">64 → 32 Nodes</p>
                  </div>
                  <ChevronRight className="w-5 h-5 text-amber-400" />
                  <div className="p-3 glass-panel rounded-xl text-center">
                    <p className="text-xs font-bold text-slate-400">Output Layer</p>
                    <p className="text-sm font-mono text-rose-400">Prob(Fall) %</p>
                  </div>
                </div>
                <p className="text-xs text-slate-400 font-mono">
                  Chạy hoàn toàn Offline trên Core 0 FreeRTOS • Không phụ thuộc Internet để dự đoán.
                </p>
              </div>
            </div>
          )}

          {activeTab === 'TELEGRAM' && (
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-center">
              <div className="lg:col-span-6 space-y-4">
                <span className="px-3 py-1 bg-blue-500/20 text-blue-400 border border-blue-500/30 rounded-lg text-xs font-bold">
                  HTTPS BOT API CLIENT
                </span>
                <h3 className="text-2xl font-bold text-white">Cảnh báo Telegram SOS Trực tiếp 24/7</h3>
                <p className="text-slate-300 text-sm leading-relaxed">
                  Khi thời gian đếm ngược 15 giây kết thúc mà người dùng không bấm HỦY, ESP32-S3 tự động gửi 
                  gói tin HTTPS POST chứa đầy đủ thông số sức khỏe và vị trí tới Telegram Group Gia đình:
                </p>

                <ul className="space-y-2 text-xs text-slate-300">
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-blue-400" />
                    <span>Gửi tin nhắn tức thì cho tất cả thành viên trong gia đình &amp; Bác sĩ.</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-blue-400" />
                    <span>Tự động đính kèm Nhịp tim (BPM), SpO2 (%) và tọa độ GPS/LBS.</span>
                  </li>
                  <li className="flex items-center space-x-2">
                    <CheckCircle2 className="w-4 h-4 text-blue-400" />
                    <span>Không mất phí dịch vụ SMS hàng tháng — Hoạt động qua Wi-Fi / Hotspot.</span>
                  </li>
                </ul>
              </div>

              <div className="lg:col-span-6 glass-panel bg-slate-950/90 p-5 rounded-2xl border border-blue-500/20 font-mono text-xs text-slate-200">
                <div className="flex items-center justify-between border-b border-slate-800 pb-3 mb-3">
                  <span className="flex items-center space-x-2 text-blue-400 font-bold">
                    <Send className="w-4 h-4" />
                    <span>Telegram Bot Alert Message</span>
                  </span>
                  <span className="text-[10px] text-slate-500">14:22:15 • LIVE</span>
                </div>
                <div className="space-y-2 text-slate-300">
                  <p className="text-rose-400 font-bold">🚨 [CẢNH BÁO TÉ NGÃ KHẨN CẤP] 🚨</p>
                  <p>👤 Bệnh nhân: <strong>Nguyễn Thị Mai Lan (62 tuổi)</strong></p>
                  <p>❤️ Nhịp tim: <span className="text-rose-400 font-bold">118 BPM</span> | SpO2: <span className="text-cyan-400 font-bold">98%</span></p>
                  <p>⚡ Gia tốc va chạm: <strong>3.85g (IMU 3D)</strong></p>
                  <p>📍 Vị trí: <strong>Phòng khách - Nhà riêng (10.7769° N, 106.7009° E)</strong></p>
                  <p className="text-amber-300">⚠️ Vui lòng liên hệ người thân hoặc cấp cứu ngay lập tức!</p>
                </div>
              </div>
            </div>
          )}

        </div>
      </ScrollReveal>

    </section>
  );
}
