/*
 * kernel/arch/x86_64/dev/ps2/keymap.h
 * © suhas pai
 */

#pragma once

// From https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Set_2
static const char ps2_key_to_char[] = {
    '\0', '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '
};

static const char ps2_key_to_char_shift[] = {
    '\0', '\e', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', '\0', '\0', '\0', ' '
};

static const char ps2_key_to_char_capslock[] = {
    '\0', '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', '\0', '\0', '\0', ' '
};

enum ps2_scancode_keys {
    PS2_SCANCODE_CTRL = 0x1d,
    PS2_SCANCODE_CTRL_REL = 0x9d,

    PS2_SCANCODE_SHIFT_RIGHT = 0x36,
    PS2_SCANCODE_SHIFT_RIGHT_REL = 0xb6,

    PS2_SCANCODE_SHIFT_LEFT = 0x2a,
    PS2_SCANCODE_SHIFT_LEFT_REL = 0xaa,

    PS2_SCANCODE_ALT_LEFT = 0x38,
    PS2_SCANCODE_ALT_LEFT_REL = 0xb8,

    PS2_SCANCODE_CAPSLOCK = 0x3a,
    PS2_SCANCODE_NUMLOCK = 0x45,

    PS2_SCANNODE_REL_1 = 0x90,
    PS2_SCANNODE_REL_2 = 0x91,
    PS2_SCANNODE_REL_3 = 0x92,
    PS2_SCANNODE_REL_4 = 0x93,
    PS2_SCANNODE_REL_5 = 0x94,
    PS2_SCANNODE_REL_6 = 0x95,
    PS2_SCANNODE_REL_7 = 0x96,
    PS2_SCANNODE_REL_8 = 0x97,
    PS2_SCANNODE_REL_9 = 0x98,
    PS2_SCANNODE_REL_10 = 0x99,
    PS2_SCANNODE_REL_11 = 0x9e,
    PS2_SCANNODE_REL_12 = 0x9f,
    PS2_SCANNODE_REL_13 = 0xa0,
    PS2_SCANNODE_REL_14 = 0xa1,
    PS2_SCANNODE_REL_15 = 0xa3,
    PS2_SCANNODE_REL_16 = 0xa4,
    PS2_SCANNODE_REL_17 = 0xa5,
    PS2_SCANNODE_REL_18 = 0xa6,
    PS2_SCANNODE_REL_19 = 0xac,
    PS2_SCANNODE_REL_20 = 0xae,
    PS2_SCANNODE_REL_21 = 0xb1,
    PS2_SCANNODE_REL_22 = 0xb2,
    PS2_SCANNODE_REL_23 = 0xba,
};

#define PS2_SCANNODE_E0 0xe0


enum ps2_scancode_e0_keys {
    PS2_SCANNODE_E0_RIGHT_CTRL = 0x38,
    PS2_SCANNODE_E0_RIGHT_CTRL_REL = 0xb8,

    PS2_SCANNODE_E0_LEFT_CMD = 0x5b,
    PS2_SCANNODE_E0_LEFT_CMD_REL = 0xdb,

    PS2_SCANNODE_E0_RIGHT_CMD = 0x5c,
    PS2_SCANNODE_E0_RIGHT_CMD_REL = 0xdc,

    PS2_SCANNODE_E0_UP_ARROW = 0x48,
    PS2_SCANNODE_E0_UP_ARROW_REL = 0xc8,

    PS2_SCANNODE_E0_LEFT_ARROW = 0x4b,
    PS2_SCANNODE_E0_LEFT_ARROW_REL = 0xcb,

    PS2_SCANNODE_E0_RIGHT_ARROW = 0x4d,
    PS2_SCANNODE_E0_RIGHT_ARROW_REL = 0xcd,

    PS2_SCANNODE_E0_DOWN_ARROW = 0x50,
    PS2_SCANNODE_E0_DOWN_ARROW_REL = 0xd0,

};