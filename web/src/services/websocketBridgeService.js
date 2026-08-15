// WebSocket Bridge Service with Edge-Triggered Fall Alerts and Cancel SOS

class WebSocketBridgeService {
  constructor() {
    this.ws = null;
    const savedIP = typeof window !== 'undefined' ? localStorage.getItem('safewatch_ip') : null;
    this.ip = savedIP && savedIP.trim() !== '' ? savedIP.trim() : '192.168.20.152';
    this.port = 8080;
    this.isConnected = false;
    this.autoReconnect = true;
    this.reconnectTimer = null;
    this.telemetryListeners = new Set();
    this.fallAlertListeners = new Set();
    this.statusListeners = new Set();
    this.latestTelemetry = null;
    this.lastFallState = 0;

    // Start zero-click auto-connect immediately on load
    if (typeof window !== 'undefined') {
      setTimeout(() => {
        this.connect();
      }, 500);
    }
  }

  setIP(ipAddress) {
    if (ipAddress && ipAddress.trim() !== '') {
      this.ip = ipAddress.trim();
      if (typeof window !== 'undefined') {
        localStorage.setItem('safewatch_ip', this.ip);
      }
    }
  }

  connect(customIP) {
    if (customIP) this.setIP(customIP);

    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      return;
    }

    const wsUrl = `ws://${this.ip}:${this.port}`;
    this.notifyStatus('CONNECTING', `Đang tự động kết nối SafeWatch (${this.ip})...`);

    try {
      this.ws = new WebSocket(wsUrl);

      this.ws.onopen = () => {
        console.log(`[WebSocket Bridge] 🟢 Auto-Connected successfully to ${wsUrl}`);
        this.isConnected = true;
        this.notifyStatus('CONNECTED', `Đã tự động kết nối SafeWatch (${this.ip})`);
        
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
        this.isConnected = false;
      };

      this.ws.onclose = (event) => {
        this.isConnected = false;
        this.notifyStatus('DISCONNECTED', `Đang tự động dò tìm đồng hồ SafeWatch (${this.ip})...`);
        
        if (this.autoReconnect && !this.reconnectTimer) {
          this.reconnectTimer = setTimeout(() => {
            this.reconnectTimer = null;
            this.connect();
          }, 2500);
        }
      };
    } catch (err) {
      this.isConnected = false;
      if (this.autoReconnect && !this.reconnectTimer) {
        this.reconnectTimer = setTimeout(() => {
          this.reconnectTimer = null;
          this.connect();
        }, 2500);
      }
    }
  }

  disconnect(manual = true) {
    this.autoReconnect = !manual;
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.isConnected = false;
    this.latestTelemetry = null;
    if (manual) {
      this.notifyStatus('DISCONNECTED', 'Đã ngắt kết nối');
    }
  }

  handleIncomingData(data) {
    if (!data) return;

    if (data.type === 'telemetry' || data.pulse !== undefined) {
      const currentFallState = data.fallState || 0;

      const telemetry = {
        pulse: data.pulse,
        spo2: data.spo2,
        quality: data.quality,
        skinContact: Boolean(data.skinContact),
        hrValid: Boolean(data.hrValid),
        spo2Valid: Boolean(data.spo2Valid),
        motionArtifact: Boolean(data.motionArtifact),
        accel: data.accel || { x: 0, y: 0, z: 0 },
        gyro: data.gyro || { x: 0, y: 0, z: 0 },
        sensors: data.sensors || { imuOk: true, hrOk: true, touchOk: true },
        touch: data.touch || { touched: false, x: 0, y: 0, gesture: "NONE" },
        screen: data.screen !== undefined ? data.screen : 0,
        fallState: currentFallState,
        countdown: data.countdown || 15,
        battery: data.battery || 0,
        voltage: data.voltage || 0.0,
        charging: Boolean(data.charging),
        rssi: data.rssi || 0,
        uptime: data.uptime || 0,
        ip: data.ip || this.ip,
        timestamp: new Date().toLocaleTimeString('vi-VN')
      };
      this.latestTelemetry = telemetry;
      this.telemetryListeners.forEach(fn => fn(telemetry));

      // CRITICAL FIX: Only fire fall alert listeners on EDGE TRIGGER (rising edge)
      if (data.type === 'fall_alert' || (currentFallState >= 2 && this.lastFallState < 2)) {
        console.log('[WebSocket Bridge] 🚨 New Fall Incident Alert (Edge Triggered)');
        this.fallAlertListeners.forEach(fn => fn(data));
      }

      this.lastFallState = currentFallState;
    }
  }

  sendCancelSOS() {
    this.lastFallState = 0;
    if (this.ws && this.isConnected) {
      this.ws.send(JSON.stringify({ command: 'cancel_sos' }));
      console.log('[WebSocket Bridge] Sent cancel_sos command to ESP32-S3 Watch');
    }
  }

  sendTriggerSOS() {
    if (this.ws && this.isConnected) {
      this.ws.send(JSON.stringify({ command: 'trigger_sos' }));
      console.log('[WebSocket Bridge] Sent trigger_sos command to ESP32-S3 Watch');
    }
  }

  onTelemetry(fn) {
    this.telemetryListeners.add(fn);
    if (this.latestTelemetry) fn(this.latestTelemetry);
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
