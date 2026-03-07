#include "Shared.h"
#include "Console\Console.h"
#include <intrin.h>
#include "CrucialInternals\GDT.h"
#include "CrucialInternals\IDT.h"

extern "C" void RemapPIC();

// Marks the function as the entry point in the PE header
extern "C" __declspec(dllexport) void KernelMain(BOOT_CONFIG* config) {
    if (!config || !config->BaseAddress) {
        while (1) { __halt(); }
    }

    Console::Init(config);

    InitGDT();
    RemapPIC();
    InitIDT();

    Console::Print("GDT and IDT Loaded. Type something!\n", 0x00FFFF);

    uint32_t* fb = config->BaseAddress;
    uint32_t pps = config->PixelsPerScanLine;
    uint32_t width = config->Width;
    uint32_t height = config->Height;

	Console::Clear(0x000000);

    // 2. Hello World Square: Bright Green
    uint32_t centerX = width / 2;
    uint32_t centerY = height / 2;
    uint32_t size = 50;

    for (uint32_t y = centerY - size; y < centerY + size; y++) {
        for (uint32_t x = centerX - size; x < centerX + size; x++) {
            fb[x + (y * pps)] = 0x00FF00;
        }
    }

    // Inside KernelMain after clearing the screen
    Console::Print("WinOS Kernel Loaded!\n", 0xFFFFFF);
    Console::Print("PE Image Mapping: Success\n", 0x00FF00);

    // Final infinite loop
    while (true) {
        __halt();
    }
}