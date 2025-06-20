#pragma once
#include <stdint.h>

int fb_init(uint32_t width, uint32_t height, uint32_t depth);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void fb_puts(uint32_t x, uint32_t y, const char *s, uint32_t fg, uint32_t bg);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);