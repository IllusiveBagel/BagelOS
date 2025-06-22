#include "uart.h"
#include "shell.h"

#define SHELL_BUF_SIZE 128

void shell()
{
    char buf[SHELL_BUF_SIZE];
    while (1)
    {
        uart_puts("> ");
        int i = 0;
        while (1)
        {
            char c = uart_getc();
            if (c == '\r' || c == '\n')
            {
                uart_puts("\r\n");
                buf[i] = 0;
                break;
            }
            else if (c == 127 || c == 8)
            { // Backspace
                if (i > 0)
                {
                    i--;
                    uart_puts("\b \b");
                }
            }
            else if (i < SHELL_BUF_SIZE - 1 && c >= 32 && c < 127)
            {
                buf[i++] = c;
                uart_send(c);
            }
        }
        uart_puts("You typed: ");
        uart_puts(buf);
        uart_puts("\r\n");
    }
}