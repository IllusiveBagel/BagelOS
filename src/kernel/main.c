#include "uart.h"
#include "sd.h"
#include "fat.h"

void main()
{
    // set up serial console
    uart_init();

    // initialize EMMC and detect SD card type
    if (sd_init() == SD_OK)
    {
        // read the master boot record and find our partition
        if (fat_getpartition())
        {
            // list root directory entries
            fat_listdirectory();
        }
        else
        {
            uart_puts("FAT partition not found???\n");
        }
    }

    // echo everything back
    while (1)
    {
        uart_send(uart_getc());
    }
}