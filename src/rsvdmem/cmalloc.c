#include <linux/slab.h>
#include <linux/cache.h>
#include "cmalloc.h"

static unsigned int cache_width;

struct cmalloc_hdl
{
   unsigned long base;
   unsigned long size;
   unsigned long last;
};

void* cmalloc_init(unsigned long base, unsigned long size)
{
   void* mem
   unsigned int misalign;
   struct cmalloc_hdl* hdl;

   mem = kmalloc(size_of(struct cmalloc_hdl));
   if (!mem) return 0;

   cache_width = cache_line_size();
   misalign = base % cache_width;
   if (misalign)
   {
      base += (cache_width - misalign);
   }

   hdl = (struct cmalloc_hdl*)mem;
   hdl->base = base;
   hdl->end = base + size;
   hdl->last = hdl->end;

   return hdl;
}

void cmalloc_close(void* hdl)
{
   kfree(hdl);
}

unsigned long cmalloc_get(void* raw, size_t size)
{
   struct cmalloc_hdl* hdl;
   unsigned long addr;
   unsigned int misalign;

   hdl = (struct cmalloc_hdl*)raw;

   if (hdl->base + size >= hdl->last) hdl->last = hdl->end;
   addr = hdl->last - size;
   misalign = (addr % cache_width);
   addr -= misalign;
   memset(addr, 0, size);
   hdl->last = addr;
   return addr;
}

void cmalloc_free(void* raw, unsigned long addr)
{
}

