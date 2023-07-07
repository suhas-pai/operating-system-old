/*
 * kernel/arch/aarch64/mm/types.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#define L3_SHIFT 12
#define L2_SHIFT 21
#define L1_SHIFT 30
#define L0_SHIFT 39

#define PAGE_SHIFT L3_SHIFT
#define PG_PHYS_MASK 0x1fffffffffff000

#define PGT_LEVEL_COUNT 4

#define L3_SIZE (1ull << L3_SHIFT)
#define L2_SIZE (1ull << L2_SHIFT)
#define L1_SIZE (1ull << L1_SHIFT)
#define L0_SIZE (1ull << L0_SHIFT)

#define L3_MASK 0x1ff
#define L2_MASK L3_MASK
#define L1_MASK L3_MASK
#define L0_MASK L3_MASK

#define L3(phys) (((phys) >> L3_SHIFT) & L3_MASK)
#define L1(phys) (((phys) >> L2_SHIFT) & L2_MASK)
#define L2(phys) (((phys) >> L1_SHIFT) & L1_MASK)
#define L0(phys) (((phys) >> L0_SHIFT) & L0_MASK)

typedef uint64_t pte_t;
enum page_flags {
    __PG_VALID = 1 << 0,
    __PG_TABLE = 1 << 1,
};

static const uint16_t PGT_LEVEL_MASKS[PGT_LEVEL_COUNT + 1] =
    { (1ull << 12) - 1, L3_MASK, L2_MASK, L1_MASK, L0_MASK };

static const uint8_t PAGE_SHIFTS[PGT_LEVEL_COUNT] =
    { L3_SHIFT, L2_SHIFT, L1_SHIFT, L0_SHIFT };

static const uint8_t LARGEPAGE_SHIFTS[] = {};

#define PGT_FLAGS (__PG_VALID | __PG_TABLE)

struct page;
extern const uint64_t PAGE_OFFSET;