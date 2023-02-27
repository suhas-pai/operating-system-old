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

bool range_has_index(struct range range, uint64_t index);
bool range_has_loc(struct range range, uint64_t loc);
bool range_get_end(struct range range, uint64_t *end_out);

struct range subrange_from_index(struct range range, uint64_t index);

uint64_t range_loc_for_index(struct range range, uint64_t index);
uint64_t range_index_for_loc(struct range range, uint64_t loc);

bool range_has(struct range range, struct range other);
bool range_overlaps(struct range range, struct range other);