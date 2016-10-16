#include <linux/module.h>
#include <linux/device.h>
#include <linus/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/errno.h>

#include "rsvdmem.h"
#include "relmalloc.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "rsvdmem"
#define DEVICE_CLASS "kmodc"
#define MINOR_NUMBER 0

static int majorNumber = -1;
static void* kmemBase = 0;
static void* relmallocHdl = 0;
static struct class* devClass = 0;
static struct device* rmemDev = 0;

static unsigned long mem_base = 0;
static unsigned long mem_size = 0;

static unsigned long relmalloc_buff;

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
   unsigned long base_pfn;

   base_pfn = (mem_base << PAGE_SHIFT);

   if (base_pfn != vma->vm_pgoff)
   {
      print(KERN_ALERT "rsvdmem: Using mmap() with %dK page frame %ld rather than %ld.\n",
            (PAGE_SIZE >> 10), vma->vm_pgoff, base_pfn);
      return -EAGAIN;
   }

   if (vma->vm_end - vma->vm_start != mem_size)
   {
      printk(KERN_ALERT "rsvdmem: Using mmap() with memory size %ld rather than %ld.\n",
             vma->vm_end - vma->vm_start, mem_size);
      return -EAGAIN;
   }

   // vmOffset = mem_size + (vma->vm_pgoff << PAGE_SHIFT);
   // ret = remap_pfn_range(vma, vma->vm_start, (vmOffset >> PAGE_SHIFT),
   //                       mem_size, PAGE_SHARED);
   ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                         mem_size, PAGE_SHARED);
   if (ret)
   {
      printk(KERN_ALERT "rsvdmem: remap_pfn_range() failed in mmap().\n");
      return ret;
   }

   return 0;
}

static ssize_t rsvdmem_write(struct file* filp, const char __user * data, size_t size, loff_t* offset)
{
   struct rsvdmem_buffref buffref;
   unsigned long lres;
   unsigned long raddr;

   if (size != sizeof(struct rsvdmem_buffref))
   {
      printk(KERN_ALERT "rsvdmem: input size %d differs from struct size %d.\n",
             size, sizeof(struct rsvdmem_buffref));
      return -EFAULT;
   }
   lres = copy_from_user(&buffref, data, size);
   if (lres != size)
   {
      printk(KERN_ALERT "rsvdmem: copy_from_user on input returned %ld rather than %ld.\n", lres, size);
      return -EFAULT;
   }
   raddr = relmalloc_get(relmallocHdl, buffref.size);
   lres = copy_from_user(kmemBase+raddr, buffref.buffer, buffref.size);
   if (lres != buffref.size)
   {
      relmalloc_free(relmallocHdl, raddr);
      printk(KERN_ALERT "rsvdmem: copy_from_user on buffer returned %ld rather than %ld.\n",
             lres, buffref.size);
      return -EFAULT;
   }
   relmalloc_buff = raddr;
   ++(*offset);
   return lres;
}

static ssize_t rsvdmem_read(struct file* filp, char __user * data, size_t size, loff_t* wtf)
{
   struct rsvdmem_buffref buffref;
   unsigned long lres;

   if (size != sizeof(struct rsvdmem_buffref))
   {
      printk(KERN_ALERT "rsvdmem: input size %d differs from struct size %d.\n",
             size, sizeof(struct rsvdmem_buffref));
      return -EFAULT;
   }
   lres = copy_from_user(&buffref, data, size);
   if (lres != size)
   {
      printk(KERN_ALERT "rsvdmem: copy_from_user on input returned %ld rather than %ld.\n", lres, size);
      return -EFAULT;
   }
   lres = copy_to_user(buffref.buffer, kmemBase + relmalloc_buff, buffref.size);
   if (lres != buffref.size)
   {
      printk(KERN_ALERT "rsvdmem: copy_to_user on buffer returned %ld rather than %ld.\n",
             lres, buffref.size);
      return -EFAULT;
   }
   relmalloc_free(relmallocHdl, relmalloc_buff);
   ++(*offset);
   return lres;
}

