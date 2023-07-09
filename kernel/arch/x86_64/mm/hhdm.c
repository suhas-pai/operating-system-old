/*
 * kernel/arch/x86_64/mm/hhdm.c
 * © suhas pai
 */

#include "lib/assert.h"
#include "lib/overflow.h"

#include "limine.h"

extern volatile struct limine_hhdm_request hhdm_request;

void *phys_to_virt(const uint64_t phys) {
    return (void *)chk_add_overflow_assert(hhdm_request.response->offset, phys);
}

uint64_t virt_to_phys(const void *const virt) {
    assert((uint64_t)virt >= hhdm_request.response->offset);
    return (uint64_t)virt - hhdm_request.response->offset;
}
