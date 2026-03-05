#pragma once
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t Limit; // Size of the table minus 1
    uint64_t Base;  // Memory address of the table
} GDT_Descriptor;
#pragma pack(pop)

// Our function to set it up
void InitGDT();