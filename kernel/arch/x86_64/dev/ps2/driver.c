/*
 * kernel/arch/x86_64/dev/ps2.c
 * © suhas pai
 */

#include "asm/pause.h"

#include "dev/port.h"
#include "dev/printk.h"
#include "dev/ps2/keyboard.h"

#include "driver.h"

#define RETRY_LIMIT 10

int16_t ps2_read_input_byte() {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        const uint8_t byte = port_in8(PORT_PS2_READ_STATUS);
        if ((byte & __PS2_STATUS_REG_OUTPUT_BUFFER_FULL) == 0) {
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
        if (byte & __PS2_STATUS_REG_INPUT_BUFFER_FULL) {
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

    return ps2_read_input_byte();
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

void send_byte_to_device(const enum ps2_device_id device, const uint8_t byte) {
    if (device != PS2_FIRST_DEVICE) {
        ps2_write(PORT_PS2_WRITE_CMD,
                  PS2_CMD_WRITE_NEXT_BYTE_TO_2ND_DEVICE_INPUT);
    }

    ps2_write(PORT_PS2_INPUT_BUFFER, byte);
}

int16_t
ps2_send_to_device(const enum ps2_device_id device, const uint8_t byte) {
    for (uint64_t i = 0; i != RETRY_LIMIT; i++) {
        send_byte_to_device(device, byte);

        const int16_t response = ps2_read_input_byte();
        if (response != PS2_RESPONSE_RESEND) {
            return response;
        }
    }

    return PS2_RESPONSE_FAIL;
}

// FIXME: ps2_get_device_kind() will not properly re-enable scanning.

static bool
ps2_get_device_kind(const enum ps2_device_id device_id,
                    enum ps2_device_kind *const result_out)
{
    // NOTE: TODO: This system of verification isn't verified.

    /*
     * Stage 1:
     *  Send the "disable scanning" command 0xF5 to the device
     *  Wait for the device to send "ACK" back (0xFA)
     */

    int16_t disable_scanning_response =
        ps2_send_to_device(device_id, PS2_CMD_DISABLE_SCANNING);

    if (disable_scanning_response != 0x77 &&
        disable_scanning_response != PS2_RESPONSE_ACKNOWLEDGE)
    {
        printk(LOGLEVEL_WARN,
               "ps2: disable scanning for device %" PRIu8 " failed with "
               "response: 0x%" PRIx32 "\n",
               device_id,
               disable_scanning_response);
        return false;
    }

    /*
     * Stage 2:
     *  Send the "identify" command 0xF2 to the device
     *  Wait for the device to send "ACK" back (0xFA).
     *
     * NOTE: A device may also return 0xAA.
     */

    int16_t identity_cmd_response =
        ps2_send_to_device(device_id, PS2_CMD_IDENTIFY);

    if (identity_cmd_response != PS2_RESPONSE_ACKNOWLEDGE &&
        identity_cmd_response != PS2_KBD_SPECIAL_BYTE_SELF_TEST_PASS)
    {
        printk(LOGLEVEL_INFO,
               "ps2: identify command for device %" PRIu8 " failed with "
               "response: 0x%" PRIx16 "\n",
               device_id,
               identity_cmd_response);
    }

    /* NOTE: This is a hack. We're not supposed to have to have these checks. */

    int16_t first = ps2_read_input_byte();
    if (first == PS2_KBD_SPECIAL_BYTE_SELF_TEST_PASS) {
        first = ps2_read_input_byte();
    }

    int16_t second = ps2_read_input_byte();
    if (second == PS2_KBD_SPECIAL_BYTE_ERROR) {
        second = ps2_read_input_byte();
    }

    printk(LOGLEVEL_INFO,
           "ps2: Device %" PRIu8 " identity: 0x%" PRIx16 " 0x%" PRIx16 "\n",
           device_id,
           first,
           second);

    if (second != PS2_RESPONSE_ACKNOWLEDGE && second != PS2_READ_PORT_FAIL) {
        *result_out =
            (enum ps2_device_kind)((uint16_t)first << 8 | (uint8_t)second);

        return true;
    }

    const int16_t enable_scanning_response =
        ps2_send_to_device(device_id, PS2_CMD_ENABLE_SCANNING);

    if (enable_scanning_response != PS2_RESPONSE_ACKNOWLEDGE) {
        printk(LOGLEVEL_WARN,
               "ps2: enable scanning for device %" PRIu8 " failed with "
               "response: 0x%" PRIx32 "\n",
               device_id,
               disable_scanning_response);
    }

    *result_out = (enum ps2_device_kind)first;
    return true;
}

__unused static void
ps2_init_device(const enum ps2_device_id device_id) {
    enum ps2_device_kind device_kind = PS2_DEVICE_KIND_MF2_KBD;
    if (!ps2_get_device_kind(device_id, &device_kind)) {
        return;
    }

    switch (device_kind) {
        case PS2_DEVICE_KIND_MF2_KBD:
            printk(LOGLEVEL_INFO,
                   "ps2: got mf2 keyboard w/o translation. ignoring\n");
            return;
        case PS2_DEVICE_KIND_STANDARD_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got standard mouse. ignoring\n");
            return;
        case PS2_DEVICE_KIND_SCROLL_WHEEL_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got scroll wheel mouse. ignoring\n");
            return;
        case PS2_DEVICE_KIND_5_BUTTON_MOUSE:
            printk(LOGLEVEL_INFO, "ps2: got 5 button mouse. ignoring\n");
            return;
        case PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_1:
        case PS2_DEVICE_KIND_MF2_KBD_WITH_TRANSL_2:
            printk(LOGLEVEL_INFO,
                   "ps2: got mf2 keyboard (with translation). Initializing\n");

            ps2_keyboard_init(device_id);
            return;
    }

    printk(LOGLEVEL_WARN,
           "ps2: unrecognized device: %" PRIu8 "\n",
           device_kind);
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
        __PS2_CNTRLR_CONFIG_1ST_DEVICE_INTERRUPT |
        __PS2_CNTRLR_CONFIG_1ST_DEVICE_TRANSLATION;

    // Enable mouse interrupt if any
    const bool has_second_device =
        (ps2_config & __PS2_CNTRLR_CONFIG_2ND_DEVICE_CLOCK) != 0;

    if (has_second_device) {
        ps2_config |= __PS2_CNTRLR_CONFIG_2ND_DEVICE_INTERRUPT;
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
    bool second_device_enabled = false;
    if (has_second_device) {
        if (!ps2_write(PORT_PS2_WRITE_CMD, PS2_CMD_ENABLE_2ND_DEVICE)) {
            printk(LOGLEVEL_WARN, "ps2: failed to enable mouse port\n");
        } else {
            second_device_enabled = true;
        }
    }

    //ps2_init_device(PS2_FIRST_DEVICE);
    ps2_keyboard_init(PS2_FIRST_DEVICE);
    if (second_device_enabled) {
        //ps2_init_device(PS2_SECOND_DEVICE);
    }
}