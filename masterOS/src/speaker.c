#include "speaker.h"
#include "util.h"

void speaker_play(uint32_t freq) { 
	uint32_t div = 1193180 / freq;
	outPortB(0x43, 0xB6);
	outPortB(0x42, (uint8_t)(div));
	outPortB(0x42, (uint8_t)(div >> 8));
	uint8_t temp = inPortB(0x61);
	if (temp != (temp | 3)){
		outPortB(0x61, temp | 3);
	}
}

void speaker_stop(){
	uint8_t tmp = inPortB(0x61) & 0xFC;
	outPortB(0x61, tmp);
}
