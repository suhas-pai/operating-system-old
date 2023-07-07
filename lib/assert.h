/*
 * lib/assert.h
 */

#pragma once
#include "macros.h"

#if defined(BUILD_KERNEL)
	#include "kernel/cpu/panic.h"
	#define assert(cond) if (!(cond)) panic(TO_STRING(cond) "\n")
	#define assert_msg(cond, msg) if (!(cond)) panic(msg "\n")
#elif !defined(BUILD_TEST)
    #define assert(cond) (void)(cond) // TODO:
#else
    #include <assert.h>
#endif

#if defined(BUILD_KERNEL)
	#include "kernel/cpu/panic.h"
	#define verify_not_reached(msg) panic(msg "\n")
#else
	#define verify_not_reached(msg) assert(0 && (msg))
#endif
