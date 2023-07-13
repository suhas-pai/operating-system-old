/*
 * kernel/arch/x86_64/asm/cpuid.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

/* Vendor-strings. */
/* early engineering samples of AMD K5 processor */
#define CPUID_VENDOR_OLDAMD       "AMDisbetter!"
#define CPUID_VENDOR_AMD          "AuthenticAMD"
#define CPUID_VENDOR_INTEL        "GenuineIntel"
#define CPUID_VENDOR_VIA          "CentaurHauls"
#define CPUID_VENDOR_OLDTRANSMETA "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA    "GenuineTMx86"
#define CPUID_VENDOR_CYRIX        "CyrixInstead"
#define CPUID_VENDOR_CENTAUR      "CentaurHauls"
#define CPUID_VENDOR_NEXGEN       "NexGenDriven"
#define CPUID_VENDOR_UMC          "UMC UMC UMC "
#define CPUID_VENDOR_SIS          "SiS SiS SiS "
#define CPUID_VENDOR_NSC          "Geode by NSC"
#define CPUID_VENDOR_RISE         "RiseRiseRise"
#define CPUID_VENDOR_VORTEX       "Vortex86 SoC"
#define CPUID_VENDOR_VIA_OTHER    "VIA VIA VIA "

/* Vendor-strings from Virtual Machines. */
#define CPUID_VENDOR_VMWARE       "VMwareVMware"
#define CPUID_VENDOR_XENHVM       "XenVMMXenVMM"
#define CPUID_VENDOR_MICROSOFT_HV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS    " lrpepyh vr"

enum {
    CPUID_FEAT_EAX_AVXVNNI    = 1 << 4,
    CPUID_FEAT_EAX_AVX512BF16 = 1 << 5,
    CPUID_FEAT_EAX_HRESET     = 1 << 22,

    /* SSE3 = Streaming SIMD Extensions 3 */
    CPUID_FEAT_ECX_SSE3    = 1 << 0,

    /* PCLMUL = Polynomial Multiplication */
    CPUID_FEAT_ECX_PCLMUL  = 1 << 1,

    /* DTES64 = 64-bit Debug Store */
    CPUID_FEAT_ECX_DTES64  = 1 << 2,

    /* Supports MONITOR/MWAIT instructions */
    CPUID_FEAT_ECX_MONITOR = 1 << 3,

    /* DS-CPL = CPL Qualified Debug Store */
    CPUID_FEAT_ECX_DS_CPL  = 1 << 4,

    /* VMX = Virtual Machine Extensions */
    CPUID_FEAT_ECX_VMX     = 1 << 5,

    /* SMX = Safer Mode Extensions */
    CPUID_FEAT_ECX_SMX     = 1 << 6,

    /* EST = Enhanced SpeedStep */
    CPUID_FEAT_ECX_EST     = 1 << 7,

    /* TM2 = Thermal Monitor 2 */
    CPUID_FEAT_ECX_TM2     = 1 << 8,

    /* SSSE3 = Supplemental Streaming SIMD Extensions 3 */
    CPUID_FEAT_ECX_SSSE3   = 1 << 9,

    /* CID = Context ID */
    CPUID_FEAT_ECX_CID     = 1 << 10,

    /* FMA = Fused Multiply Add */
    CPUID_FEAT_ECX_FMA     = 1 << 12,

    /* Supports CMPXCHG16B instruction */
    CPUID_FEAT_ECX_CX16    = 1 << 13,

    /* ETRPD = Extended Topology Enumeration and CPUID Leaf Identifiers */
    CPUID_FEAT_ECX_ETPRD   = 1 << 14,

    /* PDCM = Perfmon and Debug Capability MSR */
    CPUID_FEAT_ECX_PDCM    = 1 << 15,

    /* PCIDE = Performance-Monitoring Counter Interrupts */
    CPUID_FEAT_ECX_PCIDE   = 1 << 17,

    /* DCA = Direct Cache Access */
    CPUID_FEAT_ECX_DCA     = 1 << 18,

