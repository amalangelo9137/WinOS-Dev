; Interrupts.asm (MASM / ml64)

EXTERN MouseHandler : PROC
EXTERN KeyboardHandler : PROC

PUBLIC MouseHandler_Wrapper
PUBLIC KeyboardHandler_Wrapper

.code

; Mouse wrapper
MouseHandler_Wrapper PROC
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11

    sub rsp, 28h
    call MouseHandler
    add rsp, 28h

    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax

    iretq
MouseHandler_Wrapper ENDP

; Keyboard wrapper
KeyboardHandler_Wrapper PROC
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11

    sub rsp, 28h
    call KeyboardHandler
    add rsp, 28h

    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax

    iretq
KeyboardHandler_Wrapper ENDP

END