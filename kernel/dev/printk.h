/*
 * kernel/dev/printk.h
 * © suhas pai
 */

#pragma once

#include <stdarg.h>
#include <stdatomic.h>

#include "lib/adt/string_view.h"
#include "lib/inttypes.h"

struct console;
struct console {
    struct console *_Atomic next;

    void (*emit_ch)(struct console *console, char ch, uint32_t amt);
    void (*emit_sv)(struct console *console, const struct string_view sv);

    /* Useful for panic() */
    void (*bust_locks)(struct console *);
};

void printk_add_console(struct console *console);

enum log_level {
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR,
    LOGLEVEL_CRITICAL
};

__printf_format(2, 3)
void printk(enum log_level loglevel, const char *string, ...);
void vprintk(enum log_level loglevel, const char *string, va_list list);