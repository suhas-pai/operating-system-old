/*
 * kernel/arch/riscv64/init.c
 * © suhas pai
 */

#include "dev/init.h"
#include "mm/init.h"

void arch_init() {
    mm_init();
}