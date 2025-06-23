#include "uart.h"
#include "lfb.h"

void main()
{
    // set up serial console and linear frame buffer
    uart_init();
    lfb_init();

    // display a pixmap
    lfb_showpicture();

    // echo everything back
    while (1)
    {
        uart_send(uart_getc());
    }
}