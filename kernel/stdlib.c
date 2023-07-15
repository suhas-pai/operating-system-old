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
                               size_t len,                                     \
                               const void **const left_out,                    \
                               const void **const right_out,                   \
                               size_t *const len_out)                          \
    {                                                                          \
        if (len < sizeof(type)) {                                              \
            return 0;                                                          \
        }                                                                      \
                                                                               \
        do {                                                                   \
            const int left_ch = (int)(*(const type *)left);                    \
            const int right_ch = (int)(*(const type *)right);                  \
                                                                               \
            if (left_ch != right_ch) {                                         \
                return (left_ch - right_ch);                                   \
            }                                                                  \
                                                                               \
            left += sizeof(type);                                              \
            right += sizeof(type);                                             \
            len -= sizeof(type);                                               \
        } while (len >= sizeof(type));                                         \
                                                                               \
        *left_out = left;                                                      \
        *right_out = right;                                                    \
        *len_out = len;                                                        \
                                                                               \
        return 0;                                                              \
    }

DECL_MEM_CMP_FUNC(uint8_t)
DECL_MEM_CMP_FUNC(uint16_t)
DECL_MEM_CMP_FUNC(uint32_t)
DECL_MEM_CMP_FUNC(uint64_t)

#if !defined(__x86_64__)
    #define DECL_MEM_COPY_FUNC(type) \
        static inline unsigned long  \
        VAR_CONCAT(_memcpy_, type)(void *dst,                  \
                                   const void *src,            \
                                   unsigned long n,            \
                                   void **const dst_out,       \
                                   const void **const src_out) \
        {                                                                      \
            while (n >= sizeof(type)) {                                        \
                *(type *)dst = *(const type *)src;                             \
                                                                               \
                dst += sizeof(type);                                           \
                src += sizeof(type);                                           \
                                                                               \
                n -= sizeof(type);                                             \
            }                                                                  \
                                                                               \
            *dst_out = dst;                                                    \
            *src_out = src;                                                    \
                                                                               \
            return n;                                                          \
        }

    DECL_MEM_COPY_FUNC(uint8_t)
    DECL_MEM_COPY_FUNC(uint16_t)
    DECL_MEM_COPY_FUNC(uint32_t)
    DECL_MEM_COPY_FUNC(uint64_t)
#endif /* !defined(__x86_64__) */

__optimize(3)
int
memcmp(const void *left,
       const void *right,
       size_t len)
{
    int res = _memcmp_uint64_t(left, right, len, &left, &right, &len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint32_t(left, right, len, &left, &right, &len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint16_t(left, right, len, &left, &right, &len);
    if (res != 0) {
        return res;
    }

    res = _memcmp_uint8_t(left, right, len, &left, &right, &len);
    return res;
}

__optimize(3)
void *memcpy(void *dst, const void *src, unsigned long n) {
    void *ret = dst;

#if defined(__x86_64__)
    asm volatile ("rep movsb" :: "D"(dst), "S"(src), "c"(n) : "memory");
#else
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
#endif /* defined(__x86_64__) */

    return ret;
}

#if !defined(__x86_64__)
    #define DECL_MEM_COPY_BACK_FUNC(type) \
        static inline unsigned long  \
        VAR_CONCAT(_memcpy_bw_, type)(void *const dst, \
                                      const void *src, \
                                      const unsigned long n, \
                                      void **const dst_out, \
                                      const void **const src_out) \
        {                                                                      \
            for (int off = n - sizeof(type);                                   \
                 off >= 0;                                                     \
                 off -= (int)sizeof(type)) {                                   \
                ((type *)dst)[off] = ((const type *)src)[off];                 \
            }                                                                  \
                                                                               \
            *dst_out = dst;                                                    \
            *src_out = src;                                                    \
                                                                               \
            return n; \
        }

    DECL_MEM_COPY_BACK_FUNC(uint8_t)
    DECL_MEM_COPY_BACK_FUNC(uint16_t)
    DECL_MEM_COPY_BACK_FUNC(uint32_t)
    DECL_MEM_COPY_BACK_FUNC(uint64_t)
#endif

__optimize(3)
void *memmove(void *dst, const void *src, unsigned long n) {
    void *ret = dst;

#if defined(__x86_64__)
    if (src > dst) {
        memmove(dst, src, n);
    } else {
        void *dst_back = dst + (n - 1);
        const void *src_back = src + (n - 1);

        asm volatile ("std;"
                      "rep movsb;"
                      "cld"
                      :: "D"(dst_back), "S"(src_back), "c"(n) : "memory");
    }
#else
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
#endif /* defined(__x86_64__) */

    return ret;
}

__optimize(3)
void *memset(void *const dst, const int val, const unsigned long n) {
#if defined(__x86_64__)
    asm volatile ("rep stosb" :: "D"(dst), "al"(val), "c"(n) : "memory");
#else
    for (unsigned long i = 0; i != n; i++) {
        ((uint8_t *)dst)[i] = (uint8_t)val;
    }
#endif /* defined(__x86_64__) */

    return dst;
}
