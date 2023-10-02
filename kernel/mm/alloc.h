/*
 * kernel/mm/alloc.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

// For use when inside an internal mm function

void mm_alloc_init();

void *mm_alloc(uint64_t size);
void mm_alloc_free(void *buffer);

void *mm_realloc(void *obj, uint64_t size);