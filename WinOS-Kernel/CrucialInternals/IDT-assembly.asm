.code

extern KeyboardHandler : proc
extern MouseHandler : proc

; RCX holds the pointer to GDTDescriptor
public LoadGDT
LoadGDT proc
    lgdt fword ptr [rcx]
    
    ; Setup Data Segments (0x10)
    mov ax, 10h 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup Code Segment (0x08) using a far return
    pop rdi
    mov rax, 08h 
    push rax
    push rdi
    retfq
LoadGDT endp

; RCX holds the pointer to IDTR
public LoadIDT
LoadIDT proc
    lidt fword ptr [rcx]
    sti ; Turn interrupts back on
    ret
LoadIDT endp

; --- INTERRUPT MACROS ---
SAVE_REGS macro
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
endm

RESTORE_REGS macro
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
endm

; ISR 33 - Keyboard (IRQ 1)
public isr33
isr33 proc
    SAVE_REGS
    sub rsp, 32 ; Allocate MS x64 shadow space
    call KeyboardHandler
    add rsp, 32 ; Clean up shadow space
    
    mov al, 20h
    out 20h, al ; EOI to Master PIC
    RESTORE_REGS
    iretq
isr33 endp

; ISR 44 - Mouse (IRQ 12)
public isr44
isr44 proc
    SAVE_REGS
    sub rsp, 32 
    call MouseHandler
    add rsp, 32
    
    mov al, 20h
    out 0A0h, al ; EOI to Slave PIC
    out 20h, al  ; EOI to Master PIC
    RESTORE_REGS
    iretq
isr44 endp

end