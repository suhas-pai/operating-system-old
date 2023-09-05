/*
 * kernel/arch/x86_64/mm/init.c
 * © suhas pai
 */

#include "asm/msr.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "mm/early.h"
#include "mm/pgmap.h"

#include "boot.h"
#include "cpu.h"

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
                page | PTE_LARGE_FLAGS(3) | pte_flags;

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
                page | PTE_LARGE_FLAGS(2) | pte_flags;

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
                page | PTE_LEAF_FLAGS | pte_flags;

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
        panic("mm: failed to initialize memory, overflow error when aligning");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    // Map struct page table
    const uint64_t pte_flags = __PTE_WRITE | __PTE_GLOBAL | __PTE_NOEXEC;
    map_region(PAGE_OFFSET, map_size, pte_flags);
}

static void
map_into_kernel_pagemap(const struct range phys_range,
                        const uint64_t virt_addr,
                        const uint64_t pte_flags)
{
    const bool supports_2mib = (pte_flags & __PTE_PAT) == 0;
    const bool supports_1gib =
        supports_2mib && get_cpu_capabilities()->supports_1gib_pages;

    const struct pgmap_options options = {
        .pte_flags = __PTE_GLOBAL | pte_flags,
        .supports_largepage_at_level_mask =
            supports_1gib << 3 | supports_2mib << 2,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .is_in_early = true,
        .is_overwrite = false
    };

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
    const uint64_t root = early_alloc_page();
    if (root == INVALID_PHYS) {
        panic("mm: failed to allocate root page for kernel-pagemap");
    }

    kernel_pagemap.root = phys_to_page(root);
    uint64_t kernel_memmap_size = 0;

    // Map all 'good' regions into the hhdm, except the kernel, which goes in
    // its own range
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + i;

        // ACPI's RSDP pointer and other ACPI information is currently stored in
        // a reserved memmap, so map reserved memmaps in the HHDM for now.

        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY) {
            continue;
        }

        // Only usable memmaps are guaranteed to be page-aligned.
        // Align out a memmap's range so we don't lose access to valuable
        // physical memory, for e.g. a portion of the framebuffer.

        struct range range = RANGE_EMPTY();
        if (!range_align_out(memmap->range, PAGE_SIZE, &range) ||
            range_empty(range))
        {
            continue;
        }

        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = range.size;
            map_into_kernel_pagemap(/*phys_range=*/range,
                                    /*virt_addr=*/KERNEL_BASE,
                                    __PTE_WRITE);
        } else {
            uint64_t flags = __PTE_WRITE | __PTE_NOEXEC;
            if (memmap->kind == MM_MEMMAP_KIND_FRAMEBUFFER) {
                flags |= __PTE_PAT;
            }

            map_into_kernel_pagemap(/*phys_range=*/range,
                                    (uint64_t)phys_to_virt(range.front),
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

extern uint64_t memmap_first_repr_index;
extern uint64_t memmap_last_repr_index;

void mm_init() {
    const uint64_t pat_msr =
        read_msr(IA32_MSR_PAT) & ~(0b111ull << MSR_PAT_INDEX_PAT4);
    const uint64_t write_combining_mask =
        (uint64_t)MSR_PAT_ENCODING_WRITE_COMBINING << MSR_PAT_INDEX_PAT4;

    write_msr(IA32_MSR_PAT, pat_msr | write_combining_mask);

    const struct mm_memmap *const first_repr_memmap =
        mm_get_memmap_list() + memmap_first_repr_index;
    const struct mm_memmap *const last_repr_memmap =
        mm_get_memmap_list() + memmap_last_repr_index;
    const uint64_t total_bytes_repr_by_structpage_table =
        range_get_end_assert(last_repr_memmap->range) -
        first_repr_memmap->range.front;

    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(total_bytes_repr_by_structpage_table,
                         &kernel_memmap_size);

    mm_early_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}