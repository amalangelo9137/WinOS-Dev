#include "Renderer.h"
#include <intrin.h> // Required for __movsd

void SwitchBuffer(BOOT_CONFIG* config) {
    // __movsd copies 4 bytes at a time using hardware acceleration
    // Number of dwords = (Width * Height)
    __movsd((unsigned long*)config->BaseAddress,
        (unsigned long*)config->BackBuffer,
        (config->Width * config->Height));
}

void Clear(uint32_t color, BOOT_CONFIG* config) {
    if (config == nullptr) return;

    // Efficiently fill the entire frame buffer
    uint64_t totalPixels = (uint64_t)config->PixelsPerScanLine * config->Height;
    for (uint64_t i = 0; i < totalPixels; i++) {
        config->BackBuffer[i] = color;
    }
}