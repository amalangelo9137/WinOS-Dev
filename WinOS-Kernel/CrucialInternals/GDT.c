#include "GDT.h"

int _fltused = 0;
GDT_Descriptor GDT_Pointer;

// The table itself (3 entries). Use standard x64 descriptors.
static uint64_t DefaultGDT[3] = {
	0x0000000000000000ULL, // 0: Null
	0x00AF9A000000FFFFULL, // 1: Kernel Code (selector = 0x08) - long mode code
	0x00AF92000000FFFFULL  // 2: Kernel Data (selector = 0x10) - data segment
};

extern void LoadGDT(GDT_Descriptor* gdt_ptr);

void InitGDT() {
	GDT_Pointer.Limit = (uint16_t)(sizeof(DefaultGDT) - 1);
	GDT_Pointer.Base = (uint64_t)&DefaultGDT;

	LoadGDT(&GDT_Pointer);
}