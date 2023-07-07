/*
 * lib/adt/string.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/adt/string_view.h"
#include "lib/macros.h"

struct string {
    char *buffer;
    uint32_t length;
    uint32_t free;
};

#define STRUCT_STRING(cstr) string_create_alloc(cstr, LEN_OF(cstr))

struct string string_create_alloc(struct string_view sv);

struct string *string_append_ch(struct string *string, char ch, uint32_t amt);
struct string *string_append_sv(struct string *string, struct string_view sv);

__printf_format(2, 3)
struct string *
string_append_format(struct string *string, const char *fmt, ...);

struct string *
string_append(struct string *string, const struct string *append);

char string_front(struct string string);
char string_back(struct string string);

struct string *string_remove_index(struct string *string, uint32_t index);
struct string *string_remove_range(struct string *string, struct range range);

int64_t string_find_char(struct string *string, char ch);
int64_t string_find_sv(struct string *string, struct string_view sv);
int64_t string_find_string(struct string *string, const struct string *ch);

struct string_view string_to_sv(struct string string);
void string_free(struct string *string);

