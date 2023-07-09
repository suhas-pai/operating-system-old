/*
 * lib/adt/growable_buffer.h
 * © suhas pai
 */

#pragma once

#include "mutable_buffer.h"
#include "range.h"
#include "string_view.h"

#include "../alloc.h"

struct growable_buffer {
    void *begin;
    uint64_t index;
    const void *end;

    bool is_alloc : 1;
};

struct growable_buffer gbuffer_alloc(uint64_t init_cap);
struct growable_buffer gbuffer_alloc_copy(void *data, const uint64_t size);

struct growable_buffer
gbuffer_open(void *buffer,
             uint64_t used,
             uint64_t capacity,
             bool is_alloc);

struct growable_buffer
gbuffer_open_mutable_buffer(struct mutable_buffer mbuffer, bool is_alloc);

bool gbuffer_ensure_can_add_capacity(struct growable_buffer *gb, uint64_t add);

struct mutable_buffer
gbuffer_get_mutable_buffer(struct growable_buffer gbuffer);

const void *gbuffer_end(struct growable_buffer gbuffer);
void *gbuffer_current_ptr(struct growable_buffer gbuffer);

uint8_t *
gbuffer_ptr_for_byte_index(struct growable_buffer gbuffer, uint64_t index);

uint64_t gbuffer_free_space(struct growable_buffer gbuffer);
uint64_t gbuffer_used_size(struct growable_buffer gbuffer);
uint64_t gbuffer_capacity(struct growable_buffer gbuffer);

bool gbuffer_can_add_size(struct growable_buffer gbuffer, uint64_t size);
bool gbuffer_empty(struct growable_buffer gbuffer);

uint64_t gbuffer_incr_ptr(struct growable_buffer *gbuffer, uint64_t amt);
uint64_t gbuffer_decr_ptr(struct growable_buffer *gbuffer, uint64_t amt);

uint64_t
gbuffer_append_data(struct growable_buffer *gbuffer,
                    const void *data,
                    uint64_t length);

uint64_t
gbuffer_append_byte(struct growable_buffer *gbuffer,
                    const uint8_t byte,
                    uint64_t count);

bool
gbuffer_append_gbuffer_data(struct growable_buffer *gbuffer,
                            const struct growable_buffer *append);

uint64_t
gbuffer_append_sv(struct growable_buffer *gbuffer, struct string_view sv);

void gbuffer_remove_index(struct growable_buffer *gbuffer, uint64_t index);
void gbuffer_remove_range(struct growable_buffer *gbuffer, struct range range);

void
gbuffer_truncate_to_byte_index(struct growable_buffer *gbuffer,
                               uint64_t byte_index);

void *gbuffer_take_data(struct growable_buffer *gbuffer);

void gbuffer_clear(struct growable_buffer *gbuffer);
void gbuffer_destroy(struct growable_buffer *gbuffer);