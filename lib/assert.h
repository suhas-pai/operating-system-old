/*
 * lib/assert.h
 * © suhas pai
 */

#pragma once
#include "macros.h"

#if defined(BUILD_KERNEL)
    #include "kernel/cpu/panic.h"

    #define assert(cond) if (!(cond)) panic(TO_STRING(cond) "\n")
    #define assert_msg(cond, msg, ...) \
        if (!(cond)) panic(msg "\n", ##__VA_ARGS__)

    #define verify_not_reached() panic("verify_not_reached()\n")
#elif defined(BUILD_TEST)
    #include <assert.h>

    #define assert_msg(cond, msg, ...) assert(cond && (msg))
    #define verify_not_reached() assert(0 && "verify_not_reached()")
#else
    #include <assert.h>
#endif

#if !defined(BUILD_KERNEL)
    #define panic(msg, ...) assert(0 && (msg))
#endif /* !defined(BUILD_KERNEL) */
