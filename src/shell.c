#include "uart.h"
#include <string.h>

static void shell_help(void)
{
    uart_puts("Commands:\n");
    uart_puts("  help   - Show this help\n");
    uart_puts("  echo   - Echo input\n");
    uart_puts("  clear  - Clear screen (if supported)\n");
}

static void shell_clear(void)
{
    // ANSI escape code for clear screen (works in many terminals)
    uart_puts("\033[2J\033[H");
}

void shell_run(void)
{
    char buf[128];

    uart_puts("Welcome to BagelOS shell!\nType 'help' for commands.\n");

    while (1)
    {
        uart_puts("> ");
        uart_gets(buf, sizeof(buf));

        if (strcmp(buf, "help") == 0)
        {
            shell_help();
        }
        else if (strncmp(buf, "echo ", 5) == 0)
        {
            uart_puts(buf + 5);
            uart_puts("\n");
        }
        else if (strcmp(buf, "clear") == 0)
        {
            shell_clear();
        }
        else if (buf[0] == 0)
        {
            // Ignore empty input
        }
        else
        {
            uart_puts("Unknown command. Type 'help'.\n");
        }
    }
}