/*
 * lib/adt/array.c
 * © suhas pai
 */

#include "array.h"

struct array array_create(const uint64_t object_size) {
    return (struct array){
        .gbuffer = {},
        .object_size = object_size
    };
}

struct array array_alloc(const uint64_t obj_size, const uint64_t item_cap) {
    return (struct array){
        .gbuffer = gbuffer_alloc(chk_mul_overflow_assert(obj_size, item_cap)),
        .object_size = obj_size
    };
}

void array_append(struct array *const array, const void *const item) {
    gbuffer_append_data(&array->gbuffer, item, array->object_size);
}

void array_remove_index(struct array *const array, const uint64_t index) {
    const uint64_t byte_index =
        chk_mul_overflow_assert(index, array->object_size);
    const uint64_t end =
        chk_add_overflow_assert(byte_index, array->object_size);

    gbuffer_remove_range(&array->gbuffer, range_create(byte_index, end));
}

void array_remove_range(struct array *const array, const struct range range) {
    const struct range byte_range = range_multiply(range, array->object_size);
    gbuffer_remove_range(&array->gbuffer, byte_range);
}

void *array_begin(const struct array array) {
    return array.gbuffer.begin;
}

const void *array_end(struct array array) {
    return gbuffer_end(array.gbuffer);
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