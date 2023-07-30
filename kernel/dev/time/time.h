/*
 * kernel/dev/time/time.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

void arch_init_time();
uint64_t nsec_since_boot();