/*
 * kernel/cpu/irq_frame.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct x86_64_irq_frame {
    /* Pushed by pushaq */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx,
             rax;

    /* Error Code, 0 if not applicable */
    uint64_t err_code;

    /* Pushed by the processor automatically. */
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct x86_64_irq_frame irq_frame_t;