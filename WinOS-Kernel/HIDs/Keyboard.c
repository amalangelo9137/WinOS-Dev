#include <intrin.h>
#include "Keyboard.h"

// Small ring buffer for key events
#define KEYBUF_SIZE 64
static KEY_EVENT keybuf[KEYBUF_SIZE];
static volatile uint32_t k_head = 0;
static volatile uint32_t k_tail = 0;

static inline int keybuf_empty() { return k_head == k_tail; }
static inline int keybuf_full() { return ((k_head + 1) % KEYBUF_SIZE) == k_tail; }

void PushKeyEvent(KEY_EVENT e) {
    if (keybuf_full()) return; // drop if full
    keybuf[k_head] = e;
    k_head = (k_head + 1) % KEYBUF_SIZE;
}

int PollKeyEvent(KEY_EVENT* out) {
    if (keybuf_empty()) return 0;
    *out = keybuf[k_tail];
    k_tail = (k_tail + 1) % KEYBUF_SIZE;
    return 1;
}

void WaitKeyEvent(KEY_EVENT* out) {
    while (!PollKeyEvent(out)) { __halt(); }
}

// Very small init (no hardware steps for PS/2 keyboard required here)
void InitKeyboard(void) {
    // Nothing needed for basic PS/2 keyboard if PIC & IDT are set up and IRQ1 unmasked.
}

// Very small scancode -> ascii mapping (set of common keys, extend later)
char ScancodeToAscii(uint8_t sc, uint8_t pressed) {
    // Only handle make codes for simplicity
    if (!pressed) return 0;

    switch (sc) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';
        case 0x1C: return '\n'; // Enter (make)
        default: return 0;
    }
}

// This function is the C handler called by the asm wrapper.
// It must be visible to the assembler as 'KeyboardHandler'.
void KeyboardHandler() {
    uint8_t sc = __inbyte(0x60);

    // For PS/2, break codes are 0xF0 preceding the scancode (set 2) or high-bit set (set 1).
    // Here we use a simple heuristic: if sc == 0xF0 we wait for next (scan set 2 break).
    static int expect_break = 0;
    uint8_t pressed = 1;

    if (sc == 0xF0) {
        expect_break = 1;
        // send EOI for this IRQ packet? PS/2 break indicator is data, still send EOI at end.
        // Continue to next call.
    } else {
        if (expect_break) {
            pressed = 0;
            expect_break = 0;
        }
        KEY_EVENT evt = { .scancode = sc, .pressed = pressed };
        PushKeyEvent(evt);
    }

    // EOI to PIC
    __outbyte(0x20, 0x20);
}