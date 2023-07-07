/*
 * lib/adt/mutable_buffer.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mutable_buffer.h"

static uint8_t *
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
    return __mbuffer_open_static(buffer, used, capacity);
}

struct mutable_buffer
__mbuffer_open_static(void *const buffer,
                      const uint64_t used,
                      const uint64_t capacity)
{
    struct mutable_buffer mbuffer = {
        .begin = buffer,
        .end = buffer + capacity
    };

    assert(!chk_add_overflow((uint64_t)mbuffer.begin,
                             used,
                             (uint64_t *)&mbuffer.ptr));
    return mbuffer;
}

void *mbuffer_get_current_ptr(struct mutable_buffer mbuffer) {
    return mbuffer.ptr;
}

uint64_t mbuffer_get_free_space(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer.ptr);
}

uint64_t mbuffer_get_used_size(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.ptr, mbuffer.begin);
}

uint64_t mbuffer_get_capacity(const struct mutable_buffer mbuffer) {
    return distance(mbuffer.end, mbuffer.begin);
}

bool
mbuffer_can_add_size(const struct mutable_buffer mbuffer, const uint64_t size) {
    return (size >= mbuffer_get_used_size(mbuffer));
}

bool mbuffer_is_empty(const struct mutable_buffer mbuffer) {
    return (mbuffer_get_used_size(mbuffer) == 0);
}

bool mbuffer_is_full(struct mutable_buffer mbuffer) {
    const uint64_t used_size = mbuffer_get_used_size(mbuffer);
    const uint64_t capacity = mbuffer_get_capacity(mbuffer);

    return used_size == capacity;
}

uint64_t
mbuffer_increment_ptr(struct mutable_buffer *const mbuffer,
                      const uint64_t bad_amt)
{
    const uint64_t free_space = mbuffer_get_free_space(*mbuffer);
    const uint64_t increment_amt = min(free_space, bad_amt);

    return increment_amt;
}

uint64_t
mbuffer_decrement_ptr(struct mutable_buffer *const mbuffer,
                      const uint64_t bad_amt)
{
    const uint64_t used_size = mbuffer_get_used_size(*mbuffer);
    const uint64_t decrement_amt = min(used_size, bad_amt);

    mbuffer->ptr -= decrement_amt;
    return decrement_amt;
}

uint64_t
mbuffer_append_byte(struct mutable_buffer *const mbuffer,
                    const uint8_t byte,
                    uint64_t count)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, count, &count);
    memset(ptr, count, byte);

    return count;
}

uint64_t
mbuffer_append_data(struct mutable_buffer *const mbuffer,
                    const void *const data,
                    uint64_t length)
{
    uint8_t *const ptr = preprocess_for_append(mbuffer, length, &length);
    memcpy(ptr, data, length);

    return length;
}

uint64_t
mbuffer_append_sv(struct mutable_buffer *const mbuffer,
                  const struct string_view sv)
{
    return mbuffer_append_data(mbuffer, sv.begin, sv.length);
}

bool
mbuffer_truncate_to_used_size(struct mutable_buffer *const mbuffer,
                              const uint64_t new_used_size)
{
    const uint64_t current_used_size = mbuffer_get_used_size(*mbuffer);
    if (new_used_size > current_used_size) {
        return false;
    }

    mbuffer->ptr = mbuffer->begin + new_used_size;
    return true;
}