#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "app_config.h"
#include "app_state.h"
#include "ui_config.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

void initUIManager();
void renderUI();
void handleUITouchInput(int touchX, int touchY, uint8_t gestureID);

#endif // UI_MANAGER_H
