/*
 * kernel/mm/early.h
 * © suhas pai
 */

#pragma once

#include "mm_types.h"
#include "zone.h"

void mm_early_init();
void mm_early_post_arch_init();

void mm_early_refcount_alloced_map(uint64_t virt_addr, uint64_t length);

uint64_t early_alloc_page();
uint64_t early_alloc_large_page(uint32_t amount);

void
early_free_pages_to_zone(struct page *page,
                         struct page_zone *zone,
                         uint8_t order);