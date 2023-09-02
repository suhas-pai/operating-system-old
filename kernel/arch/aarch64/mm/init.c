/*
 * kernel/arch/aarch64/mm/init.c
 * © suhas pai
 */

#include "asm/mair.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "mm/early.h"
#include "mm/pgmap.h"

#include "boot.h"

static struct page *
ptwalker_alloc_pgtable_cb(struct pt_walker *const walker, void *const cb_info) {
    (void)walker;
    (void)cb_info;

    // We don't have a structpage-table setup yet when this function is called,
    // but because ptwalker never dereferences the page, we can return a pointer
    // to where the page would've been.

    const uint64_t phys = early_alloc_page();
    if (phys != INVALID_PHYS) {
        return phys_to_page(phys);
    }

    panic("mm: failed to setup page-structs, ran out of memory\n");
}

static void
map_region(uint64_t virt_addr, uint64_t map_size, const uint64_t pte_flags) {
    enum pt_walker_result walker_result = E_PT_WALKER_OK;
    struct pt_walker pt_walker;

    ptwalker_create_for_pagemap(&pt_walker,
                                &kernel_pagemap,
                                virt_addr,
                                /*alloc_pgtable=*/ptwalker_alloc_pgtable_cb,
                                /*free_pgtable=*/NULL);

    if (map_size >= PAGE_SIZE_1GIB && has_align(virt_addr, PAGE_SIZE_1GIB)) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/3,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
        panic:
            panic("mm: failed to setup page-structs, ran out of memory\n");
        }

        do {
            const uint64_t page =
                early_alloc_large_page(/*order=*/PGT_COUNT * PGT_COUNT);

            if (page == INVALID_PHYS) {
                // We failed to alloc a 1gib page, so try 2mib pages next.
                break;
            }

            pt_walker.tables[2][pt_walker.indices[2]] =
                phys_create_pte(page) | PTE_LARGE_FLAGS(3) | __PTE_INNER_SH |
                pte_flags;

            walker_result =
                ptwalker_next_with_options(&pt_walker,
                                           /*level=*/3,
                                           /*alloc_parents=*/true,
                                           /*alloc_level=*/true,
                                           /*should_ref=*/false,
                                           /*alloc_pgtable_cb_info=*/NULL,
                                           /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE_1GIB;
            if (map_size < PAGE_SIZE_1GIB) {
                break;
            }

            virt_addr += PAGE_SIZE_1GIB;
        } while (true);
    }

    if (map_size >= PAGE_SIZE_2MIB && has_align(virt_addr, PAGE_SIZE_2MIB)) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/2,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        do {
            const uint64_t page = early_alloc_large_page(/*order=*/PGT_COUNT);
            if (page == INVALID_PHYS) {
                // We failed to alloc a 2mib page, so fill with 4kib pages
                // instead.
                break;
            }

            pt_walker.tables[1][pt_walker.indices[1]] =
                phys_create_pte(page) | PTE_LARGE_FLAGS(2) | __PTE_INNER_SH |
                pte_flags;

            walker_result =
                ptwalker_next_with_options(&pt_walker,
                                           /*level=*/2,
                                           /*alloc_parents=*/true,
                                           /*alloc_level=*/true,
                                           /*should_ref=*/false,
                                           /*alloc_pgtable_cb_info=*/NULL,
                                           /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE_2MIB;
            if (map_size < PAGE_SIZE_2MIB) {
                break;
            }

            virt_addr += PAGE_SIZE_2MIB;
        } while (true);
    }

    if (map_size >= PAGE_SIZE) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/1,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        do {
            const uint64_t page = early_alloc_page();
            if (page == INVALID_PHYS) {
                goto panic;
            }

            pt_walker.tables[0][pt_walker.indices[0]] =
                phys_create_pte(page) | PTE_LEAF_FLAGS | __PTE_INNER_SH |
                pte_flags;

            walker_result =
                ptwalker_next_with_options(&pt_walker,
                                           /*level=*/1,
                                           /*alloc_parents=*/true,
                                           /*alloc_level=*/true,
                                           /*should_ref=*/false,
                                           /*alloc_pgtable_cb_info=*/NULL,
                                           /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE;
            if (map_size == 0) {
                break;
            }

            virt_addr += PAGE_SIZE;
        } while (true);
    }
}

