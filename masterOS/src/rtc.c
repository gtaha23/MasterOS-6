#include "rtc.h"
#include "util.h"

static uint8_t rtc_read_reg(uint8_t reg) {
	outPortB(0x70, reg);
	return inPortB(0x71);
}

unsigned char bcd_to_bin(unsigned char bcd) { 
	return ((bcd >> 4) * 10) + (bcd & 0x0F); 
}

void rtc_read(RTCTime* t) {
	while (rtc_read_reg(0x0A) & 0x80);
	t->seconds = rtc_read_reg(0x00); 
	t->minutes = rtc_read_reg(0x02);
	t->hours = rtc_read_reg(0x04);
	t->day = rtc_read_reg(0x07);
	t->month = rtc_read_reg(0x08);
	t->year = rtc_read_reg(0x09);
	uint8_t stat_b = rtc_read_reg(0x0B);
	if(!(stat_b & 0x04)){
		t->seconds = bcd_to_bin(t->seconds); 
		t->minutes = bcd_to_bin(t->minutes); 
		t->hours = bcd_to_bin(t->hours);
		t->day = bcd_to_bin(t->day);
		t->month = bcd_to_bin(t->month);
		t->year = bcd_to_bin(t->year);
	}
}
