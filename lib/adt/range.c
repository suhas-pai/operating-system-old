/*
 * lib/adt/range.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/math.h"
#include "lib/overflow.h"
#include "lib/util.h"

struct range range_create(const uint64_t front, const uint64_t size) {
    return (struct range){
        .front = front,
        .size = size
    };
}

struct range range_create_end(const uint64_t front, const uint64_t end) {
    assert(front <= end);
    return range_create(front, (end - front));
}

struct range range_multiply(const struct range range, const uint64_t mult) {
    return range_create(check_mul_assert(range.front, mult),
                        check_mul_assert(range.size, mult));
}

struct range range_align_in(struct range range, uint64_t boundary) {
    return range_create(align_down(range.front, boundary),
                        align_down(range.size, boundary));
}

bool
range_align_out(const struct range range,
                const uint64_t boundary,
                struct range *const result_out)
{
    uint64_t front = 0;
    uint64_t size = 0;

    if (!align_up(range.front, boundary, &front) ||
        !align_up(range.size, boundary, &size))
    {
        return false;
    }

    *result_out = range_create(front, size);
    return true;
}

bool
range_round_up(const struct range range,
               const uint64_t mult,
               struct range *const result_out)
{
    if (!round_up(range.front, mult, &result_out->front)) {
        return false;
    }

    if (!round_up(range.size, mult, &result_out->size)) {
        return false;
    }

    return true;
}

bool
range_round_up_subrange(const struct range range,
                        const uint64_t mult,
                        struct range *const result_out)
{
    struct range result = {};
    if (!round_up(range.front, mult, &result.front)) {
        return false;
    }

    if (result.front - range.front >= range.size) {
        return false;
    }

    result.size = range.size - (result.front - range.front);
    *result_out = result;

    return true;
}

bool range_has_index(const struct range range, const uint64_t index) {
    return index_in_bounds(index, range.size);
}

bool range_has_loc(const struct range range, const uint64_t loc) {
    return (loc >= range.front && (loc - range.front) < range.size);
}

bool range_has_end(const struct range range, const uint64_t loc) {
    return (loc > range.front && (loc - range.front) <= range.size);
}

bool range_get_end(const struct range range, uint64_t *const end_out) {
    return !check_add(range.front, range.size, end_out);
}

bool range_above(const struct range range, const struct range above) {
    return range_is_loc_above(range, above.front);
}

bool range_below(const struct range range, const struct range below) {
    return range_is_loc_below(range, below.front);
}

bool range_empty(const struct range range) {
    return range.size == 0;
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

struct range
subrange_to_full(const struct range range, const struct range index) {
    assert(range_has_index_range(range, index));
    const struct range result = {
        .front = range.front + index.front,
        .size = index.size
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

bool range_has_index_range(struct range range, struct range other) {
    if (!range_has_index(range, other.front)) {
        return false;
    }

    return range.size - other.front >= other.size;
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