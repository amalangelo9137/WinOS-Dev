; FastCopy.asm (MASM / ml64)
.code

; RCX = Destination
; RDX = Source
; R8  = Count (number of 8-byte chunks)
FastCopy PROC
    ; Use SSE registers to move data 16 bytes at a time
    shr r8, 1           ; Divide count by 2 because we move 16 bytes per loop
    
copy_loop:
    movdqu xmm0, xmmword ptr [rdx] ; Load 16 bytes from source
    movdqu xmmword ptr [rcx], xmm0 ; Store 16 bytes to destination
    add rdx, 16
    add rcx, 16
    dec r8
    jnz copy_loop
    
    ret
FastCopy ENDP

END