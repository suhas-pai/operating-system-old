/*
 * kernel/arch/x86_64/cpu.c
 * © suhas pai
 */

#include "asm/cpuid.h"
#include "asm/cr.h"
#include "asm/fsgsbase.h"
#include "asm/msr.h"

#include "dev/printk.h"
#include "sys/gdt.h"

#include "cpu.h"

static struct cpu_capabilities g_cpu_capabilities = {
    .supports_avx512 = false,
    .supports_x2apic = false,
    .supports_1gib_pages = false,
    .xsave_store_size = false
};

static struct cpu_info g_base_cpu_info = {
    .processor_id = 0,
    .lapic_id = 0,
    .lapic_timer_frequency = 0,
    .timer_ticks = 0,

    .pagemap = &kernel_pagemap,
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),

    .spur_int_count = 0
};

static bool g_base_cpu_init = false;
static void init_cpuid_features() {
    {
        uint64_t eax, ebx, ecx, edx;
        cpuid(CPUID_GET_FEATURES, /*subleaf=*/0, &eax, &ebx, &ecx, &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: cpuid_get_features: "
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
            g_cpu_capabilities.supports_x2apic = (ecx & CPUID_FEAT_ECX_X2APIC);
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
               "cpuid: get_features_extended_7(ecx=0): "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_ebx_features =
            CPUID_FEAT_EXT7_ECX0_EBX_FSGSBASE |
            CPUID_FEAT_EXT7_ECX0_EBX_BMI1 |
            CPUID_FEAT_EXT7_ECX0_EBX_BMI2 |
            CPUID_FEAT_EXT7_ECX0_EBX_REP_MOVSB_STOSB |
            CPUID_FEAT_EXT7_ECX0_EBX_SMAP;

        assert((ebx & expected_ebx_features) == expected_ebx_features);
        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_avx512 =
                (ebx & CPUID_FEAT_EXT7_ECX0_EBX_AVX512F);
        }
    }
    {
        uint64_t eax, ebx, ecx = 1, edx;
        cpuid(CPUID_GET_FEATURES_EXTENDED_7,
              /*subleaf=*/1,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_7(ecx=1): "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax, ebx, ecx, edx);

        const uint32_t expected_eax_features =
            CPUID_FEAT_EXT7_ECX1_EAX_FAST_ZEROLEN_REP_MOVSB |
            CPUID_FEAT_EXT7_ECX1_EAX_FAST_ZEROLEN_REP_STOSB |
            CPUID_FEAT_EXT7_ECX1_EAX_FAST_SHORT_REP_CMPSB_SCASB;

        assert((eax & expected_eax_features) == expected_eax_features);
    }
    {
        uint64_t eax, ebx, ecx = 0, edx;
        cpuid(CPUID_GET_LARGEST_EXTENDED_FUNCTION | CPUID_GET_FEATURES,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_0x800000007: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_edx_features =
            CPUID_FEAT_EXT80000001_EDX_SYSCALL_SYSRET;

        assert((edx & expected_edx_features) == expected_edx_features);
        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_1gib_pages =
                (edx & CPUID_FEAT_EXT80000001_EDX_1GIB_PAGES);

            if (g_cpu_capabilities.supports_1gib_pages) {
                printk(LOGLEVEL_INFO, "cpu: supports 1gib pages\n");
            } else {
                printk(LOGLEVEL_INFO, "cpu: does NOT support 1gib pages\n");
            }
        }
    }

    write_cr0(read_cr0() | CR0_BIT_MP);

    const uint64_t cr4_bits =
        CR4_BIT_TSD |
        CR4_BIT_DE |
        CR4_BIT_PGE |
        CR4_BIT_OSFXSR |
        CR4_BIT_OSXMMEXCPTO |
        CR4_BIT_FSGSBASE |
        CR4_BIT_SMEP |
        CR4_BIT_SMAP |
        CR4_BIT_OSXSAVE;

    write_cr4(read_cr4() | cr4_bits);

    /* Enable Syscalls and Fast-FPU */
    write_msr(IA32_MSR_EFER,
              (read_msr(IA32_MSR_EFER) | __IA32_MSR_EFER_BIT_SCE));

    /* Setup Syscall MSRs */
    write_msr(IA32_MSR_STAR,
              ((uint64_t)gdt_get_kernel_code_segment() << 32 |
               (uint64_t)gdt_get_user_data_segment() << 48));

    write_msr(IA32_MSR_FMASK, 0);
    write_msr(IA32_MSR_MISC_ENABLE,
              (read_msr(IA32_MSR_MISC_ENABLE) |
               __IA32_MSR_MISC_FAST_STRING_ENABLE));
}

const struct cpu_info *get_base_cpu_info() {
    assert(g_base_cpu_init);
    return &g_base_cpu_info;
}

const struct cpu_info *get_cpu_info() {
    assert(g_base_cpu_init);
    return (const struct cpu_info *)read_gsbase();
}

struct cpu_info *get_cpu_info_mut() {
    assert(g_base_cpu_init);
    return (struct cpu_info *)read_gsbase();
}

const struct cpu_capabilities *get_cpu_capabilities() {
    return &g_cpu_capabilities;
}

void cpu_init() {
    init_cpuid_features();

    write_gsbase((uint64_t)&g_base_cpu_info);
    list_add(&kernel_pagemap.cpu_list, &g_base_cpu_info.pagemap_node);

    g_base_cpu_init = true;
}