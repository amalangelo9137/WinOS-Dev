#include "Renderer.h"

void DrawWindow(const char* Name, unsigned int x, unsigned int y, unsigned int w, unsigned int h, BOOT_CONFIG* config) {
    int BorderPadding = 4;
    int TopPadding = 24;

    DrawRect(x, y, w, h, 0x00853D, config);
    DrawRect(x + BorderPadding, y + TopPadding, w - (2 * BorderPadding), h - (BorderPadding + TopPadding), 0x000000, config);
    PrintPos(Name, x + BorderPadding, y + BorderPadding, 0xFFFFFF, config);
}