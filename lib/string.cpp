#include "string.h"

size_t strlen_custom(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

void strcpy_custom(char* dest, const char* src) {
    int i = 0;
    while ((dest[i] = src[i]) != '\0') i++;
}

int strcmp_custom(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) break;
        i++;
    }
    return str1[i] - str2[i];
}

int strncmp_custom(const char* str1, const char* str2, size_t n) {
    for(size_t i = 0; i < n; i++) {
        if(str1[i] != str2[i] || str1[i] == '\0' || str2[i] == '\0') {
            return str1[i] - str2[i];
        }
    }
    return 0;
}