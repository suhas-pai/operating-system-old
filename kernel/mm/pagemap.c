/*
 * kernel/mm/pagemap.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#endif /* defined(__x86_64__)*/

#include "lib/align.h"
#include "lib/overflow.h"

#include "pagemap.h"

struct pagemap kernel_pagemap = {
    .root = NULL, // setup later
    .lock = SPINLOCK_INIT(),
    .refcount = refcount_create_max(),
    .vma_tree = {0},
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

#define INVALID_ADDR UINT64_MAX

enum traversal_result {
    TRAVERSAL_DONE,
    TRAVERSAL_CONTINUE,
};

static enum traversal_result
travserse_tree(struct pagemap *const pagemap,
               const struct range in_range,
               struct vm_area *vma,
               const uint64_t size,
               const uint64_t align,
               uint64_t *const result_out,
               struct vm_area **const vma_out,
               struct vm_area **const prev_out)
{
    while (true) {
        struct vm_area *const prev = vma_prev(pagemap, vma);
        const uint64_t prev_end =
            (prev != NULL ? range_get_end_assert(prev->range) : 0);

        // prev_end is the lowest possible address we can find a hole at, so if
        // prev_end is above in_range, then we can't find an acceptable free
        // physical range.

        if (!range_is_loc_above(in_range, prev_end)) {
            *result_out = INVALID_ADDR;
            return TRAVERSAL_DONE;
        }

        const uint64_t aligned_front = align_up_assert(prev_end, align);
        const struct range aligned_range = range_create(aligned_front, size);

        if (range_has(in_range, aligned_range)) {
            *prev_out = prev;
            *result_out = aligned_front;

            return TRAVERSAL_DONE;
        }

        // We fell outside of the free area we found, but we can proceed to the
        // right and find another free area (then from the left).

        if (vma->vma_node.right != NULL) {
            struct vm_area *const right = vma_of(vma->vma_node.right);
            if (right->largest_free_to_prev >= size) {
                *vma_out = right;
                return TRAVERSAL_CONTINUE;
            }
        }

        // We didn't find a right node, so we have to go up the tree to a parent
        // node and proceed with the loop from there.

        while (true) {
            if (vma->vma_node.parent == NULL) {
                // Since we're at the root, we can only see if there's space to
                // our right.

                struct vm_area *const rightmost_vma =
                    container_of(pagemap->vma_list.prev,
                                 struct vm_area,
                                 vma_list);

                uint64_t aligned_result =
                    range_get_end_assert(rightmost_vma->range);

                if (aligned_result >= range_get_end_assert(in_range) &&
                    align_up(aligned_result, align, &aligned_result))
                {
                    *result_out = INVALID_ADDR;
                    return TRAVERSAL_DONE;
                }

                uint64_t end = 0;
                if (check_add(aligned_result, size, &end)) {
                    *result_out = INVALID_ADDR;
                    return TRAVERSAL_DONE;
                }

                *result_out = aligned_result;
                *prev_out = rightmost_vma;

                return TRAVERSAL_DONE;
            }

            // Keep going up the tree until we find the parent of a left node we
            // were previously at.

            struct vm_area *const child = vma;
            vma = vma_of(child->vma_node.parent);

            if (vma->vma_node.left == &child->vma_node) {
                // We've found the parent of such a left node, break out of this
                // look and go through the entire outside loop again.

                break;
            }
        }
    }
}

static uint64_t
find_from_start(struct pagemap *const pagemap,
                const struct range in_range,
                const uint64_t size,
                const uint64_t align,
                struct vm_area **const prev_out)
{
    if (pagemap->vma_tree.root == NULL) {
        return in_range.front;
    }

    // Start from the root of the pagemap, and in a loop, proceed to the
    // leftmost node of the address-space to find a free-area. If one isn't
    // found, or isn't acceptable, proceed to the current node's right. If the
    // current node has no right node, go upwards to the parent.

