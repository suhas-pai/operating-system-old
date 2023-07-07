/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once

#define chk_add_overflow(lhs, rhs, result) \
    __builtin_add_overflow(lhs, rhs, result)
#define chk_sub_overflow(lhs, rhs, result) \
    __builtin_sub_overflow(lhs, rhs, result)
#define chk_mul_overflow(lhs, rhs, result) \
    __builtin_mul_overflow(lhs, rhs, result)
