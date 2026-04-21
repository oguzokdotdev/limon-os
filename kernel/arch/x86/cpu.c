#include "cpu.h"

void get_cpu_vendor(char* out) {
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

void get_cpu_model(char* out) {
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

uint32_t read_cr2(void) {
    uint32_t v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}