#include "rtc.h"
#include "arch/x86/cpu.h"

#define RTC_INDEX 0x70
#define RTC_DATA  0x71

static uint8_t rtc_reg(uint8_t reg) {
    outb(RTC_INDEX, reg);
    return inb(RTC_DATA);
}

static uint8_t bcd2bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

static int rtc_updating(void) {
    outb(RTC_INDEX, 0x0A);
    return inb(RTC_DATA) & 0x80;
}

void rtc_read(rtc_time_t* t) {
    while (rtc_updating());
    uint8_t regB = rtc_reg(0x0B);
    t->second = rtc_reg(0x00);
    t->minute = rtc_reg(0x02);
    t->hour   = rtc_reg(0x04);
    t->day    = rtc_reg(0x07);
    t->month  = rtc_reg(0x08);
    t->year   = rtc_reg(0x09);
    if (!(regB & 0x04)) {
        t->second = bcd2bin(t->second);
        t->minute = bcd2bin(t->minute);
        t->hour   = bcd2bin(t->hour);
        t->day    = bcd2bin(t->day);
        t->month  = bcd2bin(t->month);
        t->year   = bcd2bin(t->year);
    }
}