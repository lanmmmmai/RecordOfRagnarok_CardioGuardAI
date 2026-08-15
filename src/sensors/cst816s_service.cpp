#include "cst816s_service.h"
#include "ui/ui_manager.h"

static bool cstDetected = false;
static volatile bool cstTouchPending = false;

void IRAM_ATTR cst816s_isr() {
    cstTouchPending = true;
}

static const char* getGestureNameStr(uint8_t g) {
    switch (g) {
        case 0x01: return "SWIPE UP";
        case 0x02: return "SWIPE DOWN";
        case 0x03: return "SWIPE LEFT";
        case 0x04: return "SWIPE RIGHT";
        case 0x05: return "SINGLE CLICK";
        case 0x0B: return "DOUBLE CLICK";
        case 0x0C: return "LONG PRESS";
        default:   return "NONE";
    }
}

void initCST816SService() {
    pinMode(TOUCH_RST_PIN, OUTPUT);
    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);

    digitalWrite(TOUCH_RST_PIN, LOW);
    delay(20);
    digitalWrite(TOUCH_RST_PIN, HIGH);
    delay(50);

    Wire.beginTransmission(CST816S_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        cstDetected = true;
        attachInterrupt(digitalPinToInterrupt(TOUCH_INT_PIN), cst816s_isr, FALLING);
        Serial.println(" -> SUCCESS: CST816S Touch Service Active (Ultra-Fast 80Hz Polling).");
    } else {
        cstDetected = false;
        Serial.println(" -> ERROR: CST816S Touch Controller Not Found on I2C1 (0x15)!");
    }
    g_watchState.touchOk = cstDetected;
}

void updateCST816SService() {
    if (!cstDetected) return;

    if (!cstTouchPending && digitalRead(TOUCH_INT_PIN) == HIGH) {
        g_watchState.touched = false;
        return;
    }

    cstTouchPending = false;

    Wire.beginTransmission(CST816S_I2C_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) return;

    if (Wire.requestFrom((uint8_t)CST816S_I2C_ADDR, (uint8_t)6) == 6) {
        uint8_t raw[6];
        for (int i = 0; i < 6; i++) raw[i] = Wire.read();

        uint8_t gesture = raw[0];
        uint8_t points  = raw[1];
        uint16_t x = ((raw[2] & 0x0F) << 8) | raw[3];
        uint16_t y = ((raw[4] & 0x0F) << 8) | raw[5];

        // Fire navigation once per gesture, on the leading edge. A time gate
        // alone is not enough: the controller keeps reporting the same gesture
        // while the finger rests on the glass, which would walk the UI through
        // several screens from a single swipe.
        static uint8_t handledGesture = 0;

        if (gesture > 0) {
            g_watchState.gestureID = gesture;
            g_watchState.touchX = x;
            g_watchState.touchY = y;
            g_watchState.gestureName = getGestureNameStr(gesture);
            g_watchState.touched = true;

            if (gesture != handledGesture) {
                handledGesture = gesture;
                Serial.printf(" [TOUCH] Gesture: 0x%02X (%s) | X: %d, Y: %d\n",
                              gesture, g_watchState.gestureName.c_str(), x, y);
                handleUITouchInput(x, y, gesture);
            }
        } else if (points > 0) {
            g_watchState.touchX = x;
            g_watchState.touchY = y;
            g_watchState.touched = true;
        } else {
            // Finger lifted: arm the next gesture.
            handledGesture = 0;
            g_watchState.touched = false;
        }
    }
}
