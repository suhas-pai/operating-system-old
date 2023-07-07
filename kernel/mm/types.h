/*
 * kernel/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(__x86_64__)
    #include "arch/x86_64/mm/types.h"
#elif defined(__aarch64__)
    #include "arch/aarch64/mm/types.h"
#endif /* defined(__x86_64__) */

#define STRUCTPAGE_SIZEOF (sizeof(uint64_t) * 5)
#define PAGE_SIZE (1ull << PAGE_SHIFT)
#define LARGEPAGE_SIZE(index) (1ull << LARGEPAGE_SHIFTS[index])
#define PAGE_COUNT(size) (((uint64_t)(size) / PAGE_SIZE))

#define PGT_COUNT (PAGE_SIZE/sizeof(pte_t))

#define PG_LEVEL_INDEX(addr, level) \
    ((((uint64_t)(addr)) >> PAGE_SHIFTS[level - 1]) & PGT_LEVEL_MASKS[level])

#define phys_to_pfn(phys) ((uint64_t)(phys) >> PAGE_SHIFT)
#define pfn_to_phys(pfn) ((uint64_t)(pfn) << PAGE_SHIFT)
#define pfn_to_page(pfn) \
    ((struct page *)(PAGE_OFFSET + (STRUCTPAGE_SIZEOF * (uint64_t)(pfn))))

#define SECTION_SHIFT (sizeof(uint32_t) - sizeof(uint8_t))
#define SECTION_MASK UINT8_MAX

#define page_to_pfn(page) (((uint64_t)(page) - PAGE_OFFSET) / STRUCTPAGE_SIZEOF)

#define page_to_phys(page) pfn_to_phys(page_to_pfn(page))
#define phys_to_page(phys) pfn_to_page(phys_to_pfn(phys))
#define virt_to_page(virt) phys_to_page(virt_to_phys(virt))
#define page_to_virt(page) phys_to_virt(page_to_phys(page))

uint8_t pgt_get_top_level();
bool pte_is_present(pte_t pte);
bool pte_is_large(pte_t pte, uint8_t level);

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(const void *phys);

void pagezones_init();