#pragma once
#include <stdint.h>

// Shared variables (extern tells other files these exist in Mouse.c)
extern int MouseX;
extern int MouseY;
extern int MouseLeftDown;
extern int MouseRightDown;

void InitMouse();
void MouseHandler(); // Called by your ASM wrapper