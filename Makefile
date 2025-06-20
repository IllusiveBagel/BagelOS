CROSS_COMPILE = aarch64-none-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

SRC = src/start.S src/main.c src/uart.c
OBJ = $(patsubst src/%.c,build/%.o,$(filter %.c,$(SRC))) \
      $(patsubst src/%.S,build/%.o,$(filter %.S,$(SRC)))

INCLUDE = -Iinclude

all: build kernel8.img

build:
    mkdir -p build

kernel8.img: kernel.elf
    $(OBJCOPY) kernel.elf -O binary kernel8.img

kernel.elf: $(OBJ) linker.ld
    $(LD) -T linker.ld $(OBJ) -o kernel.elf

build/%.o: src/%.c
    $(CC) $(INCLUDE) -c $< -o $@

build/%.o: src/%.S
    $(CC) $(INCLUDE) -c $< -o $@

clean:
    rm -rf build *.elf *.img