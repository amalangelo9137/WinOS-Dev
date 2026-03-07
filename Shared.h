#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
    typedef struct {
        unsigned char Magic[2];     // 0x36, 0x04
        unsigned char Mode;         // 0 = 256 chars, 1 = 512 chars
        unsigned char CharSize;     // Height of character in pixels
    } PSF1_HEADER;

    typedef struct {
        uint32_t* BaseAddress;
        uint64_t  BufferSize;
        uint32_t  Width;
        uint32_t  Height;
        uint32_t  PixelsPerScanLine;
        PSF1_HEADER* Font;          // <--- Added this field
    } BOOT_CONFIG;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif