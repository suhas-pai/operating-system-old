/*
 * kernel/arch/x86_64/asm/cpuid.c
 * © suhas pai
 */

#include "cpuid.h"

/*
 * Issue a single request to CPUID. Fits 'intel features', for instance note
 * that even if only "eax" and "edx" are of interest, other registers will be
 * modified by the operation, so we need to tell the compiler about it
 */

void
cpuid(const uint32_t leaf,
      const uint32_t subleaf,
      uint64_t *const a,
      uint64_t *const b,
      uint64_t *const c,
      uint64_t *const d)
{
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a" (leaf), "c" (subleaf)
    );
}

/*
 * Issue a complete request, storing general registers output as a string.
 */

int cpuid_string(const int code, uint32_t where[const 4]) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*where), "=b"(*(where + 1)),
          "=c"(*(where + 2)), "=d"(*(where + 3))
        : "a"(code)
    );

    return (int)where[0];
}