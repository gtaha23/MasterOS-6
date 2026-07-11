#include "cpu.h"

void CGV(char* out) {
    uint32_t ebx, ecx, edx;
    __asm__ __volatile__(
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    *(uint32_t*)(out) = ebx;
    *(uint32_t*)(out + 4) = edx;
    *(uint32_t*)(out + 8) = ecx;
    *(uint32_t*)(out + 12) = 0; // null terminator
}