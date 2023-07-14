/*
 * lib/adt/mutable_buffer.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "string_view.h"

struct mutable_buffer {
    void *begin;
    void *ptr;
    const void *end;
};

#define MBUFFER_STATIC_STACK(name, size) \
    char RAND_VAR_NAME()[size] = {0};     \
    struct mutable_buffer name =         \
        __mbuffer_open_static(RAND_VAR_NAME(), /*used=*/0, size)

struct mutable_buffer
mbuffer_open(void *buffer, uint64_t used, uint64_t capacity);

struct mutable_buffer
__mbuffer_open_static(void *buffer, uint64_t used, uint64_t capacity);

void *mbuffer_get_current_ptr(struct mutable_buffer mbuffer);

uint64_t mbuffer_get_free_space(struct mutable_buffer mbuffer);
uint64_t mbuffer_used_size(struct mutable_buffer mbuffer);
uint64_t mbuffer_capacity(struct mutable_buffer mbuffer);

bool mbuffer_can_add_size(struct mutable_buffer mbuffer, uint64_t size);

bool mbuffer_empty(struct mutable_buffer mbuffer);
bool mbuffer_full(struct mutable_buffer mbuffer);

uint64_t mbuffer_incr_ptr(struct mutable_buffer *mbuffer, uint64_t amt);
uint64_t mbuffer_decr_ptr(struct mutable_buffer *mbuffer, uint64_t amt);

uint64_t
mbuffer_append_data(struct mutable_buffer *mbuffer,
                    const void *data,
                    uint64_t length);

uint64_t
mbuffer_append_byte(struct mutable_buffer *mbuffer,
                    const uint8_t byte,
                    uint64_t count);

uint64_t
mbuffer_append_sv(struct mutable_buffer *mbuffer,
                  struct string_view sv);

bool mbuffer_truncate(struct mutable_buffer *mbuffer, uint64_t used_size);