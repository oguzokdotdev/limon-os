#pragma once
#include <stdint.h>

int    strlen (const char* s);
int    strcmp (const char* a, const char* b);
int    strncmp(const char* a, const char* b, int n);
void   strcpy (char* dst, const char* src);
void   strcat (char* dst, const char* src);
char*  strchr (const char* s, char c);