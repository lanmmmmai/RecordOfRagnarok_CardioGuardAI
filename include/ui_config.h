#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include <Arduino.h>

// ==========================================
// CIRCULAR DISPLAY SPECIFICATIONS (240x240)
// ==========================================
#define SCREEN_WIDTH         240
#define SCREEN_HEIGHT        240
#define SCREEN_CENTER_X      120
#define SCREEN_CENTER_Y      120
#define SCREEN_RADIUS        120

// Safe Area (Section 2.2 UI Spec)
#define SAFE_AREA_X_MIN      20
#define SAFE_AREA_X_MAX      220
#define SAFE_AREA_Y_MIN      20
#define SAFE_AREA_Y_MAX      220

// ==========================================
// MANDATORY SEMANTIC COLOR PALETTE (RGB565)
// Section 2.3 UI Spec
// ==========================================
#define UI_COLOR_BLACK       0x0000 // #000000 Background
#define UI_COLOR_WHITE       0xFFFF // #FFFFFF Primary Text
#define UI_COLOR_BLUE        0x24BD // #2196F3 Primary Accent / Links
#define UI_COLOR_HEART       0xF9C6 // #FF3B30 Heart Rate Red
#define UI_COLOR_SPO2        0x055F // #00A8FF SpO2 Oxygen Cyan/Blue
#define UI_COLOR_NORMAL      0x3E9A // #39D353 Safe / Normal Green
#define UI_COLOR_WARNING     0xFD84 // #FFB020 Warning Amber/Yellow
#define UI_COLOR_CRITICAL    0xF9C6 // #FF3B30 Critical Alert Red
#define UI_COLOR_SECONDARY   0x9CF3 // #9E9E9E Secondary Text Grey
#define UI_COLOR_DARK_CARD   0x10A2 // Charcoal Card Background
#define UI_COLOR_BEZEL       0x18E3 // Concentric Bezel Ring Grey

#endif // UI_CONFIG_H