    /* SSE4_1 = Streaming SIMD Extensions 4.1 */
    CPUID_FEAT_ECX_SSE4_1  = 1 << 19,

    /* SSE4_2 = Streaming SIMD Extensions 4.2 */
    CPUID_FEAT_ECX_SSE4_2  = 1 << 20,
    CPUID_FEAT_ECX_x2APIC  = 1 << 21,

    /* Supports MOVBE Instruction */
    CPUID_FEAT_ECX_MOVBE   = 1 << 22,

    /* Supports POPCNT instruction */
    CPUID_FEAT_ECX_POPCNT  = 1 << 23,

    /* AES = Advanced Encryption Standard */
    CPUID_FEAT_ECX_AES     = 1 << 25,

    /* Supports XSAVE/XRSTOR instructions */
    CPUID_FEAT_ECX_XSAVE   = 1 << 26,

    /* OSXSAVE = XSAVE/XRSTOR enabled by OS */
    CPUID_FEAT_ECX_OSXSAVE = 1 << 27,

    /* AVX = Advanced Vector Extensions */
    CPUID_FEAT_ECX_AVX     = 1 << 28,

    /* F16 = Float16 */
    CPUID_FEAT_ECX_F16     = 1 << 29,

    /* Supports RDRAND instruction */
    CPUID_FEAT_ECX_RDRAND  = 1 << 30,

    /* bit 31 is unused */

    /* FPU = Floating Point Unit */
    CPUID_FEAT_EDX_FPU     = 1 << 0,

    /* VME = Virtual Mode Extensions */
    CPUID_FEAT_EDX_VME     = 1 << 1,

    /* DE = Debugging Extensions */
    CPUID_FEAT_EDX_DE      = 1 << 2,

    /* PSE = Page Size Extensions */
    CPUID_FEAT_EDX_PSE     = 1 << 3,

    /* TSC = Time Stamp Counter */
    CPUID_FEAT_EDX_TSC     = 1 << 4,

    /* MSR = Model Specific Registers */
    CPUID_FEAT_EDX_MSR     = 1 << 5,

    /* PAE = Page Address Extension */
    CPUID_FEAT_EDX_PAE     = 1 << 6,

    /* MCE = Machine Check Exception */
    CPUID_FEAT_EDX_MCE     = 1 << 7,

    /* Supports CMPXCHG8B instruction */
    CPUID_FEAT_EDX_CX8     = 1 << 8,

    /* APIC = Advanced Programmable Interrupt Controller */
    CPUID_FEAT_EDX_APIC    = 1 << 9,

    /* SEP = SYSENTER/SYSEXIT */
    CPUID_FEAT_EDX_SEP     = 1 << 11,

    /* MTRR = Memory Type Range Registers */
    CPUID_FEAT_EDX_MTRR    = 1 << 12,

    /* PGE = Page Global Enable */
    CPUID_FEAT_EDX_PGE     = 1 << 13,

    /* MCA = Machine Check Architecture */
    CPUID_FEAT_EDX_MCA     = 1 << 14,

    /* CMOV = Conditional Move */
    CPUID_FEAT_EDX_CMOV    = 1 << 15,

    /* PAT = Page Attribute Table */
    CPUID_FEAT_EDX_PAT     = 1 << 16,

    /* PSE36 = Page Size Extensions */
    CPUID_FEAT_EDX_PSE36   = 1 << 17,

    /* PSN = Processor Serial Number */
    CPUID_FEAT_EDX_PSN     = 1 << 18,

    /* Supports CLFLUSHOPT, CLFLUSH = CLFLUSHNOP instructions */
    CPUID_FEAT_EDX_CLF = 1 << 19,

    /* DTES = Data Trace Extensions */
    CPUID_FEAT_EDX_DTES = 1 << 21,

    /* ACPI = Advanced Configuration and Power Interface */
    CPUID_FEAT_EDX_ACPI = 1 << 22,

    /* MMX = M */
    CPUID_FEAT_EDX_MMX = 1 << 23,

