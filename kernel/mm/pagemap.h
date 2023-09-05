/*
 * kernel/mm/pagemap.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/addrspace.h"

#include "page.h"
#include "vma.h"

struct pagemap {
#if defined(__aarch64__)
    struct page *lower_root;
    struct page *higher_root;
#else
    struct page *root;
#endif /* defined(__aarch64__) */

    struct list cpu_list;
    struct spinlock cpu_lock;

    struct refcount refcount;

    struct address_space addrspace;
    struct spinlock addrspace_lock;
};

#if defined(__aarch64__)
    struct pagemap
    pagemap_create(struct page *lower_root, struct page *higher_root);
#else
    struct pagemap pagemap_create(struct page *root);
#endif

extern struct pagemap kernel_pagemap;

bool
pagemap_find_space_and_add_vma(struct pagemap *pagemap,
                               struct vm_area *vma,
                               struct range in_range,
                               uint64_t phys_addr,
                               uint64_t align);

bool
pagemap_add_vma_at(struct pagemap *pagemap,
                   struct vm_area *vma,
                   struct range in_range,
                   uint64_t phys_addr,
                   uint64_t align);

void switch_to_pagemap(struct pagemap *pagemap);