#pragma once
#include "Shared.h"

// Fills the screen with a given colour.
void Clear(uint32_t color, BOOT_CONFIG* config);

// Applies the backbuffer to the main framebuffer.
void SwitchBuffer(BOOT_CONFIG* config);

// Draw a rectangle with given x position, y position, width and height.
void DrawRect(int x, int y, int width, int height, uint32_t color, BOOT_CONFIG* config);

// Draw a rectangle with given x position, y position, width and height.
void DrawEllipse(int xCenter, int yCenter, int width, int height, uint32_t color, BOOT_CONFIG* config);

// Draw a bitmap image at a specified position.
void DrawBMP(void* assetAddress, int targetX, int targetY, int targetW, int targetH, BOOT_CONFIG* config);

// Print text on the specified position.
void PrintPos(const char* str, unsigned int x, unsigned int y, uint32_t color, BOOT_CONFIG* config);

// Print a letter on a specified position
void PutChar(char c, unsigned int xOff, unsigned int yOff, uint32_t color, BOOT_CONFIG* config);

// Create a window at a specified position.
void DrawWindow(const char* Name, unsigned int x, unsigned int y, unsigned int w, unsigned int h, BOOT_CONFIG* config);