    /* FXSR is a non-standard extension to the CPUID feature flags. */
    CPUID_FEAT_EDX_FXSR = 1 << 24,

    /* SSE = Streaming SIMD Extensions */
    CPUID_FEAT_EDX_SSE = 1 << 25,

    /* SSE2 = Streaming SIMD Extensions 2 */
    CPUID_FEAT_EDX_SSE2 = 1 << 26,

    CPUID_FEAT_EDX_SS = 1 << 27,

    /* HTT = Hyper-Threading Technology */
    CPUID_FEAT_EDX_HTT = 1 << 28,

    /* TM1 = Thermal Monitor 1 */
    CPUID_FEAT_EDX_TM1 = 1 << 29,

    CPUID_FEAT_EDX_IA64 = 1 << 30,

    /* PBE is a synonym for PSE36. */
    CPUID_FEAT_EDX_PBE = 1 << 31,

    /* FSGSBASE = FS/GS BASE access instructions */
    CPUID_FEAT_EXT_7_EBX_FSGSBASE = 1 << 0,

    /* IA32_TSC_ADJUST MSR is supported if 1 */
    CPUID_FEAT_EXT_7_EBX_MSR_TSC_ADJUST  = 1 << 1,

    /* SGX = Software Guard Extensions */
    CPUID_FEAT_EXT_7_EBX_SGX = 1 << 2,

    /* BMI = Bit Manipulation Instructions */
    CPUID_FEAT_EXT_7_EBX_BMI1 = 1 << 3,

    /*
     * Supports Intel® Memory Protection Extensions (Intel® MLE Extensions) if
     * 1.
     */

    CPUID_FEAT_EXT_7_EBX_HLE = 1 << 4,

    /* Supports Advanced Vector Extensions (AVX) if 1. */
    CPUID_FEAT_EXT_7_EBX_AVX = 1 << 5,

    /* x87 FPU Data Pointer updated only on x87 exceptions if 1 */
    CPUID_FEAT_EXT_7_EBX_FDP_EXCPTN_ONLY = 1 << 6,

    /* SMEP = Supervisor Mode Execution Protection */
    CPUID_FEAT_EXT_7_EBX_SMEP = 1 << 7,

    /* BMI2 = Bit Manipulation Extensions 2 */
    CPUID_FEAT_EXT_7_EBX_BMI2 = 1 << 8,

    /* Supports Enhanced REP MOVSB/STOSB if 1. */
    CPUID_FEAT_EXT_7_EBX_REP_MOVSB_STOSB = 1 << 9,

    /*
     * Supports INVPCID instruction for system software that manages
     * process-context id
     */

    CPUID_FEAT_EXT_7_EBX_INVPCID = 1 << 10,

    /*
     * Supports Restricted Transactional Memory (RTM) if 1.
     *
     * RTM is a feature that allows a transaction to be executed
     * atomically on a processor.
     *
     * The feature is supported if CPUID.7H.EBX[11] = 1.
     */

    CPUID_FEAT_EXT_7_EBX_RTM = 1 << 11,

    /*
     * Supports Intel Resource Director Technology (Intel® Resource Director
     * Technology) if 1.
     *
     * Intel® Resource Director Technology (Intel® Resource Director Technology)
     * is a collection of instructions that provide a processor ID.
     */

    CPUID_FEAT_EXT_7_EBX_RDT_M = 1 << 12,

    /* Deprecates FPU CS and FPU DS values if 1 */
    CPUID_FEAT_EXT_7_EBX_DPRC_FPU_FS = 1 << 13,

    /* MPX = Memory Protection Extensions */
    /* Supports Intel® Memory Protection Extensions if 1 */

    CPUID_FEAT_EXT_7_EBX_MPX = 1 << 14,

    /*
     * Supports Intel® Resource Director Technology (Intel® RDT) Allocation
     * capability if 1.
     */

    CPUID_FEAT_EXT_7_EBX_RDT_A = 1 << 15,

