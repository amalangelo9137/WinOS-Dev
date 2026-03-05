; IDT-assembly.asm (MASM / ml64)

PUBLIC LoadIDT

.code

; This is the function C calls to give the IDT to the CPU
LoadIDT PROC
    lidt fword ptr [rcx]
    ret
LoadIDT ENDP

END