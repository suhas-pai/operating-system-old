/*
 * kernel/stdlib.c
 * © suhas pai
 */

#include <stdint.h>

#include "dev/printk.h"
#include "lib/macros.h"
#include "lib/string.h"

size_t strlen(const char *str) {
    size_t result = 0;

    const char *iter = str;
    for (char ch = *iter; ch != '\0'; ch = *(++iter), result++) {}

    return result;
}

size_t strnlen(const char *const str, const size_t limit) {
    size_t result = 0;

    const char *iter = str;
    char ch = *iter;

    for (size_t i = 0; i != limit && ch != '\0'; ch = *(++iter), i++) {
        result++;
    }

    return result;
}

int strcmp(const char *const str1, const char *const str2) {
    const char *iter = str1;
    const char *jter = str2;

    char ch = *iter, jch = *jter;
    for (; ch != '\0' && jch != '\0'; ch = *(++iter), jch = *(++jter)) {}

    return ch - jch;
}

int strncmp(const char *str1, const char *const str2, size_t length) {
    const char *iter = str1;
    const char *jter = str2;

    char ch = *iter, jch = *jter;
    for (size_t i = 0; i != length; i++) {
        if (ch == '\0' || jch == '\0') {
            break;
        }
    }

    return ch - jch;
}

char *strchr(const char *const str, const int ch) {
    c_string_foreach (str, iter) {
        if (*iter == ch) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-qual"
            return (char *)iter;
        #pragma GCC diagnostic pop
        }
    }

    return NULL;
}

// TODO: Fix
#define DECL_MEM_CMP_FUNC(type)                                                \
    static inline int                                                          \
    VAR_CONCAT(_memcmp_, type)(const void *left,                               \
                               const void *right,                              \
                               const uint64_t len)                             \
    {                                                                          \
        for (uint64_t i = 0;                                                   \
             i + sizeof(type) < len;                                           \
             i += sizeof(type), left += sizeof(type), right += sizeof(type))   \
        {                                                                      \
            const int left_ch = (int)(*(const type *)left);                    \
            const int right_ch = (int)(*(const type *)right);                  \
                                                                               \
            if (left_ch != right_ch) {                                         \
                return (left_ch - right_ch);                                   \
            }                                                                  \
        }                                                                      \
                                                                               \
        return 0;                                                              \
    }

#define DECL_MEM_COPY_FUNC(type) \
    static inline unsigned long  \
    VAR_CONCAT(_memcpy_, type)(void *const dst,            \
                               const void *src,            \
                               const unsigned long n,      \
                               void **const dst_out,       \
                               const void **const src_out) \
    {                                                                          \
        for (uint8_t off = 0; off < n; off += sizeof(type)) {                  \
            ((type *)dst)[off] = ((const type *)src)[off];                     \
        }                                                                      \
                                                                               \
        *dst_out = dst;                                                        \
        *src_out = src;                                                        \
                                                                               \
        return n;                                                              \
    }

DECL_MEM_CMP_FUNC(uint8_t)
DECL_MEM_CMP_FUNC(uint16_t)
DECL_MEM_CMP_FUNC(uint32_t)
DECL_MEM_CMP_FUNC(uint64_t)

DECL_MEM_COPY_FUNC(uint8_t)
DECL_MEM_COPY_FUNC(uint16_t)
DECL_MEM_COPY_FUNC(uint32_t)
DECL_MEM_COPY_FUNC(uint64_t)

