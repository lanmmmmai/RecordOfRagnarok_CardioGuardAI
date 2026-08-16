/**
 * Automated Verification & Telemetry Audit Test Script
 * Target: ESP32-S3 SafeWatch WebSocket Server (ws://192.168.244.152:8080)
 */

const WebSocket = require('ws');

const WATCH_IP = process.argv[2] || '192.168.244.152';
const WATCH_PORT = 8080;
const WS_URL = `ws://${WATCH_IP}:${WATCH_PORT}`;

console.log(`==========================================================================`);
console.log(` 🛡️ SAFEWATCH WEBSOCKET TELEMETRY INTEGRITY & AUDIT TEST`);
console.log(` Target Server: ${WS_URL}`);
console.log(`==========================================================================\n`);

const ws = new WebSocket(WS_URL);

let packetCount = 0;
const MAX_PACKETS = 10;
const timestamps = [];
const errors = [];

ws.on('open', () => {
  console.log(` -> SUCCESS: Connected to ESP32-S3 WebSocket Server at ${WS_URL}`);
  console.log(` -> Collecting ${MAX_PACKETS} sample JSON packets for structural & physiological audit...\n`);
});

ws.on('message', (data) => {
  packetCount++;
  const recvTime = Date.now();
  timestamps.push(recvTime);

  try {
    const jsonStr = data.toString('utf8');
    const payload = JSON.parse(jsonStr);

    console.log(`[PACKET #${packetCount.toString().padStart(2, '0')}] Raw Bytes: ${data.length} B`);
    console.log(`   ├─ Pulse (BPM)   : ${payload.pulse ?? 'MISSING'}`);
    console.log(`   ├─ SpO2 (%)      : ${payload.spo2 ?? 'MISSING'}%`);
    console.log(`   ├─ SQI Quality   : ${payload.quality ?? 'MISSING'}%`);
    console.log(`   ├─ Skin Contact  : ${payload.skinContact ? 'YES (Touch)' : 'NO (Open Air)'}`);
    console.log(`   ├─ Accel 3D (g)  : X=${payload.accel?.x ?? 'N/A'}, Y=${payload.accel?.y ?? 'N/A'}, Z=${payload.accel?.z ?? 'N/A'}`);
    console.log(`   ├─ Gyro 3D (dps) : X=${payload.gyro?.x ?? 'N/A'}, Y=${payload.gyro?.y ?? 'N/A'}, Z=${payload.gyro?.z ?? 'N/A'}`);
    console.log(`   ├─ Battery Level : ${payload.battery}%`);
    console.log(`   └─ IP & RSSI     : ${payload.ip} (${payload.rssi} dBm)\n`);

    // Integrity Checks
    if (payload.type !== 'telemetry') errors.push(`Packet #${packetCount}: Expected type='telemetry', got '${payload.type}'`);
    if (typeof payload.pulse !== 'number' || payload.pulse < 0 || payload.pulse > 250) errors.push(`Packet #${packetCount}: Invalid pulse ${payload.pulse}`);
    if (typeof payload.spo2 !== 'number' || payload.spo2 < 0 || payload.spo2 > 100) errors.push(`Packet #${packetCount}: Invalid SpO2 ${payload.spo2}`);
    if (!payload.accel || typeof payload.accel.z !== 'number') errors.push(`Packet #${packetCount}: Missing or malformed accel object`);

  } catch (err) {
    errors.push(`Packet #${packetCount}: JSON Parse Error - ${err.message}`);
  }

  if (packetCount >= MAX_PACKETS) {
    ws.close();
  }
});

ws.on('close', () => {
  console.log(`==========================================================================`);
  console.log(` 📊 AUDIT & TEST REPORT SUMMARY`);
  console.log(`==========================================================================`);
  console.log(` Total Packets Received : ${packetCount} / ${MAX_PACKETS}`);
  
  if (timestamps.length > 1) {
    let intervals = [];
    for (let i = 1; i < timestamps.length; i++) {
      intervals.push(timestamps[i] - timestamps[i - 1]);
    }
    const avgInterval = (intervals.reduce((a, b) => a + b, 0) / intervals.length).toFixed(1);
    const hz = (1000 / avgInterval).toFixed(1);
    console.log(` Average Packet Interval: ${avgInterval} ms (~${hz} Hz streaming frequency)`);
  }

  if (errors.length === 0) {
    console.log(` Status Integrity Check : 🟢 PASS (100% Valid JSON Structure & Physiological Fields)`);
    console.log(`==========================================================================\n`);
    process.exit(0);
  } else {
    console.log(` Status Integrity Check : 🔴 FAIL (${errors.length} errors found)`);
    errors.forEach(e => console.log(`   - ${e}`));
    console.log(`==========================================================================\n`);
    process.exit(1);
  }
});

ws.on('error', (err) => {
  console.error(`🔴 ERROR: Failed to connect to ${WS_URL}:`, err.message);
  process.exit(1);
});
