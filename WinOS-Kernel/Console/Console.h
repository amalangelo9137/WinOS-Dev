#pragma once
#include <stdint.h>
#include "Shared.h"

class Console {
public:
    // Sets up the console with the boot configuration
    static void Init(BOOT_CONFIG* config);

    // Main output methods
    static void Print(const char* str, uint32_t color = 0xFFFFFF);
    static void Clear(uint32_t color = 0x000000);
    static void NewLine();
    static void Scroll();

private:
    // Internal pixel-pusher based on your provided base code
    static void PutChar(char c, unsigned int xOff, unsigned int yOff, uint32_t color);

    static int CursorX;
    static int CursorY;
    static BOOT_CONFIG* Config;
};