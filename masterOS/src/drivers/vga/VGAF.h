#pragma once

#include "stdint.h"
#include "../../memory/util.h"
#include "VGAR.h"
#include "vga.h"

void VGAEFA(void); // Enable font access (for loading fonts)
void VGADFA(void); // Disable font access (for loading fonts)

void VGALF(const uint8_t* font, uint32_t height); // Load font into VGA memory (font must be 256*height bytes)

void VGAL8X8F(void); // Load 8x8 font into VGA memory
void VGAL8X16F(void); // Load 8x16 font into VGA memory
void VGACLS(uint8_t color);