#include "uart.h"
#include "sd.h"

void main()
{
    uart_init();
    static unsigned char mbr[512];

    if (sd_init() == SD_OK)
    {
        uart_puts("SD init OK\n");
        if (sd_readblock(0, mbr, 1))
        {
            uart_puts("Read block OK\n");
            for (int i = 0; i < 16; i++)
            {
                uart_hex(mbr[i]);
                uart_puts(" ");
            }
            uart_puts("\n");
        }
        else
        {
            uart_puts("Read block failed\n");
        }
    }
    else
    {
        uart_puts("SD init failed\n");
    }

    while (1)
    {
        uart_send(uart_getc());
    }
}