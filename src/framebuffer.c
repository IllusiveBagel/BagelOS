#include "framebuffer.h"
#include "font8x8.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Mailbox registers
#define MMIO_BASE 0x3F000000
#define MAILBOX_BASE (MMIO_BASE + 0xB880)
#define MAILBOX_READ ((volatile uint32_t *)(MAILBOX_BASE + 0x00))
#define MAILBOX_STATUS ((volatile uint32_t *)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE ((volatile uint32_t *)(MAILBOX_BASE + 0x20))
#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FB 1

static volatile uint32_t __attribute__((aligned(16))) mbox[36];
static uint32_t *fb = 0;
static uint32_t fb_width, fb_height, fb_pitch;

static void mailbox_write(uint32_t msg, uint32_t channel)
{
    while (*MAILBOX_STATUS & MAILBOX_FULL)
        ;
    *MAILBOX_WRITE = (msg & ~0xF) | (channel & 0xF);
}

static uint32_t mailbox_read(uint32_t channel)
{
    uint32_t r;
    do
    {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
            ;
        r = *MAILBOX_READ;
    } while ((r & 0xF) != channel);
    return r & ~0xF;
}

int fb_init(uint32_t width, uint32_t height, uint32_t depth)
{
    memset((void *)mbox, 0, sizeof(mbox));
    mbox[0] = 35 * 4;
    mbox[1] = 0;
    mbox[2] = 0x48003;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = width;
    mbox[6] = height;
    mbox[7] = 0x48004;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = width;
    mbox[11] = height;
    mbox[12] = 0x48005;
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = depth;
    mbox[16] = 0x48006;
    mbox[17] = 4;
    mbox[18] = 4;
    mbox[19] = 0;
    mbox[20] = 0x40001;
    mbox[21] = 8;
    mbox[22] = 8;
    mbox[23] = 16;
    mbox[24] = 0;
    mbox[25] = 0x40008;
    mbox[26] = 4;
    mbox[27] = 4;
    mbox[28] = 0;
    mbox[29] = 0; // End tag

    mailbox_write((uint32_t)mbox, MAILBOX_FB);
    mailbox_read(MAILBOX_FB);

    if (mbox[20] != 0x40001 || mbox[28] == 0)
        return -1; // Failed

    fb = (uint32_t *)((uint64_t)mbox[28]);
    fb_width = mbox[5];
    fb_height = mbox[6];
    fb_pitch = mbox[19] / 4;
    return 0;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x < fb_width && y < fb_height)
        fb[y * fb_pitch + x] = color;
}

void fb_clear(uint32_t color)
{
    for (uint32_t y = 0; y < fb_height; y++)
        for (uint32_t x = 0; x < fb_width; x++)
            fb_put_pixel(x, y, color);
}

// --- Simple 8x8 font (ASCII only, for demo) ---
extern const uint8_t font8x8_basic[128][8];

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg)
{
    if ((unsigned char)c > 127)
        return;
    for (uint32_t row = 0; row < 8; row++)
    {
        uint8_t bits = font8x8_basic[(unsigned char)c][row];
        for (uint32_t col = 0; col < 8; col++)
        {
            fb_put_pixel(x + col, y + row, (bits & (1 << col)) ? fg : bg);
        }
    }
}

void fb_puts(uint32_t x, uint32_t y, const char *s, uint32_t fg, uint32_t bg)
{
    while (*s)
    {
        fb_draw_char(x, y, *s++, fg, bg);
        x += 8;
    }
}