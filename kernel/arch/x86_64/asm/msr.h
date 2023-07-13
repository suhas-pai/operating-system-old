/*
 * kernel/arch/x86_64/asm/msr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum ia32_msr {
    IA32_MSR_APIC_BASE = 0x1B,
    IA32_MSR_EFER = 0xC0000080,

    IA32_MSR_FS_BASE = 0xC0000100,
    IA32_MSR_GS_BASE = 0xC0000101,
    IA32_MSR_KERNEL_GS_BASE = 0xC0000102,

    /* IA32_MSR_LSTAR is the address SYSCALL jumps to from ring3 */
    IA32_MSR_LSTAR = 0xC0000082,

    /* IA32_MSR_FMASK is a mask of bits to be removed from RFLAGS */
    /* In SYSCALL, RFLAGS &= ~IA32_MSR_FMASK */

    IA32_MSR_FMASK = 0xC0000084,

    /*
     * IA32_MSR_STAR stores the CS (Code Segment) and SS (Stack Segment) info
     * at bits [47:32] and [63:48] respectively.
     */

    IA32_MSR_STAR = 0xC0000081,
    IA32_MSR_TSC_DEADLINE = 0x6E0,

    IA32_MSR_X2APIC_BASE = 0x802,

    IA32_MSR_PAT = 0x277,
    IA32_MSR_MTRR_DEF_TYPE = 0x2FF,
    IA32_MSR_MTRRCAP = 0xFE,
    IA32_MSR_MTRR_PHYSBASE0 = 0x200,
    IA32_MSR_MTRR_PHYSMASK0 = 0x201,
    IA32_MSR_MTRR_PHYSBASE1 = 0x202,
};

enum ia32_msr_efer_flags {
    /* System Call Extensions */
    F_IA32_MSR_EFER_BIT_SCE = (1 << 0),

    /* Long Mode Enable */
    F_IA32_MSR_EFER_BIT_LME = (1 << 8),

    /* Long Mode Active */
    F_IA32_MSR_EFER_BIT_LMA = (1 << 10),

    /* No Execute Enable */
    F_IA32_MSR_EFER_BIT_NXE = (1 << 11),
};

uint64_t read_msr(enum ia32_msr msr);
void write_msr(enum ia32_msr msr, uint64_t value);