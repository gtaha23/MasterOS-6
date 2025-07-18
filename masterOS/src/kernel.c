#include "stdint.h"
#include "util.h"
#include "interrupts/idt.h"
#include "vga.h"
#include "timer.h"
#include "gdt.h"
#include "stdlib/stdio.h"
#include "keyboard.h"

void kmain(void);

void kmain(void) {
	Reset();
	print("MasterOS v0.6.0 Alpha \r\n");
	initGDT();
	print("GDT & TSS is working!\r\n");
	initIDT();
	initTimer();
	initKeyb();
	for(;;);
}
