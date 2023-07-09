/*
 * lib/align.c
 * © suhas pai
 */

#include "align.h"
#include "overflow.h"

uint64_t align_down(const uint64_t number, const uint64_t boundary) {
    assert(boundary != 0);

    const uint64_t mask = ~(boundary - 1);
    return (number & mask);
}

bool
align_up(const uint64_t number,
         const uint64_t boundary,
         uint64_t *const result_out)
{
    uint64_t result = 0;
    if (boundary == 0 || chk_add_overflow(number, boundary - 1, &result)) {
        return false;
    }

    *result_out = align_down(result, boundary);
    return true;
}