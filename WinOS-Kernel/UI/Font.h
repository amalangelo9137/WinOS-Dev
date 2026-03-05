#pragma once
#include <stdint.h>
#include "Shared.h"

// Simple runtime font handle referencing BOOT_CONFIG data
typedef struct {
    uint32_t* Data;      // ARGB32 pixel array (width * height)
    uint32_t  ImgWidth;
    uint32_t  ImgHeight;
    uint32_t  GlyphW;
    uint32_t  GlyphH;
    uint32_t  FirstChar;     // ASCII start (e.g. 32)
    uint32_t  CharsPerRow;
} Font;

// Initialize Font from BOOT_CONFIG (calls no allocator)
void Font_InitFromBootConfig(Font* font, BOOT_CONFIG* cfg);

// Draw a single character into a destination ARGB buffer (dest is width * height stride)
void Font_DrawCharToBuffer(Font* font, char c, uint32_t* dest, uint32_t destW, uint32_t destH, int dx, int dy);

// Draw a string; returns width in pixels drawn
int Font_DrawString(Font* font, const char* s, uint32_t* dest, uint32_t destW, uint32_t destH, int dx, int dy);