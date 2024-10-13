#include "kernel.h"
#include <stdarg.h>

// Заголовок Multiboot
__attribute__((section(".multiboot"))) 
const unsigned long multiboot_header[] = {
    0x1BADB002, // Magic number
    0x00,       // Flags
    (unsigned long)(-(0x1BADB002 + 0x00)) // Checksum
};

// Порт для считывания с клавиатуры
#define PORT 0x60

const char* prompt = "kernel> ";
char input_buffer[256];
int input_pos = 0;
unsigned char text_color = 0x07; // По умолчанию белый текст на черном фоне

// Функция для чтения байта из порта
unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0"
                      : "=a"(ret)
                      : "Nd"(port));
    return ret;
}

// Функция для чтения значения из CMOS
unsigned char read_cmos(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

// Функция для преобразования BCD в десятичное число
unsigned char bcd_to_decimal(unsigned char bcd) {
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

// Функция очистки экрана
void clear_screen() {
    char* video_memory = (char*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i + 1] = text_color;
    }
    update_cursor(0, 0);
}

// Функция вывода строки на экран
void print(const char *str) {
    static int row = 0, col = 0;
    char* video_memory = (char*)0xB8000;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            row++;
            col = 0;
        } else if (str[i] == '\b' && col > 0) {
            col--;
        } else {
            if (col >= 80) {
                row++;
                col = 0;
            }
            if (row >= 25) {
                for (int j = 0; j < 24 * 80 * 2; j++) {
                    video_memory[j] = video_memory[j + 160];
                }
                for (int j = 24 * 80 * 2; j < 25 * 80 * 2; j += 2) {
                    video_memory[j] = ' ';
                    video_memory[j + 1] = text_color;
                }
                row = 24;
            }
            int offset = (row * 80 + col) * 2;
            video_memory[offset] = str[i];
            video_memory[offset + 1] = text_color;
            col++;
        }
    }
    update_cursor(row, col);
}

// Таблица преобразования scancode в ASCII для US клавиатуры
char scancode_table[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 
    'm', ',', '.', '/', 0, '*', 0, ' ', 
    // Дополнительные scancode не используются в этом примере
};

char scancode_table_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', 
    '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 
    '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 
    'M', '<', '>', '?', 0, '*', 0, ' ', 
    // Дополнительные scancode не используются в этом примере
};

// Функция получения символа с клавиуры
bool shift_pressed = false;
bool ctrl_pressed = false;
bool alt_pressed = false;

char get_key() {
    unsigned char status, key;
    do {
        status = inb(0x64);
    } while ((status & 0x01) == 0);
    
    key = inb(PORT);
    
    switch (key) {
        case 0x2A: case 0x36: shift_pressed = true; break;
        case 0xAA: case 0xB6: shift_pressed = false; break;
        case 0x1D: ctrl_pressed = true; break;
        case 0x9D: ctrl_pressed = false; break;
        case 0x38: alt_pressed = true; break;
        case 0xB8: alt_pressed = false; break;
        default:
            if (key < 128) {
                return shift_pressed ? scancode_table_shift[key] : scancode_table[key];
            }
    }
    return 0;
}

// Простая реализация sprintf
int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int i = 0, j = 0;
    while (format[i]) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'd': {
                    int val = va_arg(args, int);
                    char num[20];
                    int k = 0;
                    if (val == 0) num[k++] = '0';
                    else {
                        while (val) {
                            num[k++] = val % 10 + '0';
                            val /= 10;
                        }
                    }
                    while (k > 0) str[j++] = num[--k];
                    break;
                }
                case 'c':
                    str[j++] = (char)va_arg(args, int);
                    break;
            }
        } else {
            str[j++] = format[i];
        }
        i++;
    }
    str[j] = '\0';
    va_end(args);
    return j;
}

