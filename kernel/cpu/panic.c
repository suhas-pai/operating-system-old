/*
 * kernel/cpu/panic.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/stack_trace.h"
#endif /* defined(__x86_64__) */

#include "cpu/util.h"
#include "dev/printk.h"

#include "lib/parse_printf.h"

void panic(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    vprintk(LOGLEVEL_CRITICAL, fmt, list);
    va_end(list);

#if defined(__x86_64__)
    print_stack_trace(/*max_lines=*/10);
#endif /* defined(__x86_64__) */

    cpu_halt();
}