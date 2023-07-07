/*
 * kernel/arch/x86_64/mm/iter.c
 * © suhas pai
 */

#include "asm/regs.h"
#include "lib/align.h"

#include "limine.h"
#include "mm/pagemap.h"

#include "iter.h"

extern struct limine_paging_mode_request paging_mode_request;

void pagemap_iterstart(struct pagemap_iter *const iter, const uint64_t virt) {
    return pagemap_iterstart_custom_root(iter,
                                         virt,
                                         /*root_phys=*/read_cr3());
}

void
pagemap_iter_create(struct pagemap_iter *const iter,
                    struct pagemap *const pagemap,
                    const uint64_t virt_addr)
{
    return pagemap_iterstart_custom_root(iter,
                                         virt_addr,
                                         page_to_phys(pagemap->root));
}

static inline void handle_pml1_changed(struct pagemap_iter *const iter) {
    const uint64_t pml1_entry = iter->pml1[iter->pml1_index];
    if (!(pml1_entry & X86_64_PG_PRESENT)) {
        return;
    }

    iter->level = PGT_LEVEL_PML1;
}

static inline void handle_pml2_changed(struct pagemap_iter *const iter) {
    const uint64_t pml2_entry = iter->pml2[iter->pml2_index];
    if (!(pml2_entry & X86_64_PG_PRESENT)) {
        iter->pml1 = NULL;
        return;
    }

    iter->pml1 = phys_to_virt(pml2_entry & PG_PHYS_MASK);
    iter->level = PGT_LEVEL_PML2;

    handle_pml1_changed(iter);
}

static inline void handle_pml3_changed(struct pagemap_iter *const iter) {
    const uint64_t pml3_entry = iter->pml3[iter->pml3_index];
    if (!(pml3_entry & X86_64_PG_PRESENT)) {
        iter->pml2 = NULL;
        iter->pml1 = NULL;

        return;
    }

    iter->pml2 = phys_to_virt(pml3_entry & PG_PHYS_MASK);
    iter->level = PGT_LEVEL_PML3;

    handle_pml2_changed(iter);
}

static inline void handle_pml4_changed(struct pagemap_iter *const iter) {
    const uint64_t pml4_entry = iter->pml4[iter->pml4_index];
    if (!(pml4_entry & X86_64_PG_PRESENT)) {
        if (iter->has_5lvl_paging) {
            iter->level = PGT_LEVEL_PML5;
        } else {
            iter->level = PGT_LEVEL_CLEAR;
        }

        iter->pml3 = NULL;
        iter->pml2 = NULL;
        iter->pml1 = NULL;

        return;
    }

    iter->pml3 = phys_to_virt(pml4_entry & PG_PHYS_MASK);
    iter->level = PGT_LEVEL_PML4;

    handle_pml3_changed(iter);
}

void
pagemap_iterstart_custom_root(struct pagemap_iter *const iter,
                              const uint64_t virt_addr,
                              const uint64_t root_phys)
{
    assert(has_align(virt_addr, PAGE_SIZE));
    assert(has_align(root_phys, PAGE_SIZE));

    iter->pml1_index = PML1(virt_addr);
    iter->pml2_index = PML2(virt_addr);
    iter->pml3_index = PML3(virt_addr);
    iter->pml4_index = PML4(virt_addr);
    iter->pml5_index = PML5(virt_addr);

    const bool has_5lvl_paging =
        paging_mode_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL;

    if (has_5lvl_paging) {
        iter->pml5 = phys_to_virt(root_phys);
        iter->pml4 = phys_to_virt(iter->pml5[iter->pml5_index] & PG_PHYS_MASK);
    } else {
        iter->pml4 = phys_to_virt(root_phys);
    }

    iter->has_5lvl_paging = has_5lvl_paging;
    handle_pml4_changed(iter);
}

enum pagemap_iter_result pagemap_iternext(struct pagemap_iter *const iter) {
    return pagemap_iternext_custom(iter,
                                   /*alloc_if_not_present=*/true,
                                   /*alloc_parents_if_not_present=*/true);
}

