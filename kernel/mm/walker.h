/*
 * kernel/arch/x86_64/mm/walker.h
 * © suhas pai
 */

#pragma once
#include "pageop.h"

#define PTWALKER_CLEAR 0
#define PTWALKER_DONE -1

struct pt_walker;

typedef struct page *
(*ptwalker_alloc_pgtable_t)(struct pt_walker *walker, void *cb_info);

typedef void
(*ptwalker_free_pgtable_t)(struct pt_walker *walker,
                           struct page *pt,
                           void *cb_info);

struct pt_walker {
    // tables and indices are stored in reverse-order.
    // i.e. tables[0] is the top level

    pte_t *tables[PGT_LEVEL_COUNT];
    uint16_t indices[PGT_LEVEL_COUNT];

    // level should always start from 1..=PGT_LEVEL_COUNT.
    int16_t level;
    uint8_t top_level;

    ptwalker_alloc_pgtable_t alloc_pgtable;
    ptwalker_free_pgtable_t free_pgtable;
};

// ptwalker with default settings expects a `struct pageop *` to be provided as
// the cb_info for free_pgtable.

void ptwalker_default(struct pt_walker *walker, uint64_t virt_addr);

void
ptwalker_default_with_root(struct pt_walker *walker,
                           uint64_t root_phys,
                           uint64_t virt_addr);

void
ptwalker_create(struct pt_walker *walker,
                uint64_t virt_addr,
                ptwalker_alloc_pgtable_t alloc_pgtable,
                ptwalker_free_pgtable_t free_pgtable);

void
ptwalker_create_customroot(struct pt_walker *walker,
                           uint64_t root_phys,
                           uint64_t virt_addr,
                           ptwalker_alloc_pgtable_t alloc_pgtable,
                           ptwalker_free_pgtable_t free_pgtable);

enum pt_walker_result {
    E_PT_WALKER_OK,
    E_PT_WALKER_REACHED_END,
    E_PT_WALKER_ALLOC_FAIL,
    E_PT_WALKER_BAD_INCR
};

pte_t *ptwalker_table_for_level(const struct pt_walker *walker, uint8_t level);
pte_t *ptwalker_pte_in_level(const struct pt_walker *walker, uint8_t level);

uint16_t ptwalker_table_index(const struct pt_walker *walker, uint8_t level);
uint16_t ptwalker_array_index(const struct pt_walker *walker, uint8_t level);

enum pt_walker_result
ptwalker_prev(struct pt_walker *walker, struct pageop *op);

enum pt_walker_result
ptwalker_next(struct pt_walker *walker, struct pageop *op);

enum pt_walker_result
ptwalker_prev_custom(struct pt_walker *walker,
                     uint8_t level,
                     bool alloc_parents,
                     bool alloc_level,
                     bool should_ref,
                     void *alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info);

enum pt_walker_result
ptwalker_next_custom(struct pt_walker *walker,
                     uint8_t level,
                     bool alloc_parents,
                     bool alloc_level,
                     bool should_ref,
                     void *alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info);

void ptwalker_fill_in_lowest(struct pt_walker *walker, struct page *page);

void
ptwalker_deref_from_level(struct pt_walker *walker,
                          uint8_t level,
                          void *free_pgtable_cb_info);

enum pt_walker_result
ptwalker_fill_in_to(struct pt_walker *walker,
                    uint8_t level,
                    bool should_ref,
                    void *alloc_pgtable_cb_info,
                    void *free_pgtable_cb_info);