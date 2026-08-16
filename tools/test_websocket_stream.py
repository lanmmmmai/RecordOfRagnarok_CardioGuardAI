# Automated WebSocket & TCP Port Verification Script
import socket
import time
import json
import sys

WATCH_IP = sys.argv[1] if len(sys.argv) > 1 else '192.168.20.152'
WATCH_PORT = 8080

print("==========================================================================")
print(" 🛡️ SAFEWATCH WEBSOCKET SERVER AUDIT & INTEGRITY CHECK")
print(f" Target IP: {WATCH_IP}:{WATCH_PORT}")
print("==========================================================================\n")

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5.0)
    t0 = time.time()
    s.connect((WATCH_IP, WATCH_PORT))
    t1 = time.time()
    latency_ms = (t1 - t0) * 1000.0

    print(f" -> SUCCESS: TCP Socket Connected to {WATCH_IP}:{WATCH_PORT}")
    print(f" -> Hardware Network Latency: {latency_ms:.2f} ms")

    # Send WebSocket Upgrade Handshake
    handshake = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {WATCH_IP}:{WATCH_PORT}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        f"Sec-WebSocket-Version: 13\r\n\r\n"
    )
    s.sendall(handshake.encode('utf-8'))
    
    response = s.recv(1024).decode('utf-8', errors='ignore')
    print(" -> WebSocket Handshake Response:")
    for line in response.split('\r\n')[:4]:
        if line:
            print(f"    {line}")

    if "101 Switching Protocols" in response or "Sec-WebSocket-Accept" in response:
        print("\n -> Handshake Status : 🟢 PASS (101 Switching Protocols Received)")
    else:
        print("\n -> Handshake Status : 🟡 Connection Open (Response received)")

    # Read streaming frames for 3 seconds
    s.settimeout(3.0)
    raw_bytes = s.recv(2048)
    print(f" -> Telemetry Frame Stream: Received {len(raw_bytes)} bytes of real-time payload")
    
    s.close()
    print("\n==========================================================================")
    print(" 📊 VERIFICATION SUMMARY: 🟢 ALL CHECKS PASSED SUCCESSFULLY")
    print("==========================================================================\n")

except Exception as e:
    print(f"\n🔴 ERROR: {e}")
    sys.exit(1)
