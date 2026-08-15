#ifndef QMI8658_SERVICE_H
#define QMI8658_SERVICE_H

#include "app_config.h"
#include "app_state.h"
#include <Wire.h>

void initQMI8658Service();
// Call every pass of loop(). Reads the IMU when a new sensor sample is due
// (QMI_SAMPLE_GAP_US) and holds the largest of the tick, without blocking.
void pollQMI8658Service();

// Call once per sensor tick. Publishes the held peak to g_watchState and
// starts a new one.
void updateQMI8658Service();

#endif // QMI8658_SERVICE_H
