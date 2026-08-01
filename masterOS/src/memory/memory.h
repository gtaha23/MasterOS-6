#include "stdint.h"
extern uint32_t initial_page_dir[1024];
#define KERNEL_START 0xC0000000
#define PAGE_FLAG_PRES (1 << 0)
#define PAGE_FLAG_WRT (1 << 1)
#define KERNEL_MALLOC 0xD0000000
#define REC_PAGEDIR ((uint32_t*)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t*) (0xFFC00000 + ((i) << 12)))
#define PAGE_FLAG_OWNER (1 << 9)
#define PAGE_FLAG_USER 0x4

void initMemory(uint32_t memHigh, uint32_t physicalAllocStart);
void pmm_init(uint32_t memLow, uint32_t memHigh);
void inval(uint32_t vaddr);
uint32_t pmmAllocPageFrame();
uint32_t* memGCPD();
void memCPD(uint32_t* pd);
void syncPageDirs();
void memMapPage(uint32_t virtualAddr, uint32_t physicalAddr, uint32_t flags);
