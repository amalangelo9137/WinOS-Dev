#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/PeImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include "Shared.h"

extern CONST UINT32 _gUefiDriverRevision = 0;
CHAR8* gEfiCallerBaseName = "WinOS-Dev";

typedef void(__cdecl* KERNEL_ENTRY)(BOOT_CONFIG*);

// Helper to load the kernel file into memory
VOID* LoadKernelFile(EFI_HANDLE ImageHandle, CHAR16* FileName) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    EFI_FILE_PROTOCOL* Root;
    EFI_FILE_PROTOCOL* File;
    VOID* Buffer = NULL;

    gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
    gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FileSystem);
    FileSystem->OpenVolume(FileSystem, &Root);

    Status = Root->Open(Root, &File, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) return NULL;

    // Get file size
    UINTN FileSize = 0;
    File->SetPosition(File, 0xFFFFFFFFFFFFFFFFULL);
    File->GetPosition(File, &FileSize);
    File->SetPosition(File, 0);

    // Read the headers first to find out how much memory the IMAGE actually needs
    EFI_IMAGE_DOS_HEADER DosHdr;
    UINTN Size = sizeof(DosHdr);
    File->Read(File, &Size, &DosHdr);

    EFI_IMAGE_NT_HEADERS64 NtHdr;
    File->SetPosition(File, DosHdr.e_lfanew);
    Size = sizeof(NtHdr);
    File->Read(File, &Size, &NtHdr);

    // CRITICAL FIX: Allocate SizeOfImage, not FileSize
    UINTN ImageSize = NtHdr.OptionalHeader.SizeOfImage;
    gBS->AllocatePool(EfiLoaderData, ImageSize, &Buffer);

    // Zero out the whole buffer first to handle BSS automatically
    SetMem(Buffer, ImageSize, 0);

    // Read the raw file into the start of the buffer
    File->SetPosition(File, 0);
    File->Read(File, &FileSize, Buffer);

    File->Close(File);
    return Buffer;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status;

    // 1. Setup Graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop);
    if (EFI_ERROR(Status)) return Status;

    BOOT_CONFIG config = {
        (uint32_t*)gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferSize,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        gop->Mode->Info->PixelsPerScanLine,
        NULL
    };

    // 2. Load Kernel raw binary
    VOID* KernelBuffer = LoadKernelFile(ImageHandle, L"KERNEL");
    if (KernelBuffer == NULL) {
        Print(L"WHERE TF IS THE KERNEL\n");
        return EFI_NOT_FOUND;
    }

    VOID* FontBuffer = LoadKernelFile(ImageHandle, L"OSRESOURCES\\kernelFont.psf");
    if (FontBuffer == NULL) {
        Print(L"bro place the font at the OSRESOURCES directory :) (name it kernelFont)\n");
		return EFI_NOT_FOUND;
    }
    config.Font = (PSF1_HEADER*)FontBuffer;
        
    // 3. PE Parsing & Section Mapping
    EFI_IMAGE_DOS_HEADER* DosHdr = (EFI_IMAGE_DOS_HEADER*)KernelBuffer;
    EFI_IMAGE_NT_HEADERS64* NtHdr = (EFI_IMAGE_NT_HEADERS64*)((UINT8*)KernelBuffer + DosHdr->e_lfanew);
    EFI_IMAGE_SECTION_HEADER* Section = (EFI_IMAGE_SECTION_HEADER*)((UINT8*)&NtHdr->OptionalHeader + NtHdr->FileHeader.SizeOfOptionalHeader);

    for (UINT16 i = 0; i < NtHdr->FileHeader.NumberOfSections; i++) {
        UINT8* Dest = (UINT8*)KernelBuffer + Section[i].VirtualAddress;
        UINT8* Src = (UINT8*)KernelBuffer + Section[i].PointerToRawData;

        if (Section[i].SizeOfRawData > 0) {
            CopyMem(Dest, Src, Section[i].SizeOfRawData);
        }

        // Fix for .Misc.VirtualSize (The BSS section)
        if (Section[i].Misc.VirtualSize > Section[i].SizeOfRawData) {
            SetMem((UINT8*)Dest + Section[i].SizeOfRawData, Section[i].Misc.VirtualSize - Section[i].SizeOfRawData, 0);
        }
    }

    // 4. Memory Map & Exit Boot Services
    UINTN MapSize = 0;
    UINTN MapKey, DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_MEMORY_DESCRIPTOR* MMap = NULL;

    // Get the buffer size needed
    gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    MapSize += (2 * DescriptorSize); // Add some padding
    gBS->AllocatePool(EfiLoaderData, MapSize, (VOID**)&MMap);

    // Get the actual map
    Status = gBS->GetMemoryMap(&MapSize, MMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) return Status;

    Status = gBS->ExitBootServices(ImageHandle, MapKey);
    if (EFI_ERROR(Status)) return Status;

    // 5. Jump to Kernel
    KERNEL_ENTRY KernelMain = (KERNEL_ENTRY)((UINTN)KernelBuffer + NtHdr->OptionalHeader.AddressOfEntryPoint);
    KernelMain(&config);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiUnload(IN EFI_HANDLE ImageHandle) {
    return EFI_SUCCESS;
}