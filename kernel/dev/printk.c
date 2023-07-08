/*
 * kernel/dev/printk.c
 * © suhas pai
 */

#include "lib/parse_printf.h"

#include "cpu/spinlock.h"
#include "printk.h"

static struct console *_Atomic g_first_console = NULL;
void printk_add_console(struct console *const console) {
    atomic_store(&console->next, g_first_console);
    atomic_store(&g_first_console, console);
}

void printk(const enum log_level loglevel, const char *const string, ...) {
    va_list list;
    (void)loglevel;

    va_start(list, string);
    vprintk(loglevel, string, list);
    va_end(list);
}

// FIXME: Allocate a formatted-string over this approach
static uint64_t
write_char(struct printf_spec_info *const spec_info,
           void *const cb_info,
           const char ch,
           const uint64_t amount,
           bool *const cont_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)cont_out;

    for (struct console *console = atomic_load(&g_first_console);
         console != NULL;
         console = atomic_load(&console->next))
    {
        console->emit_ch(console, ch, amount);
    }

    return amount;
}

static uint64_t
write_sv(struct printf_spec_info *const spec_info,
         void *const cb_info,
         const struct string_view sv,
         bool *const cont_out)
{
    (void)spec_info;
    (void)cb_info;
    (void)cont_out;

    for (struct console *console = atomic_load(&g_first_console);
         console != NULL;
         console = atomic_load(&console->next))
    {
        console->emit_sv(console, sv);
    }

    return sv.length;
}

void
vprintk(const enum log_level loglevel,
        const char *const string,
        va_list list)
{
    (void)loglevel;

    static struct spinlock lock = {};
    const int flag = spin_acquire_with_irq(&lock);

    parse_printf(string,
                 write_char,
                 /*char_cb_info=*/NULL,
                 write_sv,
                 /*sv_cb_info=*/NULL,
                 list);

    spin_release_with_irq(&lock, flag);
}