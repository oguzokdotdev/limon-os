#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IDTPtr;

// Состояние регистров на момент исключения
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushad
    uint32_t int_no;    // номер исключения
    uint32_t err_code;  // код ошибки (или 0)
    uint32_t eip, cs, eflags; // запушено CPU
} __attribute__((packed)) Registers;

void idt_init(void);
void panic(const char* msg, Registers* regs);

#endif
