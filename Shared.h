#pragma once
#include <stdint.h>

typedef struct {
    uint32_t* FrameBufferBase;
    uint64_t  FrameBufferSize;
    uint32_t  HorizontalResolution;
    uint32_t  VerticalResolution;
    uint32_t  PixelsPerScanLine;

    // Handover Sprite: Cursor
    uint32_t* LogonMousePixels;
    uint32_t  MouseW;
	uint32_t MouseSensitivity;
    uint32_t  MouseH;

    // Handover Sprite: Font Atlas
    uint32_t* FontSpriteData;   // Points to the 512x512 BMP pixels
    uint32_t  FontSpriteWidth;  // 512
    uint32_t  FontSpriteHeight; // 512
    uint32_t  FontGlyphSize;    // 32 (size of the square in the grid)
} BOOT_CONFIG;