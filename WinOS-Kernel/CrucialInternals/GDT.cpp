#include "GDT.h"

// MSVC specific command to align this table to a 4KB page boundary
__declspec(align(4096)) GDTTable DefaultGDT;

void InitGDT() {
    // 1. Null Descriptor (Required by CPU)
    DefaultGDT.Null = { 0, 0, 0, 0, 0, 0 };

    // 2. Kernel Code Segment
    // Access: 0x9A (Present, Ring 0, Executable, Readable)
    // Flags:  0x20 (64-bit Long Mode active)
    DefaultGDT.KernelCode = { 0, 0, 0, 0x9A, 0x20, 0 };

    // 3. Kernel Data Segment
    // Access: 0x92 (Present, Ring 0, Writable)
    // Flags:  0x00 
    DefaultGDT.KernelData = { 0, 0, 0, 0x92, 0x00, 0 };

    // 4. User Data Segment (For later, Ring 3)
    // Access: 0xF2 (Present, Ring 3, Writable)
    DefaultGDT.UserData = { 0, 0, 0, 0xF2, 0x00, 0 };

    // 5. User Code Segment (For later, Ring 3)
    // Access: 0xFA (Present, Ring 3, Executable, Readable)
    // Flags:  0x20
    DefaultGDT.UserCode = { 0, 0, 0, 0xFA, 0x20, 0 };

    GDTDescriptor gdtDescriptor;
    gdtDescriptor.Size = sizeof(GDTTable) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;

    LoadGDT(&gdtDescriptor);
}