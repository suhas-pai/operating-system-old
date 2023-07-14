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

#define range_create_empty() ((struct range){ .front = 0, .size = 0 })
#define range_create_max() ((struct range){ .front = 0, .size = UINT64_MAX })

struct range range_create(uint64_t front, uint64_t size);
struct range range_multiply(struct range range, uint64_t mult);

bool range_has_index(struct range range, uint64_t index);
bool range_has_loc(struct range range,  uint64_t loc);
bool range_get_end(struct range range, uint64_t *end_out);

bool range_above(struct range range, struct range above);
bool range_below(struct range range, struct range below);

bool range_is_loc_above(struct range range, uint64_t index);
bool range_is_loc_below(struct range range, uint64_t index);

bool range_has_align(struct range range, uint64_t align);

uint64_t range_get_end_assert(struct range range);
struct range subrange_from_index(struct range range, uint64_t index);

uint64_t range_loc_for_index(struct range range, uint64_t index);
uint64_t range_index_for_loc(struct range range, uint64_t loc);

bool range_has(struct range range, struct range other);
bool range_overlaps(struct range range, struct range other);