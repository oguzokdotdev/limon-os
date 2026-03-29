#include "idt.h"

#define IDT_ENTRIES 256

static IDTEntry idt[IDT_ENTRIES];
static IDTPtr   idt_ptr;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void idt_set(int i, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[i].base_low  = base & 0xFFFF;
    idt[i].base_high = (base >> 16) & 0xFFFF;
    idt[i].selector  = sel;
    idt[i].zero      = 0;
    idt[i].flags     = flags;
}

// Объявления обработчиков из idt_asm.asm
extern void isr0(void);
extern void isr_keyboard(void);

// Обработчики исключений (вызываются из ASM)
void isr0_handler(void) {
    // деление на ноль — пока просто висим
    __asm__ volatile ("cli; hlt");
}

// Обработчик клавиатуры (вызывается из ASM)
void keyboard_handler(void);  // определена в kernel.c

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    // Обнуляем таблицу
    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set(i, 0, 0, 0);

    // Настраиваем PIC (перемапируем IRQ на 0x20-0x2F)
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFD); outb(0xA1, 0xFF); // маскируем всё кроме клавиатуры

    // ISR 0 — деление на ноль
    idt_set(0,    (uint32_t)isr0,         0x08, 0x8E);
    // IRQ1 — клавиатура (0x20 + 1 = 0x21)
    idt_set(0x21, (uint32_t)isr_keyboard, 0x08, 0x8E);

    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
    __asm__ volatile ("sti");  // включаем прерывания
}