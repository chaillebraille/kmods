#include <linux/slab.h>
#include <linux/cache.h>
#include "relmalloc.h"

static unsigned int cache_width;

struct relmalloc_hdl
{
   unsigned int offset;
   unsigned long size;
   unsigned long last;
};

void* relmalloc_init(unsigned long base, unsigned long size)
{
   void* mem
   unsigned int misalign;
   struct relmalloc_hdl* hdl;

   cache_width = cache_line_size();
   misalign = base % cache_width;
   if (misalign)
   {
      misalign = cache_width - misalign;
      if (misalign >= size) return 0;
      size -= misalign;
   }

   mem = kmalloc(size_of(struct relmalloc_hdl));
   if (!mem) return 0;

   hdl = (struct relmalloc_hdl*)mem;
   hdl->offset = misalign;
   hdl->size = size;
   hdl->last = size;

   return hdl;
}

void relmalloc_close(void* hdl)
{
   kfree(hdl);
}

unsigned long relmalloc_get(void* rhdl, size_t size)
{
   struct relmalloc_hdl* hdl;
   unsigned long reladdr;
   unsigned int misalign;

   hdl = (struct relmalloc_hdl*)rhdl;

   // Ignore size > hdl->size for now
   if (size >= hdl->last) hdl->last = hdl->size;
   reladdr = hdl->last - size;
   misalign = (reladdr % cache_width);
   reladdr -= misalign;
   reladdr += hdl->offset;
   // memset(base + reladdr, 0, size);
   hdl->last = reladdr;
   return reladdr;
}

void relmalloc_free(void* raw, unsigned long addr)
{
}

