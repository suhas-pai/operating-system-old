/*
 * lib/memory.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

uint16_t *memset_16(uint16_t *buf, uint64_t count, uint16_t c);
uint32_t *memset_32(uint32_t *buf, uint64_t count, uint32_t c);
uint64_t *memset_64(uint64_t *buf, uint64_t count, uint64_t c);