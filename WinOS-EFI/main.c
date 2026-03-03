#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ShellLib.h>
#include "BaseThings/Graphics.h"
#include "BaseThings/Files.h"
#include "BaseThings/BMPParser.h"
#include "BaseThings/Mouse.h"

// We run on any UEFI Specification
extern CONST UINT32 _gUefiDriverRevision = 0;
CHAR8 *gEfiCallerBaseName = "WinOS-Dev";

EFI_STATUS EFIAPI UefiUnload (
    IN EFI_HANDLE ImageHandle
    )
{
    // This code should be compiled out and never called
    ASSERT(FALSE);
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
	// the graphics protocol uefi uses to draw on the screen
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop;
    EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	// buffer for the file and its size
    VOID* FileBuffer;
    UINTN FileSize;
    SPRITE Logo;

	// find gop
    gBS->LocateProtocol(&GopGuid, NULL, (VOID**)&Gop);

    // resolution
    UINT32 ResX = Gop->Mode->Info->HorizontalResolution;
    UINT32 ResY = Gop->Mode->Info->VerticalResolution;

    // wait
    gBS->Stall(10000);

	// clear the screen
	ClearScreen(Gop, 0x000000);

    // draw the fuggin logo
    if (LoadFile(L"\\EFI\\BOOT\\WinOS-Logo.bmp", &FileBuffer, &FileSize) == EFI_SUCCESS) {
        if (DecodeBMP(FileBuffer, &Logo) == EFI_SUCCESS) {
            DrawSprite(Gop, &Logo, ResX / 2 - 256, ResY / 2 - 256, 512, 512);
        }
    }
     
    // keep it on screen for 10 seconds
    gBS->Stall(10000000);

    ClearScreen(Gop, 0x267F00);

    // 1. Load kernel.bin into a buffer
    void* KernelEntry;
	UINTN KernelSize;
    LoadFile(L"kernel.bin", &KernelEntry, &KernelSize);

    // 2. Define what the Kernel looks like
    typedef void (*KERNEL_ENTRY)(unsigned int*, int, int);
    KERNEL_ENTRY StartKernel = (KERNEL_ENTRY)KernelEntry;

    UINTN MapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    // 1. Get the size of the buffer needed
    gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

    // 2. Allocate the buffer (add a little extra room just in case)
    MapSize += 2 * DescriptorSize;
    gBS->AllocatePool(EfiLoaderData, MapSize, (VOID**)&MemoryMap);

    // 3. Get the actual map and the real MapKey
    gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);

    // 4. NOW you can exit
    gBS->ExitBootServices(ImageHandle, MapKey);

    // 4. JUMP!
    StartKernel((unsigned int*)Gop->Mode->FrameBufferBase, ResX, ResY);

    return EFI_SUCCESS;
}

