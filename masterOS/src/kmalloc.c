#include "kmalloc.h"
#include "util.h"


static uint32_t heapStart;
static uint32_t heapSize; // heap -> öbek
static uint32_t threshold; // threshold -> eşik
static bool kmallocInitialized = false;

void changeHeapSize(int newSize){
	int oldPageTop = CEIL_DIV(heapSize, 0x1000);
	int newPageTop = CEIL_DIV(newSize, 0x1000);

	int diff = newPageTop - oldPageTop;

	for (int i = 0; i < diff; i++){
		uint32_t phys = pmmAllocPageFrame();
		memMapPage(KERNEL_MALLOC + oldPageTop * 0x1000 + i *0x1000, phys,PAGE_FLAG_WRT);
	}
}

void kmallocInit(uint32_t initialHeapSize){

	heapStart = KERNEL_MALLOC;
	heapSize = 0;
	threshold = 0;
	kmallocInitialized = true;

	changeHeapSize(initialHeapSize);
	
}
