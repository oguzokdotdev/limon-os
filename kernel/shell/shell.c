#include "shell.h"
#include "drivers/video/vga.h"
#include "drivers/input/keyboard.h"
#include "drivers/timer/pit.h"
#include "drivers/timer/rtc.h"
#include "arch/x86/cpu.h"
#include "version.h"
#include "string.h"
#include "convert.h"

static uint32_t g_mem_upper = 0;

#define HISTORY_SIZE 16
static char history[HISTORY_SIZE][80];
static int  history_count = 0;
static int  history_nav   = -1;

static void history_push(const char* cmd) {
    if (!cmd[0]) return;
    if (history_count > 0 &&
        strcmp(history[(history_count-1) % HISTORY_SIZE], cmd) == 0) return;
    strcpy(history[history_count % HISTORY_SIZE], cmd);
    history_count++;
}

static int parse_args(char* str, char* argv[], int max_args) {
    int argc = 0, in_word = 0;
    while (*str) {
        if (*str == ' ') { *str = '\0'; in_word = 0; }
        else if (!in_word && argc < max_args) { argv[argc++] = str; in_word = 1; }
        str++;
    }
    return argc;
}

static void cmd_help(char* args);
typedef void (*cmd_func)(char* args);
typedef struct { const char* name; const char* desc; int cat; cmd_func func; } Command;

static void cmd_about(char* args)  { (void)args;
    vga_print("\n");
    vga_set_color(LIGHT_CYAN, BLACK); vga_print("         LimonOS " LIMON_CODENAME "\n");
    vga_set_color(WHITE, BLACK);      vga_print("    \"Clean, minimal, yours.\"\n\n");
    vga_set_color(LIGHT_GREY, BLACK);
    vga_print("    Author:   oguzokdotdev AKA oguzok.tech\n");
    vga_print("    GitHub:   oguzokdotdev/limon-os\n");
    vga_print("\n    Inspired by VibeOS.\n");
    vga_print("    Built with assistance from Claude & Gemini.\n\n");
}

static void cmd_uname(char* args) {
    char* argv[8]; int argc = parse_args(args, argv, 8);
    if (argc == 0) { vga_set_color(WHITE, BLACK); vga_print("  LimonOS\n"); return; }
    if (strcmp(argv[0], "--help") == 0 || strcmp(argv[0], "-help") == 0) {
        vga_set_color(LIGHT_CYAN, BLACK); vga_print("  uname - print system information\n");
        vga_set_color(LIGHT_GREY, BLACK); vga_print("  Usage: uname [option]\n\n");
        vga_print("  (no args)  OS name\n  -o  OS name\n  -v  version\n");
        vga_print("  -c  codename\n  -i  arch\n  -b  build\n  -a  all\n");
        return;
    }
    for (int k = 0; k < argc; k++) {
        const char* f = argv[k];
        vga_set_color(WHITE, BLACK);
        if      (strcmp(f, "-o") == 0) vga_print("  LimonOS\n");
        else if (strcmp(f, "-v") == 0) vga_print("  " LIMON_VERSION_STRING "\n");
        else if (strcmp(f, "-c") == 0) vga_print("  " LIMON_CODENAME "\n");
        else if (strcmp(f, "-i") == 0) vga_print("  " LIMON_ARCH "\n");
        else if (strcmp(f, "-b") == 0) { vga_print("  b"); vga_print_int(LIMON_BUILD); vga_print("\n"); }
        else if (strcmp(f, "-a") == 0) {
            vga_print("  LimonOS " LIMON_VERSION_STRING "-b");
            vga_print_int(LIMON_BUILD);
            vga_print(" " LIMON_CODENAME " " LIMON_ARCH "\n");
        } else {
            vga_set_color(LIGHT_RED, BLACK); vga_print("  unknown option: ");
            vga_set_color(WHITE, BLACK); vga_print(f); vga_print("\n");
        }
    }
}

static void cmd_ver(char* args)    { (void)args;
    vga_set_color(WHITE, BLACK);
    vga_print("  LimonOS " LIMON_CODENAME " v" LIMON_VERSION_STRING "-b");
    vga_print_int(LIMON_BUILD); vga_print("\n");
}

static void cmd_clear(char* args)  { (void)args; vga_clear(); }

static void cmd_echo(char* args) {
    vga_set_color(WHITE, BLACK); vga_print("  "); vga_print(args); vga_print("\n");
}

