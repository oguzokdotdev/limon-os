#include "vga.h"
#include "arch/x86/cpu.h"

#define VGA_ADDRESS 0xB8000

static uint16_t* vga_buf   = (uint16_t*)VGA_ADDRESS;
static int       cursor_x  = 0;
static int       cursor_y  = 0;
static uint8_t   cur_color = 0x0F;

void vga_init(void) { cursor_x = 0; cursor_y = 0; cur_color = 0x0F; }

static void scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buf[y * VGA_WIDTH + x] = vga_buf[(y + 1) * VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (BLACK << 12) | (' ');
    cursor_y = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0; cursor_y++;
        if (cursor_y >= VGA_HEIGHT) scroll();
        return;
    }
    vga_buf[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (cur_color << 8);
    if (++cursor_x >= VGA_WIDTH) {
        cursor_x = 0; cursor_y++;
        if (cursor_y >= VGA_HEIGHT) scroll();
    }
}

void vga_set_color(uint8_t fg, uint8_t bg) { cur_color = (bg << 4) | fg; }

void vga_print(const char* str) {
    for (int i = 0; str[i]; i++) vga_putchar(str[i]);
}

void vga_center_print(const char* str, int row) {
    int len = 0;
    for (int i = 0; str[i]; i++) len++;
    cursor_x = (VGA_WIDTH - len) / 2;
    cursor_y = row;
    vga_print(str);
}

void vga_print_int(int n) {
    char buf[12]; int i = 0;
    if (n == 0) { buf[i++] = '0'; }
    else {
        int d = 0, tmp = n;
        while (tmp) { d++; tmp /= 10; }
        for (int j = d - 1; j >= 0; j--) { buf[j] = '0' + (n % 10); n /= 10; i++; }
    }
    buf[i] = '\0';
    vga_print(buf);
}

void vga_print_padded2(uint32_t n) {
    vga_putchar('0' + (n / 10));
    vga_putchar('0' + (n % 10));
}

void vga_print_hex(uint32_t n) {
    const char* h = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 7; i >= 0; i--) vga_putchar(h[(n >> (i * 4)) & 0xF]);
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga_buf[i] = (BLACK << 12) | (' ');
    cursor_x = 0; cursor_y = 0;
}

void vga_fill(uint8_t fg, uint8_t bg, char c) {
    uint16_t attr = (uint16_t)c | (((bg << 4) | fg) << 8);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga_buf[i] = attr;
    cursor_x = 0; cursor_y = 0;
    cur_color = (bg << 4) | fg;
}

void vga_clear_at(int x, int y) { vga_buf[y * VGA_WIDTH + x] = (cur_color << 8) | ' '; }
void vga_set_cursor(int x, int y) { cursor_x = x; cursor_y = y; }
int  vga_get_cursor_x(void)       { return cursor_x; }
int  vga_get_cursor_y(void)       { return cursor_y; }

void hw_cursor_enable(void) {
    outb(0x3D4, 0x0A); outb(0x3D5, (inb(0x3D5) & 0xC0) | 0x0D);
    outb(0x3D4, 0x0B); outb(0x3D5, (inb(0x3D5) & 0xE0) | 0x0F);
}

void hw_cursor_hide(void) {
    outb(0x3D4, 0x0A); outb(0x3D5, inb(0x3D5) | 0x20);
}

void hw_cursor_update(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}