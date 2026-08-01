#include "exec.h"
#include "../fs/fat.h"
#include "../vga/vga.h"
#include "../../memory/memory.h"
#include "../../memory/util.h"

#define LOAD_ADDRESS 0x00300000  
#define MAX_PROGRAM_SIZE (64 * 1024)  // 64KB max


static uint8_t program_buf[MAX_PROGRAM_SIZE];

int exec_run(const char* filename) {
    int size = fat_read(filename, program_buf, MAX_PROGRAM_SIZE);
    if (size < 0) {
        print("Program not found: ");
        print(filename);
        print("\r\n");
        return -1;
    }

    uint32_t numPages = CEIL_DIV((uint32_t)size, 0x1000);
    for (uint32_t p = 0; p < numPages; p++) {
        uint32_t vaddr = LOAD_ADDRESS + p * 0x1000;
        memMapPage(vaddr, vaddr, PAGE_FLAG_PRES | PAGE_FLAG_WRT);
    }

    uint8_t* dest = (uint8_t*)LOAD_ADDRESS;
    for (int i = 0; i < size; i++) {
        dest[i] = program_buf[i];
    }

    void (*program)() = (void (*)())LOAD_ADDRESS;
    program();

    return 0;
}
