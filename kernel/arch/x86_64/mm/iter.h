/*
 * kernel/arch/x86_64/mm/iter.h
 * © suhas pai
 */

#pragma once
#include "page.h"

enum pagemap_iter_level {
    PGT_LEVEL_PML1,
    PGT_LEVEL_PML2,
    PGT_LEVEL_PML3,
    PGT_LEVEL_PML4,
    PGT_LEVEL_PML5,

    PGT_LEVEL_CLEAR,
    PGT_LEVEL_DONE,
};

struct pagemap_iter {
    uint64_t *pml5;
    uint64_t *pml4;
    uint64_t *pml3;
    uint64_t *pml2;
    uint64_t *pml1;

    uint16_t pml1_index;
    uint16_t pml2_index;
    uint16_t pml3_index;
    uint16_t pml4_index;
    uint16_t pml5_index;

    enum pagemap_iter_level level : 3;
    bool has_5lvl_paging : 1;
};

void pagemap_iterstart(struct pagemap_iter *iter, uint64_t virt_addr);
struct pagemap;

void
pagemap_iter_create(struct pagemap_iter *iter,
                    struct pagemap *pagemap,
                    uint64_t virt_addr);

void
pagemap_iterstart_custom_root(struct pagemap_iter *iter,
                              uint64_t virt_addr,
                              uint64_t root_phys);

enum pagemap_iter_result {
    E_PAGEMAP_ITER_OK,
    E_PAGEMAP_ITER_REACHED_END,
    E_PAGEMAP_ITER_ALLOC_FAIL,
    E_PAGEMAP_ITER_BAD_INCREMENT
};

enum pagemap_iter_result pagemap_iternext(struct pagemap_iter *iter);
enum pagemap_iter_result
pagemap_iternext_custom(struct pagemap_iter *iter,
                        bool alloc_if_not_present,
                        bool alloc_parents_if_not_present);

enum pagemap_iter_result
pagemap_iternext_pml1(struct pagemap_iter *iter,
                      bool alloc_if_not_present,
                      bool alloc_parents_if_not_present);

enum pagemap_iter_result
pagemap_iternext_pml2(struct pagemap_iter *iter,
                      bool alloc_if_not_present,
                      bool alloc_parents_if_not_present);

enum pagemap_iter_result
pagemap_iternext_pml3(struct pagemap_iter *iter,
                      bool alloc_if_not_present,
                      bool alloc_parents_if_not_present);

enum pagemap_iter_result
pagemap_iternext_pml4(struct pagemap_iter *iter,
                      bool alloc_if_not_present,
                      bool alloc_parents_if_not_present);

enum pagemap_iter_result
pagemap_iternext_pml5(struct pagemap_iter *iter, bool alloc_if_not_present);