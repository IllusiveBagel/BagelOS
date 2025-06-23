#include "uart.h"
#include "power.h"

void main()
{
    char c;

    // set up serial console
    uart_init();

    while (1)
    {
        uart_puts(" 1 - power off\n 2 - reset\nChoose one: ");
        c = uart_getc();
        uart_send(c);
        uart_puts("\n\n");
        if (c == '1')
            power_off();
        if (c == '2')
            reset();
    }
}