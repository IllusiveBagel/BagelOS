#include "shell.h"
#include "uart.h" // Assumes you have uart_puts, uart_readline, etc.
#include <stdint.h>

#define MAX_CMD_LEN 128

// Simple string compare
static int strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

// Command handler
static void handle_command(const char *cmd)
{
    if (strcmp(cmd, "hello") == 0)
    {
        uart_puts("Hello, Pi!\n");
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        uart_puts("Reboot not implemented yet.\n");
    }
    else if (strcmp(cmd, "help") == 0)
    {
        uart_puts("Available commands:\n");
        uart_puts("  help   - Show this help message\n");
        uart_puts("  hello  - Print greeting\n");
        uart_puts("  reboot - Reboot the Pi (stub)\n");
    }
    else
    {
        uart_puts("Unknown command. Type 'help' for a list.\n");
    }
}

// Shell main loop
void shell_run(void)
{
    char cmd[MAX_CMD_LEN];

    uart_puts("\nWelcome to PiOS Shell!\n");
    uart_puts("Type 'help' to see available commands.\n\n");

    while (1)
    {
        uart_puts("> ");
        uart_readline(cmd);
        handle_command(cmd);
    }
}