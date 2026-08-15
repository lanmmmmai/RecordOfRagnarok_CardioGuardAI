#include "sos_button.h"

#include <Arduino.h>

#include "app_config.h"
#include "fall_detection/fall_detector.h"

static unsigned long pressStart = 0;
static bool fired = false;

void initSOSButton() {
    pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
}

void updateSOSButton() {
    // Active low: the BOOT button shorts the pin to ground, the internal
    // pull-up holds it high the rest of the time.
    bool pressed = (digitalRead(SOS_BUTTON_PIN) == LOW);

    if (!pressed) {
        pressStart = 0;
        fired = false;
        return;
    }

    if (pressStart == 0) {
        pressStart = millis();
        return;
    }

    // One alert per hold, not one per loop pass. Somebody in trouble will keep
    // squeezing the button, and without this flag that would queue an alert
    // every few milliseconds for as long as they held on.
    if (!fired && millis() - pressStart >= SOS_HOLD_MS) {
        fired = true;
        Serial.println(" [SOS] Physical button held, raising alert.");
        triggerSimulatedFall();
    }
}
