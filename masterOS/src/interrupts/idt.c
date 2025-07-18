#include "stdint.h"
#include "../util.h"
#include "../vga.h"
#include "idt.h"

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct idt_ptr;

extern void idt_flush(uint32_t);

void initIDT() {
	idt_ptr.lim = sizeof(struct idt_entry_struct) * 256 - 1;
	idt_ptr.base = (uint32_t) &idt_entries;

	memSet(&idt_entries, 0, sizeof(struct idt_entry_struct) * 256);

	// 0x20 commands and 0x21 for data
	// 0xA0 commands and 0xA1 for data

	outPortB(0x20, 0x11);
	outPortB(0xA0, 0x11);

	outPortB(0x21, 0x20);
	outPortB(0xA1, 0x28);

	outPortB(0x21, 0x04);
	outPortB(0xA1, 0x02);

	outPortB(0x21, 0x01);
	outPortB(0xA1, 0x01);

	outPortB(0x21, 0x0);
	outPortB(0xA1, 0x0);

	setIDTGate(0, (uint32_t)isr0, 0x08, 0x8E); // 0x8E -> 1000 1110 | 0x08 -> 0000 1000
	setIDTGate(1, (uint32_t)isr1, 0x08, 0x8E);
	setIDTGate(2, (uint32_t)isr2, 0x08, 0x8E);
	setIDTGate(3, (uint32_t)isr3, 0x08, 0x8E); 
	setIDTGate(4, (uint32_t)isr4, 0x08, 0x8E);
	setIDTGate(5, (uint32_t)isr5, 0x08, 0x8E);
	setIDTGate(6, (uint32_t)isr6, 0x08, 0x8E);
	setIDTGate(7, (uint32_t)isr7, 0x08, 0x8E);
	setIDTGate(8, (uint32_t)isr8, 0x08, 0x8E);
	setIDTGate(9, (uint32_t)isr9, 0x08, 0x8E);
	setIDTGate(10, (uint32_t)isr10, 0x08, 0x8E);
	setIDTGate(11, (uint32_t)isr11, 0x08, 0x8E);
	setIDTGate(12, (uint32_t)isr12, 0x08, 0x8E);
	setIDTGate(13, (uint32_t)isr13, 0x08, 0x8E);
	setIDTGate(14, (uint32_t)isr14, 0x08, 0x8E);
	setIDTGate(15, (uint32_t)isr15, 0x08, 0x8E);
	setIDTGate(16, (uint32_t)isr16, 0x08, 0x8E);
	setIDTGate(17, (uint32_t)isr17, 0x08, 0x8E);
	setIDTGate(18, (uint32_t)isr18, 0x08, 0x8E);
	setIDTGate(19, (uint32_t)isr19, 0x08, 0x8E);
	setIDTGate(20, (uint32_t)isr20, 0x08, 0x8E);
	setIDTGate(21, (uint32_t)isr21, 0x08, 0x8E);
	setIDTGate(22, (uint32_t)isr22, 0x08, 0x8E);
	setIDTGate(23, (uint32_t)isr23, 0x08, 0x8E);
	setIDTGate(24, (uint32_t)isr24, 0x08, 0x8E);
	setIDTGate(25, (uint32_t)isr25, 0x08, 0x8E);
	setIDTGate(26, (uint32_t)isr26, 0x08, 0x8E);
	setIDTGate(27, (uint32_t)isr27, 0x08, 0x8E);
	setIDTGate(28, (uint32_t)isr28, 0x08, 0x8E);
	setIDTGate(29, (uint32_t)isr29, 0x08, 0x8E);
	setIDTGate(30, (uint32_t)isr30, 0x08, 0x8E);
	setIDTGate(31, (uint32_t)isr31, 0x08, 0x8E);



	setIDTGate(32, (uint32_t)irq0, 0x08, 0x8E);
	setIDTGate(33, (uint32_t)irq1, 0x08, 0x8E);
    setIDTGate(34, (uint32_t)irq2, 0x08, 0x8E);
    setIDTGate(35, (uint32_t)irq3, 0x08, 0x8E);
    setIDTGate(36, (uint32_t)irq4, 0x08, 0x8E);
    setIDTGate(37, (uint32_t)irq5, 0x08, 0x8E);
    setIDTGate(38, (uint32_t)irq6, 0x08, 0x8E);
    setIDTGate(39, (uint32_t)irq7, 0x08, 0x8E);
    setIDTGate(40, (uint32_t)irq8, 0x08, 0x8E);
    setIDTGate(41, (uint32_t)irq9, 0x08, 0x8E);
    setIDTGate(42, (uint32_t)irq10, 0x08, 0x8E);
    setIDTGate(43, (uint32_t)irq11, 0x08, 0x8E);
    setIDTGate(44, (uint32_t)irq12, 0x08, 0x8E);
    setIDTGate(45, (uint32_t)irq13, 0x08, 0x8E);
    setIDTGate(46, (uint32_t)irq14, 0x08, 0x8E);
    setIDTGate(47, (uint32_t)irq15, 0x08, 0x8E);

	setIDTGate(128, (uint32_t)isr128, 0x08, 0x8E);
	setIDTGate(177, (uint32_t)isr177, 0x08, 0x8E);

	idt_flush((uint32_t)&idt_ptr);
}

void setIDTGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
	idt_entries[num].base_low = base & 0xFFFF;
	idt_entries[num].base_high = (base >> 16) & 0xFFFF;
	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags = flags | 0x60;
	
}

unsigned char* exception_messages[] =  {
	"Divison By Zero",
	"Debug",
	"Not Maskable Interrupt",
	"Breakpoint",
	"Into Detected Overflow",
	"Out of Bounds",
	"Invalid Opcode",
	"No Coprocessor",
	"Double fault",
	"Coprocessor Segment Overrun",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown Interrupt",
	"Coprocessor Fault",
	"Alignment Fault",
	"Machine Check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

void isr_handler(struct InterruptRegisters* regs) {
	if(regs->int_no < 32){
		print(exception_messages[regs->int_no]);
		print("\n");
		print("Exception! System Halted\n");
		for (;;);
	}
}

void *irq_routines[16] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};

void irq_install_handler(int irq, void (*handler)(struct InterruptRegisters *r)){
	irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq){
	irq_routines[irq] = 0;
}

void irq_handler(struct InterruptRegisters* regs){
	void (*handler)(struct InterruptRegisters *regs);

	handler = irq_routines[regs->int_no - 32];

	if (handler) {
		handler(regs);
	}

	if (regs->int_no >= 40) {
		outPortB(0xA0, 0x20);
	}

	outPortB(0x20,0x20);
}