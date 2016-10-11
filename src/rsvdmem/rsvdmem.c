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

#define DEVICE_NAME "rsvdmem"
#define DEVICE_CLASS "kmodc"
#define MINOR_NUMBER 0

static int majorNumber = -1;
static void* kmemBase = 0;
static struct class* devClass = 0;
static struct device* rmemDev = 0;

static unsigned long mem_base = 0;
static unsigned long mem_size = 0;

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
   unsigned long vmOffset;

   if (vma->vm_end - vma->vm_start != mem_size)
   {
      printk(KERN_ALERT "rsvdmem: Using mmap() with memory size %ld rather than %ld.\n",
             vma->vm_end - vma->vm_start, mem_size);
      return -EAGAIN;
   }

   vmOffset = mem_size + (vma->vm_pgoff << PAGE_SHIFT);
   ret = remap_pfn_range(vma, vma->vm_start, (vmOffset >> PAGE_SHIFT),
                         mem_size, PAGE_SHARED);
   if (ret)
   {
      printk(KERN_ALERT "rsvdmem: remap_pfn_range() failed in mmap().\n");
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
   printk(KERN_INFO "rsvdmem: Attempting to initialize loadable kernel device driver module.\n");

   // Two system command line parameter should be set in /etc/default/grub.cfg and
   // /boot/grub2/grub.cfg (or run grub2-mkconfig).
   // mem=<mem_base>
   // memmap=<mem_size>\$<mem_base>
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

   kmemBase = ioremap(mem_base, mem_size);
   if (!kmemBase)
   {
      printk(KERN_ALERT "rsvdmem: Unable to ioremap reserved memory.\n");
      return -EFAULT;
   }

   majorNumber = register_chrdev(0, DEVICE_NAME, fops);
   if (majorNumber < 0)
   {
      printk(KEN_ALERT "rsvdmem: Failed to register major device number.\n");
      return majorNumber;
   }

   devClass = class_create(THIS_MODULE, DEVICE_CLASS);
   if (IS_ERR(devClass))
   {
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "rsvdmem: Failed to register device class.\n");
      return PTR_ERR(devClass);
   }

   rmemDev = device_create(devClass, NULL, MKDEV(majorNumber,MINOR_NUMBER), NULL, DEVICE_NAME);
   if (IS_ERR(rmemDev))
   {
      class_destroy(devClass);
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "rsvdmem: Failed to create the class.\n");
      return PTR_ERR(rmemDev);
   }

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
   if (kmemBase)
   {
      iounmap(kmemBase);
   }
   printk(KERN_INFO "rsvdmem: Module clean-up completed.\n");
}

module_init(rsvdmem_init);
module_exit(rsvdmem_exit);

