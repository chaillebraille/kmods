#include <linux/module.h>
#include <linux/device.h>
#include <linus/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

unsigned long mem_base = 0;
unsigned long mem_size = 0;
void* kmem_base;

MODULE_PARM(mem_base, "l");
MODULE_PARM(mem_sizes, "l");

static int rsvdmem_open(struct inode* inode, struct file* filp)
{
   return 0;
}

static int rsvdmem_release(struct inode* inode, struct file* filp)
{
   return 0;
}

static int rsvdmem_mmap(struct file* filp, struct vm_area_struct* vma)
{
   int ret;
   unsigned long vm_offset;

   if (vma->vm_end - vma->vm_start != mem_size)
   {
      printk(KERN_ALERT "rsvdmem: Using mmap() with memory size %ld rather than %ld.",
             vma->vm_end - vma->vm_start, mem_size);
      return -EAGAIN;
   }

   vm_offset = mem_size + (vma->vm_pgoff << PAGE_SHIFT);
   ret = remap_pfn_range(vma, vma->vm_start, (vm_offset >> PAGE_SHIFT),
                         mem_size, PAGE_SHARED);
   if (ret)
   {
      printk(KERN_ALERT "rsvdmem: remap_pfn_range() failed in mmap().");
      return -EAGAIN;
   }
}

static struct file_operations fops =
{
   .owner = THIS_MODULE,
   .open = rsvdmem_open,
   .release = rsvdmem_release,
   .mmap = rsvdmem_mmap
};

static int __init rsvdmem_init(void)
{
   printk(KERN_INFO "rsvdmem: Attempting to initialize loadable kernel device driver module.");

   // Two system command line parameter should be set in /etc/default/grub.cfg and
   // /boot/grub2/grub.cfg (or run grub2-mkconfig).
   // mem=<mem_base>
   // memmap=<mem_size>\$<mem_base>
   if (!mem_base)
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_base not set to base of reserved memory area.");
      return -EFAULT;
   }
   if (!mem_size)
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_size not set to size (in bytes) of reserved memory area.");
      return -EFAULT;
   }
   printk(KERN_INFO "rsvdmem: Using mem_base=%ld mem_size=%ld", mem_base, mem_size);

   kmem_base = ioremap(mem_base, mem_size);
   if (!kmem_base)
   {
      printk(KERN_ALERT "rsvdmem: Unable to ioremap reserved memory.");
      return -EFAULT;
   }

   printk(KERN_INFO, "rsvdmem: Initialization completed.");
   return 0;
}

static void __exit rsvdmem_exit(void)
{
   printk(KERN_INFO "rsvdmem: Cleaning up module.");
   if (kmem_base)
   {
      iounmap(kmem_base);
   }
}

module_init(rsvdmem_init);
module_exit(rsvdmem_exit);

