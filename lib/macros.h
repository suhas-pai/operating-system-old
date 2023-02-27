/*
 * lib/macros.h
 * © suhas pai
 */

#pragma once

#define __unused __attribute__((unused))
#define __packed __attribute__((packed))

#define assert(cond) (void)(cond) // TODO: