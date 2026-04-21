#include "boot_log.h"
#include "drivers/video/vga.h"
#include "drivers/timer/pit.h"

void boot_log(boot_status_t status, const char* msg) {
    switch (status) {
        case BOOT_OK:
            vga_set_color(LIGHT_GREEN, BLACK); vga_print("[  OK  ] "); break;
        case BOOT_WARN:
            vga_set_color(YELLOW, BLACK);      vga_print("[ WARN ] "); break;
        case BOOT_FAIL:
            vga_set_color(LIGHT_RED, BLACK);   vga_print("[ FAIL ] "); break;
    }
    vga_set_color(LIGHT_GREY, BLACK);
    vga_print(msg);
    vga_print("\n");
    boot_delay(15);
}