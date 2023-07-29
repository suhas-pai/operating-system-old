/*
 * lib/adt/array.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/growable_buffer.h"

struct array {
    struct growable_buffer gbuffer;
    uint64_t object_size;
};

#define array_foreach(array, type, item) \
    assert(sizeof(type) == (array)->object_size);                              \
    const type *const VAR_CONCAT(__end__, __LINE__) = array_end(*(array));     \
    for (type *item = (type *)array_begin(*(array));                           \
         item != VAR_CONCAT(__end__, __LINE__);                                \
         item = (type *)((uint64_t)item + (array)->object_size))

#define ARRAY_INIT(size) \
    ((struct array){ .gbuffer = GBUFFER_INIT(), .object_size = (size)})

void array_init(struct array *array, uint64_t object_size);
struct array array_alloc(uint64_t object_size, uint64_t item_capacity);

bool array_append(struct array *array, const void *item);
void array_remove_index(struct array *array, uint64_t index);
void array_remove_range(struct array *array, struct range range);

void *array_begin(struct array array);
const void *array_end(struct array array);

void *array_at(struct array array, uint64_t index);

uint64_t array_item_count(struct array array);
uint64_t array_free_count(struct array array);

bool array_empty(struct array array);
void array_destroy(struct array *array);
