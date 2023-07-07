/*
 * kernel/mm/vma.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"

#include "lib/adt/avltree.h"
#include "lib/adt/range.h"

#include "lib/list.h"

struct pagemap;
struct vm_area {
    struct range range;
    struct pagemap *owner;

    // This lock guards the physical page tables held by this vm_area.
    struct spinlock lock;

    struct avlnode vma_node;
    struct list vma_list;

    uint32_t pg_flags;
};

enum pg_flags {
    PG_READABLE = 1 << 0,
    PG_WRITABLE = 1 << 1,
    PG_EXEC = 1 << 2
};