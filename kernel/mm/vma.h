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

enum prot_flags {
    PROT_READ = 1 << 0,
    PROT_WRITE = 1 << 1,
    PROT_EXEC = 1 << 2
};

enum mmap_flags {
    MMAP_NO_CACHE = 1 << 0,
};

struct vm_area *
mmap(struct pagemap *pagemap, uint64_t phys_addr, uint8_t prot, uint8_t flags);