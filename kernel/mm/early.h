/*
 * kernel/mm/early.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/range.h"

void mm_early_init();
void mm_early_post_arch_init();

void mm_early_refcount_alloced_map(uint64_t virt_addr, uint64_t length);

uint64_t early_alloc_page();
uint64_t early_alloc_large_page(uint32_t amount);
