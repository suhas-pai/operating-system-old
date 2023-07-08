/*
 * lib/format.c
 * © suhas pai
 */

#include "format.h"
#include "parse_printf.h"

uint64_t
format_to_buffer(char *const buffer,
                 const uint64_t buffer_len,
                 const char *const fmt,
                 ...)
{
    va_list list;
    va_start(list, fmt);

    const uint64_t result = vformat_to_buffer(buffer, buffer_len, fmt, list);

    va_end(list);
    return result;
}

struct buffer_info {
    char *buffer;
    uint64_t index;
    uint64_t capacity;
};

static uint64_t
buffer_char_callback(struct printf_spec_info *const spec_info,
                     void *const cb_info,
                     const char ch,
                     uint64_t amount,
                     bool *const cont_out)
{
    (void)spec_info;

    struct buffer_info *const info = cb_info;
    const uint64_t left = info->capacity - info->index;

    if (left <= amount) {
        amount = left;
        *cont_out = false;
    }

    memset(info->buffer + info->index, ch, amount);
    info->index += amount;

    return amount;
}

static uint64_t
buffer_sv_callback(struct printf_spec_info *const spec_info,
                   void *const cb_info,
                   const struct string_view sv,
                   bool *const cont_out)
{
    (void)spec_info;

    struct buffer_info *const info = cb_info;
    const uint64_t left = info->capacity - info->index;

    uint64_t amount = sv.length;
    if (left <= amount) {
        amount = left;
        *cont_out = false;
    }

    memcpy(info->buffer + info->index, sv.begin, amount);
    info->index += amount;

    return amount;
}

uint64_t
vformat_to_buffer(char *const buffer,
                  const uint64_t buffer_length,
                  const char *const fmt,
                  va_list list)
{
    struct buffer_info info = {
        .buffer = buffer,
        .capacity = buffer_length
    };

    const uint64_t result =
        parse_printf(fmt,
                     buffer_char_callback,
                     &info,
                     buffer_sv_callback,
                     &info,
                     list);
    return result;
}

uint64_t
format_to_string(struct string *const string,
                 const char *const fmt,
                 ...)
{
    va_list list;
    va_start(list, fmt);

    const uint64_t result = vformat_to_string(string, fmt, list);

    va_end(list);
    return result;
}

uint64_t
string_char_callback(struct printf_spec_info *const spec_info,
                     void *const cb_info,
                     const char ch,
                     uint64_t amount,
                     bool *const cont_out)
{
    (void)spec_info;

    struct string *const string = (struct string *)cb_info;
    if (!string_append_ch(string, ch, amount)) {
        *cont_out = false;
        return 0;
    }

    return amount;
}

uint64_t
string_sv_callback(struct printf_spec_info *const spec_info,
                   void *const cb_info,
                   const struct string_view sv,
                   bool *const cont_out)
{
    (void)spec_info;

    struct string *const string = (struct string *)cb_info;
    if (!string_append_sv(string, sv)) {
        *cont_out = false;
        return 0;
    }

    return sv.length;
}

uint64_t
vformat_to_string(struct string *const string,
                  const char *const fmt,
                  va_list list)
{
    const uint64_t result =
        parse_printf(fmt,
                     string_char_callback,
                     /*write_char_cb=*/string,
                     string_sv_callback,
                     /*char_cb_info=*/string,
                     list);

    return result;
}