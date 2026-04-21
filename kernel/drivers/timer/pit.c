#include "pit.h"
#include "arch/x86/cpu.h"

static volatile uint32_t timer_ticks    = 0;
static volatile uint32_t uptime_seconds = 0;

void pit_init(void) {
    uint32_t divisor = 1193180 / 100;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_handler(void) {
    timer_ticks++;
    if (timer_ticks % 100 == 0) uptime_seconds++;
    outb(0x20, 0x20);
}

void boot_delay(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks - start < ticks) __asm__ volatile("pause");
}

uint32_t pit_get_uptime(void) { return uptime_seconds; }
uint32_t pit_get_ticks(void)  { return timer_ticks; }