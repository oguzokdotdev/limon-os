#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "version.h"

// --- Multiboot ---
#define MULTIBOOT_MAGIC 0x2BADB002

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} MultibootInfo;

// --- VGA ---
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
    else {
        int d = 0, tmp = n;
        while (tmp) { d++; tmp /= 10; }
        for (int j = d - 1; j >= 0; j--) { buf[j] = '0' + (n % 10); n /= 10; i++; }
    }
    buf[i] = '\0';
    print(buf);
}

static void print_padded2(uint32_t n) {
    vga_putchar('0' + (n / 10));
    vga_putchar('0' + (n % 10));
}

// --- Utilities ---
static int kstrcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void kstrcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

static int kstrlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static int katoi(const char* str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

static int parse_args(char* str, char* argv[], int max_args) {
    int argc = 0;
    int in_word = 0;
    while (*str) {
        if (*str == ' ') {
            *str = '\0';
            in_word = 0;
        } else if (!in_word) {
            if (argc < max_args) {
                argv[argc++] = str;
                in_word = 1;
            }
        }
        str++;
    }
    return argc;
}

// --- CPUID ---
static void get_cpu_vendor(char* out) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    for (int i = 0; i < 4; i++) out[i]     = (ebx >> (i * 8)) & 0xFF;
    for (int i = 0; i < 4; i++) out[4 + i] = (edx >> (i * 8)) & 0xFF;
    for (int i = 0; i < 4; i++) out[8 + i] = (ecx >> (i * 8)) & 0xFF;
    out[12] = '\0';
}

// --- Keyboard ---
#define KEY_UP   0x01
#define KEY_DOWN 0x02

static volatile char key_buf[256];
static volatile int  key_head = 0;
static volatile int  key_tail = 0;
static volatile int  shift_pressed = 0;
static volatile int  e0_prefix = 0;

static const char keymap[] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

static const char keymap_shift[] = {
    0, 0, '!','@','#','$','%','^','&','*','(',')','-','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '
};

void keyboard_handler(void) {
    uint8_t scan = inb(0x60);

    // extended scancode prefix
    if (scan == 0xE0) { e0_prefix = 1; outb(0x20, 0x20); return; }

    if (e0_prefix) {
        e0_prefix = 0;
        if (scan == 0x48) { key_buf[key_head] = KEY_UP;   key_head = (key_head + 1) % 256; }
        if (scan == 0x50) { key_buf[key_head] = KEY_DOWN; key_head = (key_head + 1) % 256; }
        outb(0x20, 0x20);
        return;
    }

    if (scan == 0x2A || scan == 0x36) { shift_pressed = 1; outb(0x20, 0x20); return; }
    if (scan == 0xAA || scan == 0xB6) { shift_pressed = 0; outb(0x20, 0x20); return; }

    if (!(scan & 0x80)) {
        const char* map = shift_pressed ? keymap_shift : keymap;
        if (scan < sizeof(keymap) && map[scan]) {
            key_buf[key_head] = map[scan];
            key_head = (key_head + 1) % 256;
        }
    }
    outb(0x20, 0x20);
}

// --- Timer ---
static uint32_t timer_ticks = 0;
static uint32_t uptime_seconds = 0;

void timer_handler(void) {
    timer_ticks++;
    if (timer_ticks % 100 == 0)
        uptime_seconds++;
    outb(0x20, 0x20);
}

static char read_key(void) {
    if (key_head == key_tail) return 0;
    char c = key_buf[key_tail];
    key_tail = (key_tail + 1) % 256;
    return c;
}

// --- History ---
#define HISTORY_SIZE 16

static char history[HISTORY_SIZE][80];
static int  history_count = 0;
static int  history_nav   = -1; // -1 = not navigating

static void history_push(const char* cmd) {
    if (cmd[0] == '\0') return;
    // avoid duplicate of last entry
    if (history_count > 0 &&
        kstrcmp(history[(history_count - 1) % HISTORY_SIZE], cmd) == 0) return;
    kstrcpy(history[history_count % HISTORY_SIZE], cmd);
    history_count++;
}

// --- Commands ---
typedef void (*cmd_func)(char* args);

