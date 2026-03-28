#include "Renderer.h"

void DrawRect(int x, int y, int width, int height, uint32_t color, BOOT_CONFIG* config) {
    for (int i = y; i < y + height; i++) {
        for (int j = x; j < x + width; j++) {
            // Screen bounds check
            if (j >= 0 && j < config->Width && i >= 0 && i < config->Height) {
                config->BackBuffer[j + (i * config->PixelsPerScanLine)] = color;
            }
        }
    }
}

void DrawEllipse(int x, int y, int width, int height, uint32_t color, BOOT_CONFIG* config) {
    // Convert bounding box (x,y,w,h) to center and radii
    int rx = width / 2;
    int ry = height / 2;
    int xCenter = x + rx;
    int yCenter = y + ry;

    for (int cy = -ry; cy <= ry; cy++) {
        for (int cx = -rx; cx <= rx; cx++) {
            // Ellipse equation: (x^2 / rx^2) + (y^2 / ry^2) <= 1
            if (cx * cx * ry * ry + cy * cy * rx * rx <= rx * rx * ry * ry) {
                int drawX = xCenter + cx;
                int drawY = yCenter + cy;

                if (drawX >= 0 && drawX < config->Width && drawY >= 0 && drawY < config->Height) {
                    config->BackBuffer[drawX + (drawY * config->PixelsPerScanLine)] = color;
                }
            }
        }
    }
}