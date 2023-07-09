/*
 * lib/size.h
 * © suhas pai
 */

#pragma once

#include "assert.h"
#include "overflow.h"

#define kib(amount) chk_mul_overflow_assert(1ull << 10, (amount))
#define mib(amount) chk_mul_overflow_assert(1ull << 20, (amount))
#define gib(amount) chk_mul_overflow_assert(1ull << 30, (amount))
#define tib(amount) chk_mul_overflow_assert(1ull << 40, (amount))