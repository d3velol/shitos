BITS 16
ORG 0x7C00

start:
    ; Инициализация сегментов
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Загрузка ядра
    mov si, msg
    call print_string

    ; Отладочное сообщение перед переходом в защищенный режим
    mov si, protected_mode_msg
    call print_string

    ; Переход в защищенный режим
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Задержка
    mov cx, 0xFFFF
delay:
    loop delay

    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    ; Инициализация сегментов в защищенном режиме
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x9C00

    ; Отладочное сообщение перед переходом к ядру
    mov si, kernel_msg
    call print_string

    ; Переход к ядру
    mov eax, 0x100000  ; Адрес загрузки ядра
    jmp eax

hang:
    jmp hang

print_string:
    mov ah, 0x0E
.print_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .print_char
.done:
    ret

msg db 'Загрузка ядра...', 0
protected_mode_msg db 'Entering protected mode...', 0
kernel_msg db 'Jumping to kernel...', 0

gdt_start:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_start + 8
DATA_SEG equ gdt_start + 16

times 510-($-$$) db 0
dw 0xAA55
