#ifndef RELMALLOC_H_INCLUDED
#define RELMALLOC_H_INCLUDED

#include <linux/types.h>

void* relmalloc_init(unsigned long base, unsigned long size);
void relmalloc_close(void* hdl);
unsigned long relmalloc_get(void* hdl, size_t size);
void relmalloc_free(void* hdl, unsigned long addr);

#endif /* RELMALLOC_H_INCLUDED */

