#include "vga.h"

extern const uint8_t IBM_VGA_8x8_bin[];
extern const uint32_t IBM_VGA_8x8_bin_len;
extern void outPortB(uint16_t port, uint8_t val);
extern void VGALF(const uint8_t *font, uint32_t height);

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const) 0xC00B8000;

const uint16_t defaultColor = (COLOR8_BLACK << 12) | (COLOR8_LIGHT_GREY << 8);
uint16_t currColor = defaultColor;
uint16_t* videoMemory = (uint16_t*) 0xC00B8000;
uint32_t VGAWidth = 80;
uint32_t VGAHeight = 50;

void Reset() {

    uint16_t blank_cell = ' ' | currColor;
    uint32_t total_cells = VGAWidth * VGAHeight;
    
    for (uint32_t i = 0; i < total_cells; i++) {
        vga[i] = blank_cell;
    }
    column = 0;
    line = 0;
}

void newLine() {
    if (line < VGAHeight - 1) {
        line++;
        column = 0;
    } else {
        scrollUp();
        column = 0;
    }
}

void reset_color()  {
    currColor = defaultColor;
}

void set_color(uint8_t fg, uint8_t bg) {
    currColor = ((bg & 0x0F) << 12) | ((fg & 0x0F) << 8);
}

void scrollUp() {
    for (uint16_t y = 1; y < VGAHeight; y++) {
        for (uint16_t x = 0; x < VGAWidth; x++) {
            vga[(y-1) * VGAWidth + x] = vga[y * VGAWidth + x];
        }
    }

    for (uint16_t x = 0; x < VGAWidth; x++) {
        vga[(VGAHeight-1) * VGAWidth + x] = ' ' | currColor;
    }
}

void VGASM80X50() {
    VGALF(IBM_VGA_8x8_bin, 8);

    outPortB(0x3D4, 0x09); outPortB(0x3D5, 0x07);
    outPortB(0x3D4, 0x12); outPortB(0x3D5, 0x9F);
    outPortB(0x3D4, 0x15); outPortB(0x3D5, 0xBF);
    outPortB(0x3D4, 0x16); outPortB(0x3D5, 0x1F);
    outPortB(0x3D4, 0x07); outPortB(0x3D5, 0x1F);

    VGAWidth = 80;
    VGAHeight = 50;

    Reset();
}

void print(const char* s) {
    while(*s) {
        switch(*s) {
            case '\n':
                newLine();
                break;
            case '\r':
                column = 0;
                break;
            case '\b':
                if (column == 0 && line != 0){
                    line--;
                    column = VGAWidth;
                }
                if (column > 0) {
                    column--;
                    vga[line * VGAWidth + column] = ' ' | currColor;
                }
                break;
            case '\t':
                if (column >= VGAWidth) {
                    newLine();
                }
                uint16_t tabLen = 4 - (column % 4);
                while (tabLen != 0 && column < VGAWidth) {
                    vga[line * VGAWidth + column] = ' ' | currColor;
                    column++;
                    tabLen--;
                }
                break;
            default:
                if (column >= VGAWidth) {
                    newLine();
                }
                vga[line * VGAWidth + column] = *s | currColor;
                column++;
                break;
            }
        s++;
    }
}
