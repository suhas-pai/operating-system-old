/*
 * kernel/arch/x86_64/dev/ps2/keyboard.h
 * © suhas pai
 */

#pragma once

#include "asm/irq_context.h"
#include "driver.h"

enum ps2_keyboard_command {
    PS2_KEYBOARD_COMMAND_SET_LED = 0xED,
    PS2_KEYBOARD_COMMAND_ECHO = 0xEE,

    PS2_KEYBOARD_COMMAND_SCAN_CODE_SET = 0xF0,
    PS2_KEYBOARD_COMMAND_TYPEMATIC_RATE = 0xF3
};

enum ps2_keyboard_led_kind {
    PS2_KEYBOARD_LED_KIND_SCROLL_LOCK,
    PS2_KEYBOARD_LED_KIND_NUMBER_LOCK,
    PS2_KEYBOARD_LED_KIND_CAPS_LOCK,
};

enum ps2_keyboard_scan_code_set_subcommand_kind {
    PS2_KEYBOARD_SCAN_CODE_SET_SUBCOMMAND_GET,
    PS2_KEYBOARD_SCAN_CODE_SET_SUBCOMMAND_SET_SET_1,
    PS2_KEYBOARD_SCAN_CODE_SET_SUBCOMMAND_SET_SET_2,
    PS2_KEYBOARD_SCAN_CODE_SET_SUBCOMMAND_SET_SET_3
};

enum ps2_keyboard_special_byte {
    PS2_KEYBOARD_SPECIAL_BYTE_ERROR          = 0x0,
    PS2_KEYBOARD_SPECIAL_BYTE_SELF_TEST_PASS = 0xAA,
    PS2_KEYBOARD_SPECIAL_BYTE_ECHO           = 0xEE,

    /* Both bytes are sent as responses to self-test. */
    PS2_KEYBOARD_SPECIAL_BYTE_SELF_TEST_1 = 0xFC,
    PS2_KEYBOARD_SPECIAL_BYTE_SELF_TEST_2 = 0xFD,

    PS2_KEYBOARD_SPECIAL_BYTE_RESEND  = 0xFE,
    PS2_KEYBOARD_SPECIAL_BYTE_ERROR_2 = 0xFF,
};


void ps2_keyboard_init(const enum ps2_device_id device_id);
void ps2_keyboard_interrupt(uint64_t int_no, irq_context_t *context);