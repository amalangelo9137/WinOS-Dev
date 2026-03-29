#include "Shared.h"
#include <intrin.h>
#include "HIDs/HIDs.h"
#include "Rendering/WindowManager.h"

extern "C" void InitGDT();
extern "C" void InitIDT();

extern "C" BOOT_CONFIG* GlobalConfig = nullptr;

extern "C" __declspec(dllexport) void KernelMain(BOOT_CONFIG* config) {
    GlobalConfig = config;

    InitGDT();
    InitIDT();

    MouseX = (int)(config->Width / 2);
    MouseY = (int)(config->Height / 2);
    MouseLeftDown = false;

    if (config->Mouse != nullptr) {
        config->Mouse->X = MouseX;
        config->Mouse->Y = MouseY;
        config->Mouse->Left = false;
        config->Mouse->Right = false;
    }

    WindowManagerInit(config);
    WindowManagerHandleMouseState(MouseX, MouseY, MouseLeftDown);
    WindowManagerRender();

    InitMouse();

    while (true) {
        if (MouseStateDirty) {
            int mouseX = MouseX;
            int mouseY = MouseY;
            bool mouseLeftDown = MouseLeftDown;
            MouseStateDirty = false;
            WindowManagerHandleMouseState(mouseX, mouseY, mouseLeftDown);
        }

        KeyboardInputEvent keyboardEvent;
        while (PollKeyboardInput(&keyboardEvent)) {
            WindowManagerHandleKeyboardInput(keyboardEvent);
        }

        WindowManagerTick();

        if (WindowManagerNeedsRedraw()) {
            WindowManagerRender();
            continue;
        }

        __halt();
    }
}
