#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "app_config.h"
#include "app_state.h"

void initBLEService();
void updateBLEService();

// Pushes a fall event to the companion app. Uses indicate rather than notify
// so the phone has to acknowledge receipt.
void bleNotifyFallEvent(uint8_t eventType);

#endif // BLE_SERVICE_H
