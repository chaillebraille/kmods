#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/gfp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Doe");
MODULE_DESCRIPTION("Module for testing __get_free_pages");

struct memlst
{
	unsigned long mem;
	int order;
	struct memlst* nxt;
};

static int find_max_order(struct memlst** list)
{
	unsigned long mem;
	int order;
	int shift;

	printk(KERN_INFO "Module 'get' is auto-detecting maximum allocation order.");
	for (order = 30; order > 0; --order)
	{
		mem = __get_free_pages(GFP_KERNEL, 1UL << order);
		if (mem)
		{
			shift = order + PAGE_SHIFT;
			if (order > 10)
			{
				printk(KERN_INFO "Module 'get' detected maximum order to be %d, corresponding to %ldMB", order, (1UL << (shift-10)));
			}
			else
			{
				printk(KERN_INFO "Module 'get' detected maximum order to be %d, corresponding to %ldB", order, (1UL << shift));
			}
			*list = kmalloc(sizeof(struct memlst), GFP_KERNEL);
			if (!*list)
			{
				free_pages(mem, order);
				return -1;
			}
			(*list)->nxt = NULL;
			(*list)->order = order;
			(*list)->mem = mem;
			return order;
		}
	}
	return 0;
}

static struct memlst* allocate(void)
{
	struct memlst* list = NULL;
	struct memlst* node;
	unsigned long mem = 0;
	unsigned int cnt = 1;
	int order;
	int shift;
	unsigned long sz;

	order = find_max_order(&list);

	if (order < 0)
	{
		printk(KERN_INFO "Module 'get' hit an error in finding maximum allocation order.");
		return list;
	}
	if (!order)
	{
		printk(KERN_INFO "Module 'get' failed in finding the maximum allocation order.");
		return list;
	}

	shift = order + PAGE_SHIFT;
	if (shift > 10)
	{
		printk(KERN_INFO "Module 'get' attempting to allocate areas of size %ldKB.", 1UL << (shift-10));	}
	else
	{
		printk(KERN_INFO "Module 'get' attempting to allocate areas of size %ldB.", 1UL << shift);
	}

	while ((mem = __get_free_pages(GFP_KERNEL, order)))
	{
		++cnt;
		node = kmalloc(sizeof(struct memlst), GFP_KERNEL);
		if (!node)
		{
			free_pages(mem, order);
			break;
		}
		node->mem = mem;
		node->order = order;
		node->nxt = list;
		list = node;
	}

	sz = cnt * (1UL << shift);
	if (sz > (1UL << 30))
	{
		printk(KERN_INFO "Module 'get' allocated %d areas, a total of %ldMB.\n", cnt, (sz >> 20));
	}
	else if (sz > (1UL << 20))
	{
		printk(KERN_INFO "Module 'get' allocated %d areas, a total of %ldKB.\n", cnt, (sz >> 10));
	}
	else
	{
		printk(KERN_INFO "Module 'get' allocated %d areas, a total of %ldB.\n", cnt, sz);
	}

	return list;
}

static void deallocate(struct memlst* list)
{
	struct memlst* node;

	printk(KERN_INFO "Module 'get' deallocating.\n");
	while (list)
	{
		node = list;
		list = list->nxt;
		free_pages(node->mem, node->order);
		kfree(node);
	}

	return;
}

static int init_get(void)
{
	struct memlst* list = NULL;

	printk(KERN_INFO "Module 'get' is initializing.\n");

	find_max_order(&list);
	deallocate(list);
	list = allocate();
	deallocate(list);
	list = allocate();
	deallocate(list);
	list = allocate();
	deallocate(list);

	printk(KERN_INFO "Module 'get' initialized.\n");

	return 0;
}

static void exit_get(void)
{
	printk(KERN_INFO "Module 'get' cleaned up.\n");
}

module_init(init_get)
module_exit(exit_get)

