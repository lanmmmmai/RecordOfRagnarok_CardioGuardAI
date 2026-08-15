import React, { useState } from 'react';
import { History, Search, Filter, AlertTriangle, ShieldCheck, Info, Send, Eye, X } from 'lucide-react';

export default function FallLogsTable({ eventLogs }) {
  const [searchTerm, setSearchTerm] = useState('');
  const [selectedFilter, setSelectedFilter] = useState('ALL'); // ALL, CRITICAL, WARNING, INFO
  const [selectedLog, setSelectedLog] = useState(null);

  const filteredLogs = eventLogs.filter((log) => {
    const matchesSearch =
      log.id.toLowerCase().includes(searchTerm.toLowerCase()) ||
      log.eventType.toLowerCase().includes(searchTerm.toLowerCase()) ||
      log.details.toLowerCase().includes(searchTerm.toLowerCase());

    const matchesFilter = selectedFilter === 'ALL' || log.severity === selectedFilter;

    return matchesSearch && matchesFilter;
  });

  return (
    <div className="glass-panel rounded-3xl p-6 border border-slate-800 space-y-6">
      
      {/* Header & Filter Controls */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 border-b border-slate-800 pb-4">
        <div>
          <div className="flex items-center space-x-2">
            <History className="w-5 h-5 text-rose-500" />
            <h3 className="text-lg font-bold text-white">NHẬT KÝ SỰ KIỆN CẢNH BÁO SỨC KHỎE</h3>
          </div>
          <p className="text-xs text-slate-400 mt-0.5">
            Lịch sử tự động ghi nhận bởi ESP32-S3 Watch & Mô hình AI TinyML INT8
          </p>
        </div>

        {/* Search & Filter Bar */}
        <div className="flex flex-wrap items-center gap-3">
          
          {/* Search Box */}
          <div className="relative">
            <Search className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
            <input
              type="text"
              placeholder="Tìm kiếm nhật ký..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-9 pr-4 py-2 bg-slate-950 border border-slate-800 rounded-xl text-xs text-white placeholder-slate-500 focus:outline-none focus:border-cyan-500 w-48"
            />
          </div>

          {/* Severity Filter Buttons */}
          <div className="flex items-center bg-slate-950 p-1 rounded-xl border border-slate-800 text-xs">
            <button
              onClick={() => setSelectedFilter('ALL')}
              className={`px-3 py-1.5 rounded-lg font-bold transition-all ${
                selectedFilter === 'ALL' ? 'bg-cyan-500 text-slate-950' : 'text-slate-400 hover:text-white'
              }`}
            >
              Tất cả ({eventLogs.length})
            </button>
            <button
              onClick={() => setSelectedFilter('CRITICAL')}
              className={`px-3 py-1.5 rounded-lg font-bold transition-all ${
                selectedFilter === 'CRITICAL' ? 'bg-rose-500 text-white' : 'text-slate-400 hover:text-white'
              }`}
            >
              Cấp cứu
            </button>
            <button
              onClick={() => setSelectedFilter('WARNING')}
              className={`px-3 py-1.5 rounded-lg font-bold transition-all ${
                selectedFilter === 'WARNING' ? 'bg-amber-400 text-slate-950' : 'text-slate-400 hover:text-white'
              }`}
            >
              Cảnh báo
            </button>
          </div>

        </div>
      </div>

      {/* Logs Table */}
      <div className="overflow-x-auto">
        <table className="w-full text-left text-xs font-mono">
          <thead>
            <tr className="border-b border-slate-800 text-slate-400 uppercase text-[10px]">
              <th className="py-3 px-4">Mã Sự Kiện</th>
              <th className="py-3 px-4">Thời Gian</th>
              <th className="py-3 px-4">Loại Cảnh Báo</th>
              <th className="py-3 px-4">Mức Độ</th>
              <th className="py-3 px-4">Mô Hình AI</th>
              <th className="py-3 px-4">Trạng Thái</th>
              <th className="py-3 px-4 text-right">Thao Tác</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/60">
            {filteredLogs.length === 0 ? (
              <tr>
                <td colSpan="7" className="py-8 text-center text-slate-500">
                  Không tìm thấy nhật ký sự kiện nào phù hợp.
                </td>
              </tr>
            ) : (
              filteredLogs.map((log) => (
                <tr key={log.id} className="hover:bg-slate-900/50 transition-colors group">
                  <td className="py-3 px-4 font-bold text-cyan-400">{log.id}</td>
                  <td className="py-3 px-4 text-slate-300">{log.timestamp}</td>
                  <td className="py-3 px-4 text-white font-bold">{log.eventType}</td>
                  
                  {/* Severity Badge */}
                  <td className="py-3 px-4">
                    {log.severity === 'CRITICAL' && (
                      <span className="px-2.5 py-0.5 rounded-full bg-rose-500/20 text-rose-400 font-bold border border-rose-500/30 inline-flex items-center space-x-1">
                        <AlertTriangle className="w-3 h-3" />
                        <span>CRITICAL</span>
                      </span>
                    )}
                    {log.severity === 'WARNING' && (
                      <span className="px-2.5 py-0.5 rounded-full bg-amber-400/20 text-amber-300 font-bold border border-amber-400/30 inline-flex items-center space-x-1">
                        <Info className="w-3 h-3" />
                        <span>WARNING</span>
                      </span>
                    )}
                    {log.severity === 'EMERGENCY' && (
                      <span className="px-2.5 py-0.5 rounded-full bg-blue-500/20 text-blue-400 font-bold border border-blue-500/30 inline-flex items-center space-x-1">
                        <Send className="w-3 h-3" />
                        <span>EMERGENCY</span>
                      </span>
                    )}
                    {log.severity === 'INFO' && (
                      <span className="px-2.5 py-0.5 rounded-full bg-slate-800 text-slate-300 font-bold border border-slate-700">
                        INFO
                      </span>
                    )}
                  </td>

                  <td className="py-3 px-4 text-slate-400">{log.aiModel}</td>

                  {/* Status Badge */}
                  <td className="py-3 px-4">
                    {log.status === 'CANCELLED_BY_USER' ? (
                      <span className="text-emerald-400 font-bold">✓ Đã Hủy SOS</span>
                    ) : log.status === 'TELEGRAM_SENT' ? (
                      <span className="text-blue-400 font-bold">📲 Telegram OK</span>
                    ) : (
                      <span className="text-slate-300">Đã lưu trữ</span>
                    )}
                  </td>

                  {/* Actions */}
                  <td className="py-3 px-4 text-right">
                    <button
                      onClick={() => setSelectedLog(log)}
                      className="px-2.5 py-1 glass-panel text-cyan-400 hover:text-white rounded-lg transition-colors flex items-center space-x-1 ml-auto"
                    >
                      <Eye className="w-3.5 h-3.5" />
                      <span>Chi tiết</span>
                    </button>
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>

      {/* Log Detail Modal */}
      {selectedLog && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-950/80 backdrop-blur-md">
          <div className="glass-panel w-full max-w-md rounded-3xl p-6 border border-cyan-500/40 space-y-4 relative">
            <button
              onClick={() => setSelectedLog(null)}
              className="absolute top-4 right-4 text-slate-400 hover:text-white"
            >
              <X className="w-5 h-5" />
            </button>

            <div className="flex items-center space-x-2 text-cyan-400 font-bold text-sm">
              <History className="w-5 h-5" />
              <span>Chi Tiết Sự Kiện {selectedLog.id}</span>
            </div>

            <div className="space-y-2 text-xs font-mono text-slate-300 border-y border-slate-800 py-3">
              <p><strong>Thời gian:</strong> {selectedLog.timestamp}</p>
              <p><strong>Loại sự kiện:</strong> <span className="text-white font-bold">{selectedLog.eventType}</span></p>
              <p><strong>Mô hình AI:</strong> {selectedLog.aiModel} (Confidence: {selectedLog.confidence})</p>
              <p><strong>Hành động đã thực hiện:</strong> {selectedLog.actionTaken}</p>
              <p className="text-amber-300"><strong>Ghi chú:</strong> {selectedLog.details}</p>
            </div>

            <button
              onClick={() => setSelectedLog(null)}
              className="w-full py-2.5 bg-slate-800 hover:bg-slate-700 text-white font-bold text-xs rounded-xl"
            >
              Đóng
            </button>
          </div>
        </div>
      )}

    </div>
  );
}
