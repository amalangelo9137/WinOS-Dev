#ifndef FILES_H
#define FILES_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiBootServicesTableLib.h>

// Helper to load a file into a buffer
EFI_STATUS LoadFile(CHAR16* FileName, VOID** Buffer, UINTN* Size);

#endif