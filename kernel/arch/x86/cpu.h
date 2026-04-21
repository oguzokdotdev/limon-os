#ifndef ARCH_X86_CPU_H
#define ARCH_X86_CPU_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void get_cpu_vendor(char* out);
void get_cpu_model(char* out);
uint32_t read_cr2(void);

#endif