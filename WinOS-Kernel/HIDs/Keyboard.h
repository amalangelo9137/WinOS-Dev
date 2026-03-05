#pragma once
#include <stdint.h>

// Simple key event
typedef struct {
    uint8_t scancode; // raw PS/2 scancode
    uint8_t pressed;  // 1 = make (pressed), 0 = break (released)
} KEY_EVENT;

// Init is optional; handler will be wired by IDT. Call from KernelMain if you want
void InitKeyboard(void);

// Non-blocking: returns 1 if event copied to out, 0 if none available
int PollKeyEvent(KEY_EVENT* out);

// Blocking: waits until an event is available (busy-wait)
void WaitKeyEvent(KEY_EVENT* out);

// Convert scancode to simple ASCII (basic US qwerty, only common keys)
char ScancodeToAscii(uint8_t sc, uint8_t pressed);