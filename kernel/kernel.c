#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

// Цвета VGA
#define BLACK        0x0
#define BLUE         0x1
#define GREEN        0x2
#define CYAN         0x3
#define RED          0x4
#define MAGENTA      0x5
#define BROWN        0x6
#define LIGHT_GREY   0x7
#define DARK_GREY    0x8
#define LIGHT_BLUE   0x9
#define LIGHT_GREEN  0xA
#define LIGHT_CYAN   0xB
#define LIGHT_RED    0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW       0xE
#define WHITE        0xF

static uint16_t* vga = (uint16_t*)VGA_ADDRESS;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x0F;

static void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (current_color << 8);
    cursor_x++;
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
}

static void set_color(uint8_t fg, uint8_t bg) {
    current_color = (bg << 4) | fg;
}

static void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++)
        vga_putchar(str[i]);
}

static void center_print(const char* str, int row) {
    int len = 0;
    for (int i = 0; str[i]; i++) len++;
    cursor_x = (VGA_WIDTH - len) / 2;
    cursor_y = row;
    print(str);
}

void kernel_main(void) {
    // Очищаем экран
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (BLACK << 12) | (' ');

    // ASCII-арт логотип — жёлтый
    set_color(YELLOW, BLACK);
    center_print("  _     _                          ___  _____  ", 4);
    center_print(" | |   (_)_ __ ___   ___  _ __   / _ \\/ ___/  ", 5);
    center_print(" | |   | | '_ ` _ \\ / _ \\| '_ \\ | | | \\___ \\  ", 6);
    center_print(" | |___| | | | | | | (_) | | | || |_| |___) | ", 7);
    center_print(" |_____|_|_| |_| |_|\\___/|_| |_| \\___//____/  ", 8);

    // Версия — белый
    set_color(WHITE, BLACK);
    center_print("v0.1  --  clean, minimal, yours.", 10);

    // Разделитель — тёмно-серый
    set_color(DARK_GREY, BLACK);
    center_print("----------------------------------------", 12);

    // Авторы — светло-серый
    set_color(LIGHT_GREY, BLACK);
    center_print("Developed with Claude Sonnet 4.6", 14);
    center_print("Inspired by VibeOS", 15);

    // Статус загрузки — зелёный
    set_color(LIGHT_GREEN, BLACK);
    center_print("[ OK ] Booting Limon OS...", 18);

    while (1) {}
}