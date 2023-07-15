/*
 * adt/string.c
 * © suhas pai
 */

#include "lib/adt/growable_buffer.h"
#include "lib/format.h"

static inline void set_null_terminator(const struct string *const string) {
    /*
     * HACK: We can't officially include the null-terminator as part of the
     * used-size of the gbuffer.
     *
     * Instead, we can set the byte right after used-part of the gbuffer to 0
     * for the null-terminator.
     *
     * NOTE: The caller is responsible for ensuring we can add one extra byte.
     */

    const struct growable_buffer gbuffer = string->gbuffer;
    ((uint8_t *)gbuffer.begin)[gbuffer.index] = '\0';
}

struct string string_create() {
    return (struct string){.gbuffer = GBUFFER_INIT()};
}

struct string string_create_alloc(const struct string_view sv) {
    struct string result = {
        .gbuffer = gbuffer_alloc(sv.length + 1)
    };

    if (result.gbuffer.begin != NULL) {
        memcpy(result.gbuffer.begin, sv.begin, sv.length);
        set_null_terminator(&result);
    }

    return result;
}

static bool prepare_append(struct string *const string, uint32_t length) {
    return gbuffer_ensure_can_add_capacity(&string->gbuffer, length + 1);
}

struct string *
string_append_char(struct string *const string,
                   const char ch,
                   const uint32_t amount)
{
    if (!prepare_append(string, amount)) {
        return NULL;
    }

    if (gbuffer_append_byte(&string->gbuffer, ch, amount) != amount) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

struct string *
string_append_sv(struct string *const string, const struct string_view sv) {
    if (!prepare_append(string, sv.length)) {
        return NULL;
    }

    if (gbuffer_append_sv(&string->gbuffer, sv) != sv.length) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

struct string *
string_append_format(struct string *const string, const char *const fmt, ...) {
    va_list list;
    va_start(list, fmt);

    format_to_string(string, fmt, list);

    va_end(list);
    return string;
}

struct string *
string_append_vformat(struct string *const string,
                      const char *const fmt,
                      va_list list)
{
    vformat_to_string(string, fmt, list);
    return string;
}

struct string *
string_append(struct string *const string, const struct string *const append) {
    if (!prepare_append(string, string_length(*string))) {
        return NULL;
    }

    if (gbuffer_append_gbuffer_data(&string->gbuffer, &append->gbuffer)) {
        return NULL;
    }

    set_null_terminator(string);
    return string;
}

char string_front(const struct string string) {
    if (!gbuffer_empty(string.gbuffer)) {
        return ((uint8_t *)string.gbuffer.begin)[0];
    }

    return '\0';
}

char string_back(const struct string string) {
    const uint64_t length = string_length(string);
    if (length != 0) {
        return ((uint8_t *)string.gbuffer.begin)[length - 1];
    }

    return '\0';
}

uint64_t string_length(const struct string string) {
    return gbuffer_used_size(string.gbuffer);
}

struct string *
string_remove_index(struct string *const string, const uint32_t index) {
    gbuffer_remove_index(&string->gbuffer, index);
    return string;
}

struct string *
string_remove_range(struct string *const string, const struct range range) {
    gbuffer_remove_range(&string->gbuffer, range);
    return string;
}

int64_t string_find_char(struct string *const string, char ch) {
    char *const result = strchr(string->gbuffer.begin, ch);
    if (result != NULL) {
        return ((int64_t)result - (int64_t)string->gbuffer.begin);
    }

    return -1;
}

int64_t string_find_sv(struct string *string, const struct string_view sv) {
    // TODO:
    (void)string;
    (void)sv;

    return -1;
}

int64_t string_find_string(struct string *string, const struct string *find) {
    return string_find_sv(string, string_to_sv(*find));
}

struct string_view string_to_sv(const struct string string) {
    if (string_length(string) == 0) {
        return sv_create_empty();
    }

    return sv_create_nocheck(string.gbuffer.begin, string_length(string));
}

void string_destroy(struct string *const string) {
    gbuffer_destroy(&string->gbuffer);
}

