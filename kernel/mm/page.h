/*
 * kernel/mm/page.h
 * © suhas pai
 */

#pragma once
#include "lib/refcount.h"

#include "mm/section.h"
#include "mm/slab.h"

#include "mm_types.h"

struct page {
    _Atomic uint64_t flags;
    union {
        struct {
            struct list freelist;
            uint8_t order;
        } buddy;
        struct {
            union {
                struct list lru;
                struct page *head;
            };

            // An order of 0xFF means this is a tail-page of a page in the lru
            // cache.
            uint32_t amount;
        } dirty_lru;
        struct {
            struct slab_allocator *allocator;
            union {
                struct {
                    struct list slab_list;

                    uint32_t free_obj_count;
                    uint32_t first_free_index;
                } head;
                struct {
                    struct page *head;
                } tail;
            };
        } slab;
        struct {
            struct refcount refcount;
            struct list delayed_free_list;
        } table;
        struct {
            struct refcount page_refcount;
            struct list delayed_free_list;

            struct refcount refcount;
            pgt_level_t level;
        } largehead;
        struct {
            struct refcount refcount;
            struct page *head;
        } largetail;
        struct {
            struct refcount refcount;
            struct list delayed_free_list;
        } used;
    };
};

_Static_assert(sizeof(struct page) == SIZEOF_STRUCTPAGE,
               "SIZEOF_STRUCTPAGE is incorrect");

enum struct_page_flags {
    PAGE_NOT_USABLE = 1 << 0,
    PAGE_IN_FREE_LIST = 1 << 1,
    PAGE_IN_LRU_CACHE = 1 << 2,
    PAGE_IS_SLAB_HEAD = 1 << 3,
    PAGE_IS_LARGE_HEAD = 1 << 4,
    PAGE_IN_LARGE_PAGE = 1 << 5,
    PAGE_IS_TABLE = 1 << 6,
    PAGE_IS_DIRTY = 1 << 7,
};

uint32_t page_get_flags(const struct page *page);

void page_set_bit(struct page *page, enum struct_page_flags flag);
bool page_has_bit(const struct page *page, enum struct_page_flags flag);
void page_clear_bit(struct page *page, enum struct_page_flags flag);

page_section_t page_get_section(const struct page *page);
struct mm_section *page_to_mm_section(const struct page *page);

struct page *alloc_table();