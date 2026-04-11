#pragma once
#include <stdint.h>

void*  memset (void* dst, uint8_t val, int n);
void*  memcpy (void* dst, const void* src, int n);
void*  memmove(void* dst, const void* src, int n);
int    memcmp (const void* a, const void* b, int n);