#include "Files.h"
#include <Library/MemoryAllocationLib.h>
#include <Guid/FileInfo.h>

EFI_STATUS LoadFile(CHAR16* FileName, VOID** Buffer, UINTN* Size) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    EFI_FILE_PROTOCOL* Root;
    EFI_FILE_PROTOCOL* File;

    // 1. Find the File System on the boot drive
    Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&FileSystem);
    if (EFI_ERROR(Status)) return Status;

    // 2. Open the Root directory
    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) return Status;

    // 3. Open the specific file
    Status = Root->Open(Root, &File, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) return Status;

    // 4. Get File Info to find out the size
    EFI_FILE_INFO* Info;
    UINTN InfoSize = 0;
    EFI_GUID InfoGuid = EFI_FILE_INFO_ID;

    // Call once to get required size
    File->GetInfo(File, &InfoGuid, &InfoSize, NULL);
    Info = AllocatePool(InfoSize);
    Status = File->GetInfo(File, &InfoGuid, &InfoSize, Info);

    *Size = Info->FileSize;
    FreePool(Info);

    // 5. Read the file into memory
    *Buffer = AllocatePool(*Size);
    Status = File->Read(File, Size, *Buffer);

    File->Close(File);
    Root->Close(Root);

    return Status;
}