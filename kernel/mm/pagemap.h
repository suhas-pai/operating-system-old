/*
 * kernel/mm/pagemap.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "lib/adt/avltree.h"

#include "lib/list.h"
#include "lib/refcount.h"

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