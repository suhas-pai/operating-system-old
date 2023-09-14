/*
 * kernel/arch/riscv64/mm/init.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/size.h"

#include "dev/printk.h"

#include "mm/early.h"
#include "mm/pgmap.h"
#include "mm/walker.h"

#include "boot.h"

static uint64_t
ptwalker_alloc_pgtable_cb(struct pt_walker *const walker, void *const cb_info) {
    (void)walker;
    (void)cb_info;

    // We don't have a structpage-table setup yet when this function is called,
    // but because ptwalker never dereferences the page, we can return a pointer
    // to where the page would've been.

    const uint64_t phys = early_alloc_page();
    if (phys != INVALID_PHYS) {
        return phys;
    }

    panic("mm: failed to setup page-structs, ran out of memory\n");
}

static void
alloc_region(uint64_t virt_addr, uint64_t map_size, const uint64_t pte_flags) {
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

        pte_t *table = pt_walker.tables[2];
        pte_t *pte = table + pt_walker.indices[2];
        const pte_t *end = table + PGT_PTE_COUNT;

        do {
            const uint64_t page =
                early_alloc_large_page(
                    /*amount=*/PGT_PTE_COUNT * PGT_PTE_COUNT);

            if (page == INVALID_PHYS) {
                // We failed to alloc a 1gib page, so try 2mib pages next.
                break;
            }

            *pte =
                phys_create_pte(page) | PTE_LARGE_FLAGS(3) | __PTE_READ |
                pte_flags;

            pte++;
            if (pte == end) {
                pt_walker.indices[2] = PGT_PTE_COUNT - 1;
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

                table = pt_walker.tables[1];
                pte = table;
                end = table + PGT_PTE_COUNT;
            }

            map_size -= PAGE_SIZE_1GIB;
            if (map_size < PAGE_SIZE_1GIB) {
                pt_walker.indices[2] = pte - table;
                goto try_2mib;
            }

            virt_addr += PAGE_SIZE_1GIB;
        } while (true);
    }

try_2mib:
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

        pte_t *table = pt_walker.tables[1];
        pte_t *pte = table + pt_walker.indices[1];
        const pte_t *end = table + PGT_PTE_COUNT;

        do {
            const uint64_t page =
                early_alloc_large_page(/*amount=*/PGT_PTE_COUNT);

            if (page == INVALID_PHYS) {
                // We failed to alloc a 2mib page, so try 4kib pages next.
                break;
            }

            *pte =
                phys_create_pte(page) | PTE_LARGE_FLAGS(2) | __PTE_READ |
                pte_flags;

            pte++;
            if (pte == end) {
                pt_walker.indices[1] = PGT_PTE_COUNT - 1;
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

                table = pt_walker.tables[1];
                pte = table;
                end = table + PGT_PTE_COUNT;
            }

            map_size -= PAGE_SIZE_2MIB;
            if (map_size < PAGE_SIZE_2MIB) {
                pt_walker.indices[1] = pte - table;
                goto try_normal;
            }

            virt_addr += PAGE_SIZE_2MIB;
        } while (true);
    }

try_normal:
    if (map_size != 0) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/1,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        pte_t *table = pt_walker.tables[0];
        pte_t *pte = table + pt_walker.indices[0];
        const pte_t *end = table + PGT_PTE_COUNT;

        do {
            const uint64_t page = early_alloc_page();
            if (page == INVALID_PHYS) {
                panic("mm: failed to allocate page while setting up "
                      "kernel-pagemap\n");
            }

            *pte =
                phys_create_pte(page) | PTE_LEAF_FLAGS | __PTE_READ | pte_flags;

            map_size -= PAGE_SIZE;
            if (map_size == 0) {
                return;
            }

            pte++;
            if (pte == end) {
                pt_walker.indices[0] = PGT_PTE_COUNT - 1;
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

                table = pt_walker.tables[0];
                pte = table;
                end = table + PGT_PTE_COUNT;
            }

            virt_addr += PAGE_SIZE;
        } while (true);
    }
}

extern uint64_t structpage_page_count;

static void setup_pagestructs_table() {
    uint64_t map_size = structpage_page_count * SIZEOF_STRUCTPAGE;
    if (!align_up(map_size, PAGE_SIZE, &map_size)) {
        panic("mm: failed to initialize memory, overflow error when aligning");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    // Map struct page table
    const uint64_t pte_flags = __PTE_WRITE | __PTE_GLOBAL;
    alloc_region(PAGE_OFFSET, map_size, pte_flags);

    printk(LOGLEVEL_INFO, "mm: finished mapping structpage-table\n");
}

static void
map_into_kernel_pagemap(const struct range phys_range,
                        const uint64_t virt_addr,
                        uint64_t pte_flags)
{
    assert_msg(pte_flags & (__PTE_READ | __PTE_WRITE | __PTE_EXEC),
               "mm: map_into_kernel_pagemap() got flags w/o any rwx "
               "permissions");

    const struct pgmap_options options = {
        .pte_flags = __PTE_GLOBAL | pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3 | 1 << 4,

        .free_pages = false,
        .is_in_early = true,
        .is_overwrite = false,
    };

    const bool pgmap_result =
        pgmap_at(&kernel_pagemap, phys_range, virt_addr, &options);

    assert_msg(pgmap_result, "mm: failed to setup kernel-pagemap");
    printk(LOGLEVEL_INFO,
           "mm: mapped " RANGE_FMT " to " RANGE_FMT "\n",
           RANGE_FMT_ARGS(phys_range),
           RANGE_FMT_ARGS(range_create(virt_addr, phys_range.size)));
}

static void setup_kernel_pagemap(uint64_t *const kernel_memmap_size_out) {
    const uint64_t root_phys = early_alloc_page();
    if (root_phys == INVALID_PHYS) {
        panic("mm: failed to allocate root page for the kernel-pagemap");
    }

    kernel_pagemap.root = phys_to_virt(root_phys);
    map_into_kernel_pagemap(/*phys_range=*/range_create_end(kib(64), gib(4)),
                            /*virt_addr=*/kib(64),
                            __PTE_WRITE);

    // Map all 'good' regions into the hhdm
    uint64_t kernel_memmap_size = 0;
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + i;
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY ||
            memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = memmap->range.size;
            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    /*virt_addr=*/KERNEL_BASE,
                                    __PTE_READ | __PTE_WRITE | __PTE_EXEC);
        } else {
            uint64_t pte_flags = __PTE_READ | __PTE_WRITE;
            if (memmap->kind == MM_MEMMAP_KIND_FRAMEBUFFER) {
                pte_flags |= __PTE_NC;
            }

            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    (uint64_t)phys_to_virt(memmap->range.front),
                                    pte_flags);
        }
    }

    *kernel_memmap_size_out = kernel_memmap_size;

    setup_pagestructs_table();
    switch_to_pagemap(&kernel_pagemap);

    // All 'good' memmaps use pages with associated struct pages to them,
    // despite the pages being allocated before the structpage-table, so we have
    // to go through each table used, and setup all pgtable metadata inside a
    // struct page

    mm_early_refcount_alloced_map(kib(64), gib(4) - kib(64));
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
    struct vm_area *const mmio =
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
        addrspace_add_node(&kernel_pagemap.addrspace, &mmio->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &kernel->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &hhdm->node),
        "mm: failed to setup kernel-pagemap");
}

void mm_init() {
    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(&kernel_memmap_size);

    mm_early_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}