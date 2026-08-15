#ifndef SOS_BUTTON_H
#define SOS_BUTTON_H

// Physical SOS on the board's BOOT button.
//
// The touchscreen already offers SOS through the Quick Menu, but a person who
// has just fallen may not be able to see the screen, aim at a target, or use
// the hand the watch is not on. A button you can find by feel and press with a
// knuckle is a different capability, not a duplicate one.

// ⚠️ Must be called from the END of setup(), never the beginning. See the
// comment on SOS_BUTTON_PIN in app_config.h.
void initSOSButton();

void updateSOSButton();

#endif  // SOS_BUTTON_H
