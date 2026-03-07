; MASM 64-bit syntax
.code

public LoadGDT

LoadGDT proc
    ; 1. Load the GDT Pointer from RCX (1st argument in MSVC x64 calling convention)
    lgdt fword ptr [rcx]

    ; 2. Reload Data Segments to 0x10 (Kernel Data Index)
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 3. Reload Code Segment (CS) to 0x08 (Kernel Code Index)
    ; We must do a Far Return to update CS in 64-bit mode.
    
    ; Get return address currently at the top of the stack
    mov rax, qword ptr [rsp] 
    
    ; Overwrite the stack top with our new CS (0x08)
    mov r8, 08h             
    mov qword ptr [rsp], r8 
    
    ; Push the return address back on top of the new CS
    push rax                

    ; Execute Far Return (REX.W + RETF) to pop the Instruction Pointer and the new CS
    db 48h, 0cbh            
LoadGDT endp

end