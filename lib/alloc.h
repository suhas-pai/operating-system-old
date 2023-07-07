/*
 * lib/alloc.h
 * © suhas pai
 */

#pragma once

#if defined(BUILD_KERNEL)
    #include "kernel/mm/kmalloc.h"

    #define malloc(size) kmalloc(size)
    #define realloc(buffer, size) krealloc(buffer, size)
    #define free(buffer) kfree(buffer)
#elif defined(BUILD_TEST)
    #include <stdlib.h>
#else
    void malloc(uint64_t size);
    void realloc(void *buffer, uint64_t size);
    void free(void *buffer);
#endif /* BUILD_KERNEL */
