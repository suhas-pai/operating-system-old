/*
 * lib/macros.h
 * © suhas pai
 */

#pragma once
#include <stddef.h>

#if !defined(__unused)
    #define __unused __attribute__((unused))
#endif /* !defined(__unused )*/

#if !defined(__noreturn)
    #define __noreturn __attribute__((noreturn))
#endif /* !defined(__noreturn )*/

#define __packed __attribute__((packed))

#if !defined(__printf_format)
    #define __printf_format(last_arg_idx, list_idx) \
        __attribute__((format(printf, last_arg_idx, list_idx)))
#endif /* !defined(__printf_format) */

#define LEN_OF(str) (sizeof(str) - 1)

#define __VAR_CONCAT_IMPL(a, b) a##b
#define VAR_CONCAT(a, b) __VAR_CONCAT_IMPL(a, b)

#define __VAR_CONCAT_3_IMPL(a, b, c) a##b##c
#define VAR_CONCAT_3(a, b, c) __VAR_CONCAT_3_IMPL(a, b, c)

#define __TO_STRING_IMPL(x) #x
#define TO_STRING(x) __TO_STRING_IMPL(x)

#define h_var(token) VAR_CONCAT(VAR_CONCAT_3(__, token, __), __LINE__)
#define countof(carr) (sizeof(carr) / sizeof(carr[0]))
#define for_each_in_carr(arr, name) \
    for (typeof(&arr[0]) name = &arr[0]; name != (arr + countof(arr)); name++)

#define swap(a, b) ({      \
    __auto_type __tmp = b; \
    b = a;                 \
    a = __tmp;             \
})

#define max(a, b) ({       \
    __auto_type __a = (a); \
    __auto_type __b = (b); \
    __a > __b ? __a : __b; \
})

#define min(a, b) ({       \
    __auto_type __a = (a); \
    __auto_type __b = (b); \
    __a < __b ? __a : __b; \
})

#define get_to_within_size(x, size) ((x) % (size))
#define distance(begin, end) ((uint64_t)(end) - (uint64_t)(begin))
#define distance_inclusive(begin, end) distance(begin, end) + 1

#define RAND_VAR_NAME() VAR_CONCAT(__random__, __LINE__)
#define sizeof_bits(n) (sizeof(n) * 8)