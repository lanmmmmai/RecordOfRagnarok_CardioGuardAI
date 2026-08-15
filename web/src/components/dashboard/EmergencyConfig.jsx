import React, { useState } from 'react';
import { Send, PhoneCall, UserPlus, ShieldAlert, CheckCircle2, Copy } from 'lucide-react';
import { emergencyContacts as initialContacts } from '../../data/mockPersonalData';
import confetti from 'canvas-confetti';

export default function EmergencyConfig() {
  const [contacts, setContacts] = useState(initialContacts);
  const [botToken, setBotToken] = useState('7890123456:AAFx_CardioGuardAI_BotKey_INT8');
  const [chatId, setChatId] = useState('-1001987654321');
  const [testSent, setTestSent] = useState(false);

  const handleTestTelegram = () => {
    setTestSent(true);
    confetti({
      particleCount: 80,
      spread: 60,
      origin: { y: 0.7 }
    });
    setTimeout(() => setTestSent(false), 4000);
  };

  return (
    <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
      
      {/* Left Column: Emergency Contacts List (7 Cols) */}
      <div className="lg:col-span-7 glass-panel rounded-3xl p-6 border border-slate-800 space-y-4">
        
        <div className="flex items-center justify-between border-b border-slate-800 pb-3">
          <div className="flex items-center space-x-2">
            <PhoneCall className="w-5 h-5 text-rose-500" />
            <h3 className="text-lg font-bold text-white">DANH SÁCH LIÊN HỆ KHẨN CẤP (SOS CONTACTS)</h3>
          </div>
          <span className="text-xs text-slate-400 font-mono">Tự động gọi/nhắn khi có sự cố</span>
        </div>

        <div className="space-y-3">
          {contacts.map((c) => (
            <div
              key={c.id}
              className={`p-4 rounded-2xl border flex items-center justify-between ${
                c.isPrimary
                  ? 'glass-panel-glow border-cyan-500/40 bg-slate-900/90'
                  : 'glass-panel border-slate-800'
              }`}
            >
              <div className="space-y-1">
                <div className="flex items-center space-x-2">
                  <h4 className="text-sm font-bold text-white">{c.name}</h4>
                  {c.isPrimary && (
                    <span className="px-2 py-0.5 rounded bg-cyan-500/20 text-cyan-400 font-mono text-[10px] font-bold">
                      ƯU TIÊN 1
                    </span>
                  )}
                </div>
                <p className="text-xs text-slate-400">{c.role}</p>
                <div className="flex items-center space-x-3 text-xs font-mono pt-1">
                  <span className="text-emerald-400 font-bold">📞 {c.phone}</span>
                  <span className="text-blue-400 font-bold">📲 {c.telegramUsername}</span>
                </div>
              </div>

              <button className="px-3 py-1.5 glass-panel text-xs text-slate-300 hover:text-white rounded-xl">
                Sửa
              </button>
            </div>
          ))}
        </div>

      </div>

      {/* Right Column: Telegram Bot Config (5 Cols) */}
      <div className="lg:col-span-5 glass-panel rounded-3xl p-6 border border-slate-800 space-y-4">
        
        <div className="flex items-center space-x-2 border-b border-slate-800 pb-3">
          <Send className="w-5 h-5 text-blue-400" />
          <h3 className="text-lg font-bold text-white">CẤU HÌNH TELEGRAM BOT API</h3>
        </div>

        <div className="space-y-3 text-xs font-mono">
          <div>
            <label className="block text-slate-400 mb-1">TELEGRAM BOT TOKEN (HTTPS SSL)</label>
            <input
              type="text"
              value={botToken}
              onChange={(e) => setBotToken(e.target.value)}
              className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-xl text-cyan-400 font-mono text-xs focus:outline-none focus:border-blue-500"
            />
          </div>

          <div>
            <label className="block text-slate-400 mb-1">GROUP CHAT ID GIA ĐÌNH & BÁC SĨ</label>
            <input
              type="text"
              value={chatId}
              onChange={(e) => setChatId(e.target.value)}
              className="w-full px-3 py-2 bg-slate-950 border border-slate-800 rounded-xl text-cyan-400 font-mono text-xs focus:outline-none focus:border-blue-500"
            />
          </div>

          <div className="pt-2">
            <button
              onClick={handleTestTelegram}
              className="w-full py-3 bg-gradient-to-r from-blue-600 to-indigo-600 hover:from-blue-500 hover:to-indigo-500 text-white font-bold text-xs rounded-xl shadow-lg shadow-blue-500/20 flex items-center justify-center space-x-2 transition-all"
            >
              <Send className="w-4 h-4" />
              <span>GỬI THỬ TIN NHẮN TELEGRAM TEST</span>
            </button>
          </div>

          {testSent && (
            <div className="p-3 bg-blue-500/20 border border-blue-500/40 rounded-xl text-blue-300 flex items-center space-x-2 text-xs">
              <CheckCircle2 className="w-4 h-4 text-blue-400" />
              <span>Đã gửi tin nhắn Telegram thử nghiệm thành công (HTTPS 200 OK)!</span>
            </div>
          )}
        </div>

      </div>

    </div>
  );
}
