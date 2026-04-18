#include "rtc.h"
#include <stdint.h>

#define RTC_INDEX 0x70
#define RTC_DATA  0x71

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static uint8_t rtc_reg(uint8_t reg) {
    outb(RTC_INDEX, reg);
    return inb(RTC_DATA);
}

static int rtc_updating(void) {
    outb(RTC_INDEX, 0x0A);
    return inb(RTC_DATA) & 0x80;
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

void rtc_read(rtc_time_t* t) {
    rtc_time_t a, b;

    do {
        while (rtc_updating());
        a.second = rtc_reg(0x00);
        a.minute = rtc_reg(0x02);
        a.hour   = rtc_reg(0x04);
        a.day    = rtc_reg(0x07);
        a.month  = rtc_reg(0x08);
        a.year   = rtc_reg(0x09);

        while (rtc_updating());
        b.second = rtc_reg(0x00);
        b.minute = rtc_reg(0x02);
        b.hour   = rtc_reg(0x04);
        b.day    = rtc_reg(0x07);
        b.month  = rtc_reg(0x08);
        b.year   = rtc_reg(0x09);
    } while (a.second != b.second || a.minute != b.minute ||
             a.hour   != b.hour   || a.day    != b.day    ||
             a.month  != b.month  || a.year   != b.year);

    // Status B bit2: 1 = binary, 0 = BCD
    uint8_t status_b = rtc_reg(0x0B);
    if (!(status_b & 0x04)) {
        t->second = bcd_to_bin(b.second);
        t->minute = bcd_to_bin(b.minute);
        t->hour   = bcd_to_bin(b.hour);
        t->day    = bcd_to_bin(b.day);
        t->month  = bcd_to_bin(b.month);
        t->year   = bcd_to_bin(b.year);
    } else {
        *t = b;
    }
}
