#pragma once
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
	uint16_t    BaseLow;    // Lower 16 bits of handler address
	uint16_t    Selector;   // Your Kernel Code Segment (0x08 from GDT!)
	uint8_t     IST;        // Interrupt Stack Table (set to 0 for now)
	uint8_t     TypeAttr;   // Gate Type and Attributes
	uint16_t    BaseMid;    // Middle 16 bits of address
	uint32_t    BaseHigh;   // Upper 32 bits of address
	uint32_t    Reserved;   // Always 0
} IDT_Entry;

typedef struct {
	uint16_t Limit;
	uint64_t Base;
} IDTR;
#pragma pack(pop)

void InitIDT();