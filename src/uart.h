#pragma once
#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc(void);
void uart_gets(char *buf, int maxlen);