#include "stdint.h"
#include "interrupts/idt.h"
#include "drivers/vga/vga.h"
#include "timer.h"
#include "drivers/gdt.h"
#include "stdlib/stdio.h"
#include "memory/kmalloc.h"
#include "drivers/device/keyboard.h"
#include "multiboot.h"
#include "memory/memory.h"
#include "shell.h"
#include "drivers/fs/ata.h"
#include "drivers/fs/fat.h"
#include "drivers/device/speaker.h"
#include "memory/util.h"
#include "login.h"
#include "drivers/device/cpu.h"
#include "drivers/vga/VGAM.h"
#include "drivers/vga/VGAF.h"
#include "drivers/vga/VGAR.h"

uint32_t total_mem_kb = 0;

void kmain(uint32_t magic, struct multiboot_info* bootInfo);

void kmain(uint32_t magic, struct multiboot_info* bootInfo) {
    (void)magic;
	total_mem_kb = bootInfo->mem_upper;
    VGALF8X8F();
    VGAHeight = 50;
    Reset();
    speaker_play(800);
    print("Welcome to MasterOS!\r\n");
    print("VGA Driver(s) [DONE]\r\n");
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

    speaker_stop();
    initKeyb();
    print("Keyboard [DONE]\r\n");
    login();
    shell_init();
    keyboard_set_char_handler(shell_handle_char);
    print_prompt();
    reset_color();

    for(;;);
}
