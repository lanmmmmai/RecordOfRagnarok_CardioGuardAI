#include "cst816s_service.h"
#include "ui/ui_manager.h"

static bool cstDetected = false;
static volatile bool cstTouchPending = false;

// True once this contact has already produced a navigation. Cleared when the
// finger leaves, by whichever of the two lift paths in updateCST816SService()
// notices first. File scope rather than function-static because the INT-idle
// early return has to be able to clear it: that path never reaches the register
// read, and a latch only the register read could clear would stay stuck through
// every lift that happens between interrupts -- freezing touch entirely until
// the next contact. See the long comment at its use site for why the latch is
// per-contact and not per-gesture-code.
static bool gestureHandledThisContact = false;

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

    // INT idle and nothing pending: the glass is definitively untouched. This
    // is the reliable lift edge -- the register read below only sees a lift if
    // a poll happens to land while the controller still reports points == 0,
    // which a fast finger can skip entirely.
    if (!cstTouchPending && digitalRead(TOUCH_INT_PIN) == HIGH) {
        g_watchState.touched = false;
        g_watchState.gestureID = 0;
        gestureHandledThisContact = false;
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

        // Fire navigation once per *contact*, not once per gesture code.
        //
        // The controller keeps reporting the same gesture for as long as the
        // finger rests on the glass, so something has to suppress the repeats
        // or one swipe walks the UI through several screens. The previous guard
        // was `gesture != handledGesture`, which suppressed the repeats and the
        // second of two identical swipes along with them: swipe left, lift,
        // swipe left again, and the second one is dropped unless a poll happens
        // to land in the gap with points == 0 to clear the latch. That gap is a
        // few tens of milliseconds of an unhurried swipe and can vanish
        // entirely on a quick one -- and repeating the same direction is
        // exactly how the carousel is meant to be used, so the failure lands on
        // the most common action there is. A wearer swiping left twice and
        // arriving one screen short reads it as a watch that ignores them.
        //
        // Latching on contact instead makes the rule "one navigation per touch,
        // whatever the finger does next", which holds however fast the swipes
        // come.
        if (gesture > 0) {
            g_watchState.gestureID = gesture;
            g_watchState.touchX = x;
            g_watchState.touchY = y;
            g_watchState.gestureName = getGestureNameStr(gesture);
            g_watchState.touched = true;

            if (!gestureHandledThisContact) {
                gestureHandledThisContact = true;
                Serial.printf(" [TOUCH] Gesture: 0x%02X (%s) | X: %d, Y: %d\n",
                              gesture, g_watchState.gestureName.c_str(), x, y);
                handleUITouchInput(x, y, gesture);
            }
        } else if (points > 0) {
            // Finger down but the controller has not decided on a gesture yet.
            // Not a lift, so the latch stays as it is.
            g_watchState.touchX = x;
            g_watchState.touchY = y;
            g_watchState.touched = true;
        } else {
            // Finger lifted: arm the next gesture.
            gestureHandledThisContact = false;
            g_watchState.touched = false;
            g_watchState.gestureID = 0;
        }
    }
}
