/*
 * kernel/mm/kmalloc.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/string.h"
#include "mm/slab.h"

#include "kmalloc.h"

static struct slab_allocator kmalloc_slabs[13] = {0};

void kmalloc_init() {
    slab_allocator_init(&kmalloc_slabs[0], 16, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[1], 32, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[2], 64, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[3], 96, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[4], 128, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[5], 192, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[6], 256, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[7], 384, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[8], 512, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[9], 768, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[10], 1024, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[11], 1536, /*alloc_flags=*/0);
    slab_allocator_init(&kmalloc_slabs[12], 2048, /*alloc_flags=*/0);
}

void *kmalloc(const uint64_t size) {
    if (size == 0) {
        printk(LOGLEVEL_WARN, "kmalloc(): got size=0\n");
        return NULL;
    }

    if (size > 2048) {
        printk(LOGLEVEL_WARN,
               "kmalloc(): Can't allocate %" PRIu64 " bytes, max is 2048 "
               "bytes\n",
               size);
        return NULL;
    }

    struct slab_allocator *allocator = &kmalloc_slabs[0];
    while (allocator->object_size < size) {
        allocator++;
    }

    return slab_alloc(allocator);
}

void *krealloc(void *const buffer, const uint64_t size) {
    // Allow buffer=NULL to call kmalloc().

    if (size == 0) {
        printk(LOGLEVEL_WARN, "krealloc(): got size=0, use kfree() instead\n");
        kfree(buffer);

        return NULL;
    }

    const uint64_t buffer_size = slab_object_size(buffer);
    if (size <= buffer_size) {
        return buffer;
    }

    void *const ret = kmalloc(size);
    if (ret == NULL) {
        return NULL;
    }

    memcpy(ret, buffer, buffer_size);
    kfree(buffer);

    return ret;
}

void kfree(void *const buffer) {
    slab_free(buffer);
}