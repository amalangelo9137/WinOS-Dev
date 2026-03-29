#include "Renderer.h"
#include <stddef.h>
#include <intrin.h> // Required for __movsd

void SwitchBuffer(BOOT_CONFIG* config) {
    if (config == nullptr) return;

    const size_t pixelCount = (size_t)config->PixelsPerScanLine * (size_t)config->Height;
    __movsd((unsigned long*)config->BaseAddress,
        (unsigned long*)config->BackBuffer,
        pixelCount);
}

void Clear(uint32_t color, BOOT_CONFIG* config) {
    if (config == nullptr) return;

    // Efficiently fill the entire frame buffer
    uint64_t totalPixels = (uint64_t)config->PixelsPerScanLine * config->Height;
    for (uint64_t i = 0; i < totalPixels; i++) {
        config->BackBuffer[i] = color;
    }
}
