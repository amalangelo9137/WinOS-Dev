#pragma once
#include <stdint.h>

typedef struct {
    uint32_t Width;
    uint32_t Height;
    uint32_t* PixelData; // Pointing to the actual colors
} BITMAP;

// Logic to "load" from a raw memory address (where the BMP file is)
BITMAP* CreateBitmap(void* RawBuffer);
void DrawBitmap(BITMAP* bmp, int x, int y, uint32_t TransparencyKey);