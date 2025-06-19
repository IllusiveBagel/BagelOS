# Toolchain
CROSS_COMPILE = arm-none-eabi-
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# Directories
KERNEL_DIR = kernel
SHELL_DIR  = shell
TINYUSB_DIR = tinyusb/src

INCLUDES = -I$(KERNEL_DIR) -I$(SHELL_DIR) -I$(TINYUSB_DIR) -I.

# Source files
KERNEL_SRC = $(KERNEL_DIR)/kernel_main.c $(KERNEL_DIR)/uart.c $(KERNEL_DIR)/usb.c $(KERNEL_DIR)/eth.c
SHELL_SRC  = $(SHELL_DIR)/shell.c
TINYUSB_SRC = $(wildcard $(TINYUSB_DIR)/**/*.c) $(wildcard $(TINYUSB_DIR)/*.c)

SRC = $(KERNEL_SRC) $(SHELL_SRC) $(TINYUSB_SRC)

# Object files
OBJ = $(SRC:.c=.o)

# Output
ELF = kernel.elf
BIN = kernel.img

# Flags
CFLAGS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding $(INCLUDES)
LDFLAGS = -T linker.ld

# Default target
all: $(BIN)

$(BIN): $(ELF)
    $(OBJCOPY) $(ELF) -O binary $@

$(ELF): $(OBJ)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(KERNEL_DIR)/*.o $(SHELL_DIR)/*.o $(TINYUSB_DIR)/**/*.o $(TINYUSB_DIR)/*.o $(ELF) $(BIN)

.PHONY: all clean