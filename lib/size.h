/*
 * lib/size.h
 * © suhas pai
 */

#pragma once

#include "assert.h"
#include "overflow.h"

#define kib(amount) ((1ull << 10) * (amount))
#define mib(amount) ((1ull << 20) * (amount))
#define gib(amount) ((1ull << 30) * (amount))
#define tib(amount) ((1ull << 40) * (amount))
#define pib(amount) ((1ull << 50) * (amount))
#define eib(amount) ((1ull << 60) * (amount))
