#include "stdint.h"
#include "interrupts/idt.h"
#include "vga.h"
#include "timer.h"
#include "gdt.h"
#include "stdlib/stdio.h"
#include "kmalloc.h"
#include "keyboard.h"
#include "multiboot.h"
#include "memory.h"

void kmain(uint32_t magic, struct multiboot_info* bootInfo);

void kmain(uint32_t magic, struct multiboot_info* bootInfo) {
	Reset();
	print("MasterOS v0.6.2 Turan \r\n");
	initGDT();
	print("GDT [DONE]\r\n");
	initIDT();
	print("IDT [DONE]\r\n");
	initTimer();
	uint32_t mod1 = *(uint32_t*)(bootInfo->mods_addr + 4);
	uint32_t physicalAllocStart = (mod1 + 0xFF) & ~0xFFF;
		
	initMemory(bootInfo->mem_upper * 1024, physicalAllocStart);
	print("Memory Allocation [DONE]\r\n");
	kmallocInit(0x1000);
	print(">");
	initKeyb();
	
	for(;;);
}
