/*
 * lib/adt/avltree.c
 * © suhas pai
 */

#include <stdbool.h>

#include "lib/assert.h"
#include "avltree.h"

static inline
void avlnode_verify(struct avlnode *const node, struct avlnode *const parent) {
#if defined (BUILD_TEST)
    if (node == NULL) {
        return;
    }

    assert(node != node->left);
    assert(node != node->right);
    assert(node->parent == parent);

    avlnode_verify(node->left, node);
    avlnode_verify(node->right, node);
#else
    (void)(node);
    (void)(parent);
#endif /* defined(BUILD_TEST ) */
}

__unused static inline void avltree_verify(struct avltree *const tree) {
#if defined (BUILD_TEST)
    avlnode_verify(tree->root, /*parent=*/NULL);
#else
    (void)(tree);
#endif /* defined(BUILD_TEST ) */
}

void
avltree_insert(struct avltree *const tree,
               struct avlnode *const node,
               const avlnode_compare_t comparator,
               const avlnode_update_t update)
{
    struct avlnode *parent = NULL;
    struct avlnode *curr_node = tree->root;
    struct avlnode **link = &tree->root;

    while (curr_node != NULL) {
        const int compare = comparator(node, curr_node);
        parent = curr_node;

        // FIXME: Handle duplicates? (compare == 0)
        link = compare < 0 ? &curr_node->left : &curr_node->right;
        curr_node = compare < 0 ? curr_node->left : curr_node->right;
    }

    avltree_insert_at_loc(tree, node, parent, link, update);
}

static struct avlnode *rotate_left(struct avlnode *const node) {
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->right;
    struct avlnode *const tmp = new_top->left;

    avlnode_verify(new_top, new_top->parent);

    node->right = tmp;
    if (tmp != NULL) {
        tmp->parent = node;
    }

    new_top->left = node;
    new_top->parent = node->parent;

    node->parent = new_top;

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    return new_top;
}

static struct avlnode *rotate_right(struct avlnode *const node) {
    avlnode_verify(node, node->parent);

    struct avlnode *const new_top = node->left;
    struct avlnode *const tmp = new_top->right;

    avlnode_verify(new_top, new_top->parent);

    node->left = tmp;
    node->parent = new_top;

    if (tmp != NULL) {
        tmp->parent = node;
    }

    new_top->parent = node->parent;
    new_top->right = node;

    avlnode_verify(node, node->parent);
    avlnode_verify(new_top, new_top->parent);

    return new_top;
}

static inline uint32_t node_height(const struct avlnode *const node) {
    return node != NULL ? node->height : 0;
}

static inline int64_t node_balance(const struct avlnode *const node) {
    return (int64_t)node_height(node->right) - node_height(node->left);
}

static inline void reset_node_height(struct avlnode *const node) {
    node->height = 1 + max(node_height(node->left), node_height(node->right));
}

static inline struct avlnode **
get_node_link(struct avlnode *const node, struct avltree *const tree) {
    struct avlnode *const parent = node->parent;
    if (parent != NULL) {
        return (node == parent->left) ? &parent->left : &parent->right;
    }

    return &tree->root;
}

static inline
void avlnode_update(struct avlnode *const node, const avlnode_update_t update) {
    if (update != NULL) {
        update(node);
    }
}

static void
avltree_fixup(struct avltree *const tree,
              struct avlnode *node,
              const avlnode_update_t update)
{
    while (true) {
        if (node == NULL) {
            return;
        }

        struct avlnode *new_node = NULL;
        const int64_t balance = node_balance(node);

        if (balance > 1) {
            /* Right-heavy */

            new_node = node->right;
            const bool double_rotate =
                node_height(new_node->left) > node_height(new_node->right);

            if (double_rotate) {
                /* We need to perform a double rotation - Right-Left rotation */
                new_node = rotate_right(new_node);
                node->right = new_node;

                reset_node_height(new_node);
                avlnode_update(new_node, update);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_left(node);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            avlnode_update(node, update);

            node = new_node;
        } else if (balance < -1) {
            /* Left-Heavy */

            new_node = node->left;
            const bool double_rotate =
                node_height(new_node->left) < node_height(new_node->right);

            if (double_rotate) {
                /* We need to perform a double rotation - Left-Right rotation */
                new_node = rotate_left(new_node);
                node->left = new_node;

                reset_node_height(new_node);
            }

            struct avlnode **const link = get_node_link(node, tree);

            *link = rotate_right(node);
            reset_node_height(node);

            avlnode_verify(new_node, new_node->parent);
            avlnode_update(node, update);

            node = new_node;
        } else {
            reset_node_height(node);
        }

        avlnode_verify(node, node->parent);
        avlnode_update(node, update);

        node = node->parent;
    }
}

void
avltree_insert_at_loc(struct avltree *const tree,
                      struct avlnode *const node,
                      struct avlnode *const parent,
                      struct avlnode **const link,
                      const avlnode_update_t update)
{
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;

    *link = node;

    avlnode_verify(node, parent);
    avltree_fixup(tree, parent, update);
}

void
avltree_delete(struct avltree *const tree,
               void *const key,
               const avlnode_compare_key_t comparator,
               const avlnode_update_t update)
{
    struct avlnode *curr_node = tree->root;
    while (curr_node != NULL) {
        const int compare = comparator(curr_node, key);
        if (compare == 0) {
            avltree_delete_node(tree, curr_node, update);
            return;
        }

        curr_node = compare < 0 ? curr_node->left : curr_node->right;
    }
}

struct avlnode *avlnode_successor(struct avlnode *node) {
    if (node->right != NULL) {
        node = node->right;
        while (node->left != NULL) {
            node = node->left;
        }

        return node;
    }

    node = node->parent;
    while (node != NULL) {
        if (node->left != NULL) {
            return node;
        }

        node = node->parent;
    }

    return NULL;
}

void
avltree_delete_node(struct avltree *const tree,
                    struct avlnode *const node,
                    const avlnode_update_t update)
{
    struct avlnode **node_link = get_node_link(node, tree);
    struct avlnode *parent = node->parent;

    if (node->left != NULL) {
        if (node->right != NULL) {
            struct avlnode *successor = node->right;
            struct avlnode *const left = node->left;

            if (successor->left != NULL) {
                struct avlnode **successor_link = &node->right;
                do {
                    parent = successor;
                    successor_link = &successor->left;
                    successor = successor->left;
                } while (successor->left != NULL);

                successor->left = left;
                left->parent = successor;
                node->left = NULL;

                swap(node->right, successor->right);
                successor->right->parent = successor;

                if (node->right != NULL) {
                    node->right->parent = node;
                }

                *successor_link = node;
                *node_link = successor;

                successor->parent = node->parent;
                node->parent = parent;

                swap(node->height, successor->height);
                node_link = successor_link;
            } else {
                node->left = NULL;
                node->right = successor->right;

                successor->left = left;
                successor->right = node;

                left->parent = successor;
                if (node->right != NULL) {
                    node->right->parent = node;
                }

                successor->right = node;
                successor->parent = node->parent;

                node->parent = successor;
                *node_link = successor;

                swap(node->height, successor->height);

                node_link = &successor->right;
                parent = successor;
            }
        }
    }

    struct avlnode *const first_child = node->left ?: node->right;
    *node_link = first_child;

    if (first_child != NULL) {
        first_child->parent = parent;
    }

    avltree_fixup(tree, parent, update);
}