enum pagemap_iter_result
pagemap_iternext_custom(struct pagemap_iter *const iter,
                        const bool alloc_if_not_present,
                        const bool alloc_parents_if_not_present)
{
    switch (iter->level) {
        case PGT_LEVEL_PML1:
            return pagemap_iternext_pml1(iter,
                                         alloc_if_not_present,
                                         alloc_parents_if_not_present);
        case PGT_LEVEL_PML2:
            return pagemap_iternext_pml2(iter,
                                         alloc_if_not_present,
                                         alloc_parents_if_not_present);
        case PGT_LEVEL_PML3:
            return pagemap_iternext_pml3(iter,
                                         alloc_if_not_present,
                                         alloc_parents_if_not_present);
        case PGT_LEVEL_PML4:
            return pagemap_iternext_pml4(iter,
                                         alloc_if_not_present,
                                         alloc_parents_if_not_present);
        case PGT_LEVEL_PML5:
            return pagemap_iternext_pml5(iter, alloc_if_not_present);
        case PGT_LEVEL_CLEAR:
            panic("pagemap_iternext(): entire pagemap is still cleared. "
                  "Did you reset level?");
        case PGT_LEVEL_DONE:
            return E_PAGEMAP_ITER_REACHED_END;
    }

    verify_not_reached("pagemap_iternext(): Invalid pgt level");
}

enum pagemap_iter_result
pagemap_iternext_pml1(struct pagemap_iter *const iter,
                      const bool alloc_if_not_present,
                      const bool alloc_parents_if_not_present)
{
    if (iter->pml1 == NULL) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    iter->pml1_index++;
    if (iter->pml1_index == PGT_COUNT) {
        const enum pagemap_iter_result result =
            pagemap_iternext_pml2(
                iter,
                /*alloc_if_not_present=*/alloc_parents_if_not_present,
                alloc_parents_if_not_present);

        if (result != E_PAGEMAP_ITER_OK || iter->level != PGT_LEVEL_PML1) {
            return result;
        }
    }

    if (!alloc_if_not_present) {
        return E_PAGEMAP_ITER_OK;
    }

    const uint64_t pml1_entry = iter->pml1[iter->pml1_index];
    if (pml1_entry & X86_64_PG_PRESENT) {
        return E_PAGEMAP_ITER_OK;
    }

    /* TODO: Alloc */
    return E_PAGEMAP_ITER_OK;
}

enum pagemap_iter_result
pagemap_iternext_pml2(struct pagemap_iter *const iter,
                      const bool alloc_if_not_present,
                      const bool alloc_parents_if_not_present)
{
    if (iter->pml2 == NULL) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    iter->pml2_index++;
    if (iter->pml2_index == PGT_COUNT) {
        const enum pagemap_iter_result result =
            pagemap_iternext_pml3(
                iter,
                /*alloc_if_not_present=*/alloc_parents_if_not_present,
                alloc_parents_if_not_present);

        if (result != E_PAGEMAP_ITER_OK || iter->level != PGT_LEVEL_PML2) {
            return result;
        }
    } else {
        iter->pml1_index = 0;
    }

    const uint64_t pml2_entry = iter->pml2[iter->pml2_index];
    if (!(pml2_entry & X86_64_PG_PRESENT)) {
        if (!alloc_if_not_present) {
            iter->level = PGT_LEVEL_PML2;
            iter->pml1 = NULL;

            return E_PAGEMAP_ITER_OK;
        }

        /*TODO:*/
        return E_PAGEMAP_ITER_OK;
    }

    iter->pml1 = phys_to_virt(pml2_entry & PG_PHYS_MASK);
    handle_pml1_changed(iter);

    return E_PAGEMAP_ITER_OK;
}

