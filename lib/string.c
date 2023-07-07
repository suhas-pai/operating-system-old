/*
 * lib/string.c
 * © suhas pai
 */

#include <stdint.h>
#include "string.h"

#if !defined(BUILD_TEST) && !defined(BUILD_KERNEL)
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

    for (size_t i = 0; i != limit && ch != '\0'; ch = *(++iter), result++) {}
    return result;
}

int strcmp(const char *const str1, const char *const str2) {
    const char *iter = str1;
    const char *jter = str2;

    char ch = *iter, jch = *jter;
    for (; ch != '\0' && jch != '\0'; ch = *(++iter), jch = *(++jter)) {}

    return ch - jch;
}

int strncmp(const char *str1, const char *const str2, const size_t length) {
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

#endif /* !defined(BUILD_TEST) */

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