#include "heap.h"

static uint8_t *heap_ptr = 0;
static uint8_t *heap_end = 0;

void heap_init(void *heap_start, size_t heap_size)
{
    heap_ptr = (uint8_t *)heap_start;
    heap_end = heap_ptr + heap_size;
}

void *simple_malloc(size_t size)
{
    // Align to 8 bytes
    size = (size + 7) & ~7;
    if (heap_ptr + size > heap_end)
        return 0; // Out of memory
    void *ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}