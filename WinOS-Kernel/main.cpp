#include "Shared.h"
#include <intrin.h>
#include "InitFS/Assets.h"
#include "Rendering/Renderer.h"

// External declarations from headers/scripts
extern "C" void InitGDT();
extern "C" void InitIDT();
extern "C" void InitMouse();
extern "C" void SaveAndDrawMouse(int x, int y);

// Shared Global Config Pointer
extern "C" BOOT_CONFIG* GlobalConfig = nullptr;

// Shared Mouse Coords
extern float MouseX;
extern float MouseY;

extern "C" __declspec(dllexport) void KernelMain(BOOT_CONFIG* config) {
    // 1. Hook up the config so Drivers.cpp can see the framebuffer
    GlobalConfig = config;

    // 2. Init CPU Structures
    InitGDT();
    InitIDT();

    // 3. Set starting position
    MouseX = (float)(config->Width / 2);
    MouseY = (float)(config->Height / 2);

	void* bg = GetAsset("BACKGROUND", config);
    if (bg) DrawBMP(bg, 0, 0, config->Width, config->Height, config);

	SwitchBuffer(config);

    // 4. Initial capture of the background
    SaveAndDrawMouse((int)MouseX, (int)MouseY);

    // 5. Wake the mouse hardware
    InitMouse();

    // 6. Idle Loop
    while (true) {
        // The mouse moves itself via MouseHandler interrupts.
        // You can put window-drawing logic here later.
        __halt();
    }
}