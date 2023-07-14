/*
 * kernel/kernel.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "cpu/isr.h"
#include "dev/init.h"
#include "dev/printk.h"

#include "limine.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#endif
    }
}

void arch_init();

// The following will be our kernel's entry point.
void _start(void) {
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1)
    {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *const framebuffer =
        framebuffer_request.response->framebuffers[0];

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    uint32_t *const fb_ptr = framebuffer->address;
    for (size_t i = 0; i < 100; i++) {
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }

    // We're done, just hang...
    serial_init();
    printk(LOGLEVEL_INFO, "Console is working?\n");

    arch_init();
    isr_init();
    dev_init();

    enable_all_interrupts();
    printk(LOGLEVEL_INFO, "kernel: finished initializing\n");

    hcf();
}
