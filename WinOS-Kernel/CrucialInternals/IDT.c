#include "IDT.h"
#include <intrin.h>

IDT_Entry idt[256];
IDTR idtr;

void RemapPIC() {
	// ICW1: Start initialization
	__outbyte(0x20, 0x11);
	__outbyte(0xA0, 0x11);

	// ICW2: Set Offset (Hardware IRQs start at 0x20)
	__outbyte(0x21, 0x20);
	__outbyte(0xA1, 0x28);

	// ICW3: Setup Master/Slave wiring
	__outbyte(0x21, 0x04);
	__outbyte(0xA1, 0x02);

	// ICW4: 8086 mode
	__outbyte(0x21, 0x01);
	__outbyte(0xA1, 0x01);

	// Masking:
	// - Unmask cascade (IRQ2) and keyboard (IRQ1) on master.
	// - Unmask only IRQ12 on slave (mouse).
	// Master mask bits: bit=1 => masked. We want IRQ0 masked, IRQ1 unmasked, IRQ2 unmasked.
	// Binary: 11111001 = 0xF9
	__outbyte(0x21, 0xF9);
	__outbyte(0xA1, 0xEF);
}

void SetIDTEntry(uint8_t vector, void* handler) {
	uint64_t addr = (uint64_t)handler;
	idt[vector].BaseLow = (uint16_t)(addr & 0xFFFF);
	idt[vector].BaseMid = (uint16_t)((addr >> 16) & 0xFFFF);
	idt[vector].BaseHigh = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
	idt[vector].Selector = 0x08; // This MUST match your GDT Code Segment
	idt[vector].TypeAttr = 0x8E; // Interrupt Gate, Present, Ring 0
	idt[vector].IST = 0;
	idt[vector].Reserved = 0;
}

// ASM wrappers
extern void MouseHandler_Wrapper();
extern void KeyboardHandler_Wrapper();

extern void LoadIDT(IDTR* idtr_ptr);

void InitIDT() {
	RemapPIC();

	// IRQ1 -> vector 0x21 (0x20 + 1)
	SetIDTEntry(0x21, (void*)KeyboardHandler_Wrapper);

	// IRQ12 -> vector 0x2C
	SetIDTEntry(0x2C, (void*)MouseHandler_Wrapper);

	idtr.Limit = (sizeof(IDT_Entry) * 256) - 1;
	idtr.Base = (uint64_t)&idt;

	LoadIDT(&idtr);

	// Do NOT enable interrupts here; KernelMain does that after device init.
}