typedef struct {
    const char* name;
    const char* desc;
    int         category;
    cmd_func    func;
} Command;

static uint32_t g_mem_upper = 0;

static void cmd_help(char* args);

static void cmd_about(char* args) {
    set_color(LIGHT_GREY, BLACK);
    print("  " LIMON_VERSION_FULL "\n");
    print("  Build: b"); print_int(LIMON_BUILD); print("\n");
    print("  Arch: " LIMON_ARCH "\n");
    print("  Developed with Claude Sonnet 4.6\n");
    print("  Inspired by VibeOS\n");
}

static void cmd_uname(char* args) {
    set_color(WHITE, BLACK);
    print("  " LIMON_VERSION_FULL " b");
    print_int(LIMON_BUILD);
    print("\n");
}

static void cmd_clear(char* args) {
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

static void cmd_fetch(char* args) {
    char cpu[13];
    get_cpu_vendor(cpu);

    uint32_t mem_mib = (g_mem_upper + 1024) / 1024;

    uint32_t h = uptime_seconds / 3600;
    uint32_t m = (uptime_seconds % 3600) / 60;
    uint32_t s = uptime_seconds % 60;

    const char* art[] = {
        " #       ",
        " #       ",
        " #       ",
        " #       ",
        " #       ",
        " ####### ",
    };

    print("\n");

    const char* keys[] = {
        "OS", "Codename", "Build", "Arch",
        "CPU", "Memory", "Uptime", "Display", "Shell",
    };

    for (int i = 0; i < 9; i++) {
        set_color(YELLOW, BLACK);
        print(i < 6 ? art[i] : "         ");

        set_color(LIGHT_CYAN, BLACK);
        print(keys[i]);
        print(": ");

        set_color(WHITE, BLACK);
        switch (i) {
            case 0: print(LIMON_VERSION_FULL); break;
            case 1: print(LIMON_CODENAME); break;
            case 2: print("b"); print_int(LIMON_BUILD); break;
            case 3: print(LIMON_ARCH); break;
            case 4: print(cpu); break;
            case 5: print_int(mem_mib); print(" MiB"); break;
            case 6:
                print_padded2(h); print(":");
                print_padded2(m); print(":");
                print_padded2(s);
                break;
            case 7: print("VGA 80x25 16c"); break;
            case 8: print("limon"); break;
        }
        print("\n");
    }

    print("         ");
    uint8_t colors[] = {BLACK, RED, GREEN, YELLOW,
                        BLUE, MAGENTA, CYAN, LIGHT_GREY,
                        DARK_GREY, LIGHT_RED, LIGHT_GREEN, LIGHT_CYAN,
                        LIGHT_BLUE, LIGHT_MAGENTA, YELLOW, WHITE};
    for (int i = 0; i < 16; i++) {
        set_color(colors[i], colors[i]);
        vga_putchar(' ');
        vga_putchar(' ');
    }
    set_color(WHITE, BLACK);
    print("\n\n");
}

static void cmd_reboot(char* args) {
    set_color(LIGHT_GREEN, BLACK);
    print("  Rebooting...\n");
    uint8_t val = 0;
    while (val & 0x02) val = inb(0x64);
    outb(0x64, 0xFE);
    __asm__ volatile ("cli; hlt");
}

static void cmd_halt(char* args) {
    set_color(LIGHT_RED, BLACK);
    print("  System halted. You can close QEMU now.\n");
    __asm__ volatile ("cli; hlt");
}

static const Command commands[] = {
    {"help",       "show commands",  2, cmd_help},
    {"about",      "system info",    2, cmd_about},
    {"clear",      "clear screen",   1, cmd_clear},
    {"uname",      "kernel version", 2, cmd_uname},
    {"limonfetch", "system fetch",   2, cmd_fetch},
    {"reboot",     "reboot system",  1, cmd_reboot},
    {"halt",       "halt system",    1, cmd_halt},
};
#define CMD_COUNT (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(char* args) {
    char* argv[4];
    int argc = parse_args(args, argv, 4);

    if (argc == 0) {
        set_color(LIGHT_CYAN, BLACK);
        print("  Help Categories:\n");
        print("  1 - System Control\n");
        print("  2 - Info & Utilities\n");
        set_color(LIGHT_GREY, BLACK);
        print("  Usage: help [category]\n");
        return;
    }

    int cat = katoi(argv[0]);

    set_color(LIGHT_CYAN, BLACK);
    if (cat == 1) {
        print("  --- 1: System Control ---\n");
    } else if (cat == 2) {
        print("  --- 2: Info & Utilities ---\n");
    } else {
        set_color(LIGHT_RED, BLACK);
        print("  unknown category: ");
        print(argv[0]);
        print("\n");
        return;
    }

    for (int i = 0; i < (int)CMD_COUNT; i++) {
        if (commands[i].category == cat) {
            print("  ");
            print(commands[i].name);
            print("   - ");
            print(commands[i].desc);
            print("\n");
        }
    }
    
    // Explicitly add echo as it is built into the shell logic
    if (cat == 2) {
        print("  echo   - print text\n");
    }
}

