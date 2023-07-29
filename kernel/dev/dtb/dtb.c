/*
 * kernel/dev/dtb/dtb.c
 * © suhas pai
 */

#include "fdt/libfdt.h"
#include "dtb.h"

bool
dtb_get_string_prop(const void *const dtb,
                    const int nodeoff,
                    const char *const key,
                    struct string_view *const sv_out)
{
    int name_length_max = 0;
    const struct fdt_property *const prop =
        fdt_get_property(dtb, nodeoff, key, &name_length_max);

    if (prop == NULL) {
        return false;
    }

    *sv_out =
        sv_create_length(prop->data,
                         strnlen(prop->data, (uint64_t)name_length_max));
    return true;
}

bool
dtb_get_array_prop(const void *const dtb,
                   const int nodeoff,
                   const char *const key,
                   const fdt32_t **const data_out,
                   uint32_t *const length_out)
{
    int length = 0;
    const struct fdt_property *const prop =
        fdt_get_property(dtb, nodeoff, key, &length);

    if (prop == NULL) {
        return false;
    }

    *data_out = (const fdt32_t *)(uint64_t)prop->data;
    *length_out = (uint32_t)length / sizeof(fdt32_t);

    return true;
}

int dtb_node_get_parent(const void *const dtb, const int nodeoff) {
    int parents[32] = {0};
    int offset = 0;
    int depth = 0;

    while (offset >= 0 && offset <= nodeoff) {
        if (offset == nodeoff) {
            return parents[depth - 1];
        }

        int new_depth = depth;
        int new_offset = fdt_next_node(dtb, offset, &new_depth);

        if (new_depth >= depth) {
            if (depth >= 32) {
                return -FDT_ERR_NOTFOUND;
            }

            parents[depth] = offset;
        }

        offset = new_offset;
        depth = new_depth;
    }

    if (offset == -FDT_ERR_NOTFOUND || offset >= 0) {
        return -FDT_ERR_BADOFFSET;
    }

    if (offset == -FDT_ERR_BADOFFSET) {
        return -FDT_ERR_BADSTRUCTURE;
    }

    return offset; /* error from fdt_next_node() */
}

bool
dtb_get_reg_entries(const void *const dtb,
                    const int nodeoff,
                    const uint32_t start_index,
                    uint32_t *const entry_count_in,
                    struct dtb_reg_entry *const entries_out)
{
    const int parent_off = dtb_node_get_parent(dtb, nodeoff);
    if (parent_off < 0) {
        return false;
    }

    const int addr_cells = fdt_address_cells(dtb, parent_off);
    const int size_cells = fdt_size_cells(dtb, parent_off);

    if (addr_cells < 0 || size_cells < 0) {
        return false;
    }

    const fdt32_t *regs = NULL;
    uint32_t regs_length = 0;

    if (!dtb_get_array_prop(dtb, nodeoff, "reg", &regs, &regs_length)) {
        return false;
    }

    if (regs_length == 0) {
        return false;
    }

    if ((regs_length % (uint32_t)(addr_cells + size_cells)) != 0) {
        return false;
    }

    const fdt32_t *regs_iter =
        regs + ((uint32_t)(addr_cells + size_cells) * start_index);

    const uint32_t entry_spaces = *entry_count_in;
    const uint32_t addr_shift = sizeof_bits(uint64_t) / (uint32_t)addr_cells;
    const uint32_t size_shift = sizeof_bits(uint64_t) / (uint32_t)size_cells;

    for (struct dtb_reg_entry *entry = entries_out;
         entry != entries_out + entry_spaces;
         entry++)
    {
        if (addr_shift != sizeof_bits(uint64_t)) {
            for (int i = 0; i != addr_cells; i++) {
                entry->address =
                    entry->address << addr_shift | fdt32_to_cpu(*regs_iter);
                regs_iter++;
            }
        } else {
            entry->address = fdt32_to_cpu(*regs_iter);
            regs_iter++;
        }

        if (addr_shift != sizeof_bits(uint64_t)) {
            for (int i = 0; i != size_cells; i++) {
                entry->size =
                    entry->size << size_shift | fdt32_to_cpu(*regs_iter);
                regs_iter++;
            }
        } else {
            entry->size = fdt32_to_cpu(*regs_iter);
            regs_iter++;
        }
    }

    return true;
}