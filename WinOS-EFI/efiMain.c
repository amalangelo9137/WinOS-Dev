#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h> // AllocatePool / AllocatePages
#include <Library/BaseMemoryLib.h>       // CopyMem / SetMem

#include "BaseThings/Graphics.h"
#include "BaseThings/Files.h"
#include "BaseThings/BMPParser.h"
#include "BaseThings/Mouse.h"
#include <IndustryStandard/PeImage.h>

#include "Shared.h"
#include "Fonts/stb_truetype.h"

// We run on any UEFI Specification
extern CONST UINT32 _gUefiDriverRevision = 0;

CHAR8* gEfiCallerBaseName = "WinOS-Dev";
EFI_STATUS EFIAPI UefiUnload(IN EFI_HANDLE ImageHandle)
{
    // This is just to make the linker happy
    // We are jumping to the kernel anyway, so this never runs.
    return EFI_SUCCESS;
}

void* GetKernelEntry(void* KernelBuffer);

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Gop;
    EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    VOID* FileBuffer;
    UINTN FileSize;
    SPRITE Logo;
    BOOT_CONFIG Config;
    SetMem(&Config, sizeof(BOOT_CONFIG), 0); // Clear config to avoid garbage values

    // 1. Setup Graphics
    gBS->LocateProtocol(&GopGuid, NULL, (VOID**)&Gop);
    UINT32 ResX = Gop->Mode->Info->HorizontalResolution;
    UINT32 ResY = Gop->Mode->Info->VerticalResolution;

    // 2. Splash Screen
    ClearScreen(Gop, 0x000000);
    if (LoadFile(L"OSRESOURCES\\WinOS-Logo.bmp", &FileBuffer, &FileSize) == EFI_SUCCESS) {
        if (DecodeBMP(FileBuffer, &Logo) == EFI_SUCCESS) {
            DrawSprite(Gop, &Logo, (ResX / 2) - 256, (ResY / 2) - 256, 433, 512);
        }
        FreePool(FileBuffer); // Clean up splash buffer immediately
    }
    gBS->Stall(2000000); // 2 seconds is plenty

    // 3. Fill the Contract
    Config.FrameBufferBase = (uint32_t*)Gop->Mode->FrameBufferBase;
    Config.FrameBufferSize = Gop->Mode->FrameBufferSize;
    Config.HorizontalResolution = ResX;
    Config.VerticalResolution = ResY;
    Config.PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;
    Config.MouseSensitivity = 10;
    
    // ...loading kernel...
    VOID* KernelRawBuffer;
    UINTN KernelRawSize;
    if (LoadFile(L"KERNEL.BIN", &KernelRawBuffer, &KernelRawSize) != EFI_SUCCESS) {
        Print(L"CRITICAL: KERNEL.BIN not found!\n");
        while (1);
    }

    // ...loading cursor...
    VOID* MouseFileBuffer = NULL;
    UINTN MouseFileSize = 0;
    if (LoadFile(L"OSRESOURCES\\Cursor-Normal.bmp", &MouseFileBuffer, &MouseFileSize) == EFI_SUCCESS) {
        SPRITE MouseSprite;
        if (DecodeBMP(MouseFileBuffer, &MouseSprite) == EFI_SUCCESS) {
            UINTN Bytes = (UINTN)MouseSprite.Width * (UINTN)MouseSprite.Height * sizeof(UINT32);
            UINTN Pages = (Bytes + 0xFFF) / 0x1000;
            EFI_PHYSICAL_ADDRESS SpriteAddr = 0;

            // We use EfiLoaderData so the kernel can still read it after ExitBootServices
            if (gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, Pages, &SpriteAddr) == EFI_SUCCESS) {
                CopyMem((VOID*)(UINTN)SpriteAddr, MouseSprite.Data, Bytes);
                Config.LogonMousePixels = (uint32_t*)(UINTN)SpriteAddr;
                Config.MouseW = MouseSprite.Width;
                Config.MouseH = MouseSprite.Height;
            }
        }
        FreePool(MouseFileBuffer);
    }

    // ...loading font...
    // ... inside your efiMain function, after graphics are set up ...

    VOID* FontFileBuffer;
    UINTN FontFileSize;

    // 1. Load the BMP you generated from the disk
    if (LoadFile(L"OSRESOURCES\\BangersFont.bmp", &FontFileBuffer, &FontFileSize) == EFI_SUCCESS) {
        SPRITE FontSprite;

        // 2. Decode it (strip headers, get raw pixels)
        if (DecodeBMP(FontFileBuffer, &FontSprite) == EFI_SUCCESS) {

            // 3. Allocate permanent RAM for the Kernel to use
            UINTN Bytes = (UINTN)FontSprite.Width * (UINTN)FontSprite.Height * sizeof(UINT32);
            UINTN Pages = (Bytes + 0xFFF) / 0x1000;
            EFI_PHYSICAL_ADDRESS FontAddr = 0;

            if (gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, Pages, &FontAddr) == EFI_SUCCESS) {

                // 4. Copy the pixels to that permanent RAM
                // (Use your flip function here if the image is upside down)
                gBS->CopyMem((VOID*)(UINTN)FontAddr, FontSprite.Data, Bytes);

                // 5. Fill out the contract for the Kernel
                Config.FontSpriteData = (uint32_t*)(UINTN)FontAddr;
                Config.FontSpriteWidth = FontSprite.Width;
                Config.FontSpriteHeight = FontSprite.Height;
                Config.FontGlyphSize = 32; // Assuming you used a 32x32 grid
            }
        }
    }

    // ... jump to kernel ...

    void* RealEntryPoint = GetKernelEntry(KernelRawBuffer);
    if (RealEntryPoint == NULL) {
        Print(L"CRITICAL: Kernel Entry Point Invalid!\n");
        while (1);
    }

    // 5. THE POINT OF NO RETURN (Memory Map Handling)
    UINTN MapSize = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    // Get size first
    gBS->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    MapSize += (DescriptorSize * 5); // Add "Slack" for real hardware changes
    MemoryMap = AllocatePool(MapSize);

    // Final Memory Map and Exit
    EFI_STATUS Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        Print(L"Memory Map Failure: %r\n", Status);
        while (1);
    }

    Status = gBS->ExitBootServices(ImageHandle, MapKey);
    if (EFI_ERROR(Status)) {
        // If we fail here on real hardware, it's usually because 
        // a timer interrupt changed the memory map. Re-try logic is needed here in a production OS.
        Print(L"ExitBootServices failed! %r\n", Status);
        while (1);
    }

    // 6. JUMP TO KERNEL
    typedef void (*KERNEL_ENTRY)(BOOT_CONFIG*);
    KERNEL_ENTRY StartKernel = (KERNEL_ENTRY)RealEntryPoint;

    StartKernel(&Config);

    return EFI_SUCCESS;
}

