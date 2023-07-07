/*
 * kernel/mm/page.h
 */

#pragma once

#include "arch/x86_64/mm/page.h"
#include "lib/refcount.h"
#include "mm/slab.h"

struct page {
    _Atomic uint32_t flags;
    union {
        struct {
            struct list list;
            uint8_t order;
        } buddy;
        struct {
            union {
                struct {
                    struct slab_allocator *allocator;
                    struct list slab_list;

                    uint16_t free_obj_count;
                    uint32_t first_free_index;
                } head;
                struct {
                    struct page *head;
                } tail;
            };
        } slab;
        struct {
			struct list delayed_free_list;
            struct refcount refcount;
		} pte;
    };
};

_Static_assert(sizeof(struct page) == STRUCTPAGE_SIZEOF, "");

enum struct_page_flags {
    PAGE_IN_FREELIST = 1 << 0,
    PAGE_SLAB_HEAD = 1 << 1,
    PAGE_NOT_USABLE = 1 << 2
};

#define PAGE_SIZE (1ull << PAGE_SHIFT)
#define LARGEPAGE_SIZE(index) (1ull << LARGEPAGE_SHIFTS[index])

#define PAGE_COUNT(size) (((uint64_t)(size) / PAGE_SIZE))

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

void set_page_bit(struct page *page, enum struct_page_flags flag);
bool has_page_bit(struct page *page, enum struct_page_flags flag);
void clear_page_bit(struct page *page, enum struct_page_flags flag);
uint8_t page_get_section(struct page *page);

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(const void *phys);

void pagezones_init();