// WebSocket Bridge Service for ESP32-S3 SafeWatch Local Wi-Fi Telemetry

class WebSocketBridgeService {
  constructor() {
    this.ws = null;
    this.ip = '192.168.20.152';
    this.port = 8080;
    this.isConnected = false;
    this.reconnectTimer = null;
    this.telemetryListeners = new Set();
    this.fallAlertListeners = new Set();
    this.statusListeners = new Set();
  }

  setIP(ipAddress) {
    if (ipAddress && ipAddress.trim() !== '') {
      this.ip = ipAddress.trim();
    }
  }

  connect(customIP) {
    if (customIP) this.setIP(customIP);

    this.disconnect(false);

    const wsUrl = `ws://${this.ip}:${this.port}`;
    console.log(`[WebSocket Bridge] Connecting to ${wsUrl}...`);
    this.notifyStatus('CONNECTING', `Đang kết nối Wi-Fi Local: ${wsUrl}`);

    try {
      this.ws = new WebSocket(wsUrl);

      this.ws.onopen = () => {
        console.log(`[WebSocket Bridge] Connected successfully to ${wsUrl}`);
        this.isConnected = true;
        this.notifyStatus('CONNECTED', `Đã kết nối Wi-Fi Local với SafeWatch (${this.ip})`);
        
        if (this.reconnectTimer) {
          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = null;
        }
      };

      this.ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          this.handleIncomingData(data);
        } catch (err) {
          console.warn('[WebSocket Bridge] Failed to parse JSON:', event.data);
        }
      };

      this.ws.onerror = (err) => {
        console.error('[WebSocket Bridge] Error:', err);
        this.notifyStatus('ERROR', `Lỗi kết nối Wi-Fi (${wsUrl})`);
      };

      this.ws.onclose = (event) => {
        console.log('[WebSocket Bridge] Connection closed');
        this.isConnected = false;
        this.notifyStatus('DISCONNECTED', `Mất kết nối Wi-Fi với SafeWatch (${this.ip})`);
      };
    } catch (err) {
      console.error('[WebSocket Bridge] Exception during connection:', err);
      this.notifyStatus('ERROR', `Khởi tạo kết nối thất bại`);
    }
  }

  disconnect(manual = true) {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.isConnected = false;
    if (manual) {
      this.notifyStatus('DISCONNECTED', 'Đã ngắt kết nối Wi-Fi');
    }
  }

  handleIncomingData(data) {
    if (!data) return;

    if (data.type === 'telemetry' || data.pulse !== undefined) {
      const telemetry = {
        pulse: data.pulse || 86,
        spo2: data.spo2 || 98,
        quality: data.quality || 100,
        skinContact: data.skinContact ?? true,
        accel: data.accel || { x: 0, y: 0, z: -4096 },
        gyro: data.gyro || { x: 0, y: 0, z: 0 },
        fallState: data.fallState || 0,
        battery: data.battery || 100,
        rssi: data.rssi || -55,
        ip: data.ip || this.ip,
        timestamp: new Date().toLocaleTimeString('vi-VN')
      };
      this.telemetryListeners.forEach(fn => fn(telemetry));
    }

    if (data.type === 'fall_alert' || data.fallState === 2 || data.fallState === 3) {
      this.fallAlertListeners.forEach(fn => fn(data));
    }
  }

  sendCancelSOS() {
    if (this.ws && this.isConnected) {
      this.ws.send(JSON.stringify({ command: 'cancel_sos' }));
      console.log('[WebSocket Bridge] Sent cancel_sos command');
    }
  }

  sendTriggerSOS() {
    if (this.ws && this.isConnected) {
      this.ws.send(JSON.stringify({ command: 'trigger_sos' }));
      console.log('[WebSocket Bridge] Sent trigger_sos command');
    }
  }

  onTelemetry(fn) {
    this.telemetryListeners.add(fn);
    return () => this.telemetryListeners.delete(fn);
  }

  onFallAlert(fn) {
    this.fallAlertListeners.add(fn);
    return () => this.fallAlertListeners.delete(fn);
  }

  onStatusChange(fn) {
    this.statusListeners.add(fn);
    return () => this.statusListeners.delete(fn);
  }

  notifyStatus(status, message) {
    this.statusListeners.forEach(fn => fn({ status, message, isConnected: this.isConnected, ip: this.ip }));
  }
}

export const websocketBridgeService = new WebSocketBridgeService();
