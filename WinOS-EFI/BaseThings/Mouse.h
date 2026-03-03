#ifndef MOUSE_H
#define MOUSE_H

#include <Uefi.h>
#include <Protocol/SimplePointer.h>

typedef struct {
    INT32 X;
    INT32 Y;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} MOUSE_STATE;

EFI_STATUS InitMouse(EFI_SIMPLE_POINTER_PROTOCOL** Mouse);
void UpdateMouse(EFI_SIMPLE_POINTER_PROTOCOL* Mouse, MOUSE_STATE* State, UINT32 ScreenW, UINT32 ScreenH);

#endif