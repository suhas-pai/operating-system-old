/*
 * lib/list.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* By default, list is a circular doubly-linked list */
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

    prev->next = elem;
    next->prev = elem;
}

static inline void list_add(struct list *const head, struct list *const item) {
    list_add_common(item, head, head->next);
}

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
    ((type *)(void *)((char *)ptr - offsetof(type, name)))
#define list_del(type, elem, name) \
    ({ list_delete(elem); container_of(elem, type, name)})
#define list_next(ob, name) container_of(ob->name.next, typeof(*ob), name)

static inline
void *list_head_ptr(struct list *const list, const uint64_t offset) {
    if (list->next == NULL) {
        return NULL;
    }

    return (void *)((char *)list->next - offset);
}

static inline
void *list_tail_ptr(struct list *const list, const uint64_t offset) {
    if (list->prev == NULL) {
        return NULL;
    }

    return (void *)((char *)list->prev - offset);
}

#define list_head(list, type, name) \
    ((type *)((void *)((char *)(list)->next - offsetof(type, name))))
#define list_tail(list, type, name) \
    ((type *)((void *)((char *)(list)->prev - offsetof(type, name))))

#define list_foreach(iter, list, name) \
	for(iter = list_head(list, typeof(*iter), name); &iter->name != (list); \
        iter = list_next(iter, name))
