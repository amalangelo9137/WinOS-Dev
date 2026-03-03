#ifndef BMP_H
#define BMP_H

#include <Uefi.h>
#include "Graphics.h"

#pragma pack(push, 1) // Force compiler to NOT add padding
typedef struct {
    UINT16 Type;           // Magic identifier 'BM'
    UINT32 Size;           // File size
    UINT16 Reserved1, Reserved2;
    UINT32 OffBits;        // Offset to start of pixel data
} BMP_FILE_HEADER;

typedef struct {
    UINT32 Size;
    INT32  Width;
    INT32  Height;
    UINT16 Planes;
    UINT16 BitCount;       // Should be 32 for ARGB
    UINT32 Compression;
    // ... other fields we can ignore for now
} BMP_INFO_HEADER;
#pragma pack(pop)

EFI_STATUS DecodeBMP(VOID* Buffer, SPRITE* OutSprite);

#endif