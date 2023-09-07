/*
 * lib/adt/mutable_buffer.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mutable_buffer.h"

__optimize(3) static uint8_t *
preprocess_for_append(struct mutable_buffer *const mbuffer,
                      uint64_t length,
                      uint64_t *const length_out)
{
    const uint64_t free_space = mbuffer_get_free_space(*mbuffer);
    if (length >= free_space) {
        length = free_space;
    }

    uint8_t *const ptr = mbuffer->ptr;

    mbuffer->ptr = ptr + length;
    *length_out = length;

    return ptr;
}

struct mutable_buffer
mbuffer_open(void *const buffer, const uint64_t used, const uint64_t capacity) {
    return mbuffer_open_static(buffer, used, capacity);
}

struct mutable_buffer
mbuffer_open_static(void *const buffer,
                    const uint64_t used,
                    const uint64_t capacity)
{
    assert(used <= capacity);
    struct mutable_buffer mbuffer = {
        .begin = buffer,
        .ptr = (void *)check_add_assert((uint64_t)mbuffer.begin, used),
        .end = buffer + capacity
    };

    return mbuffer;
}

__optimize(3) void *mbuffer_get_current_ptr(struct mutable_buffer mbuffer) {
    return mbuffer.ptr;
}

__optimize(3)
uint64_t mbuffer_get_free_space(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer.ptr);
}

__optimize(3) uint64_t mbuffer_used_size(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.ptr, mbuffer.begin);
}

__optimize(3) uint64_t mbuffer_capacity(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer.begin);
}

__optimize(3) bool
mbuffer_can_add_size(const struct mutable_buffer mbuffer, const uint64_t size) {
    return (size <= mbuffer_get_free_space(mbuffer));
}

__optimize(3) bool mbuffer_empty(const struct mutable_buffer mbuffer) {
    return (mbuffer_used_size(mbuffer) == 0);
}

__optimize(3) bool mbuffer_full(struct mutable_buffer mbuffer) {
    const uint64_t used_size = mbuffer_used_size(mbuffer);
    const uint64_t capacity = mbuffer_capacity(mbuffer);

    return used_size == capacity;
}

__optimize(3) uint64_t
mbuffer_increment_ptr(struct mutable_buffer *const mbuffer,
                      const uint64_t bad_amt)
{
    const uint64_t free_space = mbuffer_get_free_space(*mbuffer);
    const uint64_t increment_amt = min(free_space, bad_amt);

    return increment_amt;
}

__optimize(3) uint64_t
mbuffer_decrement_ptr(struct mutable_buffer *const mbuffer,
                      const uint64_t bad_amt)
{
    const uint64_t used_size = mbuffer_used_size(*mbuffer);
    const uint64_t decrement_amt = min(used_size, bad_amt);

    mbuffer->ptr -= decrement_amt;
    return decrement_amt;
}

__optimize(3) uint64_t
mbuffer_append_byte(struct mutable_buffer *const mbuffer,
                    const uint8_t byte,
                    uint64_t count)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, count, &count);
    memset(ptr, byte, count);

    return count;
}

__optimize(3) uint64_t
mbuffer_append_data(struct mutable_buffer *const mbuffer,
                    const void *const data,
                    uint64_t length)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, length, &length);
    memcpy(ptr, data, length);

    return length;
}

__optimize(3) uint64_t
mbuffer_append_sv(struct mutable_buffer *const mbuffer,
                  const struct string_view sv)
{
    return mbuffer_append_data(mbuffer, sv.begin, sv.length);
}

__optimize(3) bool
mbuffer_truncate(struct mutable_buffer *const mbuffer,
                 const uint64_t new_used_size)
{
    const uint64_t current_used_size = mbuffer_used_size(*mbuffer);
    if (new_used_size > current_used_size) {
        return false;
    }

    mbuffer->ptr = mbuffer->begin + new_used_size;
    return true;
}