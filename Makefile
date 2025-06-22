AARCH64_TOOLCHAIN=aarch64-linux-gnu
CC=$(AARCH64_TOOLCHAIN)-gcc
LD=$(AARCH64_TOOLCHAIN)-ld
CFLAGS=-Wall -O2 -nostdlib -nostartfiles -ffreestanding -I./src
LDFLAGS=-T linker.ld

SRC := $(wildcard src/*.c src/*.S)
OBJ := $(patsubst src/%,build/%, $(SRC:.c=.o))
OBJ := $(patsubst src/%,build/%, $(OBJ:.S=.o))

all: kernel8.img

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.S | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

kernel8.img: $(OBJ)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJ)
	$(AARCH64_TOOLCHAIN)-objcopy kernel.elf -O binary kernel8.img

clean:
	rm -f kernel8.img kernel.elf src/*.o
