// Mimicing an IRQ, a timer triggers. This causes
// the trigging of a tasklet, which in turn enqueues
// work for a work queue.
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/module.h>
// For tasklets #include <linux/interrupt.h>
// New WQ implementation 2010: cmwq - Concurrency Managed WQ
// For work-queues #include <linux/workqueue.h>
// #include <linux/init.h>
// #include <linux/kernel.h>

MODULE_LICENSE("GPL");

struct isr_data
{
   struct timer_list timer;
   int other_data;
} irq_timer;

static void isr(unsigned long addr)
{
   struct isr_data* data = (struct isr_data*)addr;
   unsigned long delay = HZ;

   if (tasklet_is_to_run)
   {
      tasklet_schedule(&tasklet_data);
   }

   if (timer_is_to_run)
   {
      printk(KERN_INFO "Entering timer function. Setting new timer.");
      mod_timer(&data->timer, jiffies + delay);
   }
}

struct tasklet_struct tasklet_data;

static void tasklet_fn(unsigned long)
{
   void* the_data;

   if (workq_is_to_run)
   {
      INIT_WORK(&some_work, worker_fn, the_data);
      // PREPARE_WORK(&some_work, worker_fn, the_data);

      ret = queue_work(&workQ, &some_work);
   }
}

struct work_struct some_work;

static void worker_fn(unsigned long)
{
}

static int __init init_mod(void)
{
   unsigned long delay = HZ;
   struct timer_list* timer;
   struct workqueue_struct* workQ;

   printk(KERN_INFO "Initializing module");

   printk(KERN_INFO "Initializing work-queue");
   workQ = alloc_workqueue("theworkq", WQ_UNBOUND, 1);

   printk(KERN_INFO "Initializing tasklet");
   tasklet_init(&tasklet_data, tasklet_fn, &tasklet_data);

   timer = &irq_timer.timer;
   init_timer(timer);
   timer->expires = jiffies + delay;
   timer->function = isr;
   timer->data = (unsigned long)&irq_timer;
   irq_timer.other_data = 4711;

   printk(KERN_INFO "Adding timer");
   set_workq_is_to_run();
   set_tasklet_is_to_run();
   set_timer_is_to_run();
   add_timer(timer);

   printk(KERN_INFO "Init completing");

   return 0;
}

static void __exit cleanup_mod(void)
{
   printk(KERN_INFO "Releasing module");

   printk(KERN_INFO "Deleting timer");
   unset_timer_is_to_run();
   unset_tasklet_is_to_run();
   unset_workq_is_to_run();
   del_timer_sync(&irq_timer.timer);
   tasklet_kill(&tasklet_data);
   flush_workqueue(&workQ);
   destroy_workqueue(&workQ);

   printk(KERN_INFO "Module released");
}

module_init(init_mod)
module_exit(cleanup_mod)

