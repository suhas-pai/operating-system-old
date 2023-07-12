/*
 * kernel/mm/vma.h
 * © suhas pai
 */

#pragma once
#include "cpu/spinlock.h"

#include "lib/adt/avltree.h"
#include "mm/walker.h"

#include "types.h"

struct pagemap;
struct vm_area {
    struct range range;
    struct pagemap *pagemap;

    // This lock guards the physical page tables held by this vm_area.
    struct spinlock lock;

    struct avlnode vma_node;
    struct list vma_list;

    uint8_t prot;
    enum vma_cachekind cachekind;

    // Helper variable used internally to quickly find a free area range.
    // Defined as the maximum of the left subtree, right subtree, and the
    // distance between `range.front` and the previos vm_area's end
    uint64_t largest_free_to_prev;
};

#define vma_of(obj) container_of((obj), struct vm_area, vma_node)

struct vm_area *
vma_prev(struct pagemap *const pagemap, struct vm_area *const vma);

struct vm_area *
vma_next(struct pagemap *const pagemap, struct vm_area *const vma);

struct vm_area *
vma_create(struct pagemap *pagemap,
           struct range in_range,
           uint64_t phys_addr,
           uint64_t align,
           uint8_t prot,
           enum vma_cachekind cachekind);

bool
arch_make_mapping(struct pagemap *pagemap,
                  uint64_t phys_addr,
                  uint64_t virt_addr,
                  uint64_t size,
                  uint8_t prot,
                  enum vma_cachekind cachekind,
                  bool needs_flush);