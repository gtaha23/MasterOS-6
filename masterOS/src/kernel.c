#include "stdint.h"
#include "interrupts/idt.h"
#include "vga.h"
#include "timer.h"
#include "gdt.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "multiboot.h"

void kmain(uint32_t magic, struct multiboot_info* bootInfo);

void kmain(uint32_t magic, struct multiboot_info* bootInfo) {
	Reset();
	print("MasterOS v0.6.0 Zeta \r\n");
	initGDT();
	print("GDT & TSS is working!\r\n");
	initIDT();
	initTimer();
	initKeyb();
	//initMemory(bootInfo);
	for(;;);
}
