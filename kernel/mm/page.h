/*
 * kernel/mm/page.h
 * © suhas pai
 */

#pragma once

#include "lib/refcount.h"
#include "mm/slab.h"

#include "types.h"

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

void set_page_bit(struct page *page, enum struct_page_flags flag);
bool has_page_bit(struct page *page, enum struct_page_flags flag);
void clear_page_bit(struct page *page, enum struct_page_flags flag);

uint8_t page_get_section(struct page *page);