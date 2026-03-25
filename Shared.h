#pragma once
#include <stdint.h>
#include <stdbool.h>

// The state of the mouse (Kernel will update this later)
typedef struct {
    int X, Y;
    bool Left, Right;
} MOUSE_STATE;

// The font format for your terminal
typedef struct {
    void* psf1_Header;
    void* glyphBuffer;
} PSF1_FONT;

// The ultimate package handed from EFI to the Kernel
typedef struct {
    uint32_t* BaseAddress;       // The actual screen pixels (Front Buffer)
    uint32_t* BackBuffer;        // The hidden drawing canvas (Double Buffer)
    uint32_t Width;              // Screen Width (e.g., 1920)
    uint32_t Height;             // Screen Height (e.g., 1080)
    uint32_t PixelsPerScanLine;  // Padding (sometimes wider than Width)

    MOUSE_STATE* Mouse;

    // RAM Addresses of your loaded assets
    void* CursorBMP;
    void* WallpaperBMP;
    PSF1_FONT* Font;
} BOOT_CONFIG;