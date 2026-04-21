#include "panic.h"
#include "drivers/video/vga.h"
#include "drivers/timer/pit.h"
#include "arch/x86/cpu.h"

/* version.h генерируется Makefile-ом в kernel/core/ */
#include "version.h"

static void divider(int row) {
    vga_set_color(YELLOW, RED);
    vga_set_cursor(0, row);
    for (int i = 0; i < VGA_WIDTH; i++) vga_putchar('=');
}

void panic(const char* msg, Registers* regs) {
    __asm__ volatile ("cli");
    hw_cursor_hide();
    vga_fill(WHITE, RED, ' ');

    divider(4);
    vga_set_color(WHITE, RED);
    vga_center_print("!!!  KERNEL PANIC  !!!", 5);
    divider(6);

    vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 7); vga_print(" Exception : ");
    vga_set_color(WHITE, RED);      vga_print(msg);

    uint32_t uptime = pit_get_uptime();
    uint32_t uh = uptime / 3600, um = (uptime % 3600) / 60, us = uptime % 60;
    vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 8); vga_print(" System    : ");
    vga_set_color(WHITE, RED);
    vga_print("LimonOS " LIMON_CODENAME " v" LIMON_VERSION_STRING "-b");
    vga_print_int(LIMON_BUILD);
    vga_set_color(LIGHT_GREY, RED); vga_print("   Uptime: ");
    vga_set_color(WHITE, RED);
    vga_print_padded2(uh); vga_print(":"); vga_print_padded2(um); vga_print(":"); vga_print_padded2(us);

    char cpu[49];
    get_cpu_model(cpu);
    vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 9); vga_print(" CPU       : ");
    vga_set_color(WHITE, RED); vga_print(cpu);

    divider(10);

    vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 11); vga_print(" EIP: ");
    vga_set_color(WHITE, RED);
    if (regs) vga_print_hex(regs->eip); else vga_print("0x????????");
    vga_set_color(LIGHT_GREY, RED); vga_print("    Error: ");
    vga_set_color(WHITE, RED);
    if (regs) vga_print_hex(regs->err_code); else vga_print("0x????????");
    if (regs && regs->int_no == 14) {
        vga_set_color(LIGHT_GREY, RED); vga_print("    CR2: ");
        vga_set_color(WHITE, RED); vga_print_hex(read_cr2());
    }

    divider(12);

    if (regs) {
        vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 13);
        vga_print(" EAX: "); vga_set_color(WHITE, RED); vga_print_hex(regs->eax);
        vga_set_color(LIGHT_GREY, RED); vga_print("  EBX: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->ebx);
        vga_set_color(LIGHT_GREY, RED); vga_print("  ECX: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->ecx);
        vga_set_color(LIGHT_GREY, RED); vga_print("  EDX: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->edx);

        vga_set_color(LIGHT_GREY, RED); vga_set_cursor(0, 14);
        vga_print(" ESI: "); vga_set_color(WHITE, RED); vga_print_hex(regs->esi);
        vga_set_color(LIGHT_GREY, RED); vga_print("  EDI: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->edi);
        vga_set_color(LIGHT_GREY, RED); vga_print("  ESP: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->esp);
        vga_set_color(LIGHT_GREY, RED); vga_print("  EBP: ");
        vga_set_color(WHITE, RED); vga_print_hex(regs->ebp);
    }

    divider(15);
    vga_set_color(LIGHT_GREY, RED);
    vga_center_print("System halted. Restart your machine.", 17);
    vga_set_color(YELLOW, RED);
    vga_center_print("I'm gonna wreck it! (c) Ralph", 18);

    while (1) __asm__ volatile ("hlt");
}