    struct vm_area *vma = vma_of(pagemap->vma_tree.root);
    while (true) {
        // Move to the very left of the address space to find the left-most free
        // area available.

        if (vma->vma_list.prev != NULL) {
            struct vm_area *const prev = vma_of(vma->vma_list.prev);
            if (prev->largest_free_to_prev >= size) {
                vma = prev;
                continue;
            }
        }

        uint64_t result = 0;
        const enum traversal_result traverse_result =
            travserse_tree(pagemap,
                           in_range,
                           vma,
                           size,
                           align,
                           &result,
                           &vma,
                           prev_out);

        switch (traverse_result) {
            case TRAVERSAL_DONE:
                return result;
            case TRAVERSAL_CONTINUE:
                break;
        }
    }
}

int
vma_avltree_compare(struct avlnode *const our_node,
                    struct avlnode *const their_node)
{
    struct vm_area *const ours = vma_of(our_node);
    struct vm_area *const theirs = vma_of(their_node);

    return range_above(ours->range, theirs->range) ? -1 : 1;
}

void vma_avltree_update(struct avlnode *const node) {
    struct vm_area *const vma = vma_of(node);
    struct vm_area *const prev = vma_prev(vma->pagemap, vma);

    const uint64_t prev_end =
        prev != NULL ? range_get_end_assert(prev->range) : 0;

    uint64_t largest_free_to_prev = distance(prev_end, vma->range.front);
    if (vma->vma_node.left != NULL) {
        struct vm_area *const left = vma_of(&vma->vma_node.left);
        largest_free_to_prev =
            min(largest_free_to_prev, left->largest_free_to_prev);
    }

    if (vma->vma_node.right != NULL) {
        struct vm_area *const right = vma_of(&vma->vma_node.right);
        largest_free_to_prev =
            min(largest_free_to_prev, right->largest_free_to_prev);
    }

    vma->largest_free_to_prev = largest_free_to_prev;
}

bool
pagemap_find_space_and_add_vma(struct pagemap *const pagemap,
                               struct vm_area *const vma,
                               const struct range in_range,
                               const uint64_t phys_addr,
                               const uint64_t align)
{
    const int flag = spin_acquire_with_irq(&pagemap->lock);

    struct vm_area *prev = NULL;
    const uint64_t addr =
        find_from_start(pagemap, in_range, vma->range.size, align, &prev);

    if (addr == INVALID_ADDR) {
        spin_release_with_irq(&pagemap->lock, flag);
        return false;
    }

    list_add(&prev->vma_list, &vma->vma_list);
    avltree_insert_at_loc(&pagemap->vma_tree,
                          &vma->vma_node,
                          &prev->vma_node,
                          &prev->vma_node.right,
                          vma_avltree_update);

    if (vma->prot != PROT_NONE) {
        const bool map_result =
            arch_make_mapping(pagemap,
                            phys_addr,
                            vma->range.front,
                            vma->range.size,
                            vma->prot,
                            vma->cachekind,
                            /*is_overwrite=*/false);

        spin_release_with_irq(&pagemap->lock, flag);
        if (!map_result) {
            return false;
        }
    } else {
        spin_release_with_irq(&pagemap->lock, flag);
    }

    return true;
}

bool
pagemap_add_vma_at(struct pagemap *const pagemap,
                   struct vm_area *const vma,
                   const struct range in_range,
                   const uint64_t phys_addr,
                   const uint64_t align)
{
    const int flag = spin_acquire_with_irq(&pagemap->lock);

    /*
    list_add(&prev->vma_list, &vma->vma_list);
    avltree_insert_at_loc(&pagemap->vma_tree,
                          &vma->vma_node,
                          &prev->vma_node,
                          &prev->vma_node.right,
                          vma_avltree_update);*/

    if (vma->prot != PROT_NONE) {
        const bool map_result =
            arch_make_mapping(pagemap,
                            phys_addr,
                            vma->range.front,
                            vma->range.size,
                            vma->prot,
                            vma->cachekind,
                            /*is_overwrite=*/false);

        spin_release_with_irq(&pagemap->lock, flag);
        if (!map_result) {
            return false;
        }
    } else {
        spin_release_with_irq(&pagemap->lock, flag);
    }

    (void)in_range;
    (void)align;

    return true;
}

void switch_to_pagemap(const struct pagemap *const pagemap) {
    assert(pagemap->root != NULL);

#if defined(__x86_64__)
    write_cr3(page_to_phys(pagemap->root));
#endif /* defined(__x86_64__) */
}