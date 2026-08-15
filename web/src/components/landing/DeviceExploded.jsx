import React from 'react';
import { Cpu, Radio, BatteryCharging, Eye, Volume2, Shield } from 'lucide-react';

export default function DeviceExploded() {
  const hardwareSpecs = [
    {
      icon: Cpu,
      title: "ESP32-S3 Dual Core 240MHz",
      desc: "Vi xử lý lõi kép FreeRTOS, tích hợp tăng tốc phần cứng Vector AI cho mạng nơ-ron INT8.",
      tag: "Main Controller"
    },
    {
      icon: Eye,
      title: "GC9A01 1.28\" Round Touch LCD",
      desc: "Màn hình tròn 240x240 pixel cảm ứng mượt mà, giao diện LVGL sắc nét với semantic color palette.",
      tag: "Display UI"
    },
    {
      icon: Radio,
      title: "MAX30102 PPG Heart Sensor",
      desc: "Cảm biến đo nhịp tim & SpO2 độ chính xác cao với LED Đỏ + Hồng ngoại, tích hợp lọc nhiễu 5 tầng.",
      tag: "Bio Sensor"
    },
    {
      icon: Shield,
      title: "QMI8658 6-Axis Motion IMU",
      desc: "Cảm biến gia tốc & con quay hồi chuyển 3D tần số lấy mẫu 100Hz phục vụ dự đoán té ngã 4 giai đoạn.",
      tag: "3D Motion"
    },
    {
      icon: BatteryCharging,
      title: "Pin Lithium 3.7V + ADC Mon",
      desc: "Quản lý nguồn thông minh qua GPIO1 ADC, tự động cảnh báo pin yếu và chế độ chờ tiết kiệm điện.",
      tag: "Power System"
    },
    {
      icon: Volume2,
      title: "Còi Buzzer & Động cơ Rung",
      desc: "Phát âm thanh tần số cao và rung cảnh báo 15 giây dồn dập khi phát hiện té ngã hoặc bấm SOS.",
      tag: "Alert Output"
    }
  ];

  return (
    <section className="py-16 px-4 max-w-7xl mx-auto">
      
      <div className="text-center space-y-3 mb-12">
        <h2 className="text-xs font-extrabold uppercase tracking-widest text-rose-400">
          ĐẶC TẢ PHẦN CỨNG THIẾT BỊ ESP32-S3 WATCH
        </h2>
        <p className="text-3xl md:text-4xl font-black text-white">
          Cấu hình Phần cứng Y tế Tiêu chuẩn 🛠️
        </p>
        <p className="text-slate-400 text-sm max-w-2xl mx-auto">
          Tất cả linh kiện được tối ưu kích thước siêu nhỏ gọn vừa vặn mặt đồng hồ tròn 1.28 inch đeo cổ tay.
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {hardwareSpecs.map((spec, index) => {
          const Icon = spec.icon;
          return (
            <div
              key={index}
              className="glass-panel p-6 rounded-3xl border border-slate-800 hover:border-cyan-500/30 transition-all group hover:scale-[1.02]"
            >
              <div className="flex items-center justify-between mb-4">
                <div className="w-12 h-12 rounded-2xl bg-cyan-500/10 border border-cyan-500/20 flex items-center justify-center text-cyan-400 group-hover:bg-cyan-500 group-hover:text-slate-950 transition-colors">
                  <Icon className="w-6 h-6" />
                </div>
                <span className="text-[10px] font-mono font-bold px-2.5 py-1 bg-slate-900 text-slate-400 border border-slate-700 rounded-full">
                  {spec.tag}
                </span>
              </div>
              <h3 className="text-lg font-bold text-white mb-2 group-hover:text-cyan-400 transition-colors">
                {spec.title}
              </h3>
              <p className="text-xs text-slate-400 leading-relaxed">
                {spec.desc}
              </p>
            </div>
          );
        })}
      </div>

    </section>
  );
}