    /*
     * Supports AVX512F (AVX-512 Foundation) if 1.
     *
     * AVX-512 Foundation is a collection of instructions that provide a set of
     * 512 bits of vector registers. The 512-bit registers are used to perform
     * vector operations.
     */

    CPUID_FEAT_EXT_7_EBX_AVX512F = 1 << 16,

    /* AVX512DQ = */
    /*
     * Supports AVX512DQ (AVX-512 Doubleword and Quadword Instructions) if 1.
     *
     * AVX-512 Doubleword and Quadword Instructions is a collection of 512-bit
     * AVX-512 instructions that operate on 128-bit doublewords and 128-bit
     * quadwords. The 512-bit registers are used to perform vector operations.
     */

    CPUID_FEAT_EXT_7_EBX_AVX512DQ = 1 << 17,

    /* RDSEED = Intel® Resource Director Technology (Intel® RDT) Seed */
    CPUID_FEAT_EXT_7_EBX_RDSEED = 1 << 18,

    /* ADX = Intel® Advanced Vector Extensions-XMM */
    CPUID_FEAT_EXT_7_EBX_ADX = 1 << 19,

    /* SMAP = Supervisor Mode Access Prevention */
    /*
     * Supports Supervisor-Mode Access Prevention (and the CLAC/STAC
     * instructions) if 1.
     */

    CPUID_FEAT_EXT_7_EBX_SMAP = 1 << 20,

    /* AVX512_IFMA = AVX-512 Integer Fused Multiply-Add */
    CPUID_FEAT_EXT_7_EBX_AVX512_IFMA = 1 << 21,

    /* Bit 22 is reserved */
    /* Supports CLFLUSHOPT instruction */

    CPUID_FEAT_EXT_7_EBX_CLFLUSHOPT = 1 << 23,

    /* CLWB = Intel® Cloned Flush to WB */
    CPUID_FEAT_EXT_7_EBX_CLWB = 1 << 24,

    /* IPT = Intel Processor Trace */
    CPUID_FEAT_EXT_7_EBX_IPT = 1 << 25,

    /* AVX512PF = AVX-512 Prefetch */
    CPUID_FEAT_EXT_7_EBX_AVX512PF = 1 << 26,

    /* AVX512ER = AVX-512 Exponential and Reciprocal */
    CPUID_FEAT_EXT_7_EBX_AVX512ER = 1 << 27,

    /* AVX512CD = AVX-512 Conflict Detection */
    CPUID_FEAT_EXT_7_EBX_AVX512CD = 1 << 28,

    /*
     * Supports Intel® Secure Hash Algorithm Extensions (Intel® SHA Extensions)
     * if 1.
     */

    CPUID_FEAT_EXT_7_EBX_SHA = 1 << 29,

    /* AVX512BW = AVX-512 BW (Byte and Word) Instructions */
    CPUID_FEAT_EXT_7_EBX_AVX512BW = 1 << 30,

    /* AVX512VL = AVX-512 VL (128/256 Vector Length) Extensions */
    CPUID_FEAT_EXT_7_EBX_AVX512VL = 1 << 31,

    /* Supports PREFETCHWT1 instruction */
    CPUID_FEAT_EXT_7_ECX_PREFETCHWT1 = 1 << 0,

    /* AVX512_VBMI = AVX-512 Vector Bit Manipulation Instructions */
    CPUID_FEAT_EXT_7_ECX_AVX512_VBMI = 1 << 1,

    /* UMIP = User Mode Instruction Prevention */
    CPUID_FEAT_EXT_7_ECX_UMIP = 1 << 2,

    /* PKU = Protection Keys for User-Mode Pages */
    CPUID_FEAT_EXT_7_ECX_PKU = 1 << 3,

    /* OSPKE = OS Protection Keys Enable */
    CPUID_FEAT_EXT_7_ECX_OSPKE = 1 << 4,

    /* Supports WAITPKG Instruction */
    CPUID_FEAT_EXT_7_ECX_WAITPKG = 1 << 5,

