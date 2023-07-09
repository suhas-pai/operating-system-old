/*
 * kernel/arch/x86_64/mm/tlb.h
 * © suhas pai
 */

#pragma once
#include "page_table.h"

void tlb_flush_pageop(struct pageop *pageop);