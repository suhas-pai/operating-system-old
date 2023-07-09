/*
 * kernel/arch/x86_64/dev/ps2.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cpu/isr.h"
#include "dev/port.h"
#include "dev/printk.h"

#include "kernel/port.h"
#include "driver.h"

#define RETRY_LIMIT 10

int16_t ps2_read() {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        const uint8_t byte = port_in8(PORT_PS2_READ_STATUS);
        if ((byte & F_PS2_STATUS_REG_OUTPUT_BUFFER_FULL) == 0) {
            cpu_pause();
            continue;
        }

        return port_in8(PORT_PS2_INPUT_BUFFER);
    }

    return -1;
}

bool ps2_write(const uint16_t port, const uint8_t value) {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        const uint8_t byte = port_in8(PORT_PS2_READ_STATUS);
        if (byte & F_PS2_STATUS_REG_INPUT_BUFFER_FULL) {
            cpu_pause();
            continue;
        }

        port_out8(port, value);
        return true;
    }

    return false;
}

int16_t ps2_read_config() {
    if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_READ_INPUT_BUFFER_BYTE)) {
        return -1;
    }

    return ps2_read();
}

bool ps2_write_config(const uint8_t value) {
    if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_WRITE_INPUT_BUFFER_BYTE)) {
        return false;
    }

    if (!ps2_write(PORT_PS2_INPUT_BUFFER, value)) {
        return false;
    }

    return true;
}

void ps2_init() {
    if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_DISABLE_1ST_DEVICE)) {
        printk(LOGLEVEL_WARN, "ps2: failed to disable 1st device for init\n");
        return;
    }

    if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_DISABLE_2ND_DEVICE)) {
        printk(LOGLEVEL_WARN, "ps2: failed to disable 2nd device for init\n");
        return;
    }

    int16_t ps2_config = ps2_read_config();
    if (ps2_config == -1) {
        printk(LOGLEVEL_WARN, "ps2: failed to read config for init\n");
        return;
    }

    // Enable keyboard interrupt and keyboard scancode translation
    ps2_config |=
        F_PS2_CNTRLR_CONFIG_1ST_DEVICE_INTERRUPT |
        F_PS2_CNTRLR_CONFIG_1ST_DEVICE_TRANSLATION;

    // Enable mouse interrupt if any
    const bool has_mouse =
        (ps2_config & F_PS2_CNTRLR_CONFIG_2ND_DEVICE_CLOCK) != 0;

    if (has_mouse) {
        ps2_config |= F_PS2_CNTRLR_CONFIG_2ND_DEVICE_INTERRUPT;
    }

    if (!ps2_write_config(ps2_config)) {
        printk(LOGLEVEL_WARN, "ps2: failed to write updated config for init\n");
        return;
    }

    // Enable keyboard port
    if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_ENABLE_1ST_DEVICE)) {
        printk(LOGLEVEL_WARN, "ps2: failed to enable keyboard port\n");
        return;
    }

    // Enable mouse port if any
    bool mouse_enabled = false;
    if (has_mouse) {
        if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_ENABLE_2ND_DEVICE)) {
            printk(LOGLEVEL_WARN, "ps2: failed to enable mouse port\n");
        } else {
            mouse_enabled = true;
        }
    }

    const isr_vector_t vector = isr_alloc_vector();
    (void)vector;

    printk(LOGLEVEL_INFO, "ps2: keyboard enabled\n");
    if (mouse_enabled) {
        printk(LOGLEVEL_INFO, "ps2: mouse enabled\n");
    }
}