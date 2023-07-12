/*
 * lib/adt/range.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/assert.h"
#include "lib/overflow.h"
#include "lib/util.h"

#include "range.h"

struct range range_create(const uint64_t front, const uint64_t size) {
    return (struct range){
        .front = front,
        .size = size
    };
}

struct range range_multiply(const struct range range, const uint64_t mult) {
    return (struct range){
        .front = chk_mul_overflow_assert(range.front, mult),
        .size = chk_mul_overflow_assert(range.size, mult)
    };
}

bool range_has_index(const struct range range, const uint64_t index) {
    return index_in_bounds(index, range.size);
}

bool range_has_loc(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) < range.size);
}

bool range_get_end(const struct range range, uint64_t *const end_out) {
    return !chk_add_overflow(range.front, range.size, end_out);
}

bool range_above(const struct range range, const struct range above) {
    return range_is_loc_above(range, above.front);
}

bool range_below(const struct range range, const struct range below) {
    return range_is_loc_below(range, below.front);
}

bool range_is_loc_above(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) >= range.size);
}

bool range_is_loc_below(const struct range range, const uint64_t loc) {
    return (loc < range.front);
}

bool range_has_align(const struct range range, const uint64_t align) {
    return has_align(range.front, align) && has_align(range.size, align);
}

uint64_t range_get_end_assert(const struct range range) {
    uint64_t result = 0;
    assert(range_get_end(range, &result));

    return result;
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