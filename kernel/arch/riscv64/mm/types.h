/*
 * kernel/arch/riscv64/mm/types.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39
#define PML5_SHIFT 48

#define PAGE_SHIFT PML1_SHIFT
#define PTE_PHYS_MASK 0x003ffffffffffc00

#define PGT_LEVEL_COUNT 5

#define L3_SIZE (1ull << L3_SHIFT)
#define L2_SIZE (1ull << L2_SHIFT)
#define L1_SIZE (1ull << L1_SHIFT)
#define L0_SIZE (1ull << L0_SHIFT)

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

#define pte_to_phys(pte) (((pte) & PTE_PHYS_MASK) << 2)
#define phys_create_pte(phys) ((pte_t)(phys) >> 2)

typedef uint64_t pte_t;

static const uint16_t PT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] =
    { (1ull << 12) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK, PML5_MASK };

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] =
    { PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT, PML5_SHIFT };

static const uint8_t LARGEPAGE_LEVELS[] = { 2, 3, 4 };
static const uint8_t LARGEPAGE_SHIFTS[] = {
    PML2_SHIFT, PML3_SHIFT, PML4_SHIFT };

#define PAGE_SIZE_2MIB (1ull << LARGEPAGE_SHIFTS[0])
#define PAGE_SIZE_1GIB (1ull << LARGEPAGE_SHIFTS[1])
#define PAGE_SIZE_512GIB (1ull << LARGEPAGE_SHIFTS[2])

#define PAGE_SIZE_AT_LEVEL(level) \
    ({\
        const uint64_t __sizes__[] = { \
            PAGE_SIZE,      \
            PAGE_SIZE_2MIB, \
            PAGE_SIZE_1GIB, \
            PAGE_SIZE_1GIB * PGT_COUNT, \
            PAGE_SIZE_1GIB * PGT_COUNT * PGT_COUNT \
        }; \
       __sizes__[level - 1];\
    })

enum pte_flags {
    __PTE_VALID    = 1 << 0,
    __PTE_READ     = 1 << 1,
    __PTE_WRITE    = 1 << 2,
    __PTE_EXEC     = 1 << 3,
    __PTE_USER     = 1 << 4,
    __PTE_GLOBAL   = 1 << 5,
    __PTE_ACCESSED = 1 << 6,
    __PTE_DIRTY    = 1 << 7,
};

#define PGT_FLAGS __PTE_VALID
#define PTE_LARGE_FLAGS(level) ({ (void)(level); __PTE_VALID; })
#define PTE_LEAF_FLAGS (__PTE_VALID | __PTE_ACCESSED | __PTE_DIRTY)

struct page;