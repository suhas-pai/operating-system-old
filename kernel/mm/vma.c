/*
 * kernel/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"
#include "mm/kmalloc.h"

#include "pagemap.h"

struct vm_area *
vma_prev(struct pagemap *const pagemap, struct vm_area *const vma) {
    if (vma->vma_list.prev == &pagemap->vma_list) {
        return NULL;
    }

    return container_of(vma->vma_list.prev, struct vm_area, vma_list);
}

struct vm_area *
vma_next(struct pagemap *const pagemap, struct vm_area *const vma) {
    if (vma->vma_list.prev == &pagemap->vma_list) {
        return NULL;
    }

    return container_of(vma->vma_list.next, struct vm_area, vma_list);
}

struct vm_area *
vma_alloc(struct pagemap *const pagemap,
          const struct range range,
          const uint8_t prot,
          const enum vma_cachekind cachekind)
{
    struct vm_area *const vma = kmalloc(sizeof(struct vm_area));
    if (vma == NULL) {
        panic("vma_create(): failed to allocate memory\n");
    }

    vma->range = range;
    vma->pagemap = pagemap;
    vma->cachekind = cachekind;
    vma->prot = prot;

    return vma;
}

struct vm_area *
vma_create(struct pagemap *const pagemap,
           const struct range in_range,
           const uint64_t phys,
           const uint64_t align,
           const uint8_t prot,
           const enum vma_cachekind cachekind)
{
    assert(has_align(phys, PAGE_SIZE));

    struct vm_area *const vma = vma_alloc(pagemap, in_range, prot, cachekind);
    if (vma == NULL) {
        panic("vma_create(): failed to allocate memory\n");
    }

    if (!pagemap_find_space_and_add_vma(pagemap, vma, in_range, phys, align)) {
        kfree(vma);
        return NULL;
    }

    return vma;
}

struct vm_area *
vma_create_at(struct pagemap *const pagemap,
              const struct range range,
              const uint64_t phys_addr,
              const uint64_t align,
              const uint8_t prot,
              const enum vma_cachekind cachekind)
{
    struct vm_area *const vma = vma_alloc(pagemap, range, prot, cachekind);
    if (vma == NULL) {
        panic("vma_create(): failed to allocate memory\n");
    }

    if (!pagemap_add_vma_at(pagemap, vma, range, phys_addr, align)) {
        kfree(vma);
        return NULL;
    }

    return vma;
}