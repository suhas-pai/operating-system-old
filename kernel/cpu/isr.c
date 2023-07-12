/*
 * kernel/cpu/isr.c
 * © suhas pai
 */

#include <stddef.h>
#include <stdint.h>

#include "dev/printk.h"
#include "lib/macros.h"

#include "kernel/arch/x86_64/irq_frame.h"
#include "kernel/arch/x86_64/idt.h"

#include "isr.h"

static isr_func_t g_funcs[256] = {};

static isr_vector_t g_free_vector = 0x21;
static isr_vector_t g_timer_vector = 0;
static isr_vector_t g_spur_vector = 0;

int16_t isr_alloc_vector() {
    if (g_free_vector == 0xff) {
        return -1;
    }

    const isr_vector_t result = g_free_vector;
    g_free_vector++;

    return result;
}

isr_vector_t isr_get_timer_vector() {
    return g_timer_vector;
}

isr_vector_t isr_get_spur_vector() {
    return g_spur_vector;
}

void isr_handle_interrupt(const uint64_t vector, irq_frame_t *const frame) {
    printk(LOGLEVEL_DEBUG, "Got interrupt: %" PRIu64 "\n", vector);
    if (g_funcs[vector] != NULL) {
        g_funcs[vector](vector, frame);
    } else {
        printk(LOGLEVEL_INFO, "Got unhandled interrupt %" PRIu64 "\n", vector);
    }
}

void
isr_set_vector(const isr_vector_t vector,
               const isr_func_t func,
               const uint8_t ist)
{
    g_funcs[vector] = func;
#if defined(__x86_64__)
    idt_set_vector(vector, ist, IDT_DEFAULT_FLAGS);
#else
    (void)ist;
#endif /* defined(__x86_64__) */
}

void timer_tick() {

}

void spur_tick() {

}

void isr_init() {
    /* Setup Timer */
    isr_set_vector(isr_alloc_vector(), timer_tick, IST_NONE);

    /* Setup Spurious Interrupt */
    isr_set_vector(isr_alloc_vector(), spur_tick, IST_NONE);
}