// --- Kernel ---
void kernel_main(uint32_t magic, MultibootInfo* mbi) {
    gdt_init();
    idt_init();

    uint32_t divisor = 1193180 / 100;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    if (magic == MULTIBOOT_MAGIC && mbi && (mbi->flags & 0x1))
        g_mem_upper = mbi->mem_upper;

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
    center_print("clean, minimal, yours.", 11);

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

#define PROMPT_LEN 7  // strlen("limon> ")

    char buf[80];
    int buf_len = 0;

    while (1) {
        char c = read_key();
        if (c == 0) continue;

        // --- History navigation ---
        if (c == KEY_UP || c == KEY_DOWN) {
            if (history_count == 0) continue;

            if (c == KEY_UP) {
                if (history_nav == -1)
                    history_nav = history_count - 1;
                else if (history_nav > 0)
                    history_nav--;
            } else {
                if (history_nav == -1) continue;
                history_nav++;
                if (history_nav >= history_count) {
                    // past newest: clear input
                    history_nav = -1;
                    // erase current line
                    for (int i = buf_len; i > 0; i--) {
                        cursor_x--;
                        vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | ' ';
                    }
                    buf_len = 0;
                    continue;
                }
            }

            // erase current input on screen
            for (int i = buf_len; i > 0; i--) {
                cursor_x--;
                vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | ' ';
            }

            // load history entry
            const char* entry = history[history_nav % HISTORY_SIZE];
            kstrcpy(buf, entry);
            buf_len = kstrlen(buf);

            // print it
            set_color(WHITE, BLACK);
            print(buf);
            continue;
        }

        // --- Enter ---
        if (c == '\n') {
            buf[buf_len] = '\0';
            cursor_x = 0;
            cursor_y++;

            history_push(buf);
            history_nav = -1;

            // Extract command name and args
            char* cmd_name = buf;
            char* cmd_args = "";
            for (int i = 0; i < buf_len; i++) {
                if (buf[i] == ' ') {
                    buf[i] = '\0';
                    cmd_args = &buf[i + 1];
                    while (*cmd_args == ' ') cmd_args++; // skip extra spaces
                    break;
                }
            }

            if (kstrcmp(cmd_name, "echo") == 0) {
                cmd_echo(cmd_args);
            } else {
                int found = 0;
                for (int i = 0; i < (int)CMD_COUNT; i++) {
                    if (kstrcmp(cmd_name, commands[i].name) == 0) {
                        commands[i].func(cmd_args);
                        found = 1;
                        break;
                    }
                }
                if (!found && kstrlen(cmd_name) > 0) {
                    set_color(LIGHT_RED, BLACK);
                    print("  unknown command: ");
                    set_color(WHITE, BLACK);
                    print(cmd_name);
                    print("\n");
                }
            }

            buf_len = 0;
            set_color(YELLOW, BLACK);
            print("limon> ");
            set_color(WHITE, BLACK);

        // --- Backspace ---
        } else if (c == '\b') {
            if (buf_len > 0) {
                buf_len--;
                cursor_x--;
                vga[cursor_y * VGA_WIDTH + cursor_x] = (current_color << 8) | ' ';
            }

        // --- Regular char ---
        } else {
            if (buf_len < 79) {
                buf[buf_len++] = c;
                vga_putchar(c);
            }
        }
    }
}
