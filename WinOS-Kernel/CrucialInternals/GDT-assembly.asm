; GDT-assembly.asm (MASM / ml64 syntax)

PUBLIC LoadGDT

.code

LoadGDT PROC
    ; RCX contains the GDT_Descriptor pointer
    lgdt fword ptr [rcx]

    ; 1. Load Data Segments
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 2. Reload CS via far return
    mov rax, 08h
    push rax            ; selector
    lea rax, [f_flush]  ; return address
    push rax
    db 48h, 0CBh

f_flush:
    ret
LoadGDT ENDP

END