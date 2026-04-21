#ifndef DRIVERS_VIDEO_VGA_H
#define DRIVERS_VIDEO_VGA_H

#include <stdint.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25

#define BLACK         0x0
#define BLUE          0x1
#define GREEN         0x2
#define CYAN          0x3
#define RED           0x4
#define MAGENTA       0x5
#define BROWN         0x6
#define LIGHT_GREY    0x7
#define DARK_GREY     0x8
#define LIGHT_BLUE    0x9
#define LIGHT_GREEN   0xA
#define LIGHT_CYAN    0xB
#define LIGHT_RED     0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW        0xE
#define WHITE         0xF

void vga_init(void);
void vga_clear(void);
void vga_fill(uint8_t fg, uint8_t bg, char c);
void vga_putchar(char c);
void vga_print(const char* str);
void vga_print_int(int n);
void vga_print_padded2(uint32_t n);
void vga_print_hex(uint32_t n);
void vga_center_print(const char* str, int row);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_clear_at(int x, int y);
void vga_set_cursor(int x, int y);
int  vga_get_cursor_x(void);
int  vga_get_cursor_y(void);

void hw_cursor_enable(void);
void hw_cursor_hide(void);
void hw_cursor_update(void);

#endif