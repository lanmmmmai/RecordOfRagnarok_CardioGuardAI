"""
SafeWatch PostgreSQL Real-time Ingestion Service with Touch Event Logging
Listens to ESP32-S3 WebSocket Telemetry (ws://192.168.20.152:8080)
and writes records continuously to local PostgreSQL (safewatch_db).
"""

import asyncio
import json
import sys
import websockets
import asyncpg
from datetime import datetime

# Configuration
WATCH_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.20.152'
WATCH_PORT = 8080
WS_URL = f"ws://{WATCH_IP}:{WATCH_PORT}"

PG_HOST = "127.0.0.1"
PG_PORT = 5432
PG_USER = "postgres"
PG_PASSWORD = ""
PG_DB = "safewatch_db"

TABLE_INIT_SQL = """
CREATE TABLE IF NOT EXISTS safewatch_telemetry (
    id BIGSERIAL PRIMARY KEY,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    patient_id VARCHAR(50) DEFAULT 'CG-AI-9988-VN',
    patient_name VARCHAR(100) DEFAULT 'Nguyễn Thị Mai Lan',
    pulse INT DEFAULT 0,
    spo2 INT DEFAULT 0,
    quality INT DEFAULT 0,
    skin_contact BOOLEAN DEFAULT FALSE,
    hr_valid BOOLEAN DEFAULT FALSE,
    spo2_valid BOOLEAN DEFAULT FALSE,
    motion_artifact BOOLEAN DEFAULT FALSE,
    accel_x INT DEFAULT 0,
    accel_y INT DEFAULT 0,
    accel_z INT DEFAULT 0,
    gyro_x INT DEFAULT 0,
    gyro_y INT DEFAULT 0,
    gyro_z INT DEFAULT 0,
    touch_active BOOLEAN DEFAULT FALSE,
    touch_x INT DEFAULT 0,
    touch_y INT DEFAULT 0,
    gesture_name VARCHAR(50) DEFAULT 'NONE',
    screen_id INT DEFAULT 0,
    fall_state INT DEFAULT 0,
    countdown INT DEFAULT 15,
    battery INT DEFAULT 0,
    voltage NUMERIC(4,2) DEFAULT 0.00,
    charging BOOLEAN DEFAULT FALSE,
    rssi INT DEFAULT 0,
    uptime_sec BIGINT DEFAULT 0,
    device_ip VARCHAR(50) DEFAULT '192.168.20.152'
);

CREATE TABLE IF NOT EXISTS safewatch_fall_events (
    id BIGSERIAL PRIMARY KEY,
    occurred_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    patient_id VARCHAR(50) DEFAULT 'CG-AI-9988-VN',
    patient_name VARCHAR(100) DEFAULT 'Nguyễn Thị Mai Lan',
    event_type VARCHAR(50) NOT NULL,
    confidence_pct NUMERIC(5,2) DEFAULT 99.20,
    severity VARCHAR(20) DEFAULT 'CRITICAL',
    pulse_at_event INT DEFAULT 0,
    spo2_at_event INT DEFAULT 0,
    accel_mag_g NUMERIC(5,2) DEFAULT 0.00,
    action_taken TEXT,
    telegram_notified BOOLEAN DEFAULT TRUE,
    device_ip VARCHAR(50) DEFAULT '192.168.20.152'
);
"""

INSERT_TELEMETRY_SQL = """
INSERT INTO safewatch_telemetry (
    patient_id, patient_name, pulse, spo2, quality, skin_contact,
    hr_valid, spo2_valid, motion_artifact, accel_x, accel_y, accel_z,
    gyro_x, gyro_y, gyro_z, touch_active, touch_x, touch_y, gesture_name,
    screen_id, fall_state, countdown, battery, voltage,
    charging, rssi, uptime_sec, device_ip
) VALUES (
    $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12,
    $13, $14, $15, $16, $17, $18, $19, $20, $21, $22,
    $23, $24, $25, $26, $27, $28
);
"""

