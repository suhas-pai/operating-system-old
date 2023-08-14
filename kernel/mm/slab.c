/*
 * kernel/mm/slab.c
 * © suhas pai
 */

#include "cpu/panic.h"
#include "cpu/spinlock.h"

#include "lib/align.h"
#include "lib/overflow.h"
#include "lib/string.h"

#include "mm/page.h"
#include "mm/page_alloc.h"

#include "slab.h"

// Use space inside free slab objects to create a singly-linked list.
struct free_slab_object {
    uint32_t next;
};

bool
slab_allocator_init(struct slab_allocator *const slab_alloc,
                    uint32_t object_size_arg,
                    const uint32_t alloc_flags)
{
    uint64_t object_size = object_size_arg;

    // To store free_page_objects, every object must be at least 8 bytes large.
    // We also make this the required minimum alignment.

    if (!align_up(object_size,
                  /*boundary=*/sizeof(struct free_slab_object),
                  &object_size))
    {
        return false;
    }

    spinlock_init(&slab_alloc->lock);
    list_init(&slab_alloc->slab_head_list);

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

static struct page *alloc_slab_page(struct slab_allocator *const alloc) {
    struct page *const head =
        alloc_pages(__ALLOC_SLAB_HEAD | __ALLOC_ZERO, alloc->slab_order);

    if (head == NULL) {
        return NULL;
    }

    const struct page *const end = head + alloc->slab_order;
    for (struct page *page = head + 1; page < end; page++) {
        page->slab.tail.head = head;
    }

    list_add(&alloc->slab_head_list, &head->slab.head.slab_list);

    head->slab.head.allocator = alloc;
    head->slab.head.first_free_index = 0;
    head->slab.head.free_obj_count = alloc->object_count_per_slab;

    alloc->slab_count++;
    alloc->free_obj_count += alloc->object_count_per_slab;

    void *const page_virt = page_to_virt(head);
    uint64_t object_byte_index = 0;

    for (uint32_t i = 0;
         i != alloc->object_count_per_slab - 1;
         i++, object_byte_index += alloc->object_size)
    {
        struct free_slab_object *const free_obj =
            (struct free_slab_object *)(page_virt + object_byte_index);

        free_obj->next = i + 1;
    }

    ((struct free_slab_object *)(page_virt + object_byte_index))->next =
        UINT32_MAX;

    return head;
}

static inline uint64_t
get_free_index(struct page *const page,
               struct slab_allocator *const alloc,
               struct free_slab_object *const free_object)
{
    return distance(page_to_virt(page), free_object) / alloc->object_size;
}

static inline void *
get_free_ptr(struct page *const page, struct slab_allocator *const alloc) {
    if (page->slab.head.first_free_index == UINT32_MAX) {
        return NULL;
    }

    const uint64_t byte_index =
        check_mul_assert(page->slab.head.first_free_index, alloc->object_size);

    return page_to_virt(page) + byte_index;
}

void *slab_alloc(struct slab_allocator *const alloc) {
    const int flag = spin_acquire_with_irq(&alloc->lock);
    struct page *page = NULL;

    if (list_empty(&alloc->slab_head_list)) {
        page = alloc_slab_page(alloc);
        if (page == NULL) {
            spin_release_with_irq(&alloc->lock, flag);
            return NULL;
        }
    } else {
        page =
            list_head(&alloc->slab_head_list, struct page, slab.head.slab_list);
    }

    alloc->free_obj_count--;
    page->slab.head.free_obj_count--;

    if (page->slab.head.free_obj_count == 0) {
        list_delete(&page->slab.head.slab_list);
    }

    struct free_slab_object *const result = get_free_ptr(page, alloc);
    page->slab.head.first_free_index = result->next;

    spin_release_with_irq(&alloc->lock, flag);
    return result;
}

static inline struct page *slab_head_of(const void *const mem) {
    struct page *const page = virt_to_page(mem);
    if (page_has_bit(page, PAGE_IS_SLAB_HEAD)) {
        return page;
    }

    return page->slab.tail.head;
}

void slab_free(void *const mem) {
    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const alloc = head->slab.head.allocator;

    bzero(mem, alloc->object_size);
    const int flag = spin_acquire_with_irq(&alloc->lock);

    alloc->free_obj_count += 1;
    head->slab.head.free_obj_count += 1;

    if (head->slab.head.free_obj_count != 1) {
        if (head->slab.head.free_obj_count == alloc->object_count_per_slab) {
            list_delete(&head->slab.head.slab_list);

            alloc->free_obj_count -= alloc->object_count_per_slab;
            alloc->slab_count -= 1;

            free_pages(head, alloc->slab_order);
            spin_release_with_irq(&alloc->lock, flag);

            return;
        }
    } else {
        // This was previously a fully used slab, so we have to add this back
        // to our list of slabs.

        list_add(&alloc->slab_head_list, &head->slab.head.slab_list);
    }

    struct free_slab_object *const free_obj = (struct free_slab_object *)mem;

    free_obj->next = head->slab.head.first_free_index;
    head->slab.head.first_free_index = get_free_index(head, alloc, mem);

    spin_release_with_irq(&alloc->lock, flag);
}

uint32_t slab_object_size(void *const mem) {
    if (mem == NULL) {
        panic("slab_object_size(): Got mem=NULL");
    }

    struct page *const head = slab_head_of(mem);
    struct slab_allocator *const allocator = head->slab.head.allocator;

    return allocator->object_size;
}