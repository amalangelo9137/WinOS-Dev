#include "IDT.h"

// 4KB alignment for stability
__declspec(align(4096)) IDTEntry idt[256];

// Assembly stubs declared in idt.asm
extern "C" void isr6();  // Invalid Opcode safety net
extern "C" void isr33(); // Keyboard
extern "C" void isr44(); // Mouse

void SetIDTGate(int vector, void* handler, uint16_t selector, uint8_t attributes) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].OffsetLow = (uint16_t)(addr & 0xFFFF);
    idt[vector].Selector = selector; // Kernel Code Segment (0x08)
    idt[vector].IST = 0;
    idt[vector].Attributes = attributes;
    idt[vector].OffsetMid = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].OffsetHigh = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].Reserved = 0;
}

void InitIDT() {
    // 0x8E = Present, Ring 0, Interrupt Gate
    SetIDTGate(6, (void*)isr6, 0x08, 0x8E);
    SetIDTGate(33, (void*)isr33, 0x08, 0x8E); // IRQ 1
    SetIDTGate(44, (void*)isr44, 0x08, 0x8E); // IRQ 12

    IDTDescriptor idtr;
    idtr.Limit = (sizeof(IDTEntry) * 256) - 1;
    idtr.Address = (uint64_t)&idt;

    LoadIDT(&idtr);
}