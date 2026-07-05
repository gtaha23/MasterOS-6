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
#include "shell.h"
#include "ata.h"
#include "fat.h"
#include "speaker.h"
#include "util.h"

uint32_t total_mem_kb = 0;

void kmain(uint32_t magic, struct multiboot_info* bootInfo);

void kmain(uint32_t magic, struct multiboot_info* bootInfo) {
	total_mem_kb = bootInfo->mem_upper;
    Reset();
    speaker_play(800);
    print("Welcome to MasterOS!\r\n");
    initGDT();
    print("GDT [DONE]\r\n");
    initIDT();
    print("IDT [DONE]\r\n");
    initTimer();

    uint32_t physicalAllocStart = 0x200000;

    initMemory(bootInfo->mem_upper * 1024, physicalAllocStart);
    print("Memory Allocation [DONE]\r\n");
    kmallocInit(0x1000);
    
    if (ata_init() == 0)
        print("ATA [DONE]\r\n");
    else
        print("ATA [FAILED - no disk detected]\r\n");

    if (fat_init() == 0)
        print("FAT [DONE]\r\n");
    else
        print("FAT [FAILED]\r\n");

    shell_init();
    initKeyb();
    print("Alloc start: ");
    print_uint(physicalAllocStart);
    print("\r\n");
    speaker_stop();

    print(">");

    for(;;);
}
