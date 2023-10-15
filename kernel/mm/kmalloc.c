/*
 * kernel/mm/kmalloc.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/slab.h"

#include "kmalloc.h"

static struct slab_allocator kmalloc_slabs[16] = {0};
static bool kmalloc_is_initialized = false;

__optimize(3) bool kmalloc_initialized() {
    return kmalloc_is_initialized;
}

void kmalloc_init() {
    assert(slab_allocator_init(&kmalloc_slabs[0], 16, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[1], 32, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[2], 64, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[3], 96, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[4], 128, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[5], 192, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[6], 256, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[7], 384, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[8], 512, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[9], 768, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[10], 1024, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[11], 1536, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[12], 2048, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[13], 4096, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[14], 6102, /*alloc_flags=*/0));
    assert(slab_allocator_init(&kmalloc_slabs[15], 8192, /*alloc_flags=*/0));

    kmalloc_is_initialized = true;
}

__optimize(3) __malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc(const uint32_t size) {
    assert_msg(__builtin_expect(kmalloc_is_initialized, 1),
               "mm: kmalloc() called before kmalloc_init()");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: kmalloc() got size=0\n");
        return NULL;
    }

    if (__builtin_expect(size > KMALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: kmalloc() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               KMALLOC_MAX);
        return NULL;
    }

    struct slab_allocator *allocator = &kmalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    return slab_alloc(allocator);
}

__optimize(3) __malloclike __malloc_dealloc(kfree, 1) __alloc_size(1)
void *kmalloc_size(const uint32_t size, uint32_t *const size_out) {
    assert_msg(__builtin_expect(kmalloc_is_initialized, 1),
               "mm: kmalloc_size() called before kmalloc_init()");

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN, "mm: kmalloc_size() got size=0\n");
        return NULL;
    }

    if (__builtin_expect(size > KMALLOC_MAX, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: kmalloc_size() can't allocate %" PRIu32 " bytes, max is %d "
               "bytes\n",
               size,
               KMALLOC_MAX);
        return NULL;
    }

    struct slab_allocator *allocator = &kmalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    void *const result = slab_alloc(allocator);
    if (__builtin_expect(result != NULL, 1)) {
        *size_out = allocator->object_size;
        return result;
    }

    return NULL;
}

__optimize(3) void *krealloc(void *const buffer, const uint32_t size) {
    assert_msg(__builtin_expect(kmalloc_is_initialized, 1),
               "mm: krealloc() called before kmalloc_init()");

    // Allow buffer=NULL to call kmalloc().
    if (__builtin_expect(buffer == NULL, 0)) {
        if (__builtin_expect(size == 0, 0)) {
            return NULL;
        }

        return kmalloc(size);
    }

    if (__builtin_expect(size == 0, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: krealloc(): got size=0, use kfree() instead\n");

        kfree(buffer);
        return NULL;
    }

    const uint64_t buffer_size = slab_object_size(buffer);
    if (size <= buffer_size) {
        return buffer;
    }

    void *const ret = kmalloc(size);
    if (__builtin_expect(ret == NULL, 0)) {
        return NULL;
    }

    memcpy(ret, buffer, buffer_size);
    kfree(buffer);

    return ret;
}

__optimize(3) void kfree(void *const buffer) {
    assert_msg(__builtin_expect(kmalloc_is_initialized, 1),
               "mm: kfree() called before kmalloc_init()");
    slab_free(buffer);
}