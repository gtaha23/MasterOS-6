#include "VGAF.h"
#include "VGAM.h"
#include "vga.h"
#include "VGAR.h"
#include "../../memory/util.h"
#include "ibm_font.h"

void VGAEFA() {
    outPortB(0x3C4, 0x02); outPortB(0x3C5, 0x04);
    outPortB(0x3C4, 0x04); outPortB(0x3C5, 0x06);

    outPortB(0x3CE, 0x04); outPortB(0x3CF, 0x02);
    outPortB(0x3CE, 0x05); outPortB(0x3CF, 0x00);
    outPortB(0x3CE, 0x06); outPortB(0x3CF, 0x00); 

    asm volatile("" ::: "memory");
}

void VGADFA() {
    outPortB(0x3C4, 0x02); outPortB(0x3C5, 0x03);
    outPortB(0x3C4, 0x04); outPortB(0x3C5, 0x02); 

    outPortB(0x3CE, 0x04); outPortB(0x3CF, 0x00);
    outPortB(0x3CE, 0x05); outPortB(0x3CF, 0x10);
    outPortB(0x3CE, 0x06); outPortB(0x3CF, 0x0E);

    asm volatile("" ::: "memory");
}

void VGALF8X8F(){
    VGALF(IBM_VGA_8x8_bin, 8);
}

void VGACLS(uint8_t color) {
    volatile uint16_t *vga_buffer = (volatile uint16_t *)0xB8000;
    
    uint16_t blank_cell = (uint16_t)0x20 | ((uint16_t)color << 8);
    
    for (int i = 0; i < 4000; i++) {
        vga_buffer[i] = blank_cell;
    }
}


void VGALF(const uint8_t *font, uint32_t height) {
    __asm__ __volatile__ (
        "movw $0x3C4, %%dx\n\t"
        "movb $0x02, %%al\n\t" "outb %%al, %%dx\n\t" 
        "incw %%dx\n\t"
        "movb $0x04, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3C4, %%dx\n\t"
        "movb $0x04, %%al\n\t" "outb %%al, %%dx\n\t" 
        "incw %%dx\n\t"
        "movb $0x06, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3CE, %%dx\n\t"
        "movb $0x04, %%al\n\t" "outb %%al, %%dx\n\t" 
        "incw %%dx\n\t"
        "movb $0x02, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3CE, %%dx\n\t"
        "movb $0x05, %%al\n\t" "outb %%al, %%dx\n\t" 
        "incw %%dx\n\t"
        "movb $0x00, %%al\n\t" "outb %%al, %%dx\n\t"

        "movw $0x3CE, %%dx\n\t"
        "movb $0x06, %%al\n\t" "outb %%al, %%dx\n\t" 
        "incw %%dx\n\t"
        "movb $0x00, %%al\n\t" "outb %%al, %%dx\n\t"
        : : : "ax", "dx", "memory"
    );

    volatile uint8_t *vgaFont = (volatile uint8_t*)0xA0000;
    
    for (int character = 0; character < 256; character++) {
        for (uint32_t scan = 0; scan < height; scan++) {
            *vgaFont++ = *font++;
        }

        for (uint32_t pad = height; pad < 32; pad++) {
            *vgaFont++ = 0x00;
        }
    }

    __asm__ __volatile__ (
        "movw $0x3C4, %%dx\n\t"
        "movb $0x02, %%al\n\t" "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x03, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3C4, %%dx\n\t"
        "movb $0x04, %%al\n\t" "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x02, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3CE, %%dx\n\t"
        "movb $0x04, %%al\n\t" "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x00, %%al\n\t" "outb %%al, %%dx\n\t" 

        "movw $0x3CE, %%dx\n\t"
        "movb $0x05, %%al\n\t" "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x10, %%al\n\t" "outb %%al, %%dx\n\t"

        "movw $0x3CE, %%dx\n\t"
        "movb $0x06, %%al\n\t" "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x0E, %%al\n\t" "outb %%al, %%dx\n\t" 
        : : : "ax", "dx", "memory"
    );

    uint8_t max_scan_line = (uint8_t)(height - 1); 

    __asm__ __volatile__ (
        "movw $0x3D4, %%dx\n\t"
        "movb $0x09, %%al\n\t" 
        "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "inb %%dx, %%al\n\t"   
        "andb $0xE0, %%al\n\t" 
        "orb %0, %%al\n\t"     
        "outb %%al, %%dx\n\t"  

        "movw $0x3D4, %%dx\n\t"
        "movb $0x0A, %%al\n\t"
        "outb %%al, %%dx\n\t"
        "incw %%dx\n\t"
        "movb $0x20, %%al\n\t"
        "outb %%al, %%dx\n\t"
        :
        : "q"(max_scan_line)
        : "ax", "dx"
    );
}

