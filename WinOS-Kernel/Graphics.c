#include "Graphics.h"
#include "UI\Compositor.h"

BITMAP* CreateBitmap(void* RawBuffer) {
    // BMP Header is 54 bytes. 
    // Offset 18 = Width (4 bytes)
    // Offset 22 = Height (4 bytes)
    // Offset 10 = Pixel Data Offset (4 bytes)

    uint8_t* file = (uint8_t*)RawBuffer;

    // Simple validation: 'B' 'M'
    if (file[0] != 'B' || file[1] != 'M') return 0;

    BITMAP* bmp; // In a kernel, you'd use your own 'kmalloc' here
    // For now, let's assume we have a static struct or memory area
    static BITMAP InternalBmp;

    InternalBmp.Width = *(uint32_t*)&file[18];
    InternalBmp.Height = *(uint32_t*)&file[22];
    uint32_t dataOffset = *(uint32_t*)&file[10];

    InternalBmp.PixelData = (uint32_t*)&file[dataOffset];

    return &InternalBmp;
}

void DrawBitmap(BITMAP* bmp, int x, int y, uint32_t TransparencyKey) {
    if (!bmp) return;

    for (uint32_t i = 0; i < bmp->Height; i++) {
        for (uint32_t j = 0; j < bmp->Width; j++) {
            // BMPs are stored Bottom-to-Top. 
            // We flip the 'i' index to draw Top-to-Bottom.
            uint32_t flippedY = (bmp->Height - 1 - i);
            uint32_t color = bmp->PixelData[flippedY * bmp->Width + j];

            // Transparency check (usually 0x00FF00 or 0xFF00FF)
            if (color != TransparencyKey) {
                Compositor_PutPixel(x + j, y + i, color);
            }
        }
    }
}