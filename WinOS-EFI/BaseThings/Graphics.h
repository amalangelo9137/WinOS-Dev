#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

// Our custom "Sprite" structure
typedef struct {
    UINT32 Width;
    UINT32 Height;
    UINT32* Data; // Raw pixel data
} SPRITE;

// Engine Functions
EFI_STATUS InitGraphics(EFI_GRAPHICS_OUTPUT_PROTOCOL** Gop);
void ClearScreen(EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop, UINT32 Color);
void DrawSprite(EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop, SPRITE* Sprite, UINT32 X, UINT32 Y, UINT32 NewWidth, UINT32 NewHeight);

#endif