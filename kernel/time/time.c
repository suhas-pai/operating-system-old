/*
 * kernel/dev/time/time.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "time.h"

static struct list clock_list = LIST_INIT(clock_list);
static struct spinlock lock = {};

void add_clock_source(struct clock_source *const clock) {
    const int flag = spin_acquire_with_irq(&lock);

    list_add(&clock_list, &clock->list);
    spin_release_with_irq(&lock, flag);
}