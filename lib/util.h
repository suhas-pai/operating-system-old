/*
 * lib/util.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "macros.h"

__unused static inline
bool index_in_bounds(const uint64_t index, const uint64_t bounds) {
    return index < bounds;
}

__unused static inline
bool ordinal_in_bounds(const uint64_t ordinal, const uint64_t bounds) {
    return ordinal != 0 && ordinal <= bounds;
}