/*
 * adt/slab.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "lib/list.h"
#include "lib/macros.h"

// Structure to represent a slab allocator.
struct slab_allocator {
    // List of struct page used as slabs.
    struct list slab_head_list;
    struct spinlock lock;

    // Statistics about this allocator. These are constant and can be read w/o
    // the lock.
    uint32_t object_size;
    uint32_t object_count_per_slab;
    uint16_t slab_order;
    uint32_t alloc_flags;

    uint32_t free_obj_count;
    uint32_t slab_count;
};

bool
slab_allocator_init(struct slab_allocator *allocator,
                    uint32_t object_size,
                    uint32_t alloc_flags);

void slab_free(void *buffer);

__malloclike __malloc_dealloc(slab_free, 1)
void *slab_alloc(struct slab_allocator *allocator);

uint32_t slab_object_size(void *mem);