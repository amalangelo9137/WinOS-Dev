#include <stdint.h>
#include <intrin.h>
#include "../Console/Console.h"

extern "C" int _fltused = 0; // Required for floating point usage in kernel

// Global Mouse State
float MouseX = 500.0f;
float MouseY = 400.0f;
bool MouseLeftDown = false;

uint8_t mouse_cycle = 0;
int8_t mouse_packet[3];

// Basic QWERTY Scancode Map (Set 1)
const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void MouseWait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) while (timeout--) { if ((__inbyte(0x64) & 1) == 1) return; }
    else while (timeout--) { if ((__inbyte(0x64) & 2) == 0) return; }
}

void MouseWrite(uint8_t data) {
    MouseWait(1);
    __outbyte(0x64, 0xD4); // Next byte is for mouse
    MouseWait(1);
    __outbyte(0x60, data);
}

extern "C" void KeyboardHandler() {
        uint8_t scancode = __inbyte(0x60);
        if (!(scancode & 0x80)) {
            if (scancode < sizeof(scancode_to_ascii)) {
                char c = scancode_to_ascii[scancode];
                if (c != 0) {
                    char str[2] = { c, '\0' };
                    Console::Print(str, 0x00FF00);
                }
            }
        }
        __outbyte(0x20, 0x20);
    }

extern "C" void MouseHandler() {
        uint8_t status = __inbyte(0x64);
        if ((status & 0x01) && (status & 0x20)) {
            mouse_packet[mouse_cycle++] = __inbyte(0x60);
            if (mouse_cycle == 3) {
                mouse_cycle = 0;
                MouseLeftDown = (mouse_packet[0] & 0x01);

                // Apply movement (Byte 1 = X, Byte 2 = Y)
                MouseX += (float)mouse_packet[1];
                MouseY -= (float)mouse_packet[2]; // PS/2 Y is up, Screen Y is down

                // Clamp to screen bounds
                if (MouseX < 0) MouseX = 0;
                if (MouseY < 0) MouseY = 0;
            }
        }
        __outbyte(0xA0, 0x20); // EOI Slave
        __outbyte(0x20, 0x20); // EOI Master
    }

extern "C" void RemapPIC() {
        __outbyte(0x20, 0x11);
        __outbyte(0xA0, 0x11);
        __outbyte(0x21, 0x20);
        __outbyte(0xA1, 0x28);
        __outbyte(0x21, 0x04);
        __outbyte(0xA1, 0x02);
        __outbyte(0x21, 0x01);
        __outbyte(0xA1, 0x01);

        __outbyte(0x21, 0xF9); // 11111001 -> Kbd (1) + Cascade (2)
        __outbyte(0xA1, 0xEF); // 11101111 -> Mouse (12)
    }

extern "C" void InitMouse() {
        MouseWait(1);
        __outbyte(0x64, 0xA8); // Enable Aux
        MouseWait(1);
        __outbyte(0x64, 0x20); // Get ComByte
        MouseWait(0);
        uint8_t status = (__inbyte(0x60) | 2); // Enable IRQ 12
        MouseWait(1);
        __outbyte(0x64, 0x60); // Set ComByte
        MouseWait(1);
        __outbyte(0x60, status);

        MouseWrite(0xF4); // ENABLE DATA REPORTING (The "Wake Up")
        MouseWait(0);
        __inbyte(0x60);   // Read ACK
    }