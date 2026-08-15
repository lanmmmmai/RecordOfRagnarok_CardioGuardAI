#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "app_config.h"
#include "app_state.h"

void initBatteryMonitor();
void updateBatteryMonitor();

// Called from updateBatteryMonitor() once the percentage is fresh. Sends one
// message when the pack falls to BATTERY_LOW_PCT, and re-arms only after it
// climbs back above BATTERY_CLEAR_PCT.
void checkLowBattery();

// Raw voltage at the ADC pin, before the divider ratio is applied. Exposed so
// the serial log can show it: the ratio in app_config.h is inferred, not read
// off a schematic, and this is the number you compare against a multimeter.
uint32_t getBatteryAdcMillivolts();

#endif // BATTERY_MONITOR_H
