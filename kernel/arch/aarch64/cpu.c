/*
 * kernel/arch/aarch64/cpu.c
 * © suhas pai
 */

#include "cpu.h"

static struct cpu_info g_base_cpu_info = {0};

const struct cpu_info *get_base_cpu_info() {
    return &g_base_cpu_info;
}

const struct cpu_info *get_cpu_info() {
    return &g_base_cpu_info;
}

struct cpu_info *get_cpu_info_mut() {
    return &g_base_cpu_info;
}