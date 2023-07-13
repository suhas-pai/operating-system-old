/*
 * kernel/cpu/isr.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "asm/irq_context.h"

#if defined(__x86_64__)
    #include "idt.h"
    typedef idt_vector_t isr_vector_t;
#else
    typedef uint16_t isr_vector_t;
#endif

typedef void (*isr_func_t)(uint64_t int_no, irq_context_t *frame);

/* Returns -1 on alloc failure */
void isr_init();

isr_vector_t isr_alloc_vector();
isr_vector_t isr_get_spur_vector();
isr_vector_t isr_get_timer_vector();

void isr_handle_interrupt(uint64_t int_no, irq_context_t *reg_state);
void isr_set_vector(isr_vector_t vector, isr_func_t func, uint8_t ist);

void isr_register_for_vector(isr_vector_t vector, isr_func_t handler);

void
isr_assign_irq_to_self_cpu(uint8_t irq,
                           isr_vector_t vector,
                           isr_func_t handler,
                           bool masked);