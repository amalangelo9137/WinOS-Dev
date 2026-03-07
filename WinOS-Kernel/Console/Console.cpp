#include "Console.h"

// Initialize static members
int Console::CursorX = 0;
int Console::CursorY = 0;
BOOT_CONFIG* Console::Config = nullptr;

void Console::Init(BOOT_CONFIG* config) {
    Config = config;
    CursorX = 0;
    CursorY = 0;
}

void Console::PutChar(char c, unsigned int xOff, unsigned int yOff, uint32_t color) {
    if (Config == nullptr || Config->Font == nullptr) return;

    PSF1_HEADER* font = Config->Font;

    // THE "RESISTOR" FIX: Cast 'c' to unsigned char.
    // Without this, characters > 127 are negative, sending the pointer into random memory.
    unsigned char* glyphPtr = (unsigned char*)font + sizeof(PSF1_HEADER) +
        ((unsigned char)c * font->CharSize);

    for (unsigned long y = yOff; y < yOff + font->CharSize; y++) {
        for (unsigned long x = xOff; x < xOff + 8; x++) {
            // Bitmask check: is the bit for this pixel set?
            if ((*glyphPtr & (0b10000000 >> (x - xOff))) > 0) {
                // Buffer check to prevent out-of-bounds writes
                if (x < Config->Width && y < Config->Height) {
                    Config->BaseAddress[x + (y * Config->PixelsPerScanLine)] = color;
                }
            }
        }
        glyphPtr++;
    }
}

void Console::Print(const char* str, uint32_t color) {
    if (Config == nullptr) return;

    while (*str != '\0') {
        if (*str == '\n') {
            NewLine();
        }
        else {
            // Horizontal wrap check: if we hit the edge, go to next line
            if (CursorX + 8 > (int)Config->Width) {
                NewLine();
            }

            PutChar(*str, CursorX, CursorY, color);
            CursorX += 8; // Standard PSF1 width is 8px
        }
        str++;
    }
}

void Console::NewLine() {
    CursorX = 0;
    CursorY += Config->Font->CharSize;

    // Vertical wrap check: scroll if we hit the bottom
    if (CursorY + Config->Font->CharSize > (int)Config->Height) {
        Scroll();
    }
}

void Console::Scroll() {
    uint32_t fontHeight = Config->Font->CharSize;
    uint32_t* fb = Config->BaseAddress;
    uint32_t psl = Config->PixelsPerScanLine;

    // 1. Move everything up by one line (fontHeight)
    for (uint32_t y = 0; y < Config->Height - fontHeight; y++) {
        for (uint32_t x = 0; x < Config->Width; x++) {
            fb[y * psl + x] = fb[(y + fontHeight) * psl + x];
        }
    }

    // 2. Clear the newly created bottom line (Black)
    for (uint32_t y = Config->Height - fontHeight; y < Config->Height; y++) {
        for (uint32_t x = 0; x < Config->Width; x++) {
            fb[y * psl + x] = 0x000000;
        }
    }

    // 3. Move cursor back up to stay on the bottom line
    CursorY -= fontHeight;
}

void Console::Clear(uint32_t color) {
    if (Config == nullptr) return;

    // Efficiently fill the entire frame buffer
    uint64_t totalPixels = (uint64_t)Config->PixelsPerScanLine * Config->Height;
    for (uint64_t i = 0; i < totalPixels; i++) {
        Config->BaseAddress[i] = color;
    }

    CursorX = 0;
    CursorY = 0;
}