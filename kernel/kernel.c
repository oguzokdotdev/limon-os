#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "version.h"

// ─── VGA ───────────────────────────────────────────────
#define VGA_ADDRESS 0xB8000
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

static uint16_t* vga = (uint16_t*)VGA_ADDRESS;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x0F;

static void scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (BLACK << 12) | (' ');
    cursor_y = VGA_HEIGHT - 1;
}

static void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) scroll();
        return;
    }
    vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (current_color << 8);
    cursor_x++;
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) scroll();
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

static void print_int(int n) {
    char buf[12];
    int i = 0;
    if (n == 0) { buf[i++] = '0'; }
    else { int d = 0, tmp = n;
           while(tmp){d++;tmp/=10;}
           for(int j=d-1;j>=0;j--){buf[j]='0'+(n%10);n/=10;i++;} }
    buf[i] = '\0';
    print(buf);
}

// ─── Утилиты ───────────────────────────────────────────
static int kstrcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Буфер клавиатуры
static volatile char key_buf[256];
static volatile int  key_head = 0;
static volatile int  key_tail = 0;

static const char keymap[] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

void keyboard_handler(void) {
    uint8_t scan = inb(0x60);
    if (!(scan & 0x80)) {
        if (scan < sizeof(keymap) && keymap[scan]) {
            key_buf[key_head] = keymap[scan];
            key_head = (key_head + 1) % 256;
        }
    }
    outb(0x20, 0x20); // EOI
}

static char read_key(void) {
    if (key_head == key_tail) return 0;
    char c = key_buf[key_tail];
    key_tail = (key_tail + 1) % 256;
    return c;
}

// ─── Команды ───────────────────────────────────────────
typedef void (*cmd_func)(void);

typedef struct {
    const char* name;
    const char* desc;
    cmd_func    func;
} Command;

static void cmd_help(void);

static void cmd_about(void) {
    set_color(LIGHT_GREY, BLACK);
    print("  " LIMON_VERSION_FULL "\n");
    print("  Build: b"); print_int(LIMON_BUILD); print("\n");
    print("  Arch: " LIMON_ARCH "\n");
    print("  Developed with Claude Sonnet 4.6\n");
    print("  Inspired by VibeOS\n");
}

static void cmd_uname(void) {
    set_color(WHITE, BLACK);
    print("  " LIMON_VERSION_FULL " b");
    print_int(LIMON_BUILD);
    print("\n");
}

static void cmd_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (BLACK << 12) | (' ');
    cursor_x = 0;
    cursor_y = 0;
}

static void cmd_echo(char* args) {
    set_color(WHITE, BLACK);
    print("  ");
    print(args);
    print("\n");
}

static const Command commands[] = {
    {"help",  "show commands",  cmd_help},
    {"about", "system info",    cmd_about},
    {"clear", "clear screen",   cmd_clear},
    {"uname", "kernel version", cmd_uname},
};
#define CMD_COUNT (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(void) {
    set_color(LIGHT_CYAN, BLACK);
    for (int i = 0; i < (int)CMD_COUNT; i++) {
        print("  ");
        print(commands[i].name);
        print("   - ");
        print(commands[i].desc);
        print("\n");
    }
}

// ─── Ядро ──────────────────────────────────────────────
void kernel_main(void) {
    gdt_init();
    idt_init();
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (BLACK << 12) | (' ');

    set_color(YELLOW, BLACK);
    center_print("  _     _                          ___  _____  ", 4);
    center_print(" | |   (_)_ __ ___   ___  _ __   / _ \\/ ___/  ", 5);
    center_print(" | |   | | '_ ` _ \\ / _ \\| '_ \\ | | | \\___ \\  ", 6);
    center_print(" | |___| | | | | | | (_) | | | || |_| |___) | ", 7);
    center_print(" |_____|_|_| |_| |_|\\___/|_| |_| \\___//____/  ", 8);

    set_color(WHITE, BLACK);
    center_print(LIMON_VERSION_FULL, 10);
    center_print("clean, minimal, yours.", 10);

    set_color(DARK_GREY, BLACK);
    center_print("----------------------------------------", 12);

    set_color(LIGHT_GREY, BLACK);
    center_print("Developed with Claude Sonnet 4.6", 14);
    center_print("Inspired by VibeOS", 15);

    set_color(LIGHT_GREEN, BLACK);
    center_print("[ OK ] Booting Limon OS...", 18);

    set_color(YELLOW, BLACK);
    cursor_x = 0;
    cursor_y = 21;
    print("limon> ");
    set_color(WHITE, BLACK);

    char buf[80];
    int buf_len = 0;

    while (1) {
        char c = read_key();
        if (c == 0) continue;

        if (c == '\n') {
            buf[buf_len] = '\0';
            cursor_x = 0;
            cursor_y++;

            // echo отдельно — передаём аргументы
            if (buf_len > 5 &&
                buf[0]=='e' && buf[1]=='c' && buf[2]=='h' &&
                buf[3]=='o' && buf[4]==' ') {
                cmd_echo(buf + 5);
            } else {
                int found = 0;
                for (int i = 0; i < (int)CMD_COUNT; i++) {
                    if (kstrcmp(buf, commands[i].name) == 0) {
                        commands[i].func();
                        found = 1;
                        break;
                    }
                }
                if (!found && buf_len > 0) {
                    set_color(LIGHT_RED, BLACK);
                    print("  unknown command: ");
                    set_color(WHITE, BLACK);
                    print(buf);
                    print("\n");
                }
            }

            buf_len = 0;
            set_color(YELLOW, BLACK);
            print("limon> ");
            set_color(WHITE, BLACK);
        } else if (c == '\b') {
            if (buf_len > 0) {
                buf_len--;
                cursor_x--;
                vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | ' ';
            }
        } else {
            if (buf_len < 79) {
                buf[buf_len++] = c;
                vga_putchar(c);
            }
        }
    }
}