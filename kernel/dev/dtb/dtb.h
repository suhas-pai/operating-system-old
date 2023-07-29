/*
 * kernel/dev/dtb/dtb.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/string_view.h"
#include "fdt/libfdt_env.h"

#define for_each_dtb_compat(offset, name) \
    for (int offset = fdt_node_offset_by_compatible(dtb, -1, name); \
         offset >= 0; \
         offset = fdt_node_offset_by_compatible(dtb, offset, name))

bool
dtb_get_string_prop(const void *dtb,
                    int nodeoff,
                    const char *key,
                    struct string_view *sv_out);

bool
dtb_get_array_prop(const void *dtb,
                   int nodeoff,
                   const char *key,
                   const fdt32_t **data_out,
                   uint32_t *const length_out);

int dtb_node_get_parent(const void *const dtb, const int nodeoff);

struct dtb_addr_size_pair {
    uint64_t address;
    uint64_t size;
};

bool
dtb_get_reg_pairs(const void *dtb,
                  int nodeoff,
                  uint32_t start_index,
                  uint32_t *entry_count_in,
                  struct dtb_addr_size_pair *pairs_out);