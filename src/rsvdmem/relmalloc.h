#ifndef CMALLOC_H_INCLUDED
#define CMALLOC_H_INCLUDED

#include <linux/types.h>

void* cmalloc_init(unsigned long base, unsigned long size);
void cmalloc_close(void* hdl);
unsigned long cmalloc_get(void* hdl, size_t size);
void cmalloc_free(void* hdl, unsigned long addr);

#endif /* CMALLOC_H_INCLUDED */

