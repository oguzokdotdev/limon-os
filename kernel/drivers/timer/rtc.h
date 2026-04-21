#ifndef RTC_H
#define RTC_H

#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;   // 0-99, прибавь 2000 для полного года
} rtc_time_t;

void rtc_read(rtc_time_t* t);

#endif
