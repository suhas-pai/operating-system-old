/*
 * kernel/arch/x86_64/asm/cr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

static inline uint64_t read_cr0() {
    uint64_t cr0 = 0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));

    return cr0;
}

static inline uint64_t read_cr2() {
    uint64_t cr2 = 0;
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));

    return cr2;
}

static inline uint64_t read_cr3() {
    uint64_t cr3 = 0;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline uint64_t read_cr4() {
    uint64_t cr4 = 0;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));

    return cr4;
}

static inline void write_cr0(const uint64_t cr0) {
    asm volatile ("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline void write_cr2(const uint64_t cr2) {
    asm volatile ("mov %0, %%cr2" :: "r"(cr2) : "memory");
}

static inline void write_cr3(const uint64_t cr3) {
    asm volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static inline void write_cr4(const uint64_t cr4) {
    asm volatile ("mov %0, %%cr4" :: "r"(cr4) : "memory");
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

    /*
     * Performance Monitoring Counter Enable
     *
     * Enables execution of the RDPMC instruction for programs or procedures
     * running at any protection level when set; RDPMC instruction can be
     * executed only at protection level 0 when clear.
     */

    CR4_BIT_PCE = (1ull << 8),

    /*
     * Operating System Support for FXSAVE and FXRSTOR instructions
     *
     * When set, this flag:
     *   (1) indicates to software that the operating system supports the use of
     *       the FXSAVE and FXRSTOR instructions,
     *   (2) enables the FXSAVE and FXRSTOR instructions to save and restore the
     *       contents of the XMM and MXCSR registers along with the contents of
     *       the x87 FPU and MMX registers, and
     *   (3) enables the processor to execute SSE/SSE2/SSE3/SSSE3/SSE4
     *       instructions, with the exception of the PAUSE, PREFETCHh, SFENCE,
     *       LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and POPCNT.
     *
     *  If this flag is clear, the FXSAVE and FXRSTOR instructions will save and
     *  restore the contents of the x87 FPU and MMX registers, but they may not
     *  save and restore the contents of the XMM and MXCSR registers. Also,
     *  the processor will generate an invalid opcode exception (#UD) if it
     *  attempts to execute any SSE/SSE2/SSE3 instruction, with the exception of
     *  PAUSE, PREFETCHh, SFENCE, LFENCE, MFENCE, MOVNTI, CLFLUSH, CRC32, and
     *  POPCNT. The operating system or executive must explicitly set this flag.
     */

    CR4_BIT_OSFXSR = (1ull << 9),

    /*
     * OS Support for unmasked simd floating point exceptions
     *
     * When set, indicates that the operating system supports the handling of
     * unmasked SIMD floating-point exceptions through an exception handler that
     * is invoked when a SIMD floating-point exception (#XM) is generated. SIMD
     * floating-point exceptions are only generated by SSE/SSE2/SSE3/SSE4.1 SIMD
     * floating-point instructions.
     *
     * The operating system or executive must explicitly set this flag. If this
     * flag is not set, the processor will generate an invalid opcode exception
     * (#UD) whenever it detects an unmasked SIMD floating-point exception.
     */

    CR4_BIT_OSXMMEXCPTO = (1ull << 10),

    /*
     * When set, the following instructions cannot be executed if CPL > 0:
     * SGDT, SIDT, SLDT, SMSW, and STR. An attempt at such execution causes a
     * general-protection exception (#GP)
     */

    CR4_BIT_UMIP = (1ull << 11),

    /*
     * 57-bit linear addresses
     *
     * When set in IA-32e mode, the processor uses 5-level paging to translate
     * 57-bit linear addresses. When clear in IA-32e mode, the processor uses
     * 4-level paging to translate 48-bit linear addresses. This bit cannot be
     * modified in IA-32e mode.
     */

    CR4_BIT_LA57 = (1ull << 12),

    /* VMXE = Virtual Machine Extensions Enable */

    CR4_BIT_VMXE = (1ull << 13),

    /* SMXE = Safer Mode Extensions Enable */
    CR4_BIT_SMXE = (1ull << 14),

    /* bit 15 is reserved */

    /* Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE. */
    CR4_BIT_FSGSBASE = (1ull << 16),

    /* PCID Enable */
    /* PCID = Page-Level Caching Identifiers */

    CR4_BIT_PCIDE = (1ull << 17),

    /*
     * XSAVE And Processor Extended States Enable
     *
     * When set, this flag:
     *   (1) indicates (via CPUID.01H:ECX.OSXSAVE[bit 27]) that the operating
     *       system supports the use of the XGETBV, XSAVE, and XRSTOR
     *       instructions by general software;
     *   (2) enables the XSAVE and XRSTOR instructions to save and restore the
     *       x87 FPU state (including MMX registers), the SSE state (XMM
     *       registers and MXCSR), along with other processor extended states
     *       enabled in XCR0;
     *   (3) enables the processor to execute XGETBV and XSETBV instructions in
     *       order to read and write XCR0.
     */

    CR4_BIT_OSXSAVE = (1ull << 18),

    /*
     * Key-Locker-Enable
     *
     * When set, the LOADIWKEY instruction is enabled; in addition, if support
     * for the AES Key Locker instructions has been activated by system
     * firmware, CPUID.19H:EBX.AESKLE[bit 0] is enumerated as 1 and the AES Key
     * Locker instructions are enabled.3 When clear, CPUID.19H:EBX.AESKLE[bit 0]
     * is enumerated as 0 and execution of any Key Locker instruction causes an
     * invalid-opcode exception (#UD).
     */

    CR4_BIT_KEY_LOCKER = (1ull << 19),

    /* Supervisor Mode Executions Protection Enable */
    CR4_BIT_SMEP = (1ull << 20),

    /* Supervisor Mode Access Protection Enable */
    CR4_BIT_SMAP = (1ull << 21),

    /*
     * Enable protection keys for user-mode pages
     *
     * 4-level paging and 5-level paging associate each user-mode linear
     * address with a protection key. When set, this flag indicates
     * (via CPUID.(EAX=07H,ECX=0H):ECX.OSPKE [bit 4]) that the operating system
     * supports use of the PKRU register to specify, for each protection key,
     * whether user-mode linear addresses with that protection key can be read
     * or written.
     *
     * This bit also enables access to the PKRU register using the RDPKRU and
     * WRPKRU instructions.
     */

    CR4_BIT_PKE = (1ull << 22),

    /*
     * Enable Control-flow Enforcement Technology
     *
     * This flag can be set only if CR0.WP is set, and it must be clear before
     * CR0.WP can be cleared
     */

    CR4_BIT_CET = (1ull << 23),

    /* Enable protection keys for supervisor-mode pages */
    /*
     * 4-level paging and 5-level paging associate each supervisor-mode linear
     * address with a protection key. When set, this flag allows use of the
     * IA32_PKRS MSR to specify, for each protection key, whether
     * supervisor-mode linear addresses with that protection key can be read or
     * written.
     */

    CR4_BIT_PKS = (1ull << 24),

    /*
     * User Interrupts Enable Bit
     *
     * Enables user interrupts when set, including user-interrupt delivery,
     * user-interrupt notification identification, and the user-interrupt
     * instructions
     */

    CR4_BIT_UINTR = (1ull << 25),

    /*
     * This sets the threshold value corresponding to the highest-priority
     * interrupt to be blocked. A value of 0 means all interrupts are enabled.
     *
     * This field is available in 64-bit mode. A value of 15 means all
     * interrupts will be disabled.
     */

    CR4_BIT_TPL = (1ull << 26),
};

