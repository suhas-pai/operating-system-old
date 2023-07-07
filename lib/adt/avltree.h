/*
 * lib/adt/avltree.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct avlnode {
    struct avlnode *parent;
    struct avlnode *left;
    struct avlnode *right;

    uint32_t height;
};

struct avltree {
    struct avlnode *root;
};

typedef int (*avlnode_compare_t)(struct avlnode *ours, struct avlnode *theirs);
typedef int (*avlnode_compare_key_t)(struct avlnode *theirs, void *key);

typedef void (*avlnode_update_t)(struct avlnode *node);

void avlnode_merge(struct avlnode *left, struct avlnode *right);

void
avltree_insert(struct avltree *tree,
               struct avlnode *node,
               avlnode_compare_t comparator,
               avlnode_update_t update);

void
avltree_insert_at_loc(struct avltree *tree,
                      struct avlnode *node,
                      struct avlnode *parent,
                      struct avlnode **link,
                      avlnode_update_t update);

void
avltree_delete(struct avltree *tree,
               void *key,
               avlnode_compare_key_t identifier,
               avlnode_update_t update);

void
avltree_delete_node(struct avltree *tree,
                    struct avlnode *node,
                    avlnode_update_t update);
