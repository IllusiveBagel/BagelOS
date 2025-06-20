CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
CFLAGS = -Iinclude -fno-stack-protector -nostdlib
CPPFLAGS = -fno-stack-protector

SRC = $(wildcard src/*.c) $(wildcard src/*.S)
OBJ = $(patsubst src/%.c,build/%.o,$(filter %.c,$(SRC))) \
      $(patsubst src/%.S,build/%.o,$(filter %.S,$(SRC)))

all: build kernel8.img

build:
	mkdir -p build

kernel8.img: kernel.elf
	$(OBJCOPY) kernel.elf -O binary kernel8.img

kernel.elf: $(OBJ) linker.ld
	$(LD) -T linker.ld $(OBJ) -o kernel.elf

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build *.elf *.img