#ifndef DRIVERS_TIMER_PIT_H
#define DRIVERS_TIMER_PIT_H

#include <stdint.h>

void     pit_init(void);
void     timer_handler(void);
void     boot_delay(uint32_t ticks);
uint32_t pit_get_uptime(void);
uint32_t pit_get_ticks(void);

#endif