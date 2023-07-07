/*
 * kernel/cpu/panic.c
 * © suhas pai
 */

#include "cpu/util.h"
#include "dev/printk.h"
#include "lib/parse_printf.h"

void panic(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    vprintk(LOGLEVEL_CRITICAL, fmt, list);

    va_end(list);
    cpu_halt();
}