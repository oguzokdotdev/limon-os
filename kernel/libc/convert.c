#include "convert.h"

int atoi(const char* str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

void itoa(int n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0;
    int neg = (n < 0);
    if (neg) n = -n;
    char tmp[12];
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    if (neg) tmp[i++] = '-';
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

void itoh(uint32_t n, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 7; i >= 0; i--)
        buf[2 + (7 - i)] = hex[(n >> (i * 4)) & 0xF];
    buf[10] = '\0';
}