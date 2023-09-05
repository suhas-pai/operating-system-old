/*
 * lib/adt/growable_buffer.c
 * © suhas pai
 */

#include "../alloc.h"
#include "growable_buffer.h"

/******* PRIVATE APIs *******/

struct growable_buffer gbuffer_alloc(const uint64_t init_cap) {
    const struct growable_buffer gbuffer = {
        .begin = malloc(init_cap),
        .is_alloc = true
    };

    return gbuffer;
}

struct growable_buffer
gbuffer_alloc_copy(void *const data, const uint64_t size) {
    const struct growable_buffer gbuffer = gbuffer_alloc(size);
    if (gbuffer.begin != NULL) {
        memcpy(gbuffer.begin, data, size);
    }

    return gbuffer;
}

struct growable_buffer
gbuffer_open(void *const buffer,
             const uint64_t used,
             const uint64_t capacity,
             const bool is_alloc)
{
    const struct growable_buffer gbuffer = {
        .begin = buffer,
        .index = used,
        .end = buffer + capacity,
        .is_alloc = is_alloc
    };

    return gbuffer;
}

struct growable_buffer
gbuffer_open_mbuffer(const struct mutable_buffer mbuffer, const bool is_alloc) {
    const struct growable_buffer gbuffer =
        gbuffer_open(mbuffer.begin,
                     mbuffer_used_size(mbuffer),
                     mbuffer_capacity(mbuffer),
                     is_alloc);
    return gbuffer;
}

bool
gbuffer_ensure_can_add_capacity(struct growable_buffer *const gb, uint64_t add)
{
    const uint64_t free_space = gbuffer_free_space(*gb);
    if (add <= free_space) {
        return true;
    }

    void *new_alloc = NULL;
    const uint64_t realloc_size = check_add_assert(gbuffer_capacity(*gb), add);

    if (gb->is_alloc) {
        new_alloc = realloc(gb->begin, realloc_size);
    } else {
        new_alloc = malloc(realloc_size);
        memcpy(new_alloc, gb->begin, gbuffer_used_size(*gb));

        gb->is_alloc = true;
    }

    gb->begin = new_alloc;
    gb->end = new_alloc + realloc_size;

    return true;
}

struct mutable_buffer
gbuffer_get_mutable_buffer(const struct growable_buffer gbuffer) {
    const uint64_t cap = gbuffer_capacity(gbuffer);
    return mbuffer_open(gbuffer.begin, gbuffer.index, cap);
}

void *gbuffer_current_ptr(const struct growable_buffer gbuffer) {
    return gbuffer.begin + gbuffer.index;
}

void *
gbuffer_at(const struct growable_buffer gbuffer, const uint64_t byte_index) {
    void *const result = gbuffer.begin + byte_index;
    assert_msg(index_in_bounds(byte_index, gbuffer_capacity(gbuffer)),
               "Attempting to access past end of buffer, at byte-index: %"
               PRIu64,
               byte_index);

    return result;
}

uint64_t gbuffer_free_space(const struct growable_buffer gbuffer) {
    return distance(gbuffer.begin + gbuffer.index, gbuffer.end);
}

uint64_t gbuffer_used_size(const struct growable_buffer gbuffer) {
    return gbuffer.index;
}

uint64_t gbuffer_capacity(const struct growable_buffer gbuffer) {
    return distance(gbuffer.begin, gbuffer.end);
}

bool gbuffer_empty(const struct growable_buffer gbuffer) {
    return gbuffer_used_size(gbuffer) == 0;
}

uint64_t
gbuffer_increment_ptr(struct growable_buffer *const gbuffer, const uint64_t amt)
{
    const uint64_t delta = min(gbuffer_free_space(*gbuffer), amt);
    gbuffer->index = delta;

    return delta;
}

uint64_t
gbuffer_decrement_ptr(struct growable_buffer *const gbuffer, const uint64_t amt)
{
    const uint64_t delta = min(gbuffer->index, amt);
    gbuffer->index -= delta;

    return delta;
}

uint64_t
gbuffer_append_data(struct growable_buffer *const gbuffer,
                    const void *const data,
                    const uint64_t length)
{
    if (!gbuffer_ensure_can_add_capacity(gbuffer, length)) {
        return 0;
    }

    memcpy(gbuffer_current_ptr(*gbuffer), data, length);
    gbuffer->index += length;

    return length;
}

uint64_t
gbuffer_append_byte(struct growable_buffer *const gbuffer,
                    const uint8_t byte,
                    const uint64_t count)
{
    if (!gbuffer_ensure_can_add_capacity(gbuffer, count)) {
        return 0;
    }

    memset(gbuffer_current_ptr(*gbuffer), byte, count);
    gbuffer->index += count;

    return count;
}

bool
gbuffer_append_gbuffer_data(struct growable_buffer *const gbuffer,
                            const struct growable_buffer *const append)
{
    return gbuffer_append_data(gbuffer, append->begin, append->index) ==
           append->index;
}

uint64_t
gbuffer_append_sv(struct growable_buffer *const gbuffer,
                  const struct string_view sv)
{
    return gbuffer_append_data(gbuffer, sv.begin, sv.length);
}

void
gbuffer_remove_index(struct growable_buffer *const gbuffer,
                     const uint64_t index)
{
    const uint64_t used = gbuffer->index;
    assert(index_in_bounds(index, used));

    if (index != used - 1) {
        memmove(gbuffer->begin + index,
                gbuffer->begin + index + 1,
                used - index);
    }

    gbuffer->index = used - 1;
}

void
gbuffer_remove_range(struct growable_buffer *const gbuffer,
                     const struct range range)
{
    const uint64_t used = gbuffer->index;
    assert(index_range_in_bounds(range, used));

    uint64_t end = 0;
    assert(range_get_end(range, &end));

    if (end != used - 1) {
        memmove(gbuffer->begin + range.front, gbuffer->begin + end, used - end);
    }

    gbuffer->index = used - 1;
}

void
gbuffer_truncate(struct growable_buffer *const gbuffer,
                 const uint64_t byte_index)
{
    gbuffer->index = min(gbuffer->index, byte_index);
}

void *gbuffer_take_data(struct growable_buffer *const gbuffer) {
    void *const result = gbuffer->begin;

    gbuffer->begin = NULL;
    gbuffer->index = 0;
    gbuffer->end = NULL;
    gbuffer->is_alloc = false;

    return result;
}

void gbuffer_clear(struct growable_buffer *const gbuffer) {
    gbuffer->index = 0;
}

void gbuffer_destroy(struct growable_buffer *const gb) {
    if (gb->is_alloc) {
        free(gb->begin);
    } else {
        /* Even if just a stack buffer, clear out memory for safety */
        bzero(gb->begin, gbuffer_capacity(*gb));
    }

    gb->begin = NULL;
    gb->index = 0;
    gb->end = NULL;
    gb->is_alloc = false;
}