#include "idt.h"

#define IDT_ENTRIES 256

static IDTEntry idt[IDT_ENTRIES];
static IDTPtr   idt_ptr;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void idt_set(int i, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[i].base_low  = base & 0xFFFF;
    idt[i].base_high = (base >> 16) & 0xFFFF;
    idt[i].selector  = sel;
    idt[i].zero      = 0;
    idt[i].flags     = flags;
}

// ISR из idt_asm.asm
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

extern void isr_keyboard(void);
extern void isr_timer(void);

static const char* exception_names[32] = {
    "#DE Division Error",
    "#DB Debug",
    "NMI Non-Maskable Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR Bound Range Exceeded",
    "#UD Invalid Opcode",
    "#NM No Math Coprocessor",
    "#DF Double Fault",
    "Coprocessor Segment Overrun",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection Fault",
    "#PF Page Fault",
    "Reserved (15)",
    "#MF x87 FPU Error",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XM SIMD Floating-Point Exception",
    "#VE Virtualization Exception",
    "Reserved (21)",
    "Reserved (22)", "Reserved (23)", "Reserved (24)",
    "Reserved (25)", "Reserved (26)", "Reserved (27)",
    "Reserved (28)", "Reserved (29)",
    "#SX Security Exception",
    "Reserved (31)"
};

void exception_handler(Registers* regs) {
    const char* name = (regs->int_no < 32)
        ? exception_names[regs->int_no]
        : "Unknown Exception";
    panic(name, regs);
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set(i, 0, 0, 0);

    // Настраиваем PIC
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFC); outb(0xA1, 0xFF);

    // Регистрируем все 32 обработчика исключений
    void* isrs[32] = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };
    for (int i = 0; i < 32; i++)
        idt_set(i, (uint32_t)isrs[i], 0x08, 0x8E);

    // IRQ0 — таймер, IRQ1 — клавиатура
    idt_set(0x20, (uint32_t)isr_timer,    0x08, 0x8E);
    idt_set(0x21, (uint32_t)isr_keyboard, 0x08, 0x8E);

    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
    __asm__ volatile ("sti");
}