static void cmd_fetch(char* args)  { (void)args;
    char cpu[49]; get_cpu_model(cpu);
    uint32_t uptime = pit_get_uptime();
    uint32_t mem_mib = (g_mem_upper + 1024) / 1024;
    uint32_t h = uptime/3600, m = (uptime%3600)/60, s = uptime%60;
    const char* art[] = {
        " #       "," #       "," #       ",
        " #       "," #       "," ####### "
    };
    const char* keys[] = {
        "OS","Codename","Build","Arch","CPU","Memory","Uptime","Display","Shell"
    };
    vga_print("\n");
    for (int i = 0; i < 9; i++) {
        vga_set_color(YELLOW, BLACK);     vga_print(i < 6 ? art[i] : "         ");
        vga_set_color(LIGHT_CYAN, BLACK); vga_print(keys[i]); vga_print(": ");
        vga_set_color(WHITE, BLACK);
        switch (i) {
            case 0: vga_print(LIMON_VERSION_FULL); break;
            case 1: vga_print(LIMON_CODENAME); break;
            case 2: vga_print("b"); vga_print_int(LIMON_BUILD); break;
            case 3: vga_print(LIMON_ARCH); break;
            case 4: vga_print(cpu); break;
            case 5: vga_print_int(mem_mib); vga_print(" MiB"); break;
            case 6:
                vga_print_padded2(h); vga_print(":");
                vga_print_padded2(m); vga_print(":");
                vga_print_padded2(s); break;
            case 7: vga_print("VGA 80x25 16c"); break;
            case 8: vga_print("limon"); break;
        }
        vga_print("\n");
    }
    vga_print("         ");
    uint8_t colors[] = {BLACK,RED,GREEN,YELLOW,BLUE,MAGENTA,CYAN,LIGHT_GREY,
                        DARK_GREY,LIGHT_RED,LIGHT_GREEN,LIGHT_CYAN,
                        LIGHT_BLUE,LIGHT_MAGENTA,YELLOW,WHITE};
    for (int i = 0; i < 16; i++) {
        vga_set_color(colors[i], colors[i]); vga_putchar(' '); vga_putchar(' ');
    }
    vga_set_color(WHITE, BLACK); vga_print("\n\n");
}

static void cmd_reboot(char* args) { (void)args;
    vga_set_color(LIGHT_GREEN, BLACK); vga_print("  Rebooting...\n");
    uint8_t val = 0;
    while (val & 0x02) val = inb(0x64);
    outb(0x64, 0xFE);
    __asm__ volatile ("cli; hlt");
}

static void cmd_halt(char* args)   { (void)args;
    vga_set_color(LIGHT_RED, BLACK);
    vga_print("  System halted. You can close QEMU now.\n");
    __asm__ volatile ("cli; hlt");
}

static void cmd_div0(char* args)   { (void)args; volatile int x = 1/0; (void)x; }

static void cmd_date(char* args)   { (void)args;
    rtc_time_t t; rtc_read(&t);
    vga_set_color(WHITE, BLACK); vga_print("  ");
    vga_print_padded2(t.day);    vga_print(".");
    vga_print_padded2(t.month);  vga_print(".");
    vga_print_int(2000 + t.year); vga_print("  ");
    vga_print_padded2(t.hour);   vga_print(":");
    vga_print_padded2(t.minute); vga_print(":");
    vga_print_padded2(t.second); vga_print("\n");
}

static const Command commands[] = {
    {"help",       "show commands",            2, cmd_help},
    {"lscmd",      "list all commands",        2, 0},
    {"about",      "about LimonOS",            2, cmd_about},
    {"clear",      "clear screen",             1, cmd_clear},
    {"uname",      "system information",       2, cmd_uname},
    {"ver",        "system version",           2, cmd_ver},
    {"date",       "show current date/time",   2, cmd_date},
    {"limonfetch", "system fetch",             2, cmd_fetch},
    {"reboot",     "reboot system",            1, cmd_reboot},
    {"halt",       "halt system",              1, cmd_halt},
    {"div0",       "trigger #DE (test panic)", 1, cmd_div0},
};
#define CMD_COUNT (sizeof(commands)/sizeof(commands[0]))

