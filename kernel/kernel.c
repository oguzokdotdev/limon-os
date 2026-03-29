#include <stdint.h>

// VGA текстовый буфер
#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_COLOR   0x0F  // белый на чёрном

static uint16_t* vga = (uint16_t*)VGA_ADDRESS;
static int cursor_x = 0;
static int cursor_y = 0;

static void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (VGA_COLOR << 8);
    cursor_x++;
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
}

static void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        vga_putchar(str[i]);
}

void kernel_main(void) {
    // Очищаем экран
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (VGA_COLOR << 8) | ' ';

    print("Limon OS v0.1\n");
    print("Booting...\n");

    // Ядро не должно возвращаться
    while (1) {}
}