void* GetKernelEntry(void* KernelBuffer) {
    // 1. Point to the DOS Header (The 'MZ' part)
    EFI_IMAGE_DOS_HEADER* DosHeader = (EFI_IMAGE_DOS_HEADER*)KernelBuffer;
    if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) return NULL;

    // 2. Find the NT Headers (The 'PE' part)
    // It's located at an offset defined in the DOS header (e_lfanew)
    EFI_IMAGE_NT_HEADERS64* NtHeader = (EFI_IMAGE_NT_HEADERS64*)((UINT8*)KernelBuffer + DosHeader->e_lfanew);
    if (NtHeader->Signature != EFI_IMAGE_NT_SIGNATURE) return NULL;
    if (NtHeader->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) return NULL; // require PE32+

    UINT64 SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
    UINT64 SizeOfHeaders = NtHeader->OptionalHeader.SizeOfHeaders;
    UINT64 PreferredBase = NtHeader->OptionalHeader.ImageBase;
    UINT32 NumberOfSections = NtHeader->FileHeader.NumberOfSections;

    // Allocate pages for the image
    UINTN Pages = (SizeOfImage + 0xFFF) / 0x1000;
    EFI_PHYSICAL_ADDRESS AllocatedAddr = 0;
    EFI_STATUS Status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, Pages, &AllocatedAddr);
    if (EFI_ERROR(Status)) return NULL;

    // Zero and copy headers
    SetMem((VOID*)(UINTN)AllocatedAddr, Pages * 0x1000, 0);
    CopyMem((VOID*)(UINTN)AllocatedAddr, KernelBuffer, (UINTN)SizeOfHeaders);

    // Copy sections
    EFI_IMAGE_SECTION_HEADER* Section = (EFI_IMAGE_SECTION_HEADER*)((UINT8*)&NtHeader->OptionalHeader + NtHeader->FileHeader.SizeOfOptionalHeader);
    for (UINT32 i = 0; i < NumberOfSections; i++) {
        UINT8* Src = (UINT8*)KernelBuffer + Section[i].PointerToRawData;
        UINT8* Dest = (UINT8*)(UINTN)(AllocatedAddr + Section[i].VirtualAddress);
        UINT32 CopySize = Section[i].SizeOfRawData;
        if (CopySize > 0) {
            CopyMem(Dest, Src, CopySize);
        }
    }

    // Apply base relocations if needed
    UINT64 Delta = (UINT64)AllocatedAddr - PreferredBase;
    EFI_IMAGE_DATA_DIRECTORY RelocDir = NtHeader->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (Delta != 0 && RelocDir.Size != 0) {
        UINT8* RelocBase = (UINT8*)KernelBuffer + RelocDir.VirtualAddress;
        UINT32 Parsed = 0;
        while (Parsed < RelocDir.Size) {
            EFI_IMAGE_BASE_RELOCATION* Reloc = (EFI_IMAGE_BASE_RELOCATION*)(RelocBase + Parsed);
            if (Reloc->SizeOfBlock == 0) break;
            UINT32 EntryCount = (Reloc->SizeOfBlock - sizeof(EFI_IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
            UINT16* RelocData = (UINT16*)(Reloc + 1);
            for (UINT32 j = 0; j < EntryCount; j++) {
                UINT16 entry = RelocData[j];
                UINT16 type = entry >> 12;
                UINT16 offset = entry & 0x0FFF;
                if (type == EFI_IMAGE_REL_BASED_DIR64) {
                    UINT64* Patch = (UINT64*)(UINTN)(AllocatedAddr + Reloc->VirtualAddress + offset);
                    *Patch = *Patch + Delta;
                }
                // other relocation types can be added if necessary
            }
            Parsed += Reloc->SizeOfBlock;
        }
    }

    // Return runtime entry address
    return (void*)(UINTN)(AllocatedAddr + NtHeader->OptionalHeader.AddressOfEntryPoint);
}