#include "shell.h"
#include "usb.h"

void kernel_main(void)
{
    // ...hardware init...
    usb_init();

    // Start the shell
    shell_run();

    // Main loop (if needed)
    while (1)
    {
        usb_poll();
    }
}