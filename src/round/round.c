// Mimicing an IRQ, a timer triggers. This causes
// the trigging of a tasklet, which in turn enqueues
// work for a work queue.


struct isr_data
{
   struct timer_list timer;
   int other_data;
} irq_timer;

static void isr(unsigned long addr)
{
   struct isr_data* data = (struct isr_data*)addr;

   mod_timer(&data->timer, jiffies + delay);
}

static void tasklet_fn()
{
}

static void worker_fn()
{
}

static void __init init_mod()
{
   struct timer_list* timer = &irq_timer.timer;
   init_timer(timer);
   timer->expires = jiffies + delay;
   timer->function = isr;
   timer->data = &irq_timer;
   irq_timer.other_data = 4711;
   add_timer(timer);
}

static void __exit cleanup_mod()
{
   del_timer_sync(&irq_timer);
}

