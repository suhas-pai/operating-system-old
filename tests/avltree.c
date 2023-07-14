/*
 * tests/avltree.c
 * © suhas pai
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/adt/avltree.h"

struct node {
    struct avlnode info;
    uint32_t number;
};

static void print_node(struct node *const node, const uint64_t spaces) {
    for (uint64_t i = 0; i != spaces; i++) {
        printf(" ");
    }

    if (!node) {
        printf("(null)\n");
        return;
    }

    printf("%" PRIu32 "\n", node->number);

    print_node((struct node *)node->info.left, spaces + 4);
    print_node((struct node *)node->info.right, spaces + 4);
}

static void print_tree(struct avltree *const tree) {
    print_node((struct node *)tree->root, 0);
}

static int compare(struct node *const ours, struct node *const theirs) {
    return (int64_t)ours->number - theirs->number;
}

static int identify(struct node *const theirs, const void *const key) {
    return (int64_t)key - (int64_t)theirs->number;
}

static void insert_node(struct avltree *const tree, const uint32_t number) {
    struct node *const avl_node = malloc(sizeof(struct node));
    avl_node->number = number;

    avltree_insert(tree,
                   (struct avlnode *)avl_node,
                   (avlnode_compare_t)compare,
                   NULL);
}

void test_avltree() {
    struct avltree tree = {0};

    insert_node(&tree, 8);
    insert_node(&tree, 9);
    insert_node(&tree, 11);
    insert_node(&tree, 21);
    insert_node(&tree, 33);
    insert_node(&tree, 53);
    insert_node(&tree, 61);

    print_tree(&tree);

    avltree_delete(&tree, (void *)53, (avlnode_compare_key_t)identify, NULL);
    print_tree(&tree);
    avltree_delete(&tree, (void *)11, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)21, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)9, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)8, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)61, (avlnode_compare_key_t)identify, NULL);
    avltree_delete(&tree, (void *)33, (avlnode_compare_key_t)identify, NULL);

    printf("After deleting:\n");
    print_tree(&tree);
}