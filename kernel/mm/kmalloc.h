/*
 * kernel/mm/kmalloc.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

void kmalloc_init();
bool kmalloc_initialized();

void *kmalloc(uint64_t size);
void *krealloc(void *buffer, uint64_t size);

void kfree(void *buffer);