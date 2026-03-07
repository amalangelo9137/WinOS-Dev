.code

extern KeyboardHandler : proc
extern MouseHandler : proc

public LoadIDT
LoadIDT proc
    lidt fword ptr [rcx]
    sti ; Crucial: Re-enables interrupts on the CPU
    ret
LoadIDT endp

; Macro for IRQ Handlers to save time
IRQ_STUB macro name, handler
public name
name proc
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    sub rsp, 32
    call handler
    add rsp, 32
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
    iretq
name endp
endm

IRQ_STUB isr33, KeyboardHandler
IRQ_STUB isr44, MouseHandler

public isr6
isr6 proc
    iretq ; Ignore invalid opcodes for now
isr6 endp

end