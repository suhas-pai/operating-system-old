/*
 * kernel/mm/pageop.c
 * © suhas pai
 */

#include "lib/adt/array.h"

#if defined(__x86_64__)
    #include "mm/tlb.h"
#endif /* defined(__x86_64__) */

#include "pagemap.h"
#include "pageop.h"

#include "cpu.h"
#include "page_alloc.h"

void pageop_init(struct pageop *const pageop) {
    pageop->pagemap = get_cpu_info()->pagemap;
    pageop->flush_range = RANGE_EMPTY();

    list_init(&pageop->delayed_free);
}

struct flush_pte_info {
    struct refcount ref;
    struct list delayed_free_list;
    struct spinlock lock;

    uint64_t pte_phys;
    pgt_level_t pte_level;
};

void
pageop_flush_pte(struct pageop *const pageop,
                 pte_t *const pte,
                 const pgt_level_t level)
{
    pageop_finish(pageop);

    struct pagemap *const pagemap = pageop->pagemap;
    struct array cpu_array = ARRAY_INIT(sizeof(struct cpu_info *));

    const int flag = spin_acquire_with_irq(&pagemap->cpu_lock);
    const pte_t pte_phys = pte_to_phys(pte_read(pte));

    pte_write(pte, /*value=*/0);

    struct cpu_info *iter = NULL;
    list_foreach(iter, &pagemap->cpu_list, pagemap_node) {
        array_append(&cpu_array, &iter);
    }

    spin_release_with_irq(&pagemap->cpu_lock, flag);
    struct flush_pte_info flush_info = {
        .ref = REFCOUNT_CREATE(array_item_count(cpu_array)),
        .delayed_free_list = LIST_INIT(flush_info.delayed_free_list),
        .lock = SPINLOCK_INIT(),
        .pte_phys = pte_phys,
        .pte_level = level
    };

    array_destroy(&cpu_array);
}

void pageop_flush_address(struct pageop *const pageop, const uint64_t virt) {
    pageop_finish(pageop);
    pageop->flush_range = range_create(virt, PAGE_SIZE);
}

void pageop_finish(struct pageop *const pageop) {
#if defined(__x86_64__)
    tlb_flush_pageop(pageop);
#elif defined(__riscv64)
    asm volatile ("fence.i" ::: "memory");
#endif /* defined(__x86_64__) */

    pageop->flush_range = RANGE_EMPTY();
}

struct page *alloc_table() {
    return alloc_page(__ALLOC_TABLE | __ALLOC_ZERO);
}