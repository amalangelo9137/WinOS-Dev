#include "Renderer.h"

void DrawBMP(void* assetAddress, int targetX, int targetY, int targetW, int targetH, BOOT_CONFIG* config) {
    if (!assetAddress) return;

    uint8_t* bytePtr = (uint8_t*)assetAddress;

    // 1. Parse the BMP Header
    // 'BM' magic bytes check
    if (bytePtr[0] != 'B' || bytePtr[1] != 'M') return;

    // Extract image metadata from standard BMP header offsets
    uint32_t pixelOffset = *(uint32_t*)(bytePtr + 10);
    int32_t origW = *(int32_t*)(bytePtr + 18);
    int32_t origH = *(int32_t*)(bytePtr + 22);
    uint16_t bpp = *(uint16_t*)(bytePtr + 28); // Bits per pixel

    // We only support 32-bit (ARGB) or 24-bit (RGB) for now
    if (bpp != 32 && bpp != 24) return;

    uint8_t* pixelData = bytePtr + pixelOffset;

    // 2. Draw with Nearest-Neighbor Scaling
    for (int y = 0; y < targetH; y++) {
        for (int x = 0; x < targetW; x++) {

            // Map the target (scaled) coordinate back to the original image coordinate
            int srcX = (x * origW) / targetW;
            int srcY = (y * origH) / targetH;

            // BMPs are stored upside down usually, so we flip the Y read axis
            int actualSrcY = (origH - 1) - srcY;

            // Calculate exact byte position in the original image array
            int byteIndex = (srcX * (bpp / 8)) + (actualSrcY * ((origW * (bpp / 8) + 3) & ~3));

            uint32_t color = 0;
            if (bpp == 32) {
                // Read Alpha, Red, Green, Blue
                color = *(uint32_t*)(pixelData + byteIndex);
            }
            else if (bpp == 24) {
                // Read Blue, Green, Red (Construct a 32-bit int)
                uint8_t b = pixelData[byteIndex];
                uint8_t g = pixelData[byteIndex + 1];
                uint8_t r = pixelData[byteIndex + 2];
                color = (r << 16) | (g << 8) | b;
            }

            // Alpha testing: If it's pure magenta (0xFF00FF) or alpha is 0, skip drawing (Transparency)
            if (color == 0xFFFF00FF || (bpp == 32 && (color >> 24) == 0)) continue;

            // 3. Draw to BackBuffer
            int screenX = targetX + x;
            int screenY = targetY + y;

            if (screenX >= 0 && screenX < config->Width && screenY >= 0 && screenY < config->Height) {
                config->BackBuffer[screenX + (screenY * config->PixelsPerScanLine)] = color;
            }
        }
    }
}