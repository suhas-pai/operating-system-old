/*
 * kernel/boot.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/range.h"

void boot_early_init();
void boot_init();

uint8_t mm_get_memmap_count();
uint8_t mm_get_usable_count();

const void *boot_get_rsdp();
const void *boot_get_dtb();

int64_t boot_get_time();

void boot_merge_usable_memmaps();
void boot_recalculate_pfns();