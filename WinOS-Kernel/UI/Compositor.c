#include "Compositor.h"

uint32_t* FrontBuffer;
uint32_t* BackBuffer;
BOOT_CONFIG* GlobalConfig;

void InitCompositor(BOOT_CONFIG* Config) {
    GlobalConfig = Config;
    FrontBuffer = (uint32_t*)Config->FrameBufferBase;

    // Calculate buffer size based on ScanLine, not just Width
    // This prevents "shearing" on real monitors
    uint64_t totalPixels = (uint64_t)Config->PixelsPerScanLine * Config->VerticalResolution;
    uint64_t bufferSize = totalPixels * 4;

    // Place BackBuffer in a safe memory area. 
    // On real hardware, ensure this area isn't reserved by UEFI!
    // Often it is safer to have UEFI allocate this and pass it in.
    BackBuffer = (uint32_t*)((uint8_t*)FrontBuffer + bufferSize + 0x1000);
}

void Compositor_PutPixel(int x, int y, uint32_t color) {
    // Boundary check using Resolution
    if (x < 0 || x >= (int)GlobalConfig->HorizontalResolution ||
        y < 0 || y >= (int)GlobalConfig->VerticalResolution) {
        return;
    }

    // Index calculation using PixelsPerScanLine
    // index = (y * ScanLine) + x
    BackBuffer[y * GlobalConfig->PixelsPerScanLine + x] = color;
}

void Compositor_Clear(uint32_t color) {
    uint64_t totalPixels = (uint64_t)GlobalConfig->PixelsPerScanLine * GlobalConfig->VerticalResolution;
    for (uint64_t i = 0; i < totalPixels; i++) {
        BackBuffer[i] = color;
    }
}

void Compositor_DrawMouse(int mouseX, int mouseY) {
    if (GlobalConfig->LogonMousePixels == NULL) return;

    uint32_t* sprite = (uint32_t*)GlobalConfig->LogonMousePixels;
    uint32_t width = GlobalConfig->MouseW;
    uint32_t height = GlobalConfig->MouseH;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            // --- THE INVERSION FIX ---
            // Calculate the flipped Y index for the source sprite
            uint32_t flippedY = (height - 1 - y);
            uint32_t pixel = sprite[flippedY * width + x];

            uint32_t color = pixel & 0x00FFFFFF;

            if (color != 0x000000) {
                Compositor_PutPixel(mouseX + x, mouseY + y, color);
            }
        }
    }
}

void Compositor_Update() {
    uint64_t totalPixels = (uint64_t)GlobalConfig->PixelsPerScanLine * GlobalConfig->VerticalResolution;

    // We pass (TotalBytes / 8) to FastCopy because it moves 64-bits (8 bytes) at a time
    FastCopy(FrontBuffer, BackBuffer, (totalPixels * 4) / 8);
}