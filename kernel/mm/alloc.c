/*
 * kernel/mm/alloc.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/assert.h"

#include "alloc.h"
#include "slab.h"

static struct slab_allocator alloc_slabs[2] = {0};
static bool alloc_is_initialized = false;

void mm_alloc_init() {
    assert(slab_allocator_init(alloc_slabs + 0, 16, /*alloc_flags=*/0));
    assert(slab_allocator_init(alloc_slabs + 1, 32, /*alloc_flags=*/0));

    alloc_is_initialized = true;
}

void *mm_alloc(uint64_t size) {
    assert_msg(__builtin_expect(alloc_is_initialized, 1),
               "mm: mm_alloc() called before init");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm_alloc(): got size=0\n");
        return NULL;
    }

    if (__builtin_expect(size > 32, 0)) {
        printk(LOGLEVEL_WARN,
               "mm_alloc(): Can't allocate %" PRIu64 " bytes, max is 2048 "
               "bytes\n",
               size);
        return NULL;
    }

    struct slab_allocator *allocator = alloc_slabs + 0;
    while (allocator->object_size < size) {
        allocator++;
    }

    return slab_alloc(allocator);
}

__optimize(3) void *mm_realloc(void *const buffer, const uint64_t size) {
    assert_msg(__builtin_expect(alloc_is_initialized, 1),
               "mm: mm_realloc() called before mm_alloc_init()");

    // Allow buffer=NULL to call mm_alloc().
    if (__builtin_expect(buffer == NULL, 0)) {
        if (__builtin_expect(size == 0, 0)) {
            return NULL;
        }

        return mm_alloc(size);
    }

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: mm_realloc(): got size=0, use mm_alloc_free() instead\n");

        mm_alloc_free(buffer);
        return NULL;
    }

    const uint64_t buffer_size = slab_object_size(buffer);
    if (size <= buffer_size) {
        return buffer;
    }

    void *const ret = mm_alloc(size);
    if (__builtin_expect(ret == NULL, 0)) {
        return NULL;
    }

    memcpy(ret, buffer, buffer_size);
    mm_alloc_free(buffer);

    return ret;
}

__optimize(3) void mm_alloc_free(void *const buffer) {
    assert_msg(__builtin_expect(alloc_is_initialized, 1),
               "mm: mm_alloc_free() called before mm_alloc_init()");

    slab_free(buffer);
}