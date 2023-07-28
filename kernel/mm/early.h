/*
 * kernel/mm/early.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

void mm_early_init();
void mm_early_post_arch_init();

uint64_t early_alloc_page();
uint64_t early_alloc_cont_pages(uint32_t amount);