#include "Shared.h"
#include "CrucialInternals/GDT.h"
#include "CrucialInternals/IDT.h"
#include "Rendering/Renderer.h"
#include <intrin.h>

extern float MouseX;
extern float MouseY;
extern bool MouseLeftDown;

extern "C" void InitMouse();

extern "C" __declspec(dllexport) void KernelMain(BOOT_CONFIG* config) {
    InitGDT();
    InitIDT();
    InitMouse(); // Start the hardware

    while (true) {
        // 1. Clear Screen
        for (uint32_t i = 0; i < config->Width * config->Height; i++) {
            config->BackBuffer[i] = 0x1a1a1a;
        }

        // 2. Draw your UI/Square
        uint32_t color = MouseLeftDown ? 0xFF0000 : 0xFFFFFF;
        DrawRect((int)MouseX, (int)MouseY, 15, 15, color, config);

        // 3. Flip
        SwitchBuffer(config);

        __halt();
    }
}