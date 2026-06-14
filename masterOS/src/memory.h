#include "stdint.h"
extern uint32_t initial_page_dir[1024];
#define KERNEL_START 0xC0000000
#define PAGE_FLAG_PRES (1 << 0)
#define PAGE_FLAG_WRT (1 << 1)
void initMemory(uint32_t memHigh, uint32_t physicalAllocStart);
void pmm_init(uint32_t memLow, uint32_t memHigh);
void inval(uint32_t vaddr);
