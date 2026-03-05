#include "Shared.h"
#include "UI/Compositor.h"
#include "CrucialInternals/GDT.h"
#include "CrucialInternals/IDT.h"
#include "UI/nuklear.h" // Assuming InitNuklear and Nuklear_Render are declared here
#include "HIDs/Mouse.h"

BOOT_CONFIG* GlobalConfig;
extern struct nk_context ctx; // Pulling ctx from UI.c

void KernelMain(BOOT_CONFIG* Config) {
    GlobalConfig = Config;

    InitGDT();
    InitIDT();
    InitMouse();

    // Initialize graphics and UI
    InitCompositor(Config);
    InitNuklear(Config);

    while (1) {
        // 1. Give Nuklear the Mouse Input
        nk_input_begin(&ctx);
        nk_input_motion(&ctx, MouseX, MouseY);
        nk_input_button(&ctx, NK_BUTTON_LEFT, MouseX, MouseY, MouseLeftDown);
        nk_input_end(&ctx);

        // 2. Build the UI Window
        if (nk_begin(&ctx, "System Logon", nk_rect(300, 200, 400, 250),
            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(&ctx, 35, 1);
            nk_label(&ctx, "WFS Secure OS", NK_TEXT_CENTERED);

            nk_layout_row_dynamic(&ctx, 35, 1);
            if (nk_button_label(&ctx, "Mount Drive")) {
                // Do something when clicked
            }
        }
        nk_end(&ctx);

        // 3. Render everything to the screen
        Compositor_Clear(0x1a1a1a); // Dark grey background
        Nuklear_Render();           // Draws UI and text
        Compositor_DrawMouse(MouseX, MouseY);     // Draw cursor on top
        Compositor_Update();        // Swap buffers to display
    }
}