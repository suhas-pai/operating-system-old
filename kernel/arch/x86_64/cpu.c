/*
 * kernel/arch/x86_64/cpu.c
 * © suhas pai
 */

#include "asm/cpuid.h"
#include "asm/cr.h"
#include "asm/msr.h"

#include "dev/printk.h"

#include "cpu.h"
#include "gdt.h"

static bool g_base_cpu_init = false;
static struct cpu_capabilities g_cpu_capabilities = {};
static struct cpu_info g_base_cpu_info = {};

static void init_cpuid_features() {
    {
        uint64_t eax, ebx, ecx, edx;
        cpuid(CPUID_GET_FEATURES, /*subleaf=*/0, &eax, &ebx, &ecx, &edx);

        printk(LOGLEVEL_INFO, "cpuid: cpuid_get_features: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_ecx_features =
            CPUID_FEAT_ECX_SSE3   |
            CPUID_FEAT_ECX_SSSE3  |
            CPUID_FEAT_ECX_SSE4_1 |
            CPUID_FEAT_ECX_SSE4_2 |
            CPUID_FEAT_ECX_POPCNT |
            CPUID_FEAT_ECX_XSAVE  |
            CPUID_FEAT_ECX_RDRAND;
        const uint32_t expected_edx_features =
            CPUID_FEAT_EDX_FPU   |
            CPUID_FEAT_EDX_DE    |
            CPUID_FEAT_EDX_PSE   |
            CPUID_FEAT_EDX_TSC   |
            CPUID_FEAT_EDX_MSR   |
            CPUID_FEAT_EDX_PAE   |
            CPUID_FEAT_EDX_APIC  |
            CPUID_FEAT_EDX_SEP   |
            CPUID_FEAT_EDX_MTRR  |
            CPUID_FEAT_EDX_CMOV  |
            CPUID_FEAT_EDX_PAT   |
            CPUID_FEAT_EDX_PSE36 |
            CPUID_FEAT_EDX_PGE   |
            CPUID_FEAT_EDX_SSE   |
            CPUID_FEAT_EDX_SSE2;

        assert((ecx & expected_ecx_features) == expected_ecx_features);
        assert((edx & expected_edx_features) == expected_edx_features);

        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_x2apic = (ecx & CPUID_FEAT_ECX_x2APIC);
        }
    }
    {
        uint64_t eax, ebx, ecx = 0, edx;
        cpuid(CPUID_GET_FEATURES_EXTENDED_7,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_7: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax, ebx, ecx, edx);

        const uint32_t expected_ebx_features =
            CPUID_FEAT_EXT_7_EBX_BMI1 |
            CPUID_FEAT_EXT_7_EBX_BMI2 |
            CPUID_FEAT_EXT_7_EBX_REP_MOVSB_STOSB |
            CPUID_FEAT_EXT_7_EBX_SMAP;

        assert((ebx & expected_ebx_features) == expected_ebx_features);
        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_avx512 =
                (ebx & CPUID_FEAT_EXT_7_EBX_AVX512F);
        }
    }

    write_cr0((read_cr0() | CR0_BIT_MP) & ~(uint64_t)CR0_BIT_EM);

    const uint64_t cr4_bits =
        CR4_BIT_TSD |
        CR4_BIT_DE |
        CR4_BIT_PGE |
        CR4_BIT_OSFXSR |
        CR4_BIT_OSXMMEXCPTO |
        CR4_BIT_SMEP |
        CR4_BIT_SMAP |
        CR4_BIT_OSXSAVE;

    write_cr4(read_cr4() | cr4_bits);

    /* Enable Syscalls, No-Execute, and Fast-FPU */
    write_msr(IA32_MSR_EFER,
              (read_msr(IA32_MSR_EFER) |
               F_IA32_MSR_EFER_BIT_SCE |
               F_IA32_MSR_EFER_BIT_NXE));

    /* Setup Syscall MSRs */
    write_msr(IA32_MSR_STAR,
              ((uint64_t)gdt_get_kernel_code_segment() << 32 |
               (uint64_t)gdt_get_user_data_segment() << 48));

    write_msr(IA32_MSR_FMASK, 0);
}

const struct cpu_info *get_cpu_info() {
    return (const struct cpu_info *)read_msr(IA32_MSR_KERNEL_GS_BASE);
}

struct cpu_info *get_cpu_info_mut() {
    return (struct cpu_info *)read_msr(IA32_MSR_KERNEL_GS_BASE);
}

const struct cpu_capabilities *get_cpu_capabilities() {
    return &g_cpu_capabilities;
}

void cpu_init() {
    write_msr(IA32_MSR_KERNEL_GS_BASE, (uint64_t)&g_base_cpu_info);
    init_cpuid_features();

    g_base_cpu_init = true;
}