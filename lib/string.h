#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen_custom(const char* str);
void strcpy_custom(char* dest, const char* src);
int strcmp_custom(const char* str1, const char* str2);
int strncmp_custom(const char* str1, const char* str2, size_t n);

#endif // STRING_H