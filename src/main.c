#include "uart.h"
#include "shell.h"

void main(void)
{
    uart_init();
    uart_puts("Hello World!\n");
    shell();
}