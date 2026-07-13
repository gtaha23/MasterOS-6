#include "vga.h"
#include "stdint.h"
#include "../../memory/util.h"
#include "VGAR.h"

void VGALOADREG(const struct VGARegisters* regs)
{
    VGAWmisc(regs->misc);

    for (size_t i = 0; i < 5; i++)
        VGAWSR(i, regs->SR[i]);


    VGAWCRTC(0x11, VGARCRTC(0x11) & 0x7F);
    VGAWCRTC(0x11, VGARCRTC(0x11) & 0x7F);

    for (size_t i = 0; i < 25; i++)
        VGAWCRTC(i, regs->CRTCR[i]);

    for (size_t i = 0; i < 9; i++)
        VGAGRAPH(i, regs->GCR[i]);

    for (size_t i = 0; i < 21; i++)
        VGAWATTR(i, regs->ACR[i]);

    inPortB(0x3DA);
    outPortB(0x3C0, 0x20);
}

void VGAWATTR(uint8_t index, uint8_t value)
{
    inPortB(0x3DA);
    outPortB(0x3C0, index);
    outPortB(0x3C0, value);
}

void VGAWmisc(uint8_t value) {
    outPortB(0x3C2, value);
}

void VGAWSR(uint8_t index, uint8_t value) {
    outPortB(0x3C4, index);
    outPortB(0x3C5, value);
}

void VGAGRAPH(uint8_t index, uint8_t value) {
    outPortB(0x3CE, index);
    outPortB(0x3CF, value);
}

void VGAWCRTC(uint8_t index, uint8_t value)
{
    outPortB(0x3D4, index);
    outPortB(0x3D5, value);
}

uint8_t VGARCRTC(uint8_t index)
{
    outPortB(0x3D4, index);
    return inPortB(0x3D5);
}