#include "stdint.h"
#include "multiboot.h"
#include "stdlib/stdio.h"
#include "util.h"
#include "memory.h"

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAME (0x100000000 / 0x1000 / 8)

static uint32_t pageFrameMin;
static uint32_t pageFrameMax;
static uint32_t pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t pageDirsUsed[NUM_PAGES_DIRS];
static uint32_t totalAlloc;
uint8_t PMB[NUM_PAGE_FRAME / 8];
// physicalMemoryBitmap

void pmm_init(uint32_t memLow, uint32_t memHigh){
	pageFrameMin = CEIL_DIV(memLow, 0x1000);
	pageFrameMax = memHigh / 0x1000;
	totalAlloc = 0;

	memSet(PMB, 0, sizeof(PMB));
}

void inval(uint32_t vaddr){
	asm volatile("invlpg %0" :: "m"(vaddr));
}

// invlpg -> invalidate page (asm function)

void initMemory(uint32_t memHigh, uint32_t physicalAllocStart){
	initial_page_dir[0] = 0;
	inval(0);
	initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | PAGE_FLAG_PRES | PAGE_FLAG_WRT;
	inval(0xFFFFF000);

	pmm_init(physicalAllocStart, memHigh);
	memSet(pageDirs, 0, 0x1000 * NUM_PAGES_DIRS);
	memSet(pageDirsUsed, 0, NUM_PAGES_DIRS);
}