static void setup_pagestructs_table(const uint64_t byte_count) {
    const uint64_t structpage_count = PAGE_COUNT(byte_count);
    uint64_t map_size = structpage_count * SIZEOF_STRUCTPAGE;

    if (!align_up(map_size, PAGE_SIZE, &map_size)) {
        panic("mm: failed to initialize memory, overflow error when aligning "
              "structpage-table size to PAGE_SIZE");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    // Map struct page table
    const uint64_t pte_flags = __PTE_PXN | __PTE_UXN;
    map_region(PAGE_OFFSET, map_size, pte_flags);
}

static void
map_into_kernel_pagemap(const uint64_t phys_addr,
                        const uint64_t virt_addr,
                        const uint64_t size,
                        const uint64_t pte_flags)
{
    const struct pgmap_options options = {
        .pte_flags = __PTE_INNER_SH | __PTE_ACCESS | pte_flags,
        .is_in_early = true,
        .is_overwrite = false,
        .supports_largepage_at_level_mask = 1 << 3 | 1 << 2,
        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL
    };

    const struct range phys_range =
        range_create(phys_addr, align_up_assert(size, PAGE_SIZE));
    const bool pgmap_result =
        pgmap_at(&kernel_pagemap, phys_range, virt_addr, &options);

    assert_msg(pgmap_result, "mm: failed to setup kernel-pagemap");
    printk(LOGLEVEL_INFO,
           "mm: mapped " RANGE_FMT " to " RANGE_FMT "\n",
           RANGE_FMT_ARGS(phys_range),
           RANGE_FMT_ARGS(range_create(virt_addr, phys_range.size)));
}

static void
setup_kernel_pagemap(const uint64_t total_bytes_repr_by_structpage_table,
                     uint64_t *const kernel_memmap_size_out)
{
    const uint64_t lower_root = early_alloc_page();
    if (lower_root == INVALID_PHYS) {
        panic("mm: failed to allocate lower-half root page for the "
              "kernel-pagemap");
    }

    const uint64_t higher_root = early_alloc_page();
    if (higher_root == INVALID_PHYS) {
        panic("mm: failed to allocate higher-half root page for the "
              "kernel-pagemap");
    }

    kernel_pagemap.lower_root = phys_to_page(lower_root);
    kernel_pagemap.higher_root = phys_to_page(higher_root);

    const struct range ident_map_range = range_create_end(kib(16), gib(4));

    uint64_t kernel_memmap_size = 0;
    map_into_kernel_pagemap(ident_map_range.front,
                            ident_map_range.front,
                            ident_map_range.size,
                            __PTE_MMIO);

    // Map all 'good' regions into the hhdm
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + i;
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY ||
            memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = memmap->range.size;
            map_into_kernel_pagemap(/*phys_addr=*/memmap->range.front,
                                    /*virt_addr=*/KERNEL_BASE,
                                    kernel_memmap_size,
                                    /*pte_flags=*/0);
        } else {
            uint64_t flags = __PTE_PXN | __PTE_UXN;
            if (memmap->kind == MM_MEMMAP_KIND_FRAMEBUFFER) {
                flags |= __PTE_WC;
            }

            map_into_kernel_pagemap(/*phys_addr=*/memmap->range.front,
                                    (uint64_t)phys_to_virt(memmap->range.front),
                                    memmap->range.size,
                                    flags);
        }
    }

    *kernel_memmap_size_out = kernel_memmap_size;

    setup_pagestructs_table(total_bytes_repr_by_structpage_table);
    switch_to_pagemap(&kernel_pagemap);

    // All 'good' memmaps use pages with associated struct pages to them,
    // despite the pages being allocated before the structpage-table, so we have
    // to go through each table used, and setup all pgtable metadata inside a
    // struct page

    mm_early_refcount_alloced_map(ident_map_range.front, ident_map_range.size);
    mm_early_refcount_alloced_map(VMAP_BASE, (VMAP_END - VMAP_BASE));

    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + i;
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY ||
            memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        uint64_t virt_addr = 0;
        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            virt_addr = KERNEL_BASE;
        } else {
            virt_addr = (uint64_t)phys_to_virt(memmap->range.front);
        }

        mm_early_refcount_alloced_map(virt_addr, memmap->range.size);
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up kernel-pagemap!\n");
}

static void fill_kernel_pagemap_struct(const uint64_t kernel_memmap_size) {
    // Setup vma_tree to include all ranges we've mapped.

    struct vm_area *const null_area =
        vma_alloc(&kernel_pagemap,
                  range_create(0, PAGE_SIZE),
                  PROT_NONE,
                  VMA_CACHEKIND_NO_CACHE);

    // This range was never mapped, but is still reserved.
    struct vm_area *const vmap =
        vma_alloc(&kernel_pagemap,
                  range_create(VMAP_BASE, (VMAP_END - VMAP_BASE)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_MMIO);

    struct vm_area *const kernel =
        vma_alloc(&kernel_pagemap,
                  range_create(KERNEL_BASE, kernel_memmap_size),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    struct vm_area *const hhdm =
        vma_alloc(&kernel_pagemap,
                  range_create(HHDM_OFFSET, tib(64)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    assert_msg(
        addrspace_add_node(&kernel_pagemap.addrspace, &null_area->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &vmap->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &kernel->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &hhdm->node),
        "mm: failed to setup kernel-pagemap");
}

static void setup_mair() {
    // Device nGnRnE
    const uint64_t device_uncacheable_encoding = 0b00000000;

    // Device GRE
    const uint64_t device_write_combining_encoding = 0b00001100;

    // Normal memory, inner and outer non-cacheable
    const uint64_t memory_uncacheable_encoding = 0b01000100;

    // Normal memory inner and outer writethrough, non-transient
    const uint64_t memory_writethrough_encoding = 0b10111011;

    // Normal memory, inner and outer write-back, non-transient
    const uint64_t memory_write_back_encoding = 0b11111111;

    const uint64_t mair_value =
        memory_write_back_encoding |
        device_write_combining_encoding << 8 |
        memory_writethrough_encoding << 16 |
        device_uncacheable_encoding << 24 |
        memory_uncacheable_encoding << 32;

    write_mair_el1(mair_value);
}

extern uint64_t memmap_last_repr_index;
void mm_init() {
    setup_mair();

    const struct mm_memmap *const last_repr_memmap =
        mm_get_memmap_list() + memmap_last_repr_index;
    const uint64_t total_bytes_repr_by_structpage_table =
        range_get_end_assert(last_repr_memmap->range);

    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(total_bytes_repr_by_structpage_table,
                         &kernel_memmap_size);

    mm_early_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}