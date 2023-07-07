/*
 * kernel/mm/slab.c
 * © suhas pai
 */

#include "cpu/panic.h"
#include "cpu/spinlock.h"

#include "lib/align.h"
#include "lib/string.h"

#include "mm/page.h"
#include "mm/page_alloc.h"

#include "slab.h"

bool
slab_allocator_init(struct slab_allocator *const slab_alloc,
                    uint32_t object_size_arg,
                    const uint32_t alloc_flags)
{
    uint64_t object_size = object_size_arg;

    // To store free_page_objects, every object must be at least 8 bytes large.
    // We also make this the required minimum alignment.

    if (!align_up(object_size, /*boundary=*/8, &object_size)) {
        return false;
    }

    spinlock_init(&slab_alloc->lock);
    list_init(&slab_alloc->slab_list);

    slab_alloc->object_size = object_size;
    slab_alloc->free_obj_count = 0;
    slab_alloc->alloc_flags = alloc_flags;

    uint16_t order = 0;
    const uint8_t min_obj_per_slab = 4;

    for (; (PAGE_SIZE << order) < (object_size * min_obj_per_slab); order++) {}

    slab_alloc->slab_order = order;
    slab_alloc->object_count_per_slab = (PAGE_SIZE << order) / object_size;

    return true;
}

static struct page *alloc_slab_page(struct slab_allocator *const allocator) {
    struct page *const head =
        alloc_pages(__ALLOC_SLAB_HEAD | __ALLOC_ZERO, allocator->slab_order);

    if (head == NULL) {
        return NULL;
    }

    const struct page *const end = head + allocator->slab_order;
    for (struct page *page = head + 1; page < end; page++) {
        page->slab.tail.head = head;
    }

    list_add(&allocator->slab_list, &head->slab.head.slab_list);

    head->slab.head.allocator = allocator;
    head->slab.head.free_obj_count = allocator->object_count_per_slab;

    allocator->slab_count++;
    allocator->free_obj_count += allocator->object_count_per_slab;

    return head;
}

void *slab_alloc(struct slab_allocator *const allocator) {
    const int flag = spin_acquire_with_irq(&allocator->lock);
    struct page *page = NULL;

    if (list_empty(&allocator->slab_list)) {
        page = alloc_slab_page(allocator);
        if (page == NULL) {
            spin_release_with_irq(&allocator->lock, flag);
            return NULL;
        }
    } else {
        page =
            list_head(&allocator->slab_list, struct page, slab.head.slab_list);
    }

    page->slab.head.free_obj_count--;
    if (page->slab.head.free_obj_count == 0) {
        list_delete(&page->slab.head.slab_list);
    }

    spin_release_with_irq(&allocator->lock, flag);

    void *const result = page_to_virt(page) + page->slab.head.first_free_index;
    page->slab.head.first_free_index += allocator->object_size;

    return result;
}

static inline struct page *slab_head_of(const void *const mem) {
    struct page *const page = virt_to_page(mem);
    if (has_page_bit(page, PAGE_SLAB_HEAD)) {
        return page;
    }

    return page->slab.tail.head;
}

// Use space inside free slab objects to create a singly-linked list.
struct free_slab_object {
    struct free_slab_object *next;
};

void slab_free(void *const mem) {
    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const allocator = head->slab.head.allocator;

    memzero(mem, allocator->object_size);
	const int flag = spin_acquire_with_irq(&allocator->lock);

    allocator->free_obj_count += 1;
    head->slab.head.free_obj_count += 1;

    if (head->slab.head.free_obj_count != 1) {
        if (head->slab.head.free_obj_count == allocator->object_count_per_slab)
        {
            list_delete(&head->slab.head.slab_list);

            allocator->free_obj_count -= allocator->object_count_per_slab;
            allocator->slab_count -= 1;

            free_pages(head, allocator->slab_order);
            spin_release_with_irq(&allocator->lock, flag);

            return;
        }
    } else {
        list_add(&allocator->slab_list, &head->slab.head.slab_list);
    }

    struct free_slab_object *const free_obj = (struct free_slab_object *)mem;
    void *const head_virt = page_to_virt(head);

    free_obj->next = head_virt + head->slab.head.first_free_index;
    head->slab.head.first_free_index = (uint64_t)mem - (uint64_t)head_virt;

    spin_release_with_irq(&allocator->lock, flag);
}

uint32_t slab_object_size(void *const mem) {
    if (mem == NULL) {
        panic("slab_object_size(): Got mem==NULL");
    }

    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const allocator = head->slab.head.allocator;

    return allocator->object_size;
}