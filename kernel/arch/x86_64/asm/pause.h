/*
 * kernel/arch/x86_64/asm/defines.h
 * © suhas pai
 */

#pragma once
#define cpu_pause() __builtin_ia32_pause()