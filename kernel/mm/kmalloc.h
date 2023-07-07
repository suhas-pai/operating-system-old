/*
 * kernel/mm/kmalloc.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void kmalloc_init();

void *kmalloc(uint64_t size);
void *krealloc(void *buffer, uint64_t size);

void kfree(void *buffer);