#ifndef ALERT_DISPATCHER_H
#define ALERT_DISPATCHER_H

#include "app_config.h"
#include "app_state.h"

void initAlertDispatcher();
void updateAlertDispatcher();

// Clears any in-flight send so a cancelled alert leaves no state behind.
void resetAlertDispatcher();

// Queue a message for delivery. Sent immediately if Wi-Fi is up, otherwise
// persisted to NVS and retried when the network returns.
void queueAlertMessage(const String& message);

#endif // ALERT_DISPATCHER_H
