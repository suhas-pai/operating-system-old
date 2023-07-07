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

static const uint8_t LARGEPAGE_SHIFTS[] = { PML2_SHIFT, PML3_SHIFT };

#define PML2_SIZE (1UL << PML2_SHIFT)
#define PML3_SIZE (1UL << PML3_SHIFT)
#define PML4_SIZE (1UL << PML4_SHIFT)
#define PML5_SIZE (1UL << PML5_SHIFT)

#define PGT_COUNT (PAGE_SIZE/sizeof(uint64_t))

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

enum page_flags {
    X86_64_PG_PRESENT = 1 << 0,
    X86_64_PG_WRITE   = 1 << 1,
    X86_64_PG_USER    = 1 << 2,
    X86_64_PG_LARGE   = 1 << 7,
    X86_64_PG_GLOBAL  = 1 << 8,

    X86_64_PG_NOEXEC = 1ull << 63
};

#define PGT_FLAGS (X86_64_PG_PRESENT | X86_64_PG_WRITE | X86_64_PG_NOEXEC)

struct page;
extern const uint64_t PAGE_OFFSET;

#define STRUCTPAGE_SIZEOF (sizeof(uint64_t) * 5)
