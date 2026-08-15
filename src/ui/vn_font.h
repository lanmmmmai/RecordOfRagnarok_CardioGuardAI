#ifndef VN_FONT_H
#define VN_FONT_H

#include <TFT_eSPI.h>

// Vietnamese text rendering.
//
// The built-in TFT_eSPI fonts (LOAD_GLCD / FONT2 / FONT4) are ASCII only, so
// every accented character came out as garbage -- which is why the screens used
// to say "Da bao nguoi than". These are TFT_eSPI "smooth fonts": anti-aliased,
// with a Unicode lookup table that can hold the scattered code points
// Vietnamese needs. See tools/make_vlw.py for how they are generated.
//
// TFT_eSPI holds exactly one smooth font at a time, and loadFont() mallocs
// seven metric arrays on every call. Calling it per string at 30 FPS would mean
// hundreds of allocations per second and steady heap churn. Instead all three
// sizes are loaded once at boot and vnFont() swaps the already-built metric
// tables in and out, which is a handful of pointer assignments.

enum VnFontSize {
    VN_SMALL = 0,   // replaces built-in font 1 -- hints, footnotes
    VN_MEDIUM,      // replaces built-in font 2 -- labels, status lines
    VN_LARGE        // replaces built-in font 4 -- clock, vitals figures
};

// Loads all three sizes. Call once, after the sprite exists. Returns false if
// any size failed to allocate its metric tables, in which case text falls back
// to the built-in ASCII fonts rather than disappearing.
bool initVnFonts(TFT_eSprite& spr);

// Selects the active size. Cheap enough to call before every string.
void vnFont(TFT_eSprite& spr, VnFontSize size);

#endif  // VN_FONT_H
