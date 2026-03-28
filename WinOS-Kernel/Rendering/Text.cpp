#include "Renderer.h"

void PrintPos(const char* str, unsigned int x, unsigned int y, uint32_t color, BOOT_CONFIG* config) {
    int CursorX = x;

    while (*str != '\0') {
        PutChar(*str, CursorX, y, color, config);
        CursorX += 8; // Standard PSF1 width is 8px
        str++;
    }
}

void PutChar(char c, unsigned int xOff, unsigned int yOff, uint32_t color, BOOT_CONFIG* config) {
    if (config == nullptr || config->Font == nullptr) return;

    PSF1_HEADER* font = config->Font;

    // THE "RESISTOR" FIX: Cast 'c' to unsigned char.
    // Without this, characters > 127 are negative, sending the pointer into random memory.
    unsigned char* glyphPtr = (unsigned char*)font + sizeof(PSF1_HEADER) +
        ((unsigned char)c * font->CharSize);

    for (unsigned long y = yOff; y < yOff + font->CharSize; y++) {
        for (unsigned long x = xOff; x < xOff + 8; x++) {
            // Bitmask check: is the bit for this pixel set?
            if ((*glyphPtr & (0b10000000 >> (x - xOff))) > 0) {
                // Buffer check to prevent out-of-bounds writes
                if (x < config->Width && y < config->Height) {
                    config->BackBuffer[x + (y * config->PixelsPerScanLine)] = color;
                }
            }
        }
        glyphPtr++;
    }
}