enum {
    /*
     * This bit 0 must be 1. An attempt to write 0 to this bit causes a #GP
     * exception.
     */

    XCR0_BIT_X87 = 1ull << 0,

    /*
     * If 1, the XSAVE feature set can be used to manage MXCSR and the XMM
     * registers (XMM0- XMM15 in 64-bit mode; otherwise XMM0-XMM7)
     */

    XCR0_BIT_SSE = 1ull << 1,

    /*
     * If 1, Intel AVX instructions can be executed and the XSAVE feature set
     * can be used to manage the upper halves of the YMM registers (YMM0-YMM15
     * in 64-bit mode; otherwise YMM0-YMM7).
     */

    XCR0_BIT_AVX = 1ull << 2,

    /*
     * XCR0.BNDREG (bit 3): If 1, Intel MPX instructions can be executed and the
     * XSAVE feature set can be used to manage the bounds registers BND0–BND3.
     */

    XCR0_BIT_BNDREG = 1ull << 3,

    /*
     * If 1, Intel MPX instructions can be executed and the XSAVE feature set
     * can be used to manage the BNDCFGU and BNDSTATUS registers
     */

    XCR0_BIT_BNDCSR = 1ull << 4,

    /*
     * If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
     * set can be used to manage the opmask registers k0–k7
     */

    XCR0_BIT_OPMASK = 1ull << 5,

    /*
     * If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
     * set can be used to manage the upper halves of the lower ZMM registers
     * (ZMM0-ZMM15 in 64-bit mode; otherwise ZMM0- ZMM7).
     */

    XCR0_BIT_ZMM_HI256 = 1ull << 6,

    /*
     * If 1, Intel AVX-512 instructions can be executed and the XSAVE feature
     * set can be used to manage the upper ZMM registers (ZMM16-ZMM31, only in
     * 64-bit mode).
     */

    XCR0_BIT_HI16_ZMM = 1ull << 7,

    /* Bit 8 is reserved */

    /* If 1, the XSAVE feature set can be used to manage the PKRU register */
    XCR0_BIT_PKRU = 1ull << 9,

    /* Bits 10 - 16 are reserved */

    /*
     * If 1, and if XCR0.TILEDATA is also 1, Intel AMX instructions can be
     * executed and the XSAVE feature set can be used to manage TILECFG
     */

    XCR0_BIT_TILECFG = 1ull << 17,

    /*
     * If 1, and if XCR0.TILECFG is also 1, Intel AMX instructions can be
     * executed and the XSAVE feature set can be used to manage TILEDATA
     */

    XCR0_BIT_TILEDATA = 1ull << 18,
};