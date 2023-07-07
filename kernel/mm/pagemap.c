/*
 * kernel/mm/pagemap.c
 * © suhas pai
 */

#include "pagemap.h"

struct pagemap pagemap_create(struct page *const root) {
    struct pagemap result = {
        .root = root,
    };

    list_init(&result.vma_list);
    refcount_init(&result.refcount);

    return result;
}