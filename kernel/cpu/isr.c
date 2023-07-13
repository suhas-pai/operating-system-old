/*
 * kernel/cpu/isr.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "apic/ioapic.h"
    #include "apic/lapic.h"
#endif

#include "dev/printk.h"

#include "cpu.h"
#include "isr.h"

static isr_func_t g_funcs[256] = {};

static isr_vector_t g_free_vector = 0x21;
static isr_vector_t g_timer_vector = 0;
static isr_vector_t g_spur_vector = 0;

isr_vector_t isr_alloc_vector() {
    assert(g_free_vector != 0xff);

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

void isr_handle_interrupt(const uint64_t vector, irq_context_t *const frame) {
    if (g_funcs[vector] != NULL) {
        g_funcs[vector](vector, frame);
    } else {
        printk(LOGLEVEL_INFO, "Got unhandled interrupt %" PRIu64 "\n", vector);
    }

#if defined(__x86_64__)
    lapic_eoi();
#endif /* defined(__x86_64__) */
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

static void spur_tick(const uint64_t int_no, irq_context_t *const frame) {
    (void)int_no;
    (void)frame;

    get_cpu_info_mut()->spur_int_count++;
}

void isr_init() {
    /* Setup Timer */
    g_timer_vector = isr_alloc_vector();

    /* Setup Spurious Interrupt */
    g_spur_vector = isr_alloc_vector();

#if defined(__x86_64__)
    const uint8_t spur_ist = IST_NONE;
#else
    const uint8_t spur_ist = 0;
#endif

    isr_set_vector(g_spur_vector, spur_tick, spur_ist);

#if defined(__x86_64__)
    idt_register_exception_handlers();
#endif /* defined(__x86_64__) */
}

void
isr_register_for_vector(const isr_vector_t vector, const isr_func_t handler) {
    assert(g_funcs[vector] == NULL);
    g_funcs[vector] = handler;

    printk(LOGLEVEL_INFO,
           "isr: registered handler for vector %" PRIu8 "\n",
           vector);
}

void
isr_assign_irq_to_self_cpu(const uint8_t irq,
                           const isr_vector_t vector,
                           const isr_func_t handler,
                           const bool masked)
{
    isr_register_for_vector(vector, handler);

#if defined(__x86_64__)
    ioapic_redirect_irq(get_cpu_info()->lapic_id, irq, vector, masked);
#else
    (void)irq;
    (void)vector;
    (void)handler;
    (void)masked;
#endif /* defiend(__x86_64__)*/
}