/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once
#include "lib/assert.h"

#define chk_add_overflow(lhs, rhs, result) \
    __builtin_add_overflow(lhs, rhs, result)
#define chk_sub_overflow(lhs, rhs, result) \
    __builtin_sub_overflow(lhs, rhs, result)
#define chk_mul_overflow(lhs, rhs, result) \
    __builtin_mul_overflow(lhs, rhs, result)

#define chk_add_overflow_assert(lhs, rhs) ({ \
    __auto_type __result = lhs;                            \
    assert(!__builtin_add_overflow(lhs, rhs, &__result));  \
    __result;                                              \
})

#define chk_sub_overflow_assert(lhs, rhs) ({ \
    __auto_type __result = lhs;                            \
    assert(!__builtin_sub_overflow(lhs, rhs, &__result));  \
    __result;                                              \
})

#define chk_mul_overflow_assert(lhs, rhs) ({ \
    __auto_type __result = lhs;                            \
    assert(!__builtin_sub_overflow(lhs, rhs, &__result));  \
    __result;                                              \
})
