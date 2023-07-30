/*
 * kernel/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mm/types.h"

#define SIZEOF_STRUCTPAGE (sizeof(uint64_t) * 5)
#define PAGE_SIZE (1ull << PAGE_SHIFT)
#define LARGEPAGE_SIZE(index) (1ull << LARGEPAGE_SHIFTS[index])
#define INVALID_PHYS (uint64_t)-1

#define PAGE_COUNT(size) (((uint64_t)(size) / PAGE_SIZE))
#define PGT_COUNT (PAGE_SIZE/sizeof(pte_t))

#define SECTION_SHIFT (sizeof(uint32_t) - sizeof(uint8_t))
#define SECTION_MASK UINT8_MAX

#define phys_to_pfn(phys) ((uint64_t)(phys) >> PAGE_SHIFT)
#define pfn_to_phys(pfn) ((uint64_t)(pfn) << PAGE_SHIFT)
#define pfn_to_page(pfn) \
    ((struct page *)(PAGE_OFFSET + (SIZEOF_STRUCTPAGE * (uint64_t)(pfn))))

#define page_to_pfn(page) (((uint64_t)(page) - PAGE_OFFSET) / SIZEOF_STRUCTPAGE)

#define page_to_phys(page) pfn_to_phys(page_to_pfn(page))
#define phys_to_page(phys) pfn_to_page(phys_to_pfn(phys))
#define virt_to_page(virt) phys_to_page(virt_to_phys(virt))
#define page_to_virt(page) phys_to_virt(page_to_phys(page))

uint8_t pgt_get_top_level();

bool pte_is_present(pte_t pte);
bool pte_is_large(pte_t pte, uint8_t level);

#define pte_to_pfn(pte) phys_to_pfn(pte_to_phys(pte))
#define pte_to_virt(pte) phys_to_virt(pte_to_phys(pte))
#define pte_to_page(pte) pfn_to_page(pte_to_pfn(pte))

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(const void *phys);

extern uint64_t KERNEL_BASE;
extern uint64_t HHDM_OFFSET;

void pagezones_init();

static inline
uint16_t virt_to_pt_index(const void *const virt, const uint8_t level) {
    return ((uint64_t)virt >> PAGE_SHIFTS[level - 1]) & PT_LEVEL_MASKS[level];
}

extern const uint64_t PAGE_OFFSET;
extern const uint64_t VMAP_BASE;
extern const uint64_t VMAP_END;

// The mmio range is reserved but not actually mapped
extern uint64_t MMIO_BASE;
extern uint64_t MMIO_END;

enum prot_flags {
    PROT_NONE,
    PROT_READ = 1 << 0,
    PROT_WRITE = 1 << 1,
    PROT_EXEC = 1 << 2,

    PROT_RW = PROT_READ | PROT_WRITE,
    PROT_WX = PROT_WRITE | PROT_EXEC,
    PROT_RX = PROT_READ | PROT_EXEC,

    PROT_RWX = PROT_READ | PROT_WRITE | PROT_EXEC,

#if defined(__aarch64__)
    PROT_DEVICE = 1 << 4
#endif
};

enum vma_cachekind {
    VMA_CACHEKIND_WRITEBACK,
    VMA_CACHEKIND_DEFAULT = VMA_CACHEKIND_WRITEBACK,

    VMA_CACHEKIND_WRITETHROUGH,
    VMA_CACHEKIND_WRITECOMBINING,
    VMA_CACHEKIND_NO_CACHE,

#if defined(__aarch64__)
    VMA_CACHEKIND_MMIO,
#else
    VMA_CACHEKIND_MMIO = VMA_CACHEKIND_NO_CACHE,
#endif /* defined(__x86_64__) */
};