    /* AVX512_VBMI2 = AVX-512 Vector Bit Manipulation Instructions 2 */
    CPUID_FEAT_EXT_7_ECX_AVX512_VBMI2 = 1 << 6,

    /* CET_SS = Cache Extension Technology for SSE */
    /*
     * Supports CET shadow stack features if 1. Processors that set this bit
     * define bits 1:0 of the IA32_U_CET and IA32_S_CET MSRs. Enumerates support
     * for the following MSRs:
     *   IA32_INTERRUPT_SPP_TABLE_ADDR
     *   IA32_PL3_SSP
     *   IA32_PL2_SSP
     *   IA32_PL1_SSP
     *   IA32_PL0_SSP.
     */

    CPUID_FEAT_EXT_7_ECX_CET_SS = 1 << 7,

    /* GFNI = Galois Field New Instructions */
    /*
     * Supports Galois Field New Instructions (GFNI) if 1.
     *
     * GFNI is a collection of new instructions that operate on the Galois
     * field of floating-point values.
     */
    CPUID_FEAT_EXT_7_EXC_GFNI = 1 << 8,

    /* VAES = Vector AES */
    /*
     * Supports Vector AES (VAES) if 1.
     *
     * VAES is a collection of instructions that operate on 128-bit AES
     * encryption keys.
     */

    CPUID_FEAT_EXT_7_ECX_VAES = 1 << 9,

    /* VPCLMULQDQ = Vector Polynomial Multiplication Quadword */
    /*
     * Supports Vector Polynomial Multiplication Quadword (VPCLMULQDQ) if 1.
     *
     * VPCLMULQDQ is a collection of instructions that operate on 128-bit
     * polynomial multiplications.
     */

    CPUID_FEAT_EXT_7_ECX_VPCLMULQDQ = 1 << 10,

    /* AVX512_VNNI = AVX-512 Vector Neural Network Instructions */
    /*
     * Supports AVX-512 Vector Neural Network Instructions (AVX-512 Vector
     * Neural Network Instructions) if 1.
     *
     * AVX-512 Vector Neural Network Instructions (AVX-512 Vector Neural
     * Network Instructions) are a collection of instructions that operate on
     * 128-bit vectors of floating-point values.
     */

    CPUID_FEAT_EXT_7_ECX_AVX512_VNNI = 1 << 11,

    /* AVX512_BITALG = AVX-512 Bit Algorithms */
    /*
     * Supports AVX-512 Bit Algorithms (AVX-512 Bit Algorithms) if 1.
     *
     * AVX-512 Bit Algorithms (AVX-512 Bit Algorithms) are a collection of
     * instructions that operate on 128-bit vectors of floating-point values.
     */

    CPUID_FEAT_EXT_7_ECX_AVX512_BITALG = 1 << 12,

    /* Bit 13 is reserved */

    /* AVX512_VPOPCNTDQ = AVX-512 VPOPCNTDQ */
    /*
     * Supports AVX-512 VPOPCNTDQ (AVX-512 VPOPCNTDQ) if 1.
     *
     * AVX-512 VPOPCNTDQ (AVX-512 VPOPCNTDQ) is a collection of instructions
     * that operate on 128-bit vectors of floating-point values.
     */

    CPUID_FEAT_EXT_7_ECX_AVX512_VPOPCNTDQ = 1 << 14,

    /*
     * Supports LA57 (LA57) if 1.
     *
     * LA57 is a collection of instructions that operate on 128-bit vectors of
     * floating-point values.
     */

    CPUID_FEAT_EXT_7_ECX_LA57 = 1 << 15,

    /*
     * Supports Read Processor ID (RDPID) if 1.
     *
     * RDPID is a collection of instructions that provide a processor ID.
     */

    CPUID_FEAT_EXT_7_ECX_RDPID = 1 << 22,

    /*
     * Supports IA32_KL (KL) if 1.
     *
     * IA32_KL (KL) is a collection of instructions that provide a processor ID.
     */

    CPUID_FEAT_EXT_7_ECX_KL = 1 << 23,

