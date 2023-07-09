/*
 * kernel/mm/pagemap.c
 * © suhas pai
 */

#include "pagemap.h"

struct pagemap kernel_pagemap = {
    .root = NULL, // setup later
    .lock = {},
    .refcount = refcount_create(),
    .vma_tree = {},
    .vma_list = LIST_INIT(kernel_pagemap.vma_list)
};

struct pagemap pagemap_create(struct page *const root) {
    struct pagemap result = {
        .root = root,
    };

    list_init(&result.vma_list);
    refcount_init(&result.refcount);

    return result;
}