#include "rtc.h"
#include "memory/util.h"

static uint8_t rtc_read_reg(uint8_t reg) {
    outPortB(0x70, reg);
    return inPortB(0x71);
}

unsigned char bcd_to_bin(unsigned char bcd) { 
    return ((bcd >> 4) * 10) + (bcd & 0x0F); 
}

static void rtc_read_raw(RTCTime* t) {
    while (rtc_read_reg(0x0A) & 0x80);
    t->seconds = rtc_read_reg(0x00); 
    t->minutes = rtc_read_reg(0x02);
    t->hours   = rtc_read_reg(0x04);
    t->day     = rtc_read_reg(0x07);
    t->month   = rtc_read_reg(0x08);
    t->year    = rtc_read_reg(0x09);
}

void rtc_read(RTCTime* t) {
    RTCTime last;
    rtc_read_raw(t);
    do {
        last = *t;
        rtc_read_raw(t);
    } while (t->seconds != last.seconds || t->minutes != last.minutes || t->hours != last.hours ||
             t->day != last.day || t->month != last.month || t->year != last.year);

    uint8_t stat_b = rtc_read_reg(0x0B);
    if (!(stat_b & 0x02)) {
        uint8_t pm = t->hours & 0x80;
        t->hours &= 0x7F;  
        if (!(stat_b & 0x04)) {
            t->hours = bcd_to_bin(t->hours);
        }    
        if (pm && t->hours < 12) t->hours += 12;
        if (!pm && t->hours == 12) t->hours = 0;
    } else {
        if (!(stat_b & 0x04)) {
            t->hours = bcd_to_bin(t->hours);
        }
    }
    if (!(stat_b & 0x04)) {
        t->seconds = bcd_to_bin(t->seconds); 
        t->minutes = bcd_to_bin(t->minutes); 
        t->day     = bcd_to_bin(t->day);
        t->month   = bcd_to_bin(t->month);
        t->year    = bcd_to_bin(t->year);
    }
}