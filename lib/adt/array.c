/*
 * lib/adt/array.c
 * © suhas pai
 */

#include "array.h"

void array_init(struct array *const array, const uint64_t object_size) {
    array->gbuffer = GBUFFER_INIT();
    array->object_size = object_size;
}

struct array array_alloc(const uint64_t obj_size, const uint64_t item_cap) {
    return (struct array){
        .gbuffer = gbuffer_alloc(check_mul_assert(obj_size, item_cap)),
        .object_size = obj_size
    };
}

bool array_append(struct array *const array, const void *const item) {
    return gbuffer_append_data(&array->gbuffer, item, array->object_size) ==
           array->object_size;
}

void array_remove_index(struct array *const array, const uint64_t index) {
    const uint64_t byte_index = check_mul_assert(index, array->object_size);
    const uint64_t end = check_add_assert(byte_index, array->object_size);

    gbuffer_remove_range(&array->gbuffer, range_create(byte_index, end));
}

void array_remove_range(struct array *const array, const struct range range) {
    const struct range byte_range = range_multiply(range, array->object_size);
    gbuffer_remove_range(&array->gbuffer, byte_range);
}

void *array_begin(const struct array array) {
    return array.gbuffer.begin;
}

const void *array_end(const struct array array) {
    return array.gbuffer.end;
}

void *array_at(const struct array array, const uint64_t index) {
    assert(index_in_bounds(index, array_item_count(array)));
    const uint64_t byte_index = check_mul_assert(index, array.object_size);

    return array.gbuffer.begin + byte_index;
}

uint64_t array_item_count(const struct array array) {
    return gbuffer_used_size(array.gbuffer) / array.object_size;
}

uint64_t array_free_count(struct array array) {
    return gbuffer_free_space(array.gbuffer) / array.object_size;
}

bool array_empty(const struct array array) {
    return gbuffer_empty(array.gbuffer);
}

void array_destroy(struct array *const array) {
    gbuffer_destroy(&array->gbuffer);
}