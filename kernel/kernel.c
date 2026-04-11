#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "version.h"
#include "libc/string.h"
#include "libc/memory.h"
#include "libc/convert.h"

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
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
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

// --- Hardware cursor ---
static void hw_cursor_enable(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

static void hw_cursor_update(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
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
static void get_cpu_model(char* out);

// --- Keyboard ---
#define KEY_UP    0x01
#define KEY_DOWN  0x02
#define KEY_LEFT  0x03
#define KEY_RIGHT 0x04

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

    if (scan == 0xE0) { e0_prefix = 1; outb(0x20, 0x20); return; }

    if (e0_prefix) {
        e0_prefix = 0;
        if (scan == 0x48) { key_buf[key_head] = KEY_UP;    key_head = (key_head + 1) % 256; }
        if (scan == 0x50) { key_buf[key_head] = KEY_DOWN;  key_head = (key_head + 1) % 256; }
        if (scan == 0x4B) { key_buf[key_head] = KEY_LEFT;  key_head = (key_head + 1) % 256; }
        if (scan == 0x4D) { key_buf[key_head] = KEY_RIGHT; key_head = (key_head + 1) % 256; }
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
static int  history_nav   = -1;

static void history_push(const char* cmd) {
    if (cmd[0] == '\0') return;
    if (history_count > 0 &&
        strcmp(history[(history_count - 1) % HISTORY_SIZE], cmd) == 0) return;
    strcpy(history[history_count % HISTORY_SIZE], cmd);
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
    print("\n");
    set_color(LIGHT_CYAN, BLACK);
    print("         LimonOS " LIMON_CODENAME "\n");
    set_color(WHITE, BLACK);
    print("    \"Clean, minimal, yours.\"\n\n");
    set_color(LIGHT_GREY, BLACK);
    print("    Author:   oguzokdotdev AKA oguzok.tech\n");
    print("    GitHub Repo:   oguzokdotdev/limon-os\n");
    print("\n    Inspired by VibeOS.\n");
    print("    Built with assistance from Claude & Gemini.\n");
    print("\n");
}

static void cmd_uname(char* args) {
    char* argv[8];
    int argc = parse_args(args, argv, 8);

    if (argc == 0) {
        set_color(WHITE, BLACK);
        print("  LimonOS\n");
        return;
    }

    if (strcmp(argv[0], "--help") == 0 || strcmp(argv[0], "-help") == 0) {
        set_color(LIGHT_CYAN, BLACK);
        print("  uname - print system information\n");
        set_color(LIGHT_GREY, BLACK);
        print("  Usage: uname [option]\n\n");
        print("  (no args)  print OS name\n");
        print("  -o         print OS name\n");
        print("  -v         print kernel version\n");
        print("  -c         print codename\n");
        print("  -i         print architecture\n");
        print("  -b         print build number\n");
        print("  -a         print all\n");
        return;
    }

    for (int k = 0; k < argc; k++) {
        const char* flag = argv[k];
        if (strcmp(flag, "-o") == 0) {
            set_color(WHITE, BLACK);
            print("  LimonOS\n");
        } else if (strcmp(flag, "-v") == 0) {
            set_color(WHITE, BLACK);
            print("  " LIMON_VERSION_STRING "\n");
        } else if (strcmp(flag, "-c") == 0) {
            set_color(WHITE, BLACK);
            print("  " LIMON_CODENAME "\n");
        } else if (strcmp(flag, "-i") == 0) {
            set_color(WHITE, BLACK);
            print("  " LIMON_ARCH "\n");
        } else if (strcmp(flag, "-b") == 0) {
            set_color(WHITE, BLACK);
            print("  b"); print_int(LIMON_BUILD); print("\n");
        } else if (strcmp(flag, "-a") == 0) {
            set_color(WHITE, BLACK);
            print("  LimonOS " LIMON_VERSION_STRING "-b");
            print_int(LIMON_BUILD);
            print(" " LIMON_CODENAME " " LIMON_ARCH "\n");
        } else {
            set_color(LIGHT_RED, BLACK);
            print("  unknown option: ");
            set_color(WHITE, BLACK);
            print(flag);
            print("\n");
        }
    }
}

static void cmd_ver(char* args) {
    set_color(WHITE, BLACK);
    print("  LimonOS " LIMON_CODENAME " v" LIMON_VERSION_STRING "-b");
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
    char cpu[49];
    get_cpu_model(cpu);

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

static void cmd_div0(char* args) {
    volatile int x = 1 / 0;
    (void)x;
}

// --- Command table ---
static const Command commands[] = {
    {"help",       "show commands",      2, cmd_help},
    {"lscmd",      "list all commands",  2, 0},
    {"about",      "about LimonOS",      2, cmd_about},
    {"clear",      "clear screen",       1, cmd_clear},
    {"uname",      "system information", 2, cmd_uname},
    {"ver",        "system version",     2, cmd_ver},
    {"limonfetch", "system fetch",       2, cmd_fetch},
    {"reboot",     "reboot system",      1, cmd_reboot},
    {"halt",       "halt system",        1, cmd_halt},
    {"div0", "trigger #DE (test panic)", 1, cmd_div0},
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

    int cat = atoi(argv[0]);

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

    if (cat == 2) {
        print("  echo   - print text\n");
    }
}

static void cmd_lscmd(char* args) {
    const char* names[CMD_COUNT + 1];
    int total = 0;
    for (int i = 0; i < (int)CMD_COUNT; i++)
        names[total++] = commands[i].name;
    names[total++] = "echo";

    int cols = 2;
    if (total > 20) cols = 3;
    if (total > 30) cols = 4;

    int rows = (total + cols - 1) / cols;

    set_color(WHITE, BLACK);
    print("\n");
    for (int r = 0; r < rows; r++) {
        print("  ");
        for (int col = 0; col < cols; col++) {
            int idx = col * rows + r;
            if (idx >= total) break;
            set_color(LIGHT_CYAN, BLACK);
            print(names[idx]);
            int pad = 16 - strlen(names[idx]);
            for (int p = 0; p < pad; p++) vga_putchar(' ');
        }
        print("\n");
    }
    print("\n");
    set_color(LIGHT_GREY, BLACK);
}

// --- Tab completion ---
static int tab_complete(char* buf, int buf_len) {
    const char* names[CMD_COUNT + 1];
    int total = 0;
    for (int i = 0; i < (int)CMD_COUNT; i++)
        names[total++] = commands[i].name;
    names[total++] = "echo";

    if (buf_len == 0) return buf_len;

    const char* matches[CMD_COUNT + 1];
    int match_count = 0;
    for (int i = 0; i < total; i++) {
        int match = 1;
        for (int j = 0; j < buf_len; j++) {
            if (names[i][j] == '\0' || names[i][j] != buf[j]) {
                match = 0;
                break;
            }
        }
        if (match) matches[match_count++] = names[i];
    }

    if (match_count == 0) {
        return buf_len;

    } else if (match_count == 1) {
        const char* full = matches[0];
        int full_len = strlen(full);
        for (int i = 0; i < full_len && i < 79; i++)
            buf[i] = full[i];
        return full_len;

    } else {
        print("\n");
        for (int i = 0; i < match_count; i++) {
            set_color(LIGHT_CYAN, BLACK);
            print("  ");
            print(matches[i]);
            print("\n");
        }
        return buf_len;
    }
}

static void print_hex(uint32_t n) {
    const char* h = "0123456789ABCDEF";
    print("0x");
    for (int i = 7; i >= 0; i--)
        vga_putchar(h[(n >> (i * 4)) & 0xF]);
}

static void get_cpu_model(char* out) {
    uint32_t max_ext;
    __asm__ volatile ("cpuid" : "=a"(max_ext) : "a"(0x80000000) : "ebx", "ecx", "edx");
    if (max_ext < 0x80000004) { get_cpu_vendor(out); return; }
    uint32_t* p = (uint32_t*)out;
    for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
        uint32_t a, b, c, d;
        __asm__ volatile ("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(leaf));
        *p++ = a; *p++ = b; *p++ = c; *p++ = d;
    }
    out[48] = '\0';
}

static uint32_t read_cr2(void) {
    uint32_t v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

// --- Panic ---
void panic(const char* msg, Registers* regs) {
    __asm__ volatile ("cli");

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (uint16_t)' ' | (((RED << 4) | WHITE) << 8);

#define P_ROW(r) do { cursor_x = 0; cursor_y = (r); } while(0)
#define DIV(r) do { \
    set_color(YELLOW, RED); P_ROW(r); \
    for (int _i = 0; _i < VGA_WIDTH; _i++) vga_putchar('='); \
} while(0)

    DIV(4);
    set_color(WHITE, RED);
    center_print("!!!  KERNEL PANIC  !!!", 5);
    DIV(6);

    set_color(LIGHT_GREY, RED); P_ROW(7); print(" Exception : ");
    set_color(WHITE, RED); print(msg);

    set_color(LIGHT_GREY, RED); P_ROW(8); print(" System    : ");
    set_color(WHITE, RED);
    print("LimonOS " LIMON_CODENAME " v" LIMON_VERSION_STRING "-b");
    print_int(LIMON_BUILD);
    uint32_t uh = uptime_seconds / 3600;
    uint32_t um = (uptime_seconds % 3600) / 60;
    uint32_t us = uptime_seconds % 60;
    set_color(LIGHT_GREY, RED); print("   Uptime: ");
    set_color(WHITE, RED);
    print_padded2(uh); print(":"); print_padded2(um); print(":"); print_padded2(us);

    char cpu[49];
    get_cpu_model(cpu);
    set_color(LIGHT_GREY, RED); P_ROW(9); print(" CPU       : ");
    set_color(WHITE, RED); print(cpu);

    DIV(10);

    set_color(LIGHT_GREY, RED); P_ROW(11); print(" EIP: ");
    set_color(WHITE, RED);
    if (regs) print_hex(regs->eip); else print("0x????????");

    set_color(LIGHT_GREY, RED); print("    Error: ");
    set_color(WHITE, RED);
    if (regs) print_hex(regs->err_code); else print("0x????????");

    if (regs && regs->int_no == 14) {
        set_color(LIGHT_GREY, RED); print("    CR2: ");
        set_color(WHITE, RED); print_hex(read_cr2());
    }

    DIV(12);

    if (regs) {
        set_color(LIGHT_GREY, RED); P_ROW(13); print(" EAX: ");
        set_color(WHITE, RED); print_hex(regs->eax);
        set_color(LIGHT_GREY, RED); print("  EBX: ");
        set_color(WHITE, RED); print_hex(regs->ebx);
        set_color(LIGHT_GREY, RED); print("  ECX: ");
        set_color(WHITE, RED); print_hex(regs->ecx);
        set_color(LIGHT_GREY, RED); print("  EDX: ");
        set_color(WHITE, RED); print_hex(regs->edx);

        set_color(LIGHT_GREY, RED); P_ROW(14); print(" ESI: ");
        set_color(WHITE, RED); print_hex(regs->esi);
        set_color(LIGHT_GREY, RED); print("  EDI: ");
        set_color(WHITE, RED); print_hex(regs->edi);
        set_color(LIGHT_GREY, RED); print("  ESP: ");
        set_color(WHITE, RED); print_hex(regs->esp);
        set_color(LIGHT_GREY, RED); print("  EBP: ");
        set_color(WHITE, RED); print_hex(regs->ebp);
    }

    DIV(15);
    set_color(LIGHT_GREY, RED);
    center_print("System halted. Restart your machine.", 17);
    set_color(YELLOW, RED);
    center_print("I'm gonna wreck it! (c) Ralph", 18);

    while (1) __asm__ volatile ("hlt");
}

// --- Input line redraw ---
// Перерисовывает строку ввода начиная с позиции from_pos до buf_len,
// затем стирает лишний символ справа (при удалении) и ставит курсор на buf_pos.
static void redraw_line(char* buf, int buf_len, int buf_pos,
                        int from_pos, int prompt_x, int prompt_y) {
    cursor_x = prompt_x + from_pos;
    cursor_y = prompt_y;
    set_color(WHITE, BLACK);
    for (int i = from_pos; i < buf_len; i++)
        vga_putchar(buf[i]);
    // Стереть символ после конца буфера (нужно при backspace/delete)
    vga[prompt_y * VGA_WIDTH + (prompt_x + buf_len)] = (current_color << 8) | ' ';
    // Поставить курсор на текущую позицию ввода
    cursor_x = prompt_x + buf_pos;
    cursor_y = prompt_y;
    hw_cursor_update();
}

// --- Kernel ---
void kernel_main(uint32_t magic, MultibootInfo* mbi) {
    gdt_init();
    idt_init();
    hw_cursor_enable();

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

    // Запомнить начало зоны ввода
    int prompt_x = cursor_x;
    int prompt_y = cursor_y;
    hw_cursor_update();

    char buf[80];
    int buf_len = 0;
    int buf_pos = 0; // позиция курсора внутри буфера

    while (1) {
        char c = read_key();
        if (c == 0) continue;

        // --- Стрелки влево/вправо ---
        if (c == KEY_LEFT) {
            if (buf_pos > 0) {
                buf_pos--;
                cursor_x = prompt_x + buf_pos;
                cursor_y = prompt_y;
                hw_cursor_update();
            }
            continue;
        }
        if (c == KEY_RIGHT) {
            if (buf_pos < buf_len) {
                buf_pos++;
                cursor_x = prompt_x + buf_pos;
                cursor_y = prompt_y;
                hw_cursor_update();
            }
            continue;
        }

        // --- История (вверх/вниз) ---
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
                    history_nav = -1;
                    // Очистить строку
                    cursor_x = prompt_x;
                    cursor_y = prompt_y;
                    for (int i = 0; i < buf_len; i++)
                        vga_putchar(' ');
                    buf_len = 0;
                    buf_pos = 0;
                    cursor_x = prompt_x;
                    cursor_y = prompt_y;
                    hw_cursor_update();
                    continue;
                }
            }

            // Очистить текущую строку
            cursor_x = prompt_x;
            cursor_y = prompt_y;
            for (int i = 0; i < buf_len; i++)
                vga_putchar(' ');

            const char* entry = history[history_nav % HISTORY_SIZE];
            strcpy(buf, entry);
            buf_len = strlen(buf);
            buf_pos = buf_len;

            cursor_x = prompt_x;
            cursor_y = prompt_y;
            set_color(WHITE, BLACK);
            print(buf);
            hw_cursor_update();
            continue;
        }

        // --- Tab completion ---
        if (c == '\t') {
            int new_len = tab_complete(buf, buf_pos);
            if (new_len != buf_pos) {
                buf_len = new_len;
                buf_pos = new_len;
                // Перерисовать всю строку
                cursor_x = prompt_x;
                cursor_y = prompt_y;
                set_color(WHITE, BLACK);
                for (int i = 0; i < buf_len; i++)
                    vga_putchar(buf[i]);
                hw_cursor_update();
            } else if (new_len == buf_pos && buf_len > buf_pos) {
                // Совпадений нет, ничего не делать
            } else {
                // Несколько совпадений — tab_complete уже напечатал список,
                // нужно переprint'нуть промпт и буфер
                set_color(YELLOW, BLACK);
                print("limon> ");
                set_color(WHITE, BLACK);
                prompt_x = cursor_x;
                prompt_y = cursor_y;
                for (int i = 0; i < buf_len; i++)
                    vga_putchar(buf[i]);
                cursor_x = prompt_x + buf_pos;
                cursor_y = prompt_y;
                hw_cursor_update();
            }
            continue;
        }

        // --- Enter ---
        if (c == '\n') {
            buf[buf_len] = '\0';
            // Переместить курсор в конец строки перед переводом
            cursor_x = prompt_x + buf_len;
            cursor_y = prompt_y;
            vga_putchar('\n');

            history_push(buf);
            history_nav = -1;

            char* cmd_name = buf;
            char* cmd_args = "";
            for (int i = 0; i < buf_len; i++) {
                if (buf[i] == ' ') {
                    buf[i] = '\0';
                    cmd_args = &buf[i + 1];
                    while (*cmd_args == ' ') cmd_args++;
                    break;
                }
            }

            if (strcmp(cmd_name, "echo") == 0) {
                cmd_echo(cmd_args);
            } else if (strcmp(cmd_name, "lscmd") == 0) {
                cmd_lscmd(cmd_args);
            } else {
                int found = 0;
                for (int i = 0; i < (int)CMD_COUNT; i++) {
                    if (strcmp(cmd_name, commands[i].name) == 0) {
                        commands[i].func(cmd_args);
                        found = 1;
                        break;
                    }
                }
                if (!found && strlen(cmd_name) > 0) {
                    set_color(LIGHT_RED, BLACK);
                    print("  unknown command: ");
                    set_color(WHITE, BLACK);
                    print(cmd_name);
                    print("\n");
                }
            }

            buf_len = 0;
            buf_pos = 0;
            set_color(YELLOW, BLACK);
            print("limon> ");
            set_color(WHITE, BLACK);
            prompt_x = cursor_x;
            prompt_y = cursor_y;
            hw_cursor_update();

        // --- Backspace ---
        } else if (c == '\b') {
            if (buf_pos > 0) {
                // Сдвинуть буфер влево от buf_pos
                for (int i = buf_pos - 1; i < buf_len - 1; i++)
                    buf[i] = buf[i + 1];
                buf_len--;
                buf_pos--;
                redraw_line(buf, buf_len, buf_pos, buf_pos, prompt_x, prompt_y);
            }

        // --- Обычный символ ---
        } else {
            if (buf_len < 79) {
                // Сдвинуть буфер вправо от buf_pos
                for (int i = buf_len; i > buf_pos; i--)
                    buf[i] = buf[i - 1];
                buf[buf_pos] = c;
                buf_len++;
                buf_pos++;
                redraw_line(buf, buf_len, buf_pos, buf_pos - 1, prompt_x, prompt_y);
            }
        }
    }
}
