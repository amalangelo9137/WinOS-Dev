#pragma once

#include "Shared.h"
#include "../HIDs/HIDs.h"
#include "../Apps/Wdexe.h"

struct WindowSurface {
    uint32_t* Pixels;
    int Width;
    int Height;
    int Stride;
    bool Valid;
};

struct DirtyRect {
    int X;
    int Y;
    int Width;
    int Height;
};

void WindowManagerInit(BOOT_CONFIG* config);
void WindowManagerHandleMouseState(int mouseX, int mouseY, bool leftDown);
void WindowManagerHandleKeyboardInput(KeyboardInputEvent inputEvent);
void WindowManagerTick();
bool WindowManagerNeedsRedraw();
void WindowManagerRender();
