/*
 * lib/ctype.c
 * © suhas pai
 */

#include <stdbool.h>
#include <stdint.h>

enum desc {
    ALPHA_NUM = 1 << 0,

    ALPHA_LOWER = 1 << 1,
    ALPHA_UPPER = 1 << 2,

    ALPHA = 1 << 3,
    CNTRL = 1 << 4,
    DIGIT = 1 << 5,
    GRAPH = 1 << 6,
    PRINT = 1 << 7,
    PUNCT = 1 << 8,
    SPACE = 1 << 9,

    HEX_DIGIT = 1 << 10
};

static uint16_t ctype_array[128] = {
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL | SPACE,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    CNTRL,
    PRINT | SPACE,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    ALPHA_NUM | DIGIT | GRAPH | PRINT | HEX_DIGIT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    ALPHA_NUM | ALPHA | GRAPH | PRINT | ALPHA_UPPER,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT | HEX_DIGIT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    ALPHA_NUM | ALPHA | GRAPH | ALPHA_LOWER | PRINT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    GRAPH | PRINT | PUNCT,
    CNTRL
};

static inline bool is_ascii(const int c) {
    return (c > 0 && c < 128);
}

int isalnum(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & ALPHA_NUM) ? 1 : 0;
}

int isalpha(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & ALPHA) ? 1 : 0;
}

int iscntrl(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & CNTRL) ? 1 : 0;
}

int isdigit(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & DIGIT) ? 1 : 0;
}

int isgraph(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & GRAPH) ? 1 : 0;
}

int islower(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & ALPHA_LOWER) ? 1 : 0;
}

int isprint(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & PRINT) ? 1 : 0;
}

int ispunct(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & PUNCT) ? 1 : 0;
}

int isspace(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & SPACE) ? 1 : 0;
}

int isupper(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & ALPHA_UPPER) ? 1 : 0;
}

int isxdigit(const int c) {
    if (!is_ascii(c)) {
        return 0;
    }

    return (ctype_array[c] & HEX_DIGIT) ? 1 : 0;
}

int tolower(const int c) {
    if (!isupper(c)) {
        return c;
    }

    /* return (c - 'A') + 'a' */
    return c + 32;
}

int toupper(const int c) {
    if (!islower(c)) {
        return c;
    }

    /* return (c - 'a') + 'A' */
    return c - 32;
}