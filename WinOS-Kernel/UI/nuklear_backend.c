#include "Shared.h"
#include "Compositor.h"

#define NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"

// --- FORWARD DECLARATION ---
float Get_Font_Width(nk_handle handle, float height, const char* text, int len);

// --- NUKLEAR GLOBALS ---
struct nk_context ctx;
struct nk_user_font winos_font;

// --- FONT LOGIC ---
float Get_Font_Width(nk_handle handle, float height, const char* text, int len) {
    // 14.0f is the spacing between letters. Adjust if text overlaps or is too wide.
    return (float)len * 14.0f;
}

// Allocate 64KB of RAM for Nuklear's internal state (BSS section)
#define NUKLEAR_MEMORY_SIZE (64 * 1024)
uint8_t nuklear_memory[NUKLEAR_MEMORY_SIZE];

void InitNuklear(BOOT_CONFIG* config) {
    winos_font.userdata = nk_handle_ptr(config->FontSpriteData);
    winos_font.height = (float)config->FontGlyphSize; // 32
    winos_font.width = Get_Font_Width;

    // The ACTUAL Nuklear function for freestanding/no-malloc environments
    nk_init_fixed(&ctx, nuklear_memory, NUKLEAR_MEMORY_SIZE, &winos_font);
}

// --- RENDER LOGIC ---
void Nuklear_Render() {
    const struct nk_command* cmd;
    nk_foreach(cmd, &ctx) {

        // DRAW RECTANGLES (Window backgrounds, buttons)
        if (cmd->type == NK_COMMAND_RECT) {
            const struct nk_command_rect* r = (const struct nk_command_rect*)cmd;
            uint32_t col = (r->color.r << 16) | (r->color.g << 8) | r->color.b;

            for (int y = 0; y < r->h; y++) {
                for (int x = 0; x < r->w; x++) {
                    Compositor_PutPixel(r->x + x, r->y + y, col);
                }
            }
        }

        // DRAW TEXT
        else if (cmd->type == NK_COMMAND_TEXT) {
            const struct nk_command_text* t = (const struct nk_command_text*)cmd;
            uint32_t* atlas = GlobalConfig->FontSpriteData;
            uint32_t atlasW = GlobalConfig->FontSpriteWidth;
            uint32_t gSize = GlobalConfig->FontGlyphSize;
            int spacing = 14; // Must match what you put in Get_Font_Width

            for (int i = 0; i < t->length; i++) {
                char c = t->string[i];
                if (c < 32 || c > 126) continue;

                // Find the letter in the 16x16 grid
                int idx = c - 32;
                int srcX = (idx % 16) * gSize;
                int srcY = (idx / 16) * gSize;

                // Draw the letter pixels
                for (int gy = 0; gy < gSize; gy++) {
                    for (int gx = 0; gx < gSize; gx++) {
                        uint32_t pixel = atlas[(srcY + gy) * atlasW + (srcX + gx)];

                        // If pixel is not black, draw it
                        if ((pixel & 0xFFFFFF) != 0) {
                            uint32_t textCol = (t->foreground.r << 16) |
                                (t->foreground.g << 8) |
                                t->foreground.b;

                            Compositor_PutPixel(t->x + (i * spacing) + gx, t->y + gy, textCol);
                        }
                    }
                }
            }
        }
    }
    // Very important: Clear the queue for the next frame
    nk_clear(&ctx);
}