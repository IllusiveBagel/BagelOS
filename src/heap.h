#pragma once
#include <stddef.h>
void heap_init(void *heap_start, size_t heap_size);
void *simple_malloc(size_t size);