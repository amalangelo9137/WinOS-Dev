#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void KernelHeap_Init(void); // call from KernelMain early
void* KernelMalloc(size_t size);
void  KernelFree(void* ptr); // no-op (placeholder)

#ifdef __cplusplus
}
#endif