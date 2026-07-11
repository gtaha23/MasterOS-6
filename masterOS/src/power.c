#include "power.h"
#include "memory/util.h"

void reboot() {
    outPortB(0x64, 0xFE);
}

void shutdown() {
    outPortW(0x604, 0x2000);  // QEMU shutdown
}