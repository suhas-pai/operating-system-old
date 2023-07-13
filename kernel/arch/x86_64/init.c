/*
 * kernel/arch/x86_64/init.c
 * © suhas pai
 */

#include "mm/init.h"

#include "cpu.h"
#include "gdt.h"
#include "idt.h"

void arch_init() {
    gdt_load();
    idt_init();
    cpu_init();
    mm_init();
}