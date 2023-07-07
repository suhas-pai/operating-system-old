/*
 * kernel/arch/x86_64/mm/page.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39
#define PML5_SHIFT 48

#define PAGE_SHIFT PML1_SHIFT
#define PG_PHYS_MASK 0x1fffffffffff000

#define PGT_LEVEL_COUNT 5

#define PML2_SIZE (1ull << PML2_SHIFT)
#define PML3_SIZE (1ull << PML3_SHIFT)
#define PML4_SIZE (1ull << PML4_SHIFT)
#define PML5_SIZE (1ull << PML5_SHIFT)

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

typedef uint64_t pte_t;

static const uint16_t PGT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] =
    { (1ull << 12) - 1, PML1_MASK, PML2_MASK, PML3_MASK, PML4_MASK, PML5_MASK };

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] =
    { PML1_SHIFT, PML2_SHIFT, PML3_SHIFT, PML4_SHIFT, PML5_SHIFT };

static const uint8_t LARGEPAGE_SHIFTS[] = { PML2_SHIFT, PML3_SHIFT };
enum page_flags {
    __PG_PRESENT = 1 << 0,
    __PG_WRITE   = 1 << 1,
    __PG_USER    = 1 << 2,
    __PG_LARGE   = 1 << 7,
    __PG_GLOBAL  = 1 << 8,

    __PG_NOEXEC = 1ull << 63
};

#define PGT_FLAGS (__PG_PRESENT | __PG_WRITE | __PG_NOEXEC)

struct page;
extern const uint64_t PAGE_OFFSET;
