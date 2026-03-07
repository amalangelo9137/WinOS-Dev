#pragma once
#include <stdint.h>

// Standard size_t definition for OSDev
typedef uint64_t size_t;

#ifdef __cplusplus
extern "C" {
#endif

#pragma function(memset) // tell the compiler THIS IS MY FUNCTION NOW STFU
// Fill memory with a value (like clearing the screen)
void* memset(void* dest, int ch, size_t count) {
    unsigned char* p = (unsigned char*)dest;
    while (count--) *p++ = (unsigned char)ch;
    return dest;
}

#pragma function(memcpy)
// Copy memory (like moving a UI window or sprite)
void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) *d++ = *s++;
    return dest;
}

#pragma function(memcmp)
// Compare memory (like checking if a filename matches "Kernel.exe")
int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char *p1 = (const unsigned char*)s1, *p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif