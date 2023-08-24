/*
 * kernel/mm/pagemap.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#elif defined(__aarch64__)
    #include "asm/ttbr.h"
#endif /* defined(__x86_64__) */

#include "lib/align.h"
#include "lib/overflow.h"

#include "pagemap.h"

struct pagemap kernel_pagemap = {
#if defined(__aarch64__)
    .lower_root = NULL, // setup later
    .higher_root = NULL, // setup later
#else
    .root = NULL, // setup later
#endif /* defined(__aarch64__)*/

    .lock = SPINLOCK_INIT(),
    .refcount = refcount_create_max(),
    .addrspace = ADDRSPACE_INIT(kernel_pagemap.addrspace),
};

#if defined(__aarch64__)
    struct pagemap
    pagemap_create(struct page *const lower_root,
                   struct page *const higher_root)
    {
        struct pagemap result = {
            .lower_root = lower_root,
            .higher_root = higher_root
        };

        addrspace_init(&result.addrspace);
        refcount_init(&result.refcount);

        return result;
    }
#else
    struct pagemap pagemap_create(struct page *const root) {
        struct pagemap result = {
            .root = root,
        };

        addrspace_init(&result.addrspace);
        refcount_init(&result.refcount);

        return result;
    }
#endif /* defined(__aarch64__) */

bool
pagemap_find_space_and_add_vma(struct pagemap *const pagemap,
                               struct vm_area *const vma,
                               const struct range in_range,
                               const uint64_t phys_addr,
                               const uint64_t align)
{
    const int flag = spin_acquire_with_irq(&pagemap->lock);
    const uint64_t addr =
        addrspace_find_space_for_node(&pagemap->addrspace,
                                      in_range,
                                      &vma->node,
                                      align);

    if (addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_with_irq(&pagemap->lock, flag);
        return false;
    }

    if (vma->prot != PROT_NONE) {
        const bool map_result =
            arch_make_mapping(pagemap,
                              phys_addr,
                              vma->node.range.front,
                              vma->node.range.size,
                              vma->prot,
                              vma->cachekind,
                              /*is_overwrite=*/false);

        spin_release_with_irq(&pagemap->lock, flag);
        if (!map_result) {
            return false;
        }
    } else {
        spin_release_with_irq(&pagemap->lock, flag);
    }

    return true;
}

bool
pagemap_add_vma_at(struct pagemap *const pagemap,
                   struct vm_area *const vma,
                   const struct range in_range,
                   const uint64_t phys_addr,
                   const uint64_t align)
{
    const int flag = spin_acquire_with_irq(&pagemap->lock);

    // FIXME: check for overlaps
    addrspace_add_node(&pagemap->addrspace, &vma->node);

    if (vma->prot != PROT_NONE) {
        const bool map_result =
            arch_make_mapping(pagemap,
                              phys_addr,
                              vma->node.range.front,
                              vma->node.range.size,
                              vma->prot,
                              vma->cachekind,
                              /*is_overwrite=*/false);

        spin_release_with_irq(&pagemap->lock, flag);
        if (!map_result) {
            return false;
        }
    } else {
        spin_release_with_irq(&pagemap->lock, flag);
    }

    (void)in_range;
    (void)align;

    return true;
}

void switch_to_pagemap(const struct pagemap *const pagemap) {
#if defined(__aarch64__)
    assert(pagemap->lower_root != NULL);
    assert(pagemap->higher_root != NULL);
#else
    assert(pagemap->root != NULL);
#endif /* defined(__aarch64__) */

#if defined(__x86_64__)
    write_cr3(page_to_phys(pagemap->root));
#elif defined(__aarch64__)
    write_ttbr0_el1(page_to_phys(pagemap->lower_root));
    write_ttbr1_el1(page_to_phys(pagemap->higher_root));

    asm volatile ("dsb sy; isb" ::: "memory");
#else
    const uint64_t satp =
        (PAGING_MODE + 8) << 60 | (page_to_phys(pagemap->root) >> PML1_SHIFT);

    asm volatile ("csrw satp, %0; sfence.vma" :: "r"(satp) : "memory");
#endif /* defined(__x86_64__) */
}