    /* Bit 24 is reserved */
    /*
     * Supports Cache Line Demote (CLDEMOTE) if 1.
     *
     * CLDEMOTE is a collection of instructions that demote cache lines.
     */

    CPUID_FEAT_EXT_7_ECX_CLDEMOTE = 1 << 25,

    /* Bit 26 is reserved */

    /*
     * Supports MOVDIRI (MOVDIRI) if 1.
     *
     * MOVDIRI is a collection of instructions that move directory information
     * from one location to another.
     */

    CPUID_FEAT_EXT_7_ECX_MOVDIRI = 1 << 27,

    /*
     * Supports MOVDIR64B (MOVDIR64B) if 1.
     *
     * MOVDIR64B is a collection of instructions that move directory information
     * from one location to another.
     */

    CPUID_FEAT_EXT_7_ECX_MOVDIR64B = 1 << 28,

    /* Bit 29 is reserved */

    /*
     * Supports SGX Launch Configuration (SGX_LC) if 1.
     *
     * SGX_LC is a collection of instructions that provide information about
     * the SGX launch configuration.
     */

    CPUID_FEAT_EXT_7_ECX_SGX_LC = 1 << 30,

    /*
     * Supports Protection Keys for Supervisor-Mode Pages (PKS) if 1.
     *
     * PKS is a collection of instructions that provide protection keys for
     * supervisor-mode pages.
     */

    CPUID_FEAT_EXT_7_ECX_PKS = 1 << 31,

    /* Bit 0 is unused */
    /* Bit 1 is reserved */

    /*
     * AVX512_4VNNIW = Vector instructions for deep learning enhanced word
     * variable precision.
     */

    CPUID_FEAT_EXT_7_EDX_AVX512_4VNNIW = 1 << 2,

    /*
     * AVX512_4FMAPS = Vector instructions for deep learning enhanced word
     * variable precision.
     */

    CPUID_FEAT_EXT_7_EDX_AVX512_4FMAPS = 1 << 3,

    /*
     * Supports Fast Short Rep Prefix (FAST_SHORT_REP_PREFIX) if 1.
     *
     * FAST_SHORT_REP_PREFIX is a collection of instructions that provide a
     * fast short rep prefix.
     */

    CPUID_FEAT_EXT_7_EDX_FAST_SHORT_REP_PREFIX = 1 << 4,

    /*
     * Supports Intel Processor Trace (PT) if 1.
     *
     * PT is a collection of instructions that provide a processor trace.
     */

    CPUID_FEAT_EXT_7_EDX_PT = 1 << 5,

    /* Bit 5-7 are reserved */
    /*
     * Supports AVX-512 VP2INTERSECT (AVX-512 VP2INTERSECT) if 1.
     *
     * AVX-512 VP2INTERSECT (AVX-512 VP2INTERSECT) is a collection of
     * instructions that operate on 128-bit vectors of floating-point values.
     */

    CPUID_FEAT_EXT_7_EDX_AVX512_VP2INTERSECT = 1 << 8,

    /* Bit 9 is reserved */

    /*
     * Supports Memory Disambiguation Clear (MD_CLEAR) if 1.
     *
     * MD_CLEAR is a collection of instructions that provide memory
     * disambiguation.
     */

    CPUID_FEAT_EXT_7_EDX_MD_CLEAR = 1 << 10,

    /* Bit 11-14 are reserved */
    /* Hybrid Graphics (HYPERVISOR_GUEST) */

    CPUID_FEAT_EXT_7_EDX_HYPERVISOR_GUEST = 1 << 15,

    /* Bit 16-19 are reserved */
    /*
     * Supports CET and IBT (CET_IBT) if 1.
     *
     * CET and IBT are a collection of instructions that provide a branch
     * prediction and code execution timing.
     *
     * Processors that set this bit define [5:2] and bits 63:10 of the
     * IA32_U_CET and IA32_S_CET MSRs.
     */

    CPUID_FEAT_EXT_7_EDX_CET_IBT = 1 << 20,

