#include <stdint.h>
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "arch/x86/cpu.h"
#include "drivers/video/vga.h"
#include "drivers/timer/pit.h"
#include "drivers/timer/rtc.h"
#include "core/boot_log.h"
#include "core/version.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "shell/shell.h"
#include "convert.h"

#define MULTIBOOT_MAGIC 0x2BADB002

extern uint32_t _kernel_end;
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;   /* offset 44 */
    uint32_t mmap_addr;     /* offset 48 */
} __attribute__((packed)) MultibootInfo;

void kernel_main(uint32_t magic, MultibootInfo* mbi) {
    gdt_init();    

    vga_init();
    hw_cursor_enable();
    vga_clear();
    
    idt_init();
    pit_init();

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

    /* Инициализация PMM */
    uint32_t safe_kernel_end = (uint32_t)&_kernel_end;

    if (magic == MULTIBOOT_MAGIC && mbi) {
        uint32_t mbi_end = (uint32_t)mbi + sizeof(MultibootInfo);
        if (mbi_end > safe_kernel_end) safe_kernel_end = mbi_end;

        if (mbi->flags & 0x40) {
            uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
            if (mmap_end > safe_kernel_end) safe_kernel_end = mmap_end;
        }
    }

    uint32_t mmap_addr   = 0;
    uint32_t mmap_length = 0;
    if (magic == MULTIBOOT_MAGIC && mbi && (mbi->flags & 0x40)) {
        mmap_addr   = mbi->mmap_addr;
        mmap_length = mbi->mmap_length;
    }

    pmm_init(mmap_addr, mmap_length,
             mem_upper,
             safe_kernel_end);

    vmm_init();
    vmm_switch_page_directory(vmm_get_kernel_dir());
    boot_log(BOOT_OK, "VMM enabled");


    boot_log(BOOT_OK, "Keyboard driver ready");
    boot_log(BOOT_OK, "System ready");

    boot_delay(100);
    vga_clear();

    shell_init(mem_upper);
    shell_run();
}
