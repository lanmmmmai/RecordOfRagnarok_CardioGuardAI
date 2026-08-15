#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

#include "app_config.h"
#include "app_state.h"

void initFallDetector();
void updateFallDetector();
void cancelFallAlert();
void triggerSimulatedFall();

#endif // FALL_DETECTOR_H
