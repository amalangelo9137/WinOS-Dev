#include <stdint.h>
#include <intrin.h> // REQUIRED for __inbyte and __outbyte
#include "../Console/Console.h"

// Basic QWERTY Scancode Map (Set 1)
const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

extern "C" {
    void KeyboardHandler() {
        // 1. Read the scancode from PS/2 Data Port
        uint8_t scancode = __inbyte(0x60);

        // 2. Check if it's a "Key Press" (top bit is 0)
        if (!(scancode & 0x80)) {
            if (scancode < sizeof(scancode_to_ascii)) {
                char c = scancode_to_ascii[scancode];
                if (c != 0) {
                    char str[2] = { c, '\0' };
                    Console::Print(str, 0x00FF00); // Print in green!
                }
            }
        }

        // 3. Send End of Interrupt (EOI) to Master PIC (Port 0x20)
        __outbyte(0x20, 0x20);
    }

    void MouseHandler() {
        // 1. You MUST read the data port or the PS/2 controller will block future interrupts
        uint8_t mouseData = __inbyte(0x60);

        // (Mouse parsing logic goes here later)

        // 2. Send EOI to Slave PIC (Port 0xA0) AND Master PIC (Port 0x20)
        __outbyte(0xA0, 0x20);
        __outbyte(0x20, 0x20);
    }

    void RemapPIC() {
        // ICW1: Initialization
        __outbyte(0x20, 0x11);
        __outbyte(0xA0, 0x11);

        // ICW2: Vector Offsets (Map IRQ0-7 to 32-39, IRQ8-15 to 40-47)
        __outbyte(0x21, 0x20);
        __outbyte(0xA1, 0x28);

        // ICW3: Wiring
        __outbyte(0x21, 0x04);
        __outbyte(0xA1, 0x02);

        // ICW4: Mode (8086 mode)
        __outbyte(0x21, 0x01);
        __outbyte(0xA1, 0x01);

        // Unmask Keyboard (IRQ1) and Mouse (IRQ12)
        // 0xFD = 11111101 (Only IRQ1 enabled)
        // 0xEF = 11101111 (Only IRQ12 enabled on slave)
        __outbyte(0x21, 0xFD);
        __outbyte(0xA1, 0xEF);
    }
}