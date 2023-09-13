/*
 * kernel/mm/page.h
 * © suhas pai
 */

#pragma once

#include "lib/refcount.h"
#include "mm/slab.h"

#include "mm_types.h"

struct page {
    _Atomic uint32_t flags;
    union {
        struct {
            struct list freelist;
            uint8_t order;
        } buddy;
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
            struct list delayed_free_list;
            struct refcount refcount;
        } table;
    };
};

_Static_assert(sizeof(struct page) == SIZEOF_STRUCTPAGE,
               "SIZEOF_STRUCTPAGE is incorrect");

enum struct_page_flags {
    PAGE_NOT_USABLE = 1 << 0,
    PAGE_IN_FREE_LIST = 1 << 1,
    PAGE_IS_SLAB_HEAD = 1 << 2,
    PAGE_IS_LARGE_HEAD = 1 << 3,
};

uint32_t page_get_flags(const struct page *page);

void page_set_bit(struct page *page, enum struct_page_flags flag);
bool page_has_bit(const struct page *page, enum struct_page_flags flag);
void page_clear_bit(struct page *page, enum struct_page_flags flag);

page_section_t page_get_section(const struct page *page);
struct page *alloc_table();