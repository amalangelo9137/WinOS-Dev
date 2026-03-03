#include "BMPParser.h"

EFI_STATUS DecodeBMP(VOID* Buffer, SPRITE* OutSprite) {
    BMP_FILE_HEADER* FileHdr = (BMP_FILE_HEADER*)Buffer;

    // Check if it's actually a BMP ('BM' in little endian)
    if (FileHdr->Type != 0x4D42) return EFI_UNSUPPORTED;

    BMP_INFO_HEADER* InfoHdr = (BMP_INFO_HEADER*)((UINT8*)Buffer + sizeof(BMP_FILE_HEADER));

    OutSprite->Width = InfoHdr->Width;
    // BMP heights can be negative; if positive, the image is upside down!
    OutSprite->Height = (InfoHdr->Height < 0) ? -InfoHdr->Height : InfoHdr->Height;

    // Point to the actual pixels (skipping the headers)
    OutSprite->Data = (UINT32*)((UINT8*)Buffer + FileHdr->OffBits);

    return EFI_SUCCESS;
}