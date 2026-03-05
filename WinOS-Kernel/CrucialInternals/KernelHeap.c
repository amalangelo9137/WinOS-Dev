#include "KernelHeap.h"
#include <stdint.h>

// Very small bump allocator for early kernel use.
// Replace with a real allocator later.
#define KERNEL_HEAP_SIZE (1024 * 1024) // 1 MiB

static uint8_t kernel_heap[KERNEL_HEAP_SIZE];
static size_t heap_off = 0;

void KernelHeap_Init(void) {
    heap_off = 0;
}

void* KernelMalloc(size_t size) {
    if (size == 0) return NULL;
    // 8-byte align
    size = (size + 7) & ~((size_t)7);
    if (heap_off + size > KERNEL_HEAP_SIZE) return NULL; // OOM
    void* p = &kernel_heap[heap_off];
    heap_off += size;
    return p;
}

void KernelFree(void* ptr) {
    (void)ptr; // no-op for now
}