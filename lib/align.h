/*
 * lib/align.h
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "assert.h"

static inline bool has_align(const uint64_t number, const uint64_t boundary) {
    assert(boundary != 0);
    return (number & (boundary - 1)) == 0;
}

uint64_t align_down(uint64_t number, uint64_t boundary);
bool align_up(uint64_t number, uint64_t boundary, uint64_t *result_out);