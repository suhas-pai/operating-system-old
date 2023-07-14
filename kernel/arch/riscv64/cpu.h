/*
 * kernel/arch/riscv64/cpu.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct cpu_info {
    uint64_t spur_int_count;
};

const struct cpu_info *get_base_cpu_info();
const struct cpu_info *get_cpu_info();
struct cpu_info *get_cpu_info_mut();