int
memcmp(const void *left,
       const void *right,
       const size_t len)
{
    int res = _memcmp_uint64_t(left, right, len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint32_t(left, right, len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint16_t(left, right, len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint8_t(left, right, len);
    return res;
}

void *memcpy(void *dst, const void *src, unsigned long n) {
    void *ret = dst;
    if (n >= sizeof(uint64_t)) {
        n = _memcpy_uint64_t(dst, src, n, &dst, &src);
        n = _memcpy_uint32_t(dst, src, n, &dst, &src);
        n = _memcpy_uint16_t(dst, src, n, &dst, &src);

        _memcpy_uint8_t(dst, src, n, &dst, &src);
    } else if (n >= sizeof(uint32_t)) {
        n = _memcpy_uint32_t(dst, src, n, &dst, &src);
        n = _memcpy_uint16_t(dst, src, n, &dst, &src);

        _memcpy_uint8_t(dst, src, n, &dst, &src);
    } else if (n >= sizeof(uint16_t)) {
        n = _memcpy_uint16_t(dst, src, n, &dst, &src);
        if (n == 0) {
            return dst;
        }

        _memcpy_uint8_t(dst, src, n, &dst, &src);
    } else {
        _memcpy_uint8_t(dst, src, n, &dst, &src);
    }

    return ret;
}

#define DECL_MEM_COPY_BACK_FUNC(type) \
    static inline unsigned long  \
    VAR_CONCAT(_memcpy_bw_, type)(void *const dst, \
                                  const void *src, \
                                  const unsigned long n, \
                                  void **const dst_out, \
                                  const void **const src_out) \
    {                                                                          \
        for (int off = n - sizeof(type); off >= 0; off -= (int)sizeof(type)) { \
            ((type *)dst)[off] = ((const type *)src)[off];                     \
        }                                                                      \
                                                                               \
        *dst_out = dst;                                                        \
        *src_out = src;                                                        \
                                                                               \
        return n; \
    }

DECL_MEM_COPY_BACK_FUNC(uint8_t)
DECL_MEM_COPY_BACK_FUNC(uint16_t)
DECL_MEM_COPY_BACK_FUNC(uint32_t)
DECL_MEM_COPY_BACK_FUNC(uint64_t)

void *memmove(void *dst, const void *src, unsigned long n) {
    void *ret = dst;
    if (src > dst) {
        const uint64_t diff = (uint64_t)(src - dst);
        if (diff >= sizeof(uint64_t)) {
            memcpy(dst, src, n);
        } else if (diff >= sizeof(uint32_t)) {
            n = _memcpy_uint32_t(dst, src, n, &dst, &src);
            n = _memcpy_uint16_t(dst, src, n, &dst, &src);

            _memcpy_uint8_t(dst, src, n, &dst, &src);
        } else if (diff >= sizeof(uint16_t)) {
            n = _memcpy_uint16_t(dst, src, n, &dst, &src);
            _memcpy_uint8_t(dst, src, n, &dst, &src);
        } else if (diff >= sizeof(uint8_t)) {
            _memcpy_uint8_t(dst, src, n, &dst, &src);
        }
    } else {
        const uint64_t diff = (uint64_t)(dst - src);
        if (diff >= sizeof(uint64_t)) {
            n = _memcpy_bw_uint64_t(dst, src, n, &dst, &src);
            n = _memcpy_bw_uint32_t(dst, src, n, &dst, &src);
            n = _memcpy_bw_uint16_t(dst, src, n, &dst, &src);

            _memcpy_bw_uint8_t(dst, src, n, &dst, &src);
        } else if (diff >= sizeof(uint32_t)) {
            n = _memcpy_bw_uint32_t(dst, src, n, &dst, &src);
            n = _memcpy_bw_uint16_t(dst, src, n, &dst, &src);

            _memcpy_bw_uint8_t(dst, src, n, &dst, &src);
        } else if (diff >= sizeof(uint16_t)) {
            n = _memcpy_bw_uint16_t(dst, src, n, &dst, &src);
            _memcpy_bw_uint8_t(dst, src, n, &dst, &src);
        } else if (diff >= sizeof(uint8_t)) {
            _memcpy_bw_uint8_t(dst, src, n, &dst, &src);
        }
    }

    return ret;
}

void *memset(void *const dst, const int val, const unsigned long n) {
    for (unsigned long i = 0; i != n; i++) {
        ((uint8_t *)dst)[i] = (uint8_t)val;
    }

    return dst;
}

void *memzero(void *dst, unsigned long n) {
    void *const ret = dst;
    while (n >= sizeof(uint64_t)) {
        *(uint64_t *)dst = 0;

        dst += sizeof(uint64_t);
        n -= sizeof(uint64_t);
    }

    while (n >= sizeof(uint32_t)) {
        *(uint32_t *)dst = 0;

        dst += sizeof(uint32_t);
        n -= sizeof(uint32_t);
    }

    while (n >= sizeof(uint16_t)) {
        *(uint16_t *)dst = 0;

        dst += sizeof(uint16_t);
        n -= sizeof(uint16_t);
    }

    while (n >= sizeof(uint8_t)) {
        *(uint8_t *)dst = 0;

        dst += sizeof(uint8_t);
        n -= sizeof(uint8_t);
    }

    return ret;
}