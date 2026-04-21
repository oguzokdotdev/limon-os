#include "gdt.h"

#define GDT_ENTRIES 3

static GDTEntry gdt[GDT_ENTRIES];
static GDTPtr   gdt_ptr;

static void gdt_set(int i, uint32_t base, uint32_t limit,
                    uint8_t access, uint8_t gran) {
    gdt[i].base_low   = base & 0xFFFF;
    gdt[i].base_mid   = (base >> 16) & 0xFF;
    gdt[i].base_high  = (base >> 24) & 0xFF;
    gdt[i].limit_low  = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access     = access;
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set(0, 0, 0,          0x00, 0x00); // null
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // код
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // данные

    __asm__ volatile (
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        : : "r"(&gdt_ptr) : "ax"
    );
}