// Простая реализация sscanf
int sscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int read = 0, i = 0, j = 0;
    while (format[i] && str[j]) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'd': {
                    int *val = va_arg(args, int*);
                    *val = 0;
                    while (str[j] >= '0' && str[j] <= '9') {
                        *val = *val * 10 + (str[j] - '0');
                        j++;
                    }
                    read++;
                    break;
                }
                case 'c': {
                    char *c = va_arg(args, char*);
                    *c = str[j++];
                    read++;
                    break;
                }
            }
        } else if (format[i] == str[j]) {
            i++;
            j++;
        } else {
            break;
        }
    }
    va_end(args);
    return read;
}

// Обработка ввода
char last_command[256] = {0};

void handle_input(const char* input) {
    if (strcmp_custom(input, "help") == 0) {
        help_command();
    } else if (strncmp_custom(input, "echo ", 5) == 0) {
        echo_command(input + 5);
    } else if (strncmp_custom(input, "calc ", 5) == 0) {
        calc_command(input + 5);
    } else if (strcmp_custom(input, "reboot") == 0) {
        reboot_command();
    } else if (strcmp_custom(input, "clear") == 0) {
        clear_command();
    } else {
        print("Command not found. Type 'help' for a list of commands.\n");
    }
    strcpy_custom(last_command, input);
}

// Команда echo
void echo_command(const char* args) {
    print(args);
    print("\n");
}

// Команда help
void help_command() {
    print("Available commands:\n");
    print("echo <text> - Displays the entered text\n");
    print("help - Displays a list of available commands\n");
    print("calc <num1><op><num2> - Performs a simple calculation\n");
    print("color <color> - Changes the text color\n");
    print("reboot - Reboots the system\n");
    print("clear - Clears the screen\n");
}

// Команда calc
void calc_command(const char* args) {
    int a = 0, b = 0;
    char op = 0;
    
    // Используем sscanf для разбора выражения
    if (sscanf(args, "%d%c%d", &a, &op, &b) != 3) {
        print("Usage: calc <number><operation><number>\n");
        return;
    }
    
    int result;
    switch(op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b == 0) {
                print("Error: Division by zero\n");
                return;
            }
            result = a / b; 
            break;
        default:
            print("Unsupported operation\n");
            return;
    }
    
    char result_str[50];
    sprintf(result_str, "%d%c%d = %d\n", a, op, b, result);
    print(result_str);
}

// Команда color
void color_command(const char* args) {
    if (strcmp_custom(args, "red") == 0) {
        text_color = 0x04;
    } else if (strcmp_custom(args, "green") == 0) {
        text_color = 0x02;
    } else if (strcmp_custom(args, "blue") == 0) {
        text_color = 0x01;
    } else if (strcmp_custom(args, "white") == 0) {
        text_color = 0x07;
    } else {
        print("Available colors: red, green, blue, white\n");
        return;
    }
    print("Text color changed\n");
}

// Команда reboot
void reboot_command() {
    print("Rebooting...\n");
    outb(0x64, 0xFE);
}

// Команда clear
void clear_command() {
    clear_screen();
}

extern "C" void kernel_main() {
    clear_screen();
    print("Welcome to ShitOS!\n");
    print(prompt);

    while (1) {
        char key = get_key();

        if (key == '\n') {
            input_buffer[input_pos] = '\0';
            print("\n");
            handle_input(input_buffer);
            input_pos = 0;
            print("\n");
            print(prompt);
        } else if (key == '\b') { // Backspace
            if (input_pos > 0) {
                input_pos--;
                print("\b \b");
                input_buffer[input_pos] = '\0';
            }
        } else if (key == 0x48) { // Up arrow key
            strcpy_custom(input_buffer, last_command);
            input_pos = strlen_custom(last_command);
            print("\r");
            print(prompt);
            print(input_buffer);
        } else if (key >= 32 && key <= 126) { // Printable characters
            if (input_pos < 255) {
                input_buffer[input_pos++] = key;
                char str[2] = {key, '\0'};
                print(str);
            }
        }
    }
}

void move_cursor(int pos) {
    unsigned short position = (unsigned short)pos;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void update_cursor(int row, int col) {
    unsigned short position = (row * 80) + col;
 
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}