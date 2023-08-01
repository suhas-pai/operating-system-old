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

#if !defined(__packed)
    #define __packed __attribute__((packed))
#endif /* !defined(__packed) */

#if !defined(__printf_format)
    #define __printf_format(last_arg_idx, list_idx) \
        __attribute__((format(printf, last_arg_idx, list_idx)))
#endif /* !defined(__printf_format) */

#if !defined(__optimize)
    #define __optimize(n) __attribute__((optimize(n)))
#endif /* !defined(__optimize) */

#define sizeof_field(type, field) sizeof(((type *)0)->field)
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

#define reg_to_ptr(type, base, reg) ((type *)((uint64_t)(base) + (reg)))
#define field_to_ptr(type, base, field) \
    reg_to_ptr(type, base, offsetof(type, field))
#define get_to_within_size(x, size) ((x) % (size))

#define distance(begin, end) ((uint64_t)(end) - (uint64_t)(begin))
#define distance_incl(begin, end) (distance(begin, end) + 1)

#define RAND_VAR_NAME() VAR_CONCAT(__random__, __LINE__)

#define bits_to_bytes_roundup(bits) \
    (((bits) % sizeof_bits(uint8_t)) ? \
        (((bits) / sizeof_bits(uint8_t)) + 1) : ((bits) / sizeof_bits(uint8_t)))

#define bits_to_bytes_noround(bits) ((bits) / sizeof_bits(uint8_t))
#define bytes_to_bits(bits) ((bits) * 8)

#define sizeof_bits(n) bytes_to_bits(sizeof(n))
#define mask_for_n_bits(n) ((1ull << (n)) - 1)
#define set_bits_for_mask(ptr, mask, value) ({ \
    if (value) {                         \
        *(ptr) |= (mask);                \
    } else {                             \
        *(ptr) &= (typeof(mask))~(mask); \
    }                                    \
})
