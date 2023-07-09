/*
 * lib/adt/string_view.h
 * © suhas pai
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/assert.h"
#include "lib/overflow.h"
#include "lib/string.h"

#include "../util.h"

struct string_view {
    const char *begin;
    uint64_t length;
};

#define SV_STATIC(c_str) sv_create_nocheck(c_str, LEN_OF(c_str))
#define sv_foreach(sv, iter) \
    for (const char *iter = sv.begin; iter != (sv.begin + sv.length); iter++)

#define SV_FMT "%.*s"
#define SV_FMT_ARGS(sv) (int)(sv).length, (sv).begin

static inline struct string_view sv_create_empty() {
    return (struct string_view){ .begin = NULL, .length = 0 };
}

static inline struct string_view
sv_create_nocheck(const char *const c_str, const uint64_t length) {
    return (struct string_view){
        .begin = c_str,
        .length = length
    };
}

static inline struct string_view
sv_create_end(const char *const c_str, const char *const end) {
    assert(c_str <= end);
    return sv_create_nocheck(c_str, (uint64_t)(end - c_str));
}

static inline struct string_view
sv_create_length(const char *const c_str, const uint64_t length) {
    chk_add_overflow_assert((uint64_t)c_str, length);
    return sv_create_nocheck(c_str, length);
}

struct string_view sv_drop_front(const struct string_view sv);

char *sv_get_begin_mut(const struct string_view sv);
const char *sv_get_end(const struct string_view sv);

bool sv_compare_c_str(struct string_view sv, const char *c_str);
int sv_compare(struct string_view sv, struct string_view sv2);

static inline
bool sv_equals_c_str(const struct string_view sv, const char *const c_str) {
    return sv_compare_c_str(sv, c_str) == 0;
}

static inline
bool sv_equals(const struct string_view sv, const struct string_view sv2) {
    return sv_compare(sv, sv2) == 0;
}