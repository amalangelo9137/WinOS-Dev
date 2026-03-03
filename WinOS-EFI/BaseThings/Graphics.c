#include "Graphics.h"
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS InitGraphics(EFI_GRAPHICS_OUTPUT_PROTOCOL** Gop) {
    EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    return gBS->LocateProtocol(&GopGuid, NULL, (VOID**)Gop);
}

void ClearScreen(EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop, UINT32 Color) {
    UINT32* FrameBuffer = (UINT32*)Gop->Mode->FrameBufferBase;

    // Calculate total pixels: Horizontal Resolution * Vertical Resolution
    // Note: Some screens have a 'ScanLine' wider than the resolution, 
    // but for a simple clear, we can just fill the whole buffer.
    UINTN TotalPixels = Gop->Mode->Info->PixelsPerScanLine * Gop->Mode->Info->VerticalResolution;

    for (UINTN i = 0; i < TotalPixels; i++) {
        FrameBuffer[i] = Color;
    }
}

// or scaled :)
void DrawSprite(EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop, SPRITE* Sprite, UINT32 X, UINT32 Y, UINT32 NewWidth, UINT32 NewHeight) {
    UINT32* FrameBuffer = (UINT32*)Gop->Mode->FrameBufferBase;
    UINT32 PPL = Gop->Mode->Info->PixelsPerScanLine;
    UINT32 ScreenW = Gop->Mode->Info->HorizontalResolution;
    UINT32 ScreenH = Gop->Mode->Info->VerticalResolution;

    // Calculate the scale ratios
    // We use Fixed Point math (multiplying by 65536) because UEFI doesn't always like floats
    UINT32 ScaleX = (Sprite->Width << 16) / NewWidth;
    UINT32 ScaleY = (Sprite->Height << 16) / NewHeight;

    for (UINT32 i = 0; i < NewHeight; i++) {
        for (UINT32 j = 0; j < NewWidth; j++) {
            // Clipping
            if ((X + j) >= ScreenW || (Y + i) >= ScreenH) continue;

            // Map New coordinate back to Original coordinate
            UINT32 OrigX = (j * ScaleX) >> 16;
            UINT32 OrigY = (i * ScaleY) >> 16;

            // FLIP LOGIC: BMPs are bottom-to-top. 
            // Instead of reading row 'OrigY', we read 'Height - 1 - OrigY'
            UINT32 FlippedY = (Sprite->Height - 1) - OrigY;

            UINT32 Pixel = Sprite->Data[FlippedY * Sprite->Width + OrigX];

            if ((Pixel >> 24) != 0) {
                FrameBuffer[(Y + i) * PPL + (X + j)] = Pixel;
            }
        }
    }
}