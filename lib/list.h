/*
 * lib/list.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/assert.h"

/* list is a circular doubly-linked list */
struct list {
    struct list *prev;
    struct list *next;
};

#define LIST_INIT(name) { .prev = &(name), .next = &(name) }

static inline void list_init(struct list *const head) {
    head->prev = head;
    head->next = head;
}

static inline void
list_add_common(struct list *const elem,
                struct list *const prev,
                struct list *const next)
{
    elem->prev = prev;
    elem->next = next;

    assert(next != NULL);

    prev->next = elem;
    next->prev = elem;
}

// Add to front of list
static inline void list_add(struct list *const head, struct list *const item) {
    list_add_common(item, head, head->next);
}

// Add to back of list
static inline void list_radd(struct list *const head, struct list *const item) {
    list_add_common(item, head->prev, head);
}

static inline bool list_empty(struct list *const list) {
    return list == list->prev;
}

static inline void list_delete(struct list *const elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
}

#define container_of(ptr, type, name) \
    ((type *)(uint64_t)((void *)ptr - offsetof(type, name)))
#define list_del(type, elem, name) \
    ({ list_delete(elem); container_of(elem, type, name)})

#define list_prev(ob, name) container_of(ob->name.prev, typeof(*ob), name)
#define list_next(ob, name) container_of(ob->name.next, typeof(*ob), name)

#define list_head(list, type, name) \
    ((type *)((void *)((char *)(list)->next - offsetof(type, name))))
#define list_tail(list, type, name) \
    ((type *)((void *)((char *)(list)->prev - offsetof(type, name))))

#define list_foreach(iter, list, name) \
    for(iter = list_head(list, typeof(*iter), name); &iter->name != (list); \
        iter = list_next(iter, name))

#define list_foreach_mut(iter, tmp, list, name) \
    for(iter = list_head(list, typeof(*iter), name), \
            tmp = list_next(iter, name);             \
        &iter->name != (list);                       \
        iter = tmp, tmp = list_next(iter, name))