enum pagemap_iter_result
pagemap_iternext_pml3(struct pagemap_iter *const iter,
                      const bool alloc_if_not_present,
                      const bool alloc_parents_if_not_present)
{
    if (iter->pml3 == NULL) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    iter->pml3_index++;
    if (iter->pml3_index == PGT_COUNT) {
        const enum pagemap_iter_result result =
            pagemap_iternext_pml4(
                iter,
                /*alloc_if_not_present=*/alloc_parents_if_not_present,
                alloc_parents_if_not_present);

        if (result != E_PAGEMAP_ITER_OK || iter->level != PGT_LEVEL_PML3) {
            return result;
        }
    } else {
        iter->pml1_index = 0;
        iter->pml2_index = 0;
    }

    const uint64_t pml3_entry = iter->pml3[iter->pml3_index];
    if (!(pml3_entry & X86_64_PG_PRESENT)) {
        if (!alloc_if_not_present) {
            iter->level = PGT_LEVEL_PML3;

            iter->pml1 = NULL;
            iter->pml2 = NULL;

            return E_PAGEMAP_ITER_OK;
        }

        /*TODO:*/
        return E_PAGEMAP_ITER_OK;
    }

    iter->pml2 = phys_to_virt(pml3_entry & PG_PHYS_MASK);
    iter->level = PGT_LEVEL_PML2;

    handle_pml2_changed(iter);
    return E_PAGEMAP_ITER_OK;
}

enum pagemap_iter_result
pagemap_iternext_pml4(struct pagemap_iter *const iter,
                      const bool alloc_if_not_present,
                      const bool alloc_parents_if_not_present)
{
    if (iter->pml4 == NULL) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    iter->pml4_index++;
    if (iter->pml4_index == PGT_COUNT) {
        const enum pagemap_iter_result result =
            pagemap_iternext_pml5(iter, alloc_parents_if_not_present);

        if (result != E_PAGEMAP_ITER_OK || iter->level != PGT_LEVEL_PML4) {
            return result;
        }
    } else {
        iter->pml1_index = 0;
        iter->pml2_index = 0;
        iter->pml3_index = 0;
    }

    const uint64_t pml4_entry = iter->pml4[iter->pml4_index];
    if (!(pml4_entry & X86_64_PG_PRESENT)) {
        if (!alloc_if_not_present) {
            iter->level = PGT_LEVEL_PML4;

            iter->pml1 = NULL;
            iter->pml2 = NULL;
            iter->pml3 = NULL;

            return E_PAGEMAP_ITER_OK;
        }

        /*TODO:*/
        return E_PAGEMAP_ITER_OK;
    }

    iter->pml3 = phys_to_virt(pml4_entry & PG_PHYS_MASK);
    iter->level = PGT_LEVEL_PML3;

    handle_pml3_changed(iter);
    return E_PAGEMAP_ITER_OK;
}

enum pagemap_iter_result
pagemap_iternext_pml5(struct pagemap_iter *const iter,
                      const bool alloc_if_not_present)
{
    if (!iter->has_5lvl_paging) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    if (iter->pml5 == NULL) {
        return E_PAGEMAP_ITER_BAD_INCREMENT;
    }

    iter->pml5_index++;

    iter->pml1_index = 0;
    iter->pml2_index = 0;
    iter->pml3_index = 0;
    iter->pml4_index = 0;

    if (iter->pml5_index == PGT_COUNT) {
        iter->pml1 = NULL;
        iter->pml2 = NULL;
        iter->pml3 = NULL;
        iter->pml4 = NULL;

        iter->level = PGT_LEVEL_DONE;
        return E_PAGEMAP_ITER_REACHED_END;
    }

    iter->level = PGT_LEVEL_PML5;
    const uint64_t pml5_entry = iter->pml5[iter->pml5_index];

    if (!(pml5_entry & X86_64_PG_PRESENT)) {
        if (!alloc_if_not_present) {
            iter->pml1 = NULL;
            iter->pml2 = NULL;
            iter->pml3 = NULL;
            iter->pml4 = NULL;

            return E_PAGEMAP_ITER_OK;
        }

        /*TODO:*/
        return E_PAGEMAP_ITER_OK;
    }

    iter->pml4 = phys_to_virt(pml5_entry & PG_PHYS_MASK);
    handle_pml4_changed(iter);

    return E_PAGEMAP_ITER_OK;
}
