/*
 * lib/overflow.h
 * © suhas pai
 */

#pragma once

#define ck_add_overflow(lhs, rhs, result) \
    __builtin_add_overflow(lhs, rhs, result)
#define ck_sub_overflow(lhs, rhs, result) \
    __builtin_sub_overflow(lhs, rhs, result)
#define ck_mul_overflow(lhs, rhs, result) \
    __builtin_mul_overflow(lhs, rhs, result)