static struct file_operations fops =
{
   .owner = THIS_MODULE,
   .open = rsvdmem_open,
   .read = rsvdmem_read,
   .write = rsvdmem_write,
   .release = rsvdmem_release,
   .mmap = rsvdmem_mmap
};

static int __init rsvdmem_init(void)
{
   printk(KERN_INFO "rsvdmem: Attempting to initialize loadable kernel device driver module.\n");

   // A system command line parameter should be set in /etc/default/grub.cfg and
   // /boot/grub2/grub.cfg (or run grub2-mkconfig):
   //   memmap=<mem_size>\$<mem_base>
   // (Note the backslash for grub2.) Ensure that the physical memory is not used
   // by the core system (including PCI). This might be done by looking at /proc/iomem
   if (!mem_base)
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_base not set to base of reserved memory area.\n");
      return -EFAULT;
   }
   if (!mem_size)
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_size not set to size (in bytes) of reserved memory area.\n");
      return -EFAULT;
   }
   printk(KERN_INFO "rsvdmem: Using mem_base=%ld mem_size=%ld\n", mem_base, mem_size);

   if (mem_base & ((1 << PAGE_SHIFT) - 1))
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_base is not aligned to page size %dB.\n", PAGE_SIZE);
      return -EFAULT;
   }
   if (mem_size & ((1 << PAGE_SHIFT) - 1))
   {
      printk(KERN_ALERT "rsvdmem: Parameter mem_size is not aligned to page size %dB.\n", PAGE_SIZE);
      return -EFAULT;
   }

   kmemBase = ioremap(mem_base, mem_size);
   if (!kmemBase)
   {
      printk(KERN_ALERT "rsvdmem: Unable to ioremap reserved memory.\n");
      return -EFAULT;
   }
   printk(KERN_INFO "rsvdmem: ioremap of reserved memory completed.\n");

   relmallocHdl = relmalloc_init(mem_base, mem_size);
   if (!relmallocHdl)
   {
      printk(KERN_ALERT "rsvdmem: Unable to create contiguous memory allocator.\n");
      return -EFAULT;
   }

   majorNumber = register_chrdev(0, DEVICE_NAME, fops);
   if (majorNumber < 0)
   {
      printk(KERN_ALERT "rsvdmem: Failed to register major device number.\n");
      return majorNumber;
   }
   printk(KERN_INFO "rsvdmem: Successfully allocated major device number %d.\n", majorNumber);

   devClass = class_create(THIS_MODULE, DEVICE_CLASS);
   if (IS_ERR(devClass))
   {
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "rsvdmem: Failed to register device class.\n");
      return PTR_ERR(devClass);
   }
   printk(KERN_INFO "rsvdmem: Successfully registered device class.\n");

   rmemDev = device_create(devClass, NULL, MKDEV(majorNumber,MINOR_NUMBER), NULL, DEVICE_NAME);
   if (IS_ERR(rmemDev))
   {
      class_destroy(devClass);
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "rsvdmem: Failed to create the class.\n");
      return PTR_ERR(rmemDev);
   }
   printk(KERN_INFO "rsvdmem: Successfully created device class.\n");

   printk(KERN_INFO, "rsvdmem: Initialization completed.\n");
   return 0;
}

static void __exit rsvdmem_exit(void)
{
   printk(KERN_INFO "rsvdmem: Cleaning up module.\n");
   device_destroy(rmemDev, MKDEV(majorNumber, MINOR_NUMBER));
   class_unregister(devClass);
   class_destroy(devClass);
   unregister_chrdev(majorNumber, DEVICE_NAME);
   relmalloc_close(relmallocHdl);
   if (kmemBase)
   {
      iounmap(kmemBase);
   }
   printk(KERN_INFO "rsvdmem: Module clean-up completed.\n");
}

module_init(rsvdmem_init);
module_exit(rsvdmem_exit);

