/*
 * lib/adt/array.c
 * © suhas pai
 */

#include "lib/util.h"
#include "array.h"

void array_init(struct array *const array, const uint32_t object_size) {
    array->gbuffer = GBUFFER_INIT();
    array->object_size = object_size;
}

__optimize(3)
struct array array_alloc(const uint32_t obj_size, const uint32_t item_cap) {
    return (struct array){
        .gbuffer = gbuffer_alloc(check_mul_assert(obj_size, item_cap)),
        .object_size = obj_size
    };
}

__optimize(3)
bool array_append(struct array *const array, const void *const item) {
    return gbuffer_append_data(&array->gbuffer, item, array->object_size) ==
           array->object_size;
}

__optimize(3)
void array_remove_index(struct array *const array, const uint32_t index) {
    const uint64_t byte_index = check_mul_assert(index, array->object_size);
    const uint64_t end = check_add_assert(byte_index, array->object_size);

    gbuffer_remove_range(&array->gbuffer, range_create(byte_index, end));
}

__optimize(3)
bool array_remove_range(struct array *const array, const struct range range) {
    struct range byte_range = RANGE_EMPTY();
    if (!range_multiply(range, array->object_size, &byte_range)) {
        return false;
    }

    gbuffer_remove_range(&array->gbuffer, byte_range);
    return true;
}

__optimize(3) void *array_begin(const struct array array) {
    return array.gbuffer.begin;
}

__optimize(3) const void *array_end(const struct array array) {
    return array.gbuffer.end;
}

__optimize(3) void *array_at(const struct array array, const uint64_t index) {
    assert(index_in_bounds(index, array_item_count(array)));
    const uint64_t byte_index = check_mul_assert(index, array.object_size);

    return array.gbuffer.begin + byte_index;
}

__optimize(3) void *array_front(const struct array array) {
    assert(!array_empty(array));
    return array_begin(array);
}

__optimize(3) void *array_back(const struct array array) {
    assert(!array_empty(array));
    return gbuffer_current_ptr(array.gbuffer) - array.object_size;
}

__optimize(3) uint64_t array_item_count(const struct array array) {
    return gbuffer_used_size(array.gbuffer) / array.object_size;
}

__optimize(3) uint64_t array_free_count(struct array array) {
    return gbuffer_free_space(array.gbuffer) / array.object_size;
}

__optimize(3) void *array_take(struct array *const array) {
    return gbuffer_take_data(&array->gbuffer);
}

__optimize(3) bool array_empty(const struct array array) {
    return gbuffer_empty(array.gbuffer);
}

__optimize(3) void array_destroy(struct array *const array) {
    gbuffer_destroy(&array->gbuffer);
}