async def main():
    print("==========================================================================")
    print(" 🐘 SAFEWATCH REALTIME POSTGRESQL INGESTION SERVICE (TOUCH SYNC ACTIVE)")
    print(f" Target WebSocket : {WS_URL}")
    print(f" PostgreSQL Server: {PG_HOST}:{PG_PORT}/{PG_DB}")
    print("==========================================================================\n")

    try:
        pool = await asyncpg.create_pool(
            user=PG_USER,
            password=PG_PASSWORD,
            database=PG_DB,
            host=PG_HOST,
            port=PG_PORT,
            min_size=2,
            max_size=10
        )
        print(" -> SUCCESS: Connected to PostgreSQL Connection Pool.")
        
        async with pool.acquire() as conn:
            await conn.execute(TABLE_INIT_SQL)
            # Add columns if table existed prior
            try:
                await conn.execute("ALTER TABLE safewatch_telemetry ADD COLUMN IF NOT EXISTS touch_active BOOLEAN DEFAULT FALSE;")
                await conn.execute("ALTER TABLE safewatch_telemetry ADD COLUMN IF NOT EXISTS touch_x INT DEFAULT 0;")
                await conn.execute("ALTER TABLE safewatch_telemetry ADD COLUMN IF NOT EXISTS touch_y INT DEFAULT 0;")
                await conn.execute("ALTER TABLE safewatch_telemetry ADD COLUMN IF NOT EXISTS gesture_name VARCHAR(50) DEFAULT 'NONE';")
                await conn.execute("ALTER TABLE safewatch_telemetry ADD COLUMN IF NOT EXISTS screen_id INT DEFAULT 0;")
            except Exception:
                pass
            print(" -> SUCCESS: PostgreSQL Schemas 'safewatch_telemetry' Ready.")
    except Exception as e:
        print(f"🔴 PostgreSQL Connection Error: {e}")
        return

    while True:
        try:
            print(f"\n -> Connecting to SafeWatch WebSocket at {WS_URL}...")
            async with websockets.connect(WS_URL, ping_interval=10, ping_timeout=5) as ws:
                print(f" -> 🟢 CONNECTED to SafeWatch on {WS_URL}! Streaming telemetry & touch events to PostgreSQL...\n")
                
                count = 0
                async for message in ws:
                    try:
                        data = json.loads(message)
                        if data.get("type") == "telemetry" or "pulse" in data:
                            accel = data.get("accel", {})
                            gyro = data.get("gyro", {})
                            touch = data.get("touch", {})

                            async with pool.acquire() as conn:
                                await conn.execute(
                                    INSERT_TELEMETRY_SQL,
                                    'CG-AI-9988-VN',
                                    'Nguyễn Thị Mai Lan',
                                    int(data.get("pulse", 0)),
                                    int(data.get("spo2", 0)),
                                    int(data.get("quality", 0)),
                                    bool(data.get("skinContact", False)),
                                    bool(data.get("hrValid", False)),
                                    bool(data.get("spo2Valid", False)),
                                    bool(data.get("motionArtifact", False)),
                                    int(accel.get("x", 0)),
                                    int(accel.get("y", 0)),
                                    int(accel.get("z", 0)),
                                    int(gyro.get("x", 0)),
                                    int(gyro.get("y", 0)),
                                    int(gyro.get("z", 0)),
                                    bool(touch.get("touched", False)),
                                    int(touch.get("x", 0)),
                                    int(touch.get("y", 0)),
                                    str(touch.get("gesture", "NONE")),
                                    int(data.get("screen", 0)),
                                    int(data.get("fallState", 0)),
                                    int(data.get("countdown", 15)),
                                    int(data.get("battery", 0)),
                                    float(data.get("voltage", 0.0)),
                                    bool(data.get("charging", False)),
                                    int(data.get("rssi", 0)),
                                    int(data.get("uptime", 0)),
                                    str(data.get("ip", WATCH_IP))
                                )

                            count += 1
                            if touch.get("touched"):
                                print(f" 👉 [TOUCH EVENT SAVED] X={touch.get('x')} Y={touch.get('y')} Gesture={touch.get('gesture')} Screen={data.get('screen')}")
                            elif count % 20 == 0:
                                print(f" [DB INGEST] Saved {count} records | Pulse: {data.get('pulse', 0)} BPM | SpO2: {data.get('spo2', 0)}% | Skin: {data.get('skinContact', False)}")

                    except Exception as parse_err:
                        print(f"⚠️ JSON Parse / Insert Error: {parse_err}")

        except Exception as ws_err:
            print(f"🟡 WebSocket Connection dropped ({ws_err}). Reconnecting in 3s...")
            await asyncio.sleep(3.0)

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n -> Ingestion Service Stopped.")
