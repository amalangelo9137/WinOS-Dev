#include "Mouse.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimplePointer.h>

// This manually provides the GUID the linker is looking for
EFI_GUID gEfiSimplePointerProtocolGuid = {
    0x3187edc3, 0x055d, 0x4ef5, { 0xad, 0x3e, 0xee, 0xda, 0x73, 0x41, 0x21, 0xaa }
};

EFI_STATUS InitMouse(EFI_SIMPLE_POINTER_PROTOCOL** Mouse) {
    EFI_STATUS Status = gBS->LocateProtocol(&gEfiSimplePointerProtocolGuid, NULL, (VOID**)Mouse);
    if (!EFI_ERROR(Status)) {
        (*Mouse)->Reset(*Mouse, TRUE);
    }
    return Status;
}

void UpdateMouse(EFI_SIMPLE_POINTER_PROTOCOL* Mouse, MOUSE_STATE* State, UINT32 ScreenW, UINT32 ScreenH) {
    EFI_SIMPLE_POINTER_STATE UefiState;

    // Check if the mouse has moved
    if (Mouse->GetState(Mouse, &UefiState) == EFI_SUCCESS) {
        // SimplePointer gives relative movement (delta)
        // We add that to our current X and Y
        State->X += (UefiState.RelativeMovementX / 10); // Sensitivity adjustment
        State->Y += (UefiState.RelativeMovementY / 10);

        // Clamp to screen edges (Unity's Math.Clamp)
        if (State->X < 0) State->X = 0;
        if (State->Y < 0) State->Y = 0;
        if (State->X > (INT32)ScreenW) State->X = ScreenW;
        if (State->Y > (INT32)ScreenH) State->Y = ScreenH;

        State->LeftButton = UefiState.LeftButton;
        State->RightButton = UefiState.RightButton;
    }
}