/*
 * kernel/idt.c
 * © suhas pai
 */

#include <stdint.h>

#include "lib/macros.h"
#include "pic.h"

#include "gdt.h"
#include "idt.h"

struct idt_descriptor {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t reserved;
};

struct idt_register {
    uint16_t idt_size_minus_one;
    struct idt_descriptor *idt;
} __packed;

enum exception {
    EXCEPTION_DIVIDE_BY_ZERO,
    EXCEPTION_DEBUG,

    /* NMI = Non-Maskable Interrupt */
    EXCEPTION_NMI,

    /* Breakpoint = Debug Trap */
    EXCEPTION_BREAKPOINT,

    /* Overflow = Arithmetic Overflow */
    EXCEPTION_OVERFLOW,

    /* Bound = Array Bounds Exceeded. For BOUND instruction */
    EXCEPTION_BOUND,

    EXCEPTION_INVALID_OPCODE,
    EXCEPTION_DEVICE_NOT_AVAILABLE,
    EXCEPTION_DOUBLE_FAULT,

    EXCEPTION_COPROC_SEGMENT_OVERRUN,
    EXCEPTION_INVALID_TSS,

    EXCEPTION_SEGMENT_NOT_PRESENT,
    EXCEPTION_STACK_FAULT,
    EXCEPTION_GENERAL_PROTECTION_FAULT,
    EXCEPTION_PAGE_FAULT,

    /* 15 = Reserved */

    EXCEPTION_FPU_FAULT = 16,
    EXCEPTION_ALIGNMENT_CHECK,
    EXCEPTION_MACHINE_CHECK,
    EXCEPTION_SIMD_FLOATING_POINT,
    EXCEPTION_VIRTUALIZATION_EXCEPTION,
    EXCEPTION_CONTROL_PROTECTION_EXCEPTION,

    /* 22-27 = Reserved */
    EXCEPTION_HYPERVISOR_EXCEPTION = 28,
    EXCEPTION_VMM_EXCEPTION,
    EXCEPTION_SECURITY_EXCEPTION,
};

static struct idt_descriptor g_idt[256] = {0};
extern void *const idt_thunks[];

void
idt_set_vector(const idt_vector_t vector,
               const uint8_t ist,
               const uint8_t flags)
{
    const uint64_t handler_addr = (uint64_t)idt_thunks[vector];
    g_idt[vector] = (struct idt_descriptor){
        .offset_low = (uint16_t)handler_addr,
        .selector = gdt_get_kernel_code_segment(),
        .ist = ist,
        .flags = flags,
        .offset_mid = (uint16_t)(handler_addr >> 16),
        .offset_hi = (uint32_t)(handler_addr >> 32),
        .reserved = 0
    };
}

void idt_load() {
    static struct idt_register idt_reg = {
        .idt_size_minus_one = sizeof(g_idt) - 1,
        .idt = g_idt
    };

    asm volatile ("lidt %0" :: "m"(idt_reg) : "memory");
}

void idt_init() {
    pic_remap(247, 255);
    for (uint16_t i = 0; i != 0xFF; i++) {
        idt_set_vector(i, IST_NONE, 0x8e);
    }

    idt_load();
}