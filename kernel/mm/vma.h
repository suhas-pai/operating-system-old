/*
 * kernel/mm/vma.h
 * © suhas pai
 */

#pragma once
#include "cpu/spinlock.h"

#include "lib/adt/addrspace.h"
#include "mm/walker.h"

#include "mm_types.h"

struct pagemap;
struct vm_area {
    struct addrspace_node node;

    // This lock guards the physical page tables held by this vm_area.
    struct spinlock lock;
    uint8_t prot;

    enum vma_cachekind cachekind;
};

#define vma_of(obj) container_of((obj), struct vm_area, node.avlnode)

struct vm_area *vma_prev(struct vm_area *const vma);
struct vm_area *vma_next(struct vm_area *const vma);

struct vm_area *
vma_alloc(struct pagemap *pagemap,
          struct range range,
          uint8_t prot,
          enum vma_cachekind cachekind);

// Whereas alloc will only allocate a `vm_area`, create will allocate and add to
// the tree.

struct vm_area *
vma_create(struct pagemap *pagemap,
           struct range in_range,
           uint64_t virt_addr,
           uint64_t size,
           uint64_t align,
           uint8_t prot,
           enum vma_cachekind cachekind);

struct vm_area *
vma_create_at(struct pagemap *pagemap,
              struct range range,
              uint64_t virt_addr,
              uint64_t align,
              uint8_t prot,
              enum vma_cachekind cachekind);

struct pagemap *vma_pagemap(struct vm_area *const vma);

extern bool
arch_make_mapping(struct pagemap *pagemap,
                  uint64_t phys_addr,
                  uint64_t virt_addr,
                  uint64_t size,
                  uint8_t prot,
                  enum vma_cachekind cachekind,
                  bool is_overwrite);

bool
arch_unmap_mapping(struct pagemap *pagemap,
                   uint64_t virt_addr,
                   uint64_t size);