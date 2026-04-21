#include "string.h"

int strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

int strncmp(const char* a, const char* b, int n) {
    while (n-- && *a && *b && *a == *b) { a++; b++; }
    return n < 0 ? 0 : *a - *b;
}

void strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void strcat(char* dst, const char* src) {
    dst += strlen(dst);
    strcpy(dst, src);
}

char* strchr(const char* s, char c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return 0;
}