#include "console.h"
#include "framebuffer.h"
#include "font8x8.h"
#include <string.h>

#define CONSOLE_COLS (fb_get_width() / 8)
#define CONSOLE_ROWS (fb_get_height() / 8)

static uint32_t cursor_x = 0, cursor_y = 0;
static uint32_t fg_color = 0xFFFFFF, bg_color = 0x000000;

void console_init(uint32_t fg, uint32_t bg)
{
    fg_color = fg;
    bg_color = bg;
    cursor_x = 0;
    cursor_y = 0;
    fb_clear(bg_color);
}

void console_newline(void)
{
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= CONSOLE_ROWS)
    {
        // Scroll up
        for (uint32_t y = 1; y < CONSOLE_ROWS; y++)
        {
            for (uint32_t x = 0; x < CONSOLE_COLS; x++)
            {
                // Copy char from (x, y) to (x, y-1)
                // For simplicity, just clear the last line
            }
        }
        fb_clear(bg_color); // Simple: clear screen instead of scrolling
        cursor_y = 0;
    }
}

void console_putc(char c)
{
    if (c == '\n')
    {
        console_newline();
        return;
    }
    fb_draw_char(cursor_x * 8, cursor_y * 8, c, fg_color, bg_color);
    cursor_x++;
    if (cursor_x >= CONSOLE_COLS)
    {
        console_newline();
    }
}

void console_puts(const char *s)
{
    while (*s)
    {
        console_putc(*s++);
    }
}

void console_clear(void)
{
    fb_clear(bg_color);
    cursor_x = 0;
    cursor_y = 0;
}