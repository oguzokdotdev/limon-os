#include <stdint.h>
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "drivers/video/vga.h"
#include "drivers/timer/pit.h"
#include "core/boot_log.h"
#include "shell/shell.h"
#include "convert.h"

#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} MultibootInfo;

void kernel_main(uint32_t magic, MultibootInfo* mbi) {
    gdt_init();
    idt_init();

    vga_init();
    hw_cursor_enable();

    pit_init();
    vga_clear();

    boot_log(BOOT_OK, "GDT initialized");
    boot_log(BOOT_OK, "IDT initialized");
    boot_log(BOOT_OK, "PIT configured at 100 Hz");
    boot_log(BOOT_OK, "RTC driver ready");

    uint32_t mem_upper = 0;
    if (magic == MULTIBOOT_MAGIC && mbi && (mbi->flags & 0x1)) {
        mem_upper = mbi->mem_upper;
        char mem_msg[40];
        uint32_t mem_mib = (mem_upper + 1024) / 1024;
        int i = 0;
        const char* prefix = "Memory: ";
        for (int j = 0; prefix[j]; j++) mem_msg[i++] = prefix[j];
        uint32_t tmp = mem_mib;
        if (tmp == 0) { mem_msg[i++] = '0'; }
        else {
            char rev[12]; int ri = 0;
            while (tmp) { rev[ri++] = '0' + (tmp % 10); tmp /= 10; }
            for (int j = ri-1; j >= 0; j--) mem_msg[i++] = rev[j];
        }
        const char* suffix = " MiB detected";
        for (int j = 0; suffix[j]; j++) mem_msg[i++] = suffix[j];
        mem_msg[i] = '\0';
        boot_log(BOOT_OK, mem_msg);
    } else {
        boot_log(BOOT_WARN, "Multiboot info unavailable, memory unknown");
    }

    boot_log(BOOT_OK, "Keyboard driver ready");
    boot_log(BOOT_OK, "System ready");

    boot_delay(100);
    vga_clear();

    shell_init(mem_upper);
    shell_run();
}