.code
extern KeyboardHandler : proc
extern MouseHandler : proc

public LoadIDT
LoadIDT proc
    lidt fword ptr [rcx]
    sti
    ret
LoadIDT endp

; Improved Macro to handle Slave EOIs
IRQ_HANDLER macro name, handler, is_slave
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
    
    ; End of Interrupt logic
    mov al, 20h
    if is_slave eq 1
        out 0A0h, al ; EOI to Slave PIC
    endif
    out 20h, al      ; EOI to Master PIC
    
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

IRQ_HANDLER isr33, KeyboardHandler, 0
IRQ_HANDLER isr44, MouseHandler, 1

public isr6
isr6 proc
    iretq
isr6 endp

end