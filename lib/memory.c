/*
 * lib/memory.c
 * © suhas pai
 */

#include "memory.h"

void memset_16(uint16_t *const buf, const uint64_t count, const uint16_t c) {
    const uint16_t *const end = buf + count;
    for (uint16_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
}

void memset_32(uint32_t *const buf, const uint64_t count, const uint32_t c) {
    const uint32_t *const end = buf + count;
    for (uint32_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
}

void memset_64(uint64_t *const buf, const uint64_t count, const uint64_t c) {
    const uint64_t *const end = buf + count;
    for (uint64_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
}