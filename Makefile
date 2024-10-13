ASM=nasm
CC=g++
CFLAGS=-ffreestanding -m32 -std=c++17 -Wall -Wextra -Iinclude -fno-stack-protector
LD=ld
LDFLAGS=-m elf_i386 -T linker.ld

OBJ=src/kernel.o src/main.o lib/string.o

all: os.iso

# Сборка загрузчика
boot/boot.bin: boot/boot.asm
	$(ASM) boot/boot.asm -f bin -o boot/boot.bin

# Сборка объектных файлов
src/kernel.o: src/kernel.cpp include/kernel.h
	$(CC) $(CFLAGS) -c src/kernel.cpp -o src/kernel.o

src/main.o: src/main.cpp include/kernel.h
	$(CC) $(CFLAGS) -c src/main.cpp -o src/main.o

lib/string.o: lib/string.cpp lib/string.h
	$(CC) $(CFLAGS) -c lib/string.cpp -o lib/string.o

# Сборка ядра
src/kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o src/kernel.bin $(OBJ)

# Создание финального образа
os.iso: src/kernel.bin
	mkdir -p iso/boot/grub
	cp src/kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o os.iso iso

clean:
	rm -f boot/boot.bin src/*.o src/kernel.bin os.iso
	rm -rf iso

.PHONY: all clean
