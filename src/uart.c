#include "uart.h"

#define UART0_BASE 0x3F201000UL

#define UART0_DR (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UART0_IBRD (*(volatile uint32_t *)(UART0_BASE + 0x24))
#define UART0_FBRD (*(volatile uint32_t *)(UART0_BASE + 0x28))
#define UART0_LCRH (*(volatile uint32_t *)(UART0_BASE + 0x2C))
#define UART0_CR (*(volatile uint32_t *)(UART0_BASE + 0x30))
#define UART0_IMSC (*(volatile uint32_t *)(UART0_BASE + 0x38))
#define UART0_ICR (*(volatile uint32_t *)(UART0_BASE + 0x44))

void uart_init(void)
{
    // Disable UART0.
    UART0_CR = 0x00000000;

    // Clear pending interrupts.
    UART0_ICR = 0x7FF;

    // Set integer & fractional part of baud rate.
    // Divider = UART_CLOCK/(16 * Baud)
    // UART_CLOCK = 48 MHz, Baud = 115200.
    // Divider = 26.041666...
    UART0_IBRD = 26;
    UART0_FBRD = 3;

    // Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
    UART0_LCRH = (1 << 4) | (3 << 5);

    // Mask all interrupts.
    UART0_IMSC = 0;

    // Enable UART0, receive & transfer part of UART.
    UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(char c)
{
    // Wait for UART to become ready to transmit.
    while (UART0_FR & (1 << 5))
        ;
    UART0_DR = c;
}

void uart_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

char uart_getc(void)
{
    // Wait until the UART has received something
    while (UART0_FR & (1 << 4))
        ;
    return (char)(UART0_DR & 0xFF);
}

void uart_gets(char *buf, int maxlen)
{
    int i = 0;
    while (i < maxlen - 1)
    {
        char c = uart_getc();
        if (c == '\r' || c == '\n')
        {
            uart_putc('\n');
            break;
        }
        if (c == 0x7F || c == 0x08)
        { // Handle backspace
            if (i > 0)
            {
                i--;
                uart_puts("\b \b");
            }
            continue;
        }
        uart_putc(c); // Echo
        buf[i++] = c;
    }
    buf[i] = 0;
}