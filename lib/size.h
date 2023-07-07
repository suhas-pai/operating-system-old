/*
 * lib/size.h
 * © suhas pai
 */

#pragma once

#include "assert.h"
#include "overflow.h"

#define kib(amount) ({ \
	uint64_t _ret_ = 0; \
	assert(!chk_mul_overflow(1ull << 10, (amount), &_ret_)); \
	_ret_;\
})

#define mib(amount) ({ \
	uint64_t _ret_ = 0; \
	assert(!chk_mul_overflow(1ull << 20, (amount), &_ret_)); \
	_ret_;\
})

#define gib(amount) ({ \
	uint64_t _ret_ = 0; \
	assert(!chk_mul_overflow(1ull << 30, (amount), &_ret_)); \
	_ret_;\
})

#define tib(amount) ({ \
	uint64_t _ret_ = 0; \
	assert(!chk_mul_overflow(1ull << 40, (amount), &_ret_)); \
	_ret_;\
})
