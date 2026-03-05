#pragma once
#include <stdint.h>
#include "Shared.h"

// Pointers to our drawing surfaces
extern uint32_t* FrontBuffer;
extern uint32_t* BackBuffer;
extern BOOT_CONFIG* GlobalConfig;

// Core Logic
void InitCompositor(BOOT_CONFIG* Config);
void Compositor_Clear(uint32_t color);
void Compositor_Update();
void Compositor_PutPixel(int x, int y, uint32_t color);

// Sprite Drawing (Handover)
void Compositor_DrawMouse(int mouseX, int mouseY);

// The high-speed MASM function
extern void FastCopy(void* dest, void* src, uint64_t count);