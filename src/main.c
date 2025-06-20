#include "uart.h"
#include "heap.h"
#include "framebuffer.h"
#include "shell.h"

#define HEAP_START 0x90000
#define HEAP_SIZE (128 * 1024) // 128 KB

extern void vectors(void);

static inline void set_vbar_el1(void *addr)
{
    asm volatile("msr VBAR_EL1, %0" ::"r"(addr));
}

void main(void)
{
    heap_init((void *)HEAP_START, HEAP_SIZE);

    void *buf = simple_malloc(64);
    set_vbar_el1((void *)vectors);
    uart_init();

    fb_init(640, 480, 32);
    console_init(0xFFFFFF, 0x000000); // White on black

    shell_run();
}