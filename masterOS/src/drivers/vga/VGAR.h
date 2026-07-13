#ifndef VGA_VGAR_H
#define VGA_VGAR_H

#include "stddef.h"
#include "vga.h"
#include "stdint.h"

struct VGARegisters {
    uint8_t misc;
    uint8_t CRTCR[25];
    uint8_t SR[5];
    uint8_t GCR[9];
    uint8_t ACR[21];
};

void VGAWCRTC(uint8_t index, uint8_t value); // Write to CRTC register (index 0-24)
void VGAWSR(uint8_t index, uint8_t value); // Write to Sequence register
uint8_t VGARSR(uint8_t index);
void VGAWmisc(uint8_t value); // Write to misc
uint8_t VGARmisc(void);
void VGAWGRAPH(uint8_t index, uint8_t value); //   
uint8_t VGARGRAPH(uint8_t index);
void VGAWATTR(uint8_t index, uint8_t value);
uint8_t VGARATTR(uint8_t index);
uint8_t VGARCRTC(uint8_t index); // Read from CRTC register
void VGALOADREG(const struct VGARegisters* regs);

#endif