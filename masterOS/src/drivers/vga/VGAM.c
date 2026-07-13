#include "vga.h"
#include "VGAM.h"
#include "../../stdlib/stdio.h"
#include "../../memory/util.h"
#include "VGAR.h"
#include "VGAF.h"

void VGASM(VGAMode mode) {
    switch (mode) {
        case VGA_MODE_80X25:
            VGALOADREG(&vga_regs_80x25);
            VGAWidth = 80;
            VGAHeight = 25;
            break;
        case VGA_MODE_80X50:
            VGALOADREG(&vga_regs_80x50);
            VGAWidth = 80;
            VGAHeight = 50;
            VGAEFA();
            VGALF8X8F();
            VGADFA();
            Reset();
            break;
        case VGA_MODE_90X60:
            // TODO
            break;
        
        default:
            break;
    }
}