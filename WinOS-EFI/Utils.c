#pragma function(memset)
#include <Library/BaseLib.h>

/**
 * memset - Fills a block of memory with a specific byte value.
 * @dest:  Pointer to the memory block to fill.
 * @ch:    The character (byte) to fill the memory with.
 * @count: The number of bytes to fill.
 */
void* memset(void* dest, int ch, UINTN count) {
    // We cast to (unsigned char*) because we want to fill memory byte-by-byte
    unsigned char* p = (unsigned char*)dest;

    while (count > 0) {
        *p = (unsigned char)ch;
        p++;
        count--;
    }

    return dest;
}