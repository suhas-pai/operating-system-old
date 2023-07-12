/*
 * kernel/mm/pagemap.h
 * © suhas pai
 */

#pragma once

#include "page.h"
#include "vma.h"

struct pagemap {
    struct page *root;
    struct spinlock lock;
    struct refcount refcount;

    struct avltree vma_tree;
    struct list vma_list;
};

struct pagemap pagemap_create(struct page *root);
extern struct pagemap kernel_pagemap;

bool
pagemap_add_vma(struct pagemap *pagemap,
                struct vm_area *vma,
                struct range in_range,
                uint64_t phys_addr,
                uint64_t align);

void switch_to_pagemap(const struct pagemap *pagemap);