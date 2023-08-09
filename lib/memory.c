/*
 * lib/memory.c
 * © suhas pai
 */

#include "lib/macros.h"
#include "memory.h"

__optimize(3) uint16_t *
memset_16(uint16_t *const buf, const uint64_t count, const uint16_t c) {
#if defined(__x86_64__)
    asm volatile ("rep stosw" :: "D"(buf), "al"(c), "c"(count) : "memory");
#else
    const uint16_t *const end = buf + count;
    for (uint16_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
#endif /* defined(__x86_64__) */

    return buf;
}

__optimize(3) uint32_t *
memset_32(uint32_t *const buf, const uint64_t count, const uint32_t c) {
#if defined(__x86_64__)
    asm volatile ("rep stosl" :: "D"(buf), "al"(c), "c"(count) : "memory");
#else
    const uint32_t *const end = buf + count;
    for (uint32_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
#endif /* defined(__x86_64__) */

    return buf;
}

__optimize(3) uint64_t *
memset_64(uint64_t *const buf, const uint64_t count, const uint64_t c) {
#if defined(__x86_64__)
    asm volatile ("rep stosq" :: "D"(buf), "al"(c), "c"(count) : "memory");
#else
    const uint64_t *const end = buf + count;
    for (uint64_t *iter = buf; iter != end; iter++) {
        *iter = c;
    }
#endif /* defined(__x86_64__) */

    return buf;
}

__optimize(3) bool
membuf_8_is_all(uint8_t *const buf, const uint64_t count, const uint8_t c) {
    const uint8_t *const end = buf + count;
    for (uint8_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_16_is_all(uint16_t *const buf, const uint64_t count, const uint16_t c) {
    const uint16_t *const end = buf + count;
    for (uint16_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_32_is_all(uint32_t *const buf, const uint64_t count, const uint32_t c) {
    const uint32_t *const end = buf + count;
    for (uint32_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}

__optimize(3) bool
membuf_64_is_all(uint64_t *const buf, const uint64_t count, const uint64_t c) {
    const uint64_t *const end = buf + count;
    for (uint64_t *iter = buf; iter != end; iter++) {
        if (*iter != c) {
            return false;
        }
    }

    return true;
}