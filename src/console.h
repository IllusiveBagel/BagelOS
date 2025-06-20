#pragma once
#include <stdint.h>

void console_init(uint32_t fg, uint32_t bg);
void console_putc(char c);
void console_puts(const char *s);
void console_newline(void);
void console_clear(void);