/*
 * kernel/arch/x86_64/asm/cr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

static inline uint64_t read_cr0() {
    uint64_t cr0 = 0;
    asm volatile ("mov %%cr0, %%rax" : "=a"(cr0));

    return cr0;
}

static inline uint64_t read_cr2() {
    uint64_t cr2 = 0;
    asm volatile ("mov %%cr2, %%rax" : "=a"(cr2));

    return cr2;
}

static inline uint64_t read_cr3() {
    uint64_t cr3 = 0;
    asm volatile ("mov %%cr3, %%rax" : "=a"(cr3));

    return cr3;
}

static inline uint64_t read_cr4() {
    uint64_t cr4 = 0;
    asm volatile ("mov %%cr4, %%rax" : "=a"(cr4));

    return cr4;
}

static inline void write_cr0(const uint64_t cr0) {
    asm volatile ("mov %%rax, %%cr0" :: "a"(cr0) : "memory");
}

static inline void write_cr2(const uint64_t cr2) {
    asm volatile ("mov %%rax, %%cr2" :: "a"(cr2) : "memory");
}

static inline void write_cr3(const uint64_t cr3) {
    asm volatile ("mov %%rax, %%cr3" :: "a"(cr3) : "memory");
}

static inline void write_cr4(const uint64_t cr4) {
    asm volatile ("mov %%rax, %%cr4" :: "a"(cr4) : "memory");
}

enum {
    /* Protection Enable */
    CR0_BIT_PE = (1ull << 0),

    /* Monitor Co-processor */
    CR0_BIT_MP = (1ull << 1),

    /* Emulation - Set for no x87 FPU, Clear for x87 FPu */
    CR0_BIT_EM = (1ull << 2),

    /* Task switched */
    CR0_BIT_TS = (1ull << 3),

    /* Extension type */
    CR0_BIT_ET = (1ull << 4),

    /* Numeric Error */
    CR0_BIT_NE = (1ull << 5),

    /* bits 6-15 are reserved */
    /* Write Protect */

    CR0_BIT_WP = (1ull << 16),

    /* bit 17 is reserved */
    /* Alignment Mask */

    CR0_BIT_AM = (1ull << 18),

    /* bits 19-28 are reserved */
    /* Not-Write Through */

    CR0_BIT_NW = (1ull << 29),

    /* Cache Disable */

    CR0_BIT_CD = (1ull << 30),

    /* Paging */

    CR0_BIT_PG = (1ull << 31)
};

enum {
    /* Virtual-8086 Mode Extensions */
    CR4_BIT_VME = (1ull << 0),

    /* Protected Mode Virtual Interrupts */
    CR4_BIT_PVI = (1ull << 1),

    /* Time Stamp enabled only in ring 0 */
    CR4_BIT_TSD = (1ull << 2),

    /* Debugging Extensions */
    CR4_BIT_DE = (1ull << 3),

    /* Page Size Extension */
    CR4_BIT_PSE = (1ull << 4),

    /* Physical Address Extension */
    CR4_BIT_PAE = (1ull << 5),

    /* Machine Check Exception */
    CR4_BIT_MCE = (1ull << 6),

    /* Page Global Enable */
    CR4_BIT_PGE = (1ull << 7),

    /* Performance Monitoring Counter Enable */
    CR4_BIT_PCE = (1ull << 8),

    /* OS support for fxsave and fxrstor instructions */
    CR4_BIT_OSFXSR = (1ull << 9),

    /* OS Support for unmasked simd floating point exceptions */
    CR4_BIT_OSXMMEXCPTO = (1ull << 10),

    /*
     * User-Mode Instruction Prevention (SGDT, SIDT, SLDT, SMSW, and STR are
     * disabled in user mode)
     */

    CR4_BIT_UMIP = (1ull << 11),

    /* bit 12 is reserved */
    /* Virtual Machine Extensions Enable */

    CR4_BIT_VMXE = (1ull << 13),

    /* Safer Mode Extensions Enable */
    CR4_BIT_SMXE = (1ull << 14),

    /* bit 15 is reserved */
    /* bit 16 is missing */

    /* PCID Enable */
    /* PCID = Page-Level Caching Identifiers */

    CR4_BIT_PCIDE = (1ull << 17),

    /* XSAVE And Processor Extended States Enable */
    CR4_BIT_OSXSAVE = (1ull << 18),

    /* bit 19 is reserved */
    /* Supervisor Mode Executions Protection Enable */

    CR4_BIT_SMEP = (1ull << 20),

    /* Supervisor Mode Access Protection Enable */
    CR4_BIT_SMAP = (1ull << 21),

    /* Enable protection keys for user-mode pages */
    CR4_BIT_PKE = (1ull << 22),

    /* Enable Control-flow Enforcement Technology */
    CR4_BIT_CET = (1ull << 23),

    /* Enable protection keys for supervisor-mode pages */
    CR4_BIT_PKS = (1ull << 24),
};