    /* Bit 21-25 are reserved */
    /*
     * Supports IBRS if 1.
     *
     * IBRS is a collection of instructions that provide indirect branch
     * restricted speculation.
     *
     * Enumerates support for indirect branch restricted speculation (IBRS) and
     * the indirect branch pre-dictor barrier (IBPB). Processors that set this
     * bit support the IA32_SPEC_CTRL MSR and the IA32_PRED_CMD MSR. They allow
     * software to set IA32_SPEC_CTRL[0] (IBRS) and IA32_PRED_CMD[0] (IBPB).
     */

    CPUID_FEAT_EXT_7_EDX_IBRS = 1 << 26,

    /*
     * Enumerates support for single thread indirect branch predictors (STIBP).
     * Processors that set this bit support the IA32_SPEC_CTRL MSR. They allow
     * software to set IA32_SPEC_CTRL[1] (STIBP)
     */

    CPUID_FEAT_EXT_7_EDX_STIBP = 1 << 27,

    /*
     * Enumerates support for L1D_FLUSH. Processors that set this bit support
     * the IA32_FLUSH_CMD MSR. They allow software to set IA32_FLUSH_CMD[0]
     * (L1D_FLUSH)
     */

    CPUID_FEAT_EXT_7_EDX_L1D_FLUSH = 1 << 28,

    /*
     * Enumerates support for the IA32_ARCH_CAPABILITIES MSR.
     *
     * IA32_CORE_CAPABILITIES is an architectural MSR that enumerates
     * model-specific features. A bit being set in this MSR indicates that a
     * model specific feature is supported; software must still consult CPUID
     * family/model/stepping to determine the behavior of the enumerated feature
     * as features enumerated in IA32_CORE_CAPABILITIES may have different
     * behavior on different processor models.
     *
     * Additionally, on hybrid parts (CPUID.07H.0H:EDX[15]=1), software must
     * consult the native model ID and core type from the Hybrid Information
     * Enumeration Leaf.
     */

    CPUID_FEAT_EXT_7_EDX_ARCH_CAPABILITIES = 1 << 29,
    CPUID_FEAT_EXT_7_EDX_ARCH_CAPABILITIES_2 = 1 << 30,

    /*
     * Speculative Store Bypass Disable (SSBD) is a collection of
     * instructions that provide Speculative Store Bypass Disable (SSBD).
     *
     * Enumerates support for Speculative Store Bypass Disable (SSBD).
     * Processors that set this bit support the IA32_SPEC_CTRL MSR. They allow
     * software to set IA32_SPEC_CTRL[2] (SSBD).
     */

    CPUID_FEAT_EXT_7_EDX_SSBD = 1 << 31,
};

enum cpuid_requests {
    CPUID_GET_VENDOR_STRING,
    CPUID_GET_FEATURES,
    CPUID_GET_CACHE_DESCRIPTORS,
    CPUID_GET_SERIAL_NUMBER,
    CPUID_GET_CACHE_PARAMETERS,
    CPUID_GET_MONITOR_INSTR,
    CPUID_GET_POWER_MANAGEMENT_INSTR,

    CPUID_GET_FEATURES_EXTENDED_7,

    /* CPUID 8 is reserved */

    CPUID_GET_DIRECT_CACHE_ACCESS = 9,
    CPUID_GET_ARCH_PERF_MONITOR,

    CPUID_GET_CPU_TOPOLOGY,
    CPUID_GET_FEATURES_XSAVE = 13,

    CPUID_GET_LARGEST_EXTENDED_FUNCTION = 0x80000000,
    CPUID_GET_FEATURES_EXTENDED,

    CPUID_GET_EXTENDED_BRAND_STRING,
    CPUID_GET_EXTENDED_BRAND_STRING_MORE,
    CPUID_GET_EXTENDED_BRAND_STRING_END,
};

void
cpuid(uint32_t leaf,
      uint32_t subleaf,
      uint64_t *a,
      uint64_t *b,
      uint64_t *c,
      uint64_t *d);

int cpuid_string(int code, uint32_t where[static 4]);