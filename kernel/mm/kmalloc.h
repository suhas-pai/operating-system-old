/*
 * kernel/mm/kmalloc.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

void kmalloc_init();
bool kmalloc_initialized();

void kfree(void *buffer);

__malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc(uint64_t size);

__alloc_size(2) void *krealloc(void *buffer, uint64_t size);