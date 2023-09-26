/*
 * kernel/arch/x86_64/mm/types.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39
#define PML5_SHIFT 48

#define PAGE_SHIFT PML1_SHIFT
#define PTE_PHYS_MASK 0x0000fffffffff000

#define PGT_LEVEL_COUNT 5
#define PGT_PTE_COUNT 512

#define PML1_MASK 0x1ff
#define PML2_MASK PML1_MASK
#define PML3_MASK PML1_MASK
#define PML4_MASK PML1_MASK
#define PML5_MASK PML1_MASK

#define PML1(phys) (((phys) >> PML1_SHIFT) & PML1_MASK)
#define PML2(phys) (((phys) >> PML2_SHIFT) & PML2_MASK)
#define PML3(phys) (((phys) >> PML3_SHIFT) & PML3_MASK)
#define PML4(phys) (((phys) >> PML4_SHIFT) & PML4_MASK)
#define PML5(phys) (((phys) >> PML5_SHIFT) & PML5_MASK)

#define pte_to_phys(pte) ((pte) & PTE_PHYS_MASK)
#define phys_create_pte(phys) (phys)

typedef uint64_t pte_t;

static const uint16_t PT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] =
    { (1ull << 12) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK, PML5_MASK };

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] =
    { PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT, PML5_SHIFT };

static const uint8_t LARGEPAGE_LEVELS[] = { 2, 3 };
static const uint8_t LARGEPAGE_SHIFTS[] = { PML2_SHIFT, PML3_SHIFT };

#define PAGE_SIZE_2MIB (1ull << LARGEPAGE_SHIFTS[0])
#define PAGE_SIZE_1GIB (1ull << LARGEPAGE_SHIFTS[1])

#define LARGEPAGE_LEVEL_2MIB LARGEPAGE_LEVELS[0]
#define LARGEPAGE_LEVEL_1GIB LARGEPAGE_LEVELS[1]

struct largepage_level_info {
    uint8_t order;
    uint8_t level; // can't use pgt_level_t
    uint64_t size;
};

__unused static struct largepage_level_info largepage_level_info_list[] = {
    { .order = 10, .level = LARGEPAGE_LEVEL_2MIB, .size = PAGE_SIZE_2MIB },
    { .order = 20, .level = LARGEPAGE_LEVEL_1GIB, .size = PAGE_SIZE_1GIB }
};

#define PAGE_SIZE_AT_LEVEL(level) \
    ({\
        const uint64_t __sizes__[] = { \
            PAGE_SIZE,      \
            PAGE_SIZE_2MIB, \
            PAGE_SIZE_1GIB, \
            /* The following aren't valid largepage sizes, but keep them */ \
            PAGE_SIZE_1GIB * PGT_PTE_COUNT, \
            PAGE_SIZE_1GIB * PGT_PTE_COUNT * PGT_PTE_COUNT \
        }; \
       __sizes__[level - 1];\
    })

enum pte_flags {
    __PTE_PRESENT  = 1ull << 0,
    __PTE_WRITE    = 1ull << 1,
    __PTE_USER     = 1ull << 2,
    __PTE_PWT      = 1ull << 3,
    __PTE_PCD      = 1ull << 4,
    __PTE_ACCESSED = 1ull << 5,
    __PTE_LARGE    = 1ull << 7,
    __PTE_WC       = 1ull << 7, // Only on PML1 pages
    __PTE_GLOBAL   = 1ull << 8,

    __PTE_NOEXEC = 1ull << 63
};

#define PGT_FLAGS (__PTE_PRESENT | __PTE_WRITE)
#define PTE_LARGE_FLAGS(level) ({ (void)(level); __PTE_PRESENT | __PTE_LARGE; })
#define PTE_LEAF_FLAGS __PTE_PRESENT

struct page;