/*
 * lib/adt/range.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct range {
    uint64_t front;
    uint64_t size;
};

bool range_has_index(const struct range range, const uint64_t index);
bool range_has_loc(const struct range range, const uint64_t loc);
bool range_get_end(const struct range range, uint64_t *const end_out);

struct range
subrange_from_index(const struct range range, const uint64_t index);

uint64_t range_loc_for_index(const struct range range, const uint64_t index);
uint64_t range_index_for_loc(const struct range range, const uint64_t loc);

bool range_has(const struct range range, const struct range other);
bool range_overlaps(const struct range range, const struct range other);