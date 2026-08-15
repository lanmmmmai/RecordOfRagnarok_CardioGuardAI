#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "app_config.h"
#include "app_state.h"
#include <WiFi.h>
#include <time.h>

void initWiFiManager();
void updateWiFiManager();
void syncNTPTimeService();

#endif // WIFI_MANAGER_H
