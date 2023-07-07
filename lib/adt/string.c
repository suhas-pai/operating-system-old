/*
 * adt/string.c
 * © suhas pai
 */

#include "lib/alloc.h"
#include "lib/format.h"
#include "lib/string.h"

struct string string_create_alloc(const struct string_view sv) {
    struct string result = {
        .buffer = malloc(sv.length + 1),
        .length = sv.length,
        .free = 0
    };

    if (result.buffer != NULL) {
        memcpy(result.buffer, sv.begin, sv.length);
        result.buffer[sv.length] = '\0';
    } else {
        result.length = 0;
    }

    return result;
}

static struct string *
prepare_append(struct string *const string, uint32_t length) {
    if (length < 16) {
        length = 16;
    }

    if (string->free < length) {
        void *const buffer =
            realloc(string->buffer, string->length + length + 1);

        if (buffer == NULL) {
            return NULL;
        }

        string->buffer = buffer;
    }

    string->free = 0;
    return string;
}

struct string *
string_append_ch(struct string *const string,
                 const char ch,
                 const uint32_t amt)
{
    if (prepare_append(string, amt) == NULL) {
        return NULL;
    }

    memset(string->buffer + string->length, ch, amt);
    string->length += amt;

    return string;
}

struct string *
string_append_sv(struct string *const string, const struct string_view sv) {
    if (prepare_append(string, sv.length) == NULL) {
        return NULL;
    }

    memcpy(string->buffer + string->length, sv.begin, sv.length);
    string->length += sv.length;

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
    if (prepare_append(string, append->length) == NULL) {
        return NULL;
    }

    memcpy(string->buffer + string->length, append->buffer, append->length);
    string->length += append->length;

    return string;
}

char string_front(const struct string string) {
    if (string.length != 0) {
        return string.buffer[0];
    }

    return '\0';
}

char string_back(const struct string string) {
    if (string.length != 0) {
        return string.buffer[string.length - 1];
    }

    return '\0';
}

struct string *
string_remove_index(struct string *const string, const uint32_t index) {
    assert(index_in_bounds(index, string->length));
    if (index != string->length - 1) {
        memmove(string->buffer + index, string->buffer + index + 1, 1);
    }

    string->length -= 1;
    return string;
}

struct string *
string_remove_range(struct string *const string, const struct range range) {
    assert(index_range_in_bounds(range, string->length));

    uint64_t end = 0;
    if (!range_get_end(range, &end)) {
        return NULL;
    }

    if (end != string->length - 1) {
        memmove(string->buffer + range.front,
                string->buffer + end,
                string->length - end);
    }

    string->length -= range.size;
    return string;
}

int64_t string_find_char(struct string *const string, char ch) {
    char *const result = strchr(string->buffer, ch);
    if (result != NULL) {
        return ((int64_t)result - (int64_t)string->buffer);
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
    return sv_create_nocheck(string.buffer, string.length);
}

void string_free(struct string *const string) {
    free(string->buffer);

    string->buffer = NULL;
    string->length = 0;
    string->free = 0;
}

