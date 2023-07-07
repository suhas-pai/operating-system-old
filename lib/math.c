/*
 * lib/math.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "math.h"

bool
round_up(const uint64_t number,
         const uint64_t multiple,
         uint64_t *const result_out)
{
    if (multiple == 0) {
        return false;
    }

    if (chk_add_overflow(number, multiple - 1, result_out)) {
        return false;
    }

    if (chk_mul_overflow(*result_out / multiple, multiple, result_out)) {
        return false;
    }

    return true;
}