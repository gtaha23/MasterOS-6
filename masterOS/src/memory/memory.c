#include "stdint.h"
#include "../multiboot.h"
#include "../stdlib/stdio.h"
#include "util.h"
#include "memory.h"

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAME (0x100000000 / 0x1000 / 8)

int mem_num_vpages;
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
	mem_num_vpages = 0;
	initial_page_dir[0] = 0;
	inval(0);
	initial_page_dir[1023] = ((uint32_t) initial_page_dir - KERNEL_START) | PAGE_FLAG_PRES | PAGE_FLAG_WRT;
	inval(0xFFFFF000);

	pmm_init(physicalAllocStart, memHigh);
	memSet(pageDirs, 0, 0x1000 * NUM_PAGES_DIRS);
	memSet(pageDirsUsed, 0, NUM_PAGES_DIRS);
}

uint32_t* memGCPD(){
	uint32_t pd;
	asm volatile("MOV %%cr3, %0": "=r"(pd));
	pd += KERNEL_START;

	return (uint32_t*)pd;
}

void syncPageDirs(){
	for (int i = 0; i < NUM_PAGES_DIRS; i++){
		if(pageDirsUsed[i]){
			uint32_t* pageDir = pageDirs[i];

			for (int i = 0; i< 1024; i++){
				pageDir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
			}
		}
	}
}

void memCPD(uint32_t* pd){
	pd = (uint32_t*) (((uint32_t)pd)-KERNEL_START);
	asm volatile("mov %0, %%eax \n mov %%eax, %%cr3 \n" :: "m"(pd));
}

void memMapPage(uint32_t virtualAddr, uint32_t physicalAddr, uint32_t flags){
	uint32_t *prevPageDir = 0;

	if (virtualAddr >= KERNEL_START){
		prevPageDir = memGCPD();
		if (prevPageDir != initial_page_dir){
			memCPD(initial_page_dir);
		}
	}

	uint32_t pdIndex = virtualAddr >> 22;
	uint32_t ptIndex = virtualAddr >> 12 & 0x3FF;

	uint32_t* pageDir = REC_PAGEDIR;
	uint32_t* pt = REC_PAGETABLE(pdIndex);

	if (!(pageDir[pdIndex] & PAGE_FLAG_PRES)){
		uint32_t ptPAddr = pmmAllocPageFrame();
		pageDir[pdIndex] = ptPAddr | PAGE_FLAG_PRES | PAGE_FLAG_WRT | PAGE_FLAG_OWNER | flags;
		inval(virtualAddr);

		for (uint32_t i = 0; i < 1024; i++){
			pt[i] = 0;
		}
	}

	pt[ptIndex] = physicalAddr | PAGE_FLAG_PRES | flags;
	mem_num_vpages++;
	inval(virtualAddr);

	if (prevPageDir != 0){
		syncPageDirs();
		if (prevPageDir != initial_page_dir){
			memCPD(prevPageDir);
		}
	}
}

uint32_t pmmAllocPageFrame(){
	uint32_t start = pageFrameMin / 8 + ((pageFrameMin & 7) != 0 ? 1:0);
	uint32_t end = pageFrameMax / 8 - ((pageFrameMax & 7) != 0 ? 1:0);

	for (uint32_t b = start; b < end; b++){
		uint8_t byte = PMB[b];
		if (byte == 0xFF){
			continue;
		}

		for (uint32_t i = 0; i < 8; i++){
			bool used = byte >> i & 1;

			if(!used){
				byte ^= (-1 ^byte) & (1 << i);
				PMB[b] = byte;
				totalAlloc++;
				
				uint32_t addr = (b*8*i) * 0x1000;
				return addr;
			}
		}
	}

	return 0;
}
