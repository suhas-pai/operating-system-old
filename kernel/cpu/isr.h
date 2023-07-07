/*
 * kernel/cpu/isr.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>

#include "kernel/arch/x86_64/idt.h"
#include "kernel/arch/x86_64/irq_frame.h"

typedef idt_vector_t isr_vector_t;
typedef void (*isr_func_t)(uint64_t int_no, irq_frame_t *frame);

/* Returns -1 on alloc failure */
void isr_init();
int16_t isr_alloc_vector();

void isr_handle_interrupt(uint64_t int_no, irq_frame_t *reg_state);
void isr_set_vector(isr_vector_t vector, isr_func_t func, uint8_t ist);