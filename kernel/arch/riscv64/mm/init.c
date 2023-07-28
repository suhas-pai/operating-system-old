/*
 * kernel/arch/riscv64/mm/init.c
 * © suhas pai
 */

#include "lib/align.h"
#include "lib/size.h"

#include "dev/printk.h"

#include "mm/early.h"
#include "mm/kmalloc.h"
#include "mm/page_alloc.h"
#include "mm/pagemap.h"

#include "limine.h"

volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = NULL
};

extern volatile struct limine_memmap_request memmap_request;

#define KERNEL_BASE 0xffffffff80000000

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
map_region(const uint64_t root_phys,
           uint64_t virt_addr,
           uint64_t map_size,
           const uint64_t pte_flags)
{
    enum pt_walker_result walker_result = E_PT_WALKER_OK;
    struct pt_walker pt_walker = {0};

    ptwalker_create_customroot(&pt_walker,
                               root_phys,
                               virt_addr,
                               /*alloc_pgtable=*/ptwalker_alloc_pgtable_cb,
                               /*free_pgtable=*/NULL);

    if (map_size >= PAGE_SIZE_1GIB && (virt_addr & (PAGE_SIZE_1GIB - 1)) == 0) {
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
                early_alloc_cont_pages(/*order=*/PGT_COUNT * PGT_COUNT);

            if (page == INVALID_PHYS) {
                // We failed to alloc a 1gib page, so try 2mib pages next.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/3) =
                phys_create_pte(page) | pte_flags | __PTE_VALID | __PTE_READ |
                __PTE_ACCESSED | __PTE_DIRTY;

            walker_result =
                ptwalker_next_custom(&pt_walker,
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

    if (map_size >= PAGE_SIZE_2MIB && (virt_addr & (PAGE_SIZE_2MIB - 1)) == 0) {
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
            const uint64_t page = early_alloc_cont_pages(/*order=*/PGT_COUNT);
            if (page == INVALID_PHYS) {
                // We failed to alloc a 2mib page, so fill with 4kib pages
                // instead.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/2) =
                phys_create_pte(page) | pte_flags | __PTE_VALID | __PTE_READ |
                __PTE_ACCESSED | __PTE_DIRTY;

            walker_result =
                ptwalker_next_custom(&pt_walker,
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
                // We failed to alloc a 2mib page, so fill with 4kib pages
                // instead.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/1) =
                phys_create_pte(page) | pte_flags | __PTE_VALID | __PTE_READ |
                __PTE_ACCESSED | __PTE_DIRTY;

            walker_result =
                ptwalker_next_custom(&pt_walker,
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
            if (map_size < PAGE_SIZE) {
                break;
            }

            virt_addr += PAGE_SIZE;
        } while (true);
    }
}

static void
setup_pagestructs_table(const uint64_t root_phys, const uint64_t byte_count) {
    const uint64_t structpage_count = PAGE_COUNT(byte_count);
    uint64_t map_size = structpage_count * SIZEOF_STRUCTPAGE;

    if (!align_up(map_size, PAGE_SIZE, &map_size)) {
        panic("mm: failed to initialize memory, overflow error when aligning");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    // Map struct page table
    const uint64_t pte_flags = __PTE_WRITE | __PTE_GLOBAL;
    map_region(root_phys, PAGE_OFFSET, map_size, pte_flags);
}

static void
map_into_kernel_pagemap(const uint64_t root_phys,
                        const uint64_t phys_addr,
                        const uint64_t virt_addr,
                        const uint64_t size,
                        const uint64_t pte_flags)
{
    struct pt_walker walker = {0};
    ptwalker_create_customroot(&walker,
                               root_phys,
                               virt_addr,
                               ptwalker_alloc_pgtable_cb,
                               /*free_pgtable=*/NULL);

    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(&walker,
                            /*level=*/1,
                            /*should_ref=*/false,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/NULL);

    // TODO: The kernel+modules should be mapped 1gib large pages
    for (uint64_t i = 0; i < size; i += PAGE_SIZE) {
        if (ptwalker_result != E_PT_WALKER_OK) {
            panic("mm: failed to setup kernel pagemap");
        }

        walker.tables[0][walker.indices[0]] =
            phys_create_pte(phys_addr + i) | pte_flags | __PTE_VALID |
            __PTE_READ | __PTE_ACCESSED | __PTE_DIRTY | __PTE_GLOBAL;

        ptwalker_result =
            ptwalker_next_custom(&walker,
                                 /*level=*/1,
                                 /*alloc_parents=*/true,
                                 /*alloc_level=*/true,
                                 /*should_ref=*/false,
                                 /*alloc_pgtable_cb_info=*/NULL,
                                 /*free_pgtable_cb_info=*/NULL);
    }
}

static void init_table_page(struct page *const page) {
    list_init(&page->table.delayed_free_list);
    refcount_init(&page->table.refcount);
}

static void refcount_range(const uint64_t virt_addr, const uint64_t length) {
    struct pt_walker walker = {0};
    ptwalker_create(&walker,
                    virt_addr,
                    /*alloc_pgtable=*/NULL,
                    /*free_pgtable=*/NULL);

    for (uint8_t level = (uint8_t)walker.level;
         level <= walker.top_level;
         level++)
    {
        struct page *const page = virt_to_page(walker.tables[level - 1]);
        init_table_page(page);
    }

    // Track changes to the `walker.tables` array by seeing if the index of
    // the lowest level ever reaches (PGT_COUNT - 1). When it does,
    // set ref_pages = true

    bool ref_pages = false;
    for (uint64_t i = 0; i < length;) {
        const enum pt_walker_result advance_result =
            ptwalker_next(&walker, /*op=*/NULL);

        if (advance_result != E_PT_WALKER_OK) {
            panic("mm: failed to setup kernel pagemap");
        }

        // When ref_pages is true, the lowest level will have an index 0
        // after incrementing.
        // All levels higher will also be reset to 0, but only until a level
        // with an index not equal to 0.

        if (ref_pages) {
            for (uint8_t level = (uint8_t)walker.level;
                level < walker.top_level;
                level++)
            {
                const uint64_t index = walker.indices[level - 1];
                if (index != 0) {
                    break;
                }

                pte_t *const table = walker.tables[level - 1];
                struct page *const page = virt_to_page(table);

                init_table_page(page);
            }
        }

        ref_pages = walker.indices[walker.level - 1] == PGT_COUNT - 1;
        switch (walker.level) {
            case 1:
                i += PAGE_SIZE;
                break;
            case 2:
                i += PAGE_SIZE_2MIB;
                break;
            case 3:
                i += PAGE_SIZE_1GIB;
                break;
            case 4:
                i += PAGE_SIZE_512GIB;
                break;
        }
    }
}

static void
setup_kernel_pagemap(const uint64_t total_bytes_repr_by_structpage_table,
                     uint64_t *const kernel_memmap_size_out)
{
    const uint64_t root_phys = early_alloc_page();
    if (root_phys == INVALID_PHYS) {
        panic("mm: failed to allocate root page for the kernel-pagemap");
    }

    kernel_pagemap.root = phys_to_page(root_phys);
    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    struct limine_memmap_entry *const *const end = entries + resp->entry_count;

    uint64_t kernel_memmap_size = 0;

    // Map all 'good' regions into the hhdm
    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        const struct limine_memmap_entry *const memmap = *memmap_iter;
        if (memmap->type == LIMINE_MEMMAP_BAD_MEMORY ||
            memmap->type == LIMINE_MEMMAP_RESERVED)
        {
            continue;
        }

        if (memmap->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            kernel_memmap_size = memmap->length;
            map_into_kernel_pagemap(root_phys,
                                    /*phys_addr=*/memmap->base,
                                    /*virt_addr=*/KERNEL_BASE,
                                    memmap->length,
                                    __PTE_WRITE | __PTE_EXEC);
        } else {
            uint64_t flags = __PTE_WRITE;
            map_into_kernel_pagemap(root_phys,
                                    /*phys_addr=*/memmap->base,
                                    (uint64_t)phys_to_virt(memmap->base),
                                    memmap->length,
                                    flags);
        }
    }

    *kernel_memmap_size_out = kernel_memmap_size;

    setup_pagestructs_table(root_phys, total_bytes_repr_by_structpage_table);
    map_into_kernel_pagemap(root_phys,
                            /*phys_addr=*/MMIO_BASE - HHDM_OFFSET,
                            /*virt_addr=*/MMIO_BASE - HHDM_OFFSET,
                            (MMIO_END - MMIO_BASE),
                            /*pte_flags=*/__PTE_WRITE);

    switch_to_pagemap(&kernel_pagemap);

    // All 'good' memmaps use pages with associated struct pages to them,
    // despite the pages being allocated before the structpage-table, so we have
    // to go through each table used, and setup all pgtable metadata inside a
    // struct page

    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        const struct limine_memmap_entry *const memmap = *memmap_iter;
        if (memmap->type == LIMINE_MEMMAP_BAD_MEMORY ||
            memmap->type == LIMINE_MEMMAP_RESERVED)
        {
            continue;
        }

        uint64_t virt_addr = 0;
        if (memmap->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            virt_addr = KERNEL_BASE;
        } else {
            virt_addr = (uint64_t)phys_to_virt(memmap->base);
        }

        refcount_range(virt_addr, memmap->length);
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
                  range_create(MMIO_BASE, (MMIO_END - MMIO_BASE)),
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

    list_radd(&kernel_pagemap.vma_list, &null_area->vma_list);
    avltree_insert(&kernel_pagemap.vma_tree,
                   &null_area->vma_node,
                   vma_avltree_compare,
                   vma_avltree_update);

    list_radd(&kernel_pagemap.vma_list, &mmio->vma_list);
    avltree_insert(&kernel_pagemap.vma_tree,
                   &mmio->vma_node,
                   vma_avltree_compare,
                   vma_avltree_update);

    list_radd(&kernel_pagemap.vma_list, &kernel->vma_list);
    avltree_insert(&kernel_pagemap.vma_tree,
                   &kernel->vma_node,
                   vma_avltree_compare,
                   vma_avltree_update);

    list_radd(&kernel_pagemap.vma_list, &hhdm->vma_list);
    avltree_insert(&kernel_pagemap.vma_tree,
                   &hhdm->vma_node,
                   vma_avltree_compare,
                   vma_avltree_update);
}

extern uint64_t memmap_last_repr_index;
void mm_init() {
    assert(memmap_request.response != NULL);
    assert(paging_mode_request.response != NULL);

    PAGING_MODE = paging_mode_request.response->mode;

    MMIO_BASE = HHDM_OFFSET + mib(2);
    MMIO_END = MMIO_BASE + gib(4);

    const struct limine_memmap_response *const resp = memmap_request.response;
    struct limine_memmap_entry *const *entries = resp->entries;

    const struct limine_memmap_entry *const last_repr_memmap =
        entries[memmap_last_repr_index];

    const uint64_t total_bytes_repr_by_structpage_table =
        last_repr_memmap->base + last_repr_memmap->length;

    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(total_bytes_repr_by_structpage_table,
                         &kernel_memmap_size);

    mm_early_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}