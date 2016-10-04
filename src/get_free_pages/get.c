#include <linux/module.h>
#include <linux/kernel.h>
// #include <linux/slab.h>
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

	for (order = 35; order > 17; --order)
	{
		mem = __get_free_pages(GFP_KERNEL, 1UL << order);
		if (mem)
		{
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
	void* mem = NULL;
	unsigned int cnt = 0;
	int order;

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

	printk(KERN_INFO "Module 'get' attempting to allocate areas of size %ldKB.", 1UL << (order-10));

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

	printk(KERN_INFO "Module km allocated %d areas, a total of %ldMB.\n", cnt, (cnt * (1UL << (order)) >> 20));

	return list;
}

static void deallocate(struct memlst* list, int order)
{
	struct memlst* node;

	while (list)
	{
		node = list;
		list = list->nxt;
		free_pages(node->mem, node->order);
		kfree(node);
	}
}

static int init_get(void)
{
	struct memlst* list;

	list = allocate();
	deallocate(list);

	printk(KERN_INFO "Module get initialized.\n");

	return 0;
}

static void exit_get(void)
{
	printk(KERN_INFO "Module get cleaned up.\n");
}

module_init(init_get)
module_exit(exit_get)
