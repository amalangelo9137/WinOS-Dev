#pragma once
#include <stdint.h>

// We use extern "C" here so the C++ Kernel and C-style EFI 
// don't disagree on how this structure looks in memory.
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        // Framebuffer Information
        uint32_t* FrameBufferBase;
        uint64_t  FrameBufferSize;
        uint32_t  HorizontalResolution;
        uint32_t  VerticalResolution;
        uint32_t  PixelsPerScanLine;

        // Assets loaded by EFI
        uint32_t* FontAtlas;      // Raw pixels for BangersFont
        uint32_t  FontAtlasWidth;
        uint32_t  FontAtlasHeight;
        uint32_t  GlyphSize;       // e.g., 32

        uint32_t* LogoPixels;     // Raw pixels for your OS Logo
        uint32_t  LogoW;
        uint32_t  LogoH;

        // The Memory Address where the Kernel starts
        void* KernelEntry;
    } BOOT_CONFIG;

#ifdef __cplusplus
}
#endif