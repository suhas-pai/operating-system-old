/*
 * kernel/arch/x86_64/mm/walker.h
 * © suhas pai
 */

#pragma once
#include "page.h"

#define PGWALKER_CLEAR 0
#define PGWALKER_DONE -1

struct pgt_walker {
    // tables and indices are stored in reverse-order.
    // i.e. tables[0] is the top level

    pte_t *tables[PGT_LEVEL_COUNT];
    int16_t indices[PGT_LEVEL_COUNT];

    // level should always start from 1..=PGT_LEVEL_COUNT.
    int16_t level;
    uint8_t top_level;
};

void pgtwalker_default(struct pgt_walker *walker, uint64_t virt_addr);
struct pagemap;

void
pgtwalker_create(struct pgt_walker *walker,
                 struct pagemap *pagemap,
                 uint64_t virt_addr);

void
pgtwalker_create_customroot(struct pgt_walker *walker,
                            uint64_t virt_addr,
                            uint64_t root_phys);

enum pgt_walker_result {
    E_PGT_WALKER_OK,
    E_PGT_WALKER_REACHED_END,
    E_PGT_WALKER_ALLOC_FAIL,
    E_PGT_WALKER_BAD_INCR
};

pte_t *pgtwalker_table_for_level(struct pgt_walker *walker, uint8_t level);

// Get the pte that points to the table of level
pte_t *pgtwalker_pte_in_level(struct pgt_walker *walker, uint8_t level);

int16_t
pgtwalker_table_index_for_level(struct pgt_walker *walker, uint8_t level);

uint16_t pgtwalker_index_into_array(struct pgt_walker *walker, uint8_t level);

enum pgt_walker_result pgtwalker_next(struct pgt_walker *walker);
enum pgt_walker_result
pgtwalker_next_custom(struct pgt_walker *walker,
                      uint8_t level,
                      bool alloc_if_not_present,
                      bool alloc_parents_if_not_present);