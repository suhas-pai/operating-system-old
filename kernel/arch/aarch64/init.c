/*
 * kernel/arch/aarch64/init.c
 * © suhas pai
 */

#include "dev/init.h"
#include "mm/init.h"

#include "cpu.h"

void arch_init() {
    cpu_init();
    mm_init();
}