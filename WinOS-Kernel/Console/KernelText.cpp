#include <stdint.h>
#include "Shared.h"
#include "KernelText.h"

extern BOOT_CONFIG* gConfig;

void PutChar(char c, unsigned int xOff, unsigned int yOff, uint32_t color) {
    PSF1_HEADER* font = gConfig->Font;
    // Glyph location: Header + (Index * Height)
    unsigned char* glyphPtr = (unsigned char*)font + sizeof(PSF1_HEADER) + (c * font->CharSize);

    for (unsigned long y = yOff; y < yOff + font->CharSize; y++) {
        for (unsigned long x = xOff; x < xOff + 8; x++) {
            // Bitmask check: is the bit for this pixel set?
            if ((*glyphPtr & (0b10000000 >> (x - xOff))) > 0) {
                gConfig->BaseAddress[x + (y * gConfig->PixelsPerScanLine)] = color;
            }
        }
        glyphPtr++;
    }
}

void Print(const char* str, unsigned int x, unsigned int y, uint32_t color) {
    // SANITY CHECK: If these are null, the kernel has corrupted the config
    if (gConfig == nullptr || gConfig->Font == nullptr || gConfig->BaseAddress == nullptr) {
        return;
    }

    unsigned int xOffset = 0;
    while (*str != '\0') {
        PutChar(*str, x + xOffset, y, color);
        xOffset += 8; // standard PSF1 width is 8px
        str++;
    }
}