#include "exec.h"
#include "../fs/fat.h"
#include "../vga/vga.h"

#define LOAD_ADDRESS 0xC0800000  
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


    uint8_t* dest = (uint8_t*)LOAD_ADDRESS;
    for (int i = 0; i < size; i++) {
        dest[i] = program_buf[i];
    }


    void (*program)() = (void (*)())LOAD_ADDRESS;
    program();

    return 0;
}
