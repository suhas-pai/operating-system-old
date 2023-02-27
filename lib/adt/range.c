/*
 * lib/adt/range.c
 * © suhas pai
 */

#include "../overflow.h"
#include "../util.h"

#include "range.h"

bool range_has_index(const struct range range, const uint64_t index) {
    return index_in_bounds(index, range.size);
}

bool range_has_loc(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) < range.size);
}

bool range_get_end(const struct range range, uint64_t *const end_out) {
    return !ck_add_overflow(range.front, range.size, end_out);
}

struct range
subrange_from_index(const struct range range, const uint64_t index) {
    assert(range_has_index(range, index));
    const struct range result = {
        .front = index,
        .size = range.size - index
    };

    return result;
}

uint64_t range_loc_for_index(const struct range range, const uint64_t index) {
    assert(range_has_index(range, index));
    return range.front + index;
}

uint64_t range_index_for_loc(const struct range range, const uint64_t loc) {
    assert(range_has_loc(range, loc));
    return loc - range.front;
}

bool range_has(const struct range range, const struct range other) {
    if (!range_has_loc(range, other.front)) {
        return false;
    }

    const uint64_t index = range_index_for_loc(range, other.front);
    if (!range_has_index(range, index)) {
        return false;
    }

    return range.size - index >= other.size;
}

bool range_overlaps(const struct range range, const struct range other) {
    if (range_has_loc(range, other.front)) {
        return true;
    }

    if (range_has_loc(other, range.front)) {
        return true;
    }

    return false;
}