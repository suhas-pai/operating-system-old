/*
 * kernel/cpu/panic.h
 * © suhas pai
 */

#pragma once

#include <stdarg.h>
#include "lib/macros.h"

__noreturn void panic(const char *fmt, ...);
__noreturn void vpanic(const char *fmt, va_list list);