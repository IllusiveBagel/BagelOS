AARCH64_TOOLCHAIN=aarch64-linux-gnu
CC=$(AARCH64_TOOLCHAIN)-gcc
LD=$(AARCH64_TOOLCHAIN)-ld
CFLAGS=-Wall -O2 -nostdlib -nostartfiles -ffreestanding -I./src -I./src/drivers -I ./src/kernel -I ./src/startup
LDFLAGS=-T linker.ld

SRC := $(shell find src -name '*.c' -o -name '*.S')
OBJ := $(patsubst src/%,build/%, $(SRC:.c=.o))
OBJ := $(patsubst src/%,build/%, $(OBJ:.S=.o))
FONTS := build/font_psf.o build/font_sfn.o

all: kernel8.img

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build_dirs:
	mkdir -p build

build/font_psf.o: include/font.psf | build
	$(LD) -r -b binary -o build/font_psf.o include/font.psf

build/font_sfn.o: include/font.sfn | build
	$(LD) -r -b binary -o build/font_sfn.o include/font.sfn

kernel8.img: $(OBJ) $(FONTS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJ) $(FONTS)
	$(AARCH64_TOOLCHAIN)-objcopy kernel.elf -O binary kernel8.img

clean:
	rm -f kernel8.img kernel.elf
	rm -rf build
