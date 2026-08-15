#include "vn_font.h"

#include "fonts/vn_font_small.h"
#include "fonts/vn_font_medium.h"
#include "fonts/vn_font_large.h"

// A snapshot of everything TFT_eSPI needs to render one smooth font. The
// members it reads are all public, so a loaded font can be captured here and
// handed back later without touching the allocator again.
struct FontSlot {
    TFT_eSPI::fontMetrics metrics;
    uint16_t* unicode;
    uint8_t*  height;
    uint8_t*  width;
    uint8_t*  xAdvance;
    int16_t*  dY;
    int8_t*   dX;
    uint32_t* bitmap;
    bool      valid;
};

static FontSlot slots[3];
static bool fontsReady = false;

static bool captureSlot(TFT_eSprite& spr, const uint8_t* array, FontSlot& slot) {
    spr.loadFont(array);

    if (!spr.fontLoaded || spr.gUnicode == nullptr || spr.gBitmap == nullptr) {
        return false;
    }

    slot.metrics  = spr.gFont;
    slot.unicode  = spr.gUnicode;
    slot.height   = spr.gHeight;
    slot.width    = spr.gWidth;
    slot.xAdvance = spr.gxAdvance;
    slot.dY       = spr.gdY;
    slot.dX       = spr.gdX;
    slot.bitmap   = spr.gBitmap;
    slot.valid    = true;

    // Detach the arrays from the sprite before the next loadFont() call, which
    // begins with unloadFont() and would otherwise free the tables we just took
    // ownership of. Nulling the pointers as well as clearing the flag means
    // unloadFont() finds nothing to free even if it is called directly.
    spr.fontLoaded = false;
    spr.gUnicode = nullptr;
    spr.gHeight = nullptr;
    spr.gWidth = nullptr;
    spr.gxAdvance = nullptr;
    spr.gdY = nullptr;
    spr.gdX = nullptr;
    spr.gBitmap = nullptr;

    return true;
}

bool initVnFonts(TFT_eSprite& spr) {
    uint32_t heapBefore = ESP.getFreeHeap();

    bool ok = captureSlot(spr, VN_FONT_SMALL,  slots[VN_SMALL]);
    ok     &= captureSlot(spr, VN_FONT_MEDIUM, slots[VN_MEDIUM]);
    ok     &= captureSlot(spr, VN_FONT_LARGE,  slots[VN_LARGE]);

    fontsReady = ok;

    if (ok) {
        Serial.printf(" -> SUCCESS: Vietnamese fonts loaded (%u glyphs each, %u B heap).\n",
                      (unsigned)slots[VN_MEDIUM].metrics.gCount,
                      (unsigned)(heapBefore - ESP.getFreeHeap()));
        vnFont(spr, VN_MEDIUM);
    } else {
        // Falling back to the ASCII fonts drops the diacritics but keeps the
        // watch readable, which matters more than typography on a device
        // somebody is relying on.
        Serial.println(" -> WARNING: Vietnamese font load FAILED, using ASCII fonts.");
    }

    return ok;
}

void vnFont(TFT_eSprite& spr, VnFontSize size) {
    if (!fontsReady || size > VN_LARGE) return;

    const FontSlot& slot = slots[size];
    if (!slot.valid) return;

    spr.gFont     = slot.metrics;
    spr.gUnicode  = slot.unicode;
    spr.gHeight   = slot.height;
    spr.gWidth    = slot.width;
    spr.gxAdvance = slot.xAdvance;
    spr.gdY       = slot.dY;
    spr.gdX       = slot.dX;
    spr.gBitmap   = slot.bitmap;
    spr.fontLoaded = true;
}
