
#include "uart.h"
#include "sd.h"
#include "fat.h"

void main()
{
    unsigned int cluster;
    // set up serial console
    uart_init();

    // initialize EMMC and detect SD card type
    if (sd_init() == SD_OK)
    {
        // read the master boot record and find our partition
        if (fat_getpartition())
        {
            // find out file in root directory entries
            cluster = fat_getcluster("LICENC~1BRO");
            if (cluster == 0)
                cluster = fat_getcluster("KERNEL8 IMG");
            if (cluster)
            {
                // read into memory
                uart_dump(fat_readfile(cluster));
            }
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