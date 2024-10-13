#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>

// Прототипы функций ядра
extern "C" void kernel_main();
void print(const char *str);
void reboot_system();
void clear_screen();
char get_key();
unsigned char inb(unsigned short port);
void handle_input(const char* input);
void echo_command(const char* args);
void help_command();
void ps_command();

// Функции для работы со строками
size_t strlen_custom(const char* str);
void strcpy_custom(char* dest, const char* src);
int strcmp_custom(const char* str1, const char* str2);
int strncmp_custom(const char* str1, const char* str2, size_t n);

void move_cursor(int pos);
void outb(unsigned short port, unsigned char val);
void update_cursor(int row, int col);
void time_command();
void calc_command(const char* args);
void color_command(const char* args);
void reboot_command();
void clear_command();

#endif // KERNEL_H
