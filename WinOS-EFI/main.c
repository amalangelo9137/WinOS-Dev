// Standard EDK2/VisualUefi Includes
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>

#include "Shared.h"

extern CONST UINT32 _gUefiDriverRevision = 0;
CHAR8* gEfiCallerBaseName = "WinOS-Dev";

// =========================================================================
// HELPER 1: Load a file from the USB into RAM
// =========================================================================
void* LoadFileIntoRAM(EFI_FILE* Directory, CHAR16* Path) {
    EFI_FILE* LoadedFile;
    EFI_STATUS Status = Directory->Open(Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(Status)) {
        Print(L"[!] Error opening %s: %r\n", Path, Status);
        return NULL;
    }

    // 1. Get File Size
    EFI_FILE_INFO* FileInfo = NULL;
    UINTN FileInfoSize = 0;
    LoadedFile->GetInfo(LoadedFile, &gEfiFileInfoGuid, &FileInfoSize, NULL);
    gBS->AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
    LoadedFile->GetInfo(LoadedFile, &gEfiFileInfoGuid, &FileInfoSize, (void**)&FileInfo);

    UINTN FileSize = FileInfo->FileSize;
    gBS->FreePool(FileInfo);

    if (FileSize == 0) {
        Print(L"[!] File %s is empty!\n", Path);
        return NULL;
    }

    // 2. Calculate Pages (1 Page = 4096 bytes)
    UINTN NumPages = (FileSize + 4095) / 4096;
    EFI_PHYSICAL_ADDRESS FileBufferAddr;

    // 3. Allocate Pages directly from RAM
    Status = gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, NumPages, &FileBufferAddr);

    if (EFI_ERROR(Status)) {
        Print(L"[!] Failed to allocate %d pages for %s: %r\n", NumPages, Path, Status);
        return NULL;
    }

    void* FileBuffer = (void*)FileBufferAddr;

    // 4. Read the data
    Status = LoadedFile->Read(LoadedFile, &FileSize, FileBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"[!] Failed to read %s: %r\n", Path, Status);
        gBS->FreePages(FileBufferAddr, NumPages);
        return NULL;
    }

    LoadedFile->Close(LoadedFile);
    Print(L"[+] Loaded %s at 0x%llx (%d bytes)\n", Path, FileBufferAddr, FileSize);
    return FileBuffer;
}

// =========================================================================
// HELPER 2: Initialize the Screen (Graphics Output Protocol)
// =========================================================================
EFI_GRAPHICS_OUTPUT_PROTOCOL* InitGraphics() {
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP;
    EFI_STATUS Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&GOP);

    if (EFI_ERROR(Status)) {
        Print(L"[!] Failed to locate GOP (Graphics Protocol)\n");
        return NULL;
    }

    Print(L"[+] Graphics Initialized. Resolution: %dx%d\n",
        GOP->Mode->Info->HorizontalResolution,
        GOP->Mode->Info->VerticalResolution);

    return GOP;
}

// =========================================================================
// MAIN ENTRY POINT
// =========================================================================
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    // Clear the screen
    gST->ConOut->ClearScreen(gST->ConOut);
    Print(L"=== Starting WinOS Bootloader ===\n");

    // 1. Setup Graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = InitGraphics();
    if (!GOP) return EFI_UNSUPPORTED;

    // 2. Access the USB Drive (File System)
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

    EFI_FILE* RootDir;
    FileSystem->OpenVolume(FileSystem, &RootDir);

    // 3. Load UI Assets into RAM
    void* CursorBuffer = LoadFileIntoRAM(RootDir, L"OSRESOURCES\\Cursor-Normal.bmp");
    void* WallpaperBuffer = LoadFileIntoRAM(RootDir, L"OSRESOURCES\\WinOS-BG.bmp");

    // Load and format the PSF1 Font
    void* FontFileBuffer = LoadFileIntoRAM(RootDir, L"OSRESOURCES\\kernelFont.psf");
    PSF1_FONT* SystemFont;
    gBS->AllocatePool(EfiLoaderData, sizeof(PSF1_FONT), (void**)&SystemFont);
    SystemFont->psf1_Header = FontFileBuffer;
    SystemFont->glyphBuffer = (void*)((UINT8*)FontFileBuffer + 4); // Skip 4-byte header

    // 4. Allocate the BackBuffer (For flicker-free rendering later)
    UINTN BackBufferSize = GOP->Mode->Info->PixelsPerScanLine * GOP->Mode->Info->VerticalResolution * sizeof(uint32_t);
    UINTN BackBufferPages = (BackBufferSize + 4095) / 4096;
    EFI_PHYSICAL_ADDRESS BackBufferAddr;

    gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, BackBufferPages, &BackBufferAddr);

    void* BackBuffer = (void*)BackBufferAddr;

    // Allocate Mouse State
    MOUSE_STATE* MouseState;
    gBS->AllocatePool(EfiLoaderData, sizeof(MOUSE_STATE), (void**)&MouseState);
    MouseState->X = GOP->Mode->Info->HorizontalResolution / 2; // Start mouse in center
    MouseState->Y = GOP->Mode->Info->VerticalResolution / 2;
    MouseState->Left = false;
    MouseState->Right = false;

    // 5. Build the BOOT_CONFIG payload
    BOOT_CONFIG BootConfig;
    BootConfig.BaseAddress = (uint32_t*)GOP->Mode->FrameBufferBase;
    BootConfig.BackBuffer = (uint32_t*)BackBuffer;
    BootConfig.Width = GOP->Mode->Info->HorizontalResolution;
    BootConfig.Height = GOP->Mode->Info->VerticalResolution;
    BootConfig.PixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;

    BootConfig.Mouse = MouseState;
    BootConfig.CursorBMP = CursorBuffer;
    BootConfig.WallpaperBMP = WallpaperBuffer;
    BootConfig.Font = SystemFont;

    Print(L"[+] BOOT_CONFIG payload assembled.\n");

    // =========================================================================
    // TODO NEXT: 
    // 1. Load the Kernel File (kernel.elf or kernel.bin) into RAM
    // 2. Get the Memory Map
    // 3. Call gBS->ExitBootServices()
    // 4. Jump to the Kernel Entry Point passing &BootConfig
    // =========================================================================

    // Halt for now so you can see the print statements
    while (1) {}

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiUnload()
{
	return EFI_SUCCESS;
}