static void cmd_help(char* args) {
    char* argv[4]; int argc = parse_args(args, argv, 4);
    if (argc == 0) {
        vga_set_color(LIGHT_CYAN, BLACK);
        vga_print("  Help Categories:\n  1 - System Control\n  2 - Info & Utilities\n");
        vga_set_color(LIGHT_GREY, BLACK); vga_print("  Usage: help [category]\n");
        return;
    }
    int cat = atoi(argv[0]);
    vga_set_color(LIGHT_CYAN, BLACK);
    if      (cat == 1) vga_print("  --- 1: System Control ---\n");
    else if (cat == 2) vga_print("  --- 2: Info & Utilities ---\n");
    else {
        vga_set_color(LIGHT_RED, BLACK);
        vga_print("  unknown category: "); vga_print(argv[0]); vga_print("\n"); return;
    }
    for (int i = 0; i < (int)CMD_COUNT; i++) {
        if (commands[i].cat == cat) {
            vga_print("  "); vga_print(commands[i].name);
            vga_print("   - "); vga_print(commands[i].desc); vga_print("\n");
        }
    }
    if (cat == 2) vga_print("  echo   - print text\n");
}

static void cmd_lscmd(char* args)  { (void)args;
    const char* names[CMD_COUNT+1]; int total = 0;
    for (int i = 0; i < (int)CMD_COUNT; i++) names[total++] = commands[i].name;
    names[total++] = "echo";
    int cols = (total > 30) ? 4 : (total > 20) ? 3 : 2;
    int rows = (total + cols - 1) / cols;
    vga_set_color(WHITE, BLACK); vga_print("\n");
    for (int r = 0; r < rows; r++) {
        vga_print("  ");
        for (int col = 0; col < cols; col++) {
            int idx = col*rows+r; if (idx >= total) break;
            vga_set_color(LIGHT_CYAN, BLACK); vga_print(names[idx]);
            int pad = 16 - strlen(names[idx]);
            for (int p = 0; p < pad; p++) vga_putchar(' ');
        }
        vga_print("\n");
    }
    vga_print("\n"); vga_set_color(LIGHT_GREY, BLACK);
}

static int tab_complete(char* buf, int buf_len) {
    const char* names[CMD_COUNT+1]; int total = 0;
    for (int i = 0; i < (int)CMD_COUNT; i++) names[total++] = commands[i].name;
    names[total++] = "echo";
    if (!buf_len) return buf_len;
    const char* matches[CMD_COUNT+1]; int mc = 0;
    for (int i = 0; i < total; i++) {
        int ok = 1;
        for (int j = 0; j < buf_len; j++)
            if (!names[i][j] || names[i][j] != buf[j]) { ok = 0; break; }
        if (ok) matches[mc++] = names[i];
    }
    if (mc == 0) return buf_len;
    if (mc == 1) {
        int fl = strlen(matches[0]);
        for (int i = 0; i < fl && i < 79; i++) buf[i] = matches[0][i];
        return fl;
    }
    vga_print("\n");
    for (int i = 0; i < mc; i++) {
        vga_set_color(LIGHT_CYAN, BLACK); vga_print("  "); vga_print(matches[i]); vga_print("\n");
    }
    return -1;
}

static void redraw_line(char* buf, int buf_len, int buf_pos,
                        int from, int px, int py) {
    vga_set_cursor(px + from, py);
    vga_set_color(WHITE, BLACK);
    for (int i = from; i < buf_len; i++) vga_putchar(buf[i]);
    vga_clear_at(px + buf_len, py);
    vga_set_cursor(px + buf_pos, py);
    hw_cursor_update();
}

void shell_init(uint32_t mem_upper) { g_mem_upper = mem_upper; }

