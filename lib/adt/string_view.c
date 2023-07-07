/*
 * lib/adt/string_view.c
 * © suhas pai
 */

#include "lib/string.h"
#include "string_view.h"

struct string_view sv_drop_front(const struct string_view sv) {
    if (sv.length != 0) {
        return sv_create_nocheck(sv.begin + 1, sv.length - 1);
    }

    return sv_create_empty();
}

bool sv_equals_c_str(const struct string_view sv, const char *const c_str) {
    return strncmp(sv.begin, c_str, sv.length) == 0;
}

int sv_compare(const struct string_view sv, const struct string_view sv2) {
    if (sv.length > sv2.length) {
        if (sv2.length == 0) {
            return *sv.begin;
        }

        return strncmp(sv.begin, sv2.begin, sv2.length);
    } else if (sv.length < sv2.length) {
        if (sv.length == 0) {
            return -*sv2.begin;
        }

        return strncmp(sv.begin, sv2.begin, sv.length);
    }

    // Both svs are empty
    if (sv.length == 0) {
        return 0;
    }

    return strncmp(sv.begin, sv2.begin, sv.length);
}