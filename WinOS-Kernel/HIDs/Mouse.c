#include "Mouse.h"
#include "Shared.h" // For GlobalConfig
#include <intrin.h>

// Definition of globals
int MouseX = 100;
int MouseY = 100;
int MouseLeftDown = 0;
int MouseRightDown = 0;

static uint8_t MouseCycle = 0;
static uint8_t MouseData[3];

extern BOOT_CONFIG* GlobalConfig;

void MouseWait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (!(__inbyte(0x64) & 1) && timeout--);
    }
    else {
        while ((__inbyte(0x64) & 2) && timeout--);
    }
}

void MouseWrite(uint8_t data) {
    MouseWait(1);
    __outbyte(0x64, 0xD4);
    MouseWait(1);
    __outbyte(0x60, data);
}

uint8_t MouseRead() {
    MouseWait(0);
    return __inbyte(0x60);
}

void InitMouse() {
    MouseWait(1);
    __outbyte(0x64, 0xA8); // Enable auxiliary device

    MouseWait(1);
    __outbyte(0x64, 0x20); // Get command byte
    uint8_t status = MouseRead() | 2;
    MouseWait(1);
    __outbyte(0x64, 0x60); // Set command byte
    MouseWait(1);
    __outbyte(0x60, status);

    MouseWrite(0xF6); // Set default settings
    MouseRead();
    MouseWrite(0xF4); // Enable data reporting
    MouseRead();
}

void MouseHandler() {
    uint8_t status = __inbyte(0x64);

    // Bit 0 = Output Buffer Full, Bit 5 = Mouse Data
    if ((status & 0x01) && (status & 0x20)) {
        uint8_t data = __inbyte(0x60);

        // SYNC: The first byte of a packet ALWAYS has Bit 3 set to 1.
        // If we are at cycle 0 and Bit 3 is 0, we are out of sync. Drop it.
        if (MouseCycle == 0 && !(data & 0x08)) {
            goto end;
        }

        MouseData[MouseCycle++] = data;

        if (MouseCycle == 3) {
            MouseCycle = 0;

            // Buttons
            MouseLeftDown = (MouseData[0] & 0x01);
            MouseRightDown = (MouseData[0] & 0x02);

            // Relative movement (Signed 8-bit)
            int8_t rel_x = (int8_t)MouseData[1];
            int8_t rel_y = (int8_t)MouseData[2];

            // Update Position
            MouseX += rel_x;
            MouseY -= rel_y; // PS/2 Y is up-positive, Screen Y is down-positive

            // CLAMPING: Prevent underflow/overflow (keeps mouse on screen)
            if (MouseX < 0) MouseX = 0;
            if (MouseY < 0) MouseY = 0;
            if (MouseX >= (int)GlobalConfig->HorizontalResolution)
                MouseX = GlobalConfig->HorizontalResolution - 1;
            if (MouseY >= (int)GlobalConfig->VerticalResolution)
                MouseY = GlobalConfig->VerticalResolution - 1;
        }
    }

end:
    __outbyte(0xA0, 0x20); // EOI Slave
    __outbyte(0x20, 0x20); // EOI Master
}