void shell_run(void) {
    vga_set_color(YELLOW, BLACK);
    vga_center_print("  _     _                          ___  _____  ", 4);
    vga_center_print(" | |   (_)_ __ ___   ___  _ __    / _ \\/ ___/  ", 5);
    vga_center_print(" | |   | | '_ ` _ \\ / _ \\| '_ \\  | | | \\___ \\  ", 6);
    vga_center_print(" | |___| | | | | | | (_) | | | | | |_| |___) | ", 7);
    vga_center_print(" |_____|_|_| |_| |_|\\___/|_| |_|  \\___//____/  ", 8);
    vga_set_color(WHITE, BLACK);
    vga_center_print(LIMON_VERSION_FULL, 10);
    vga_center_print("clean, minimal, yours.", 11);
    vga_set_color(DARK_GREY, BLACK);
    vga_center_print("----------------------------------------", 12);
    vga_set_color(LIGHT_GREY, BLACK);
    vga_center_print("Developed with Claude Sonnet 4.6", 14);
    vga_center_print("Inspired by VibeOS", 15);

    vga_set_cursor(0, 24);
    vga_set_color(YELLOW, BLACK); vga_print("limon> ");
    vga_set_color(WHITE, BLACK);
    int px = vga_get_cursor_x(), py = vga_get_cursor_y();
    hw_cursor_enable(); hw_cursor_update();

    char buf[80]; int buf_len = 0, buf_pos = 0;

    while (1) {
        char c = keyboard_read();
        if (!c) continue;

        if (c == KEY_LEFT) {
            if (buf_pos > 0) { buf_pos--; vga_set_cursor(px+buf_pos, py); hw_cursor_update(); }
            continue;
        }
        if (c == KEY_RIGHT) {
            if (buf_pos < buf_len) { buf_pos++; vga_set_cursor(px+buf_pos, py); hw_cursor_update(); }
            continue;
        }

        if (c == KEY_UP || c == KEY_DOWN) {
            if (!history_count) continue;
            if (c == KEY_UP) {
                if (history_nav == -1) history_nav = history_count - 1;
                else if (history_nav > 0) history_nav--;
            } else {
                if (history_nav == -1) continue;
                if (++history_nav >= history_count) {
                    history_nav = -1;
                    vga_set_cursor(px, py);
                    for (int i = 0; i < buf_len; i++) vga_putchar(' ');
                    buf_len = buf_pos = 0;
                    vga_set_cursor(px, py); hw_cursor_update(); continue;
                }
            }
            vga_set_cursor(px, py);
            for (int i = 0; i < buf_len; i++) vga_putchar(' ');
            strcpy(buf, history[history_nav % HISTORY_SIZE]);
            buf_len = buf_pos = strlen(buf);
            vga_set_cursor(px, py); vga_set_color(WHITE, BLACK); vga_print(buf);
            hw_cursor_update(); continue;
        }

        if (c == '\t') {
            int nl = tab_complete(buf, buf_pos);
            if (nl == -1) {
                vga_set_color(YELLOW, BLACK); vga_print("limon> ");
                vga_set_color(WHITE, BLACK);
                px = vga_get_cursor_x(); py = vga_get_cursor_y();
                for (int i = 0; i < buf_len; i++) vga_putchar(buf[i]);
                vga_set_cursor(px+buf_pos, py); hw_cursor_update();
            } else if (nl != buf_pos) {
                buf_len = buf_pos = nl;
                vga_set_cursor(px, py); vga_set_color(WHITE, BLACK);
                for (int i = 0; i < buf_len; i++) vga_putchar(buf[i]);
                hw_cursor_update();
            }
            continue;
        }

        if (c == '\n') {
            buf[buf_len] = '\0';
            vga_set_cursor(px+buf_len, py); vga_putchar('\n');
            history_push(buf); history_nav = -1;
            hw_cursor_hide();

            char* cmd_name = buf, *cmd_args = "";
            for (int i = 0; i < buf_len; i++) {
                if (buf[i] == ' ') {
                    buf[i] = '\0'; cmd_args = &buf[i+1];
                    while (*cmd_args == ' ') cmd_args++;
                    break;
                }
            }

            if      (strcmp(cmd_name, "echo")  == 0) cmd_echo(cmd_args);
            else if (strcmp(cmd_name, "lscmd") == 0) cmd_lscmd(cmd_args);
            else {
                int found = 0;
                for (int i = 0; i < (int)CMD_COUNT; i++) {
                    if (strcmp(cmd_name, commands[i].name) == 0) {
                        commands[i].func(cmd_args); found = 1; break;
                    }
                }
                if (!found && strlen(cmd_name) > 0) {
                    vga_set_color(LIGHT_RED, BLACK); vga_print("  unknown command: ");
                    vga_set_color(WHITE, BLACK); vga_print(cmd_name); vga_print("\n");
                }
            }

            buf_len = buf_pos = 0;
            vga_set_color(YELLOW, BLACK); vga_print("limon> ");
            vga_set_color(WHITE, BLACK);
            px = vga_get_cursor_x(); py = vga_get_cursor_y();
            hw_cursor_enable(); hw_cursor_update();

        } else if (c == '\b') {
            if (buf_pos > 0) {
                for (int i = buf_pos-1; i < buf_len-1; i++) buf[i] = buf[i+1];
                buf_len--; buf_pos--;
                redraw_line(buf, buf_len, buf_pos, buf_pos, px, py);
            }
        } else {
            if (buf_len < 79) {
                for (int i = buf_len; i > buf_pos; i--) buf[i] = buf[i-1];
                buf[buf_pos] = c; buf_len++; buf_pos++;
                redraw_line(buf, buf_len, buf_pos, buf_pos-1, px, py);
            }
        }
    }
}