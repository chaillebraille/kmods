#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Doe");
MODULE_DESCRIPTION("Module for testing kmalloc");

struct memlst
{
	void* mem;
	struct memlst* nxt;
};

static int find_max_order(struct memlst** list)
{
	void* mem;
	int order;

	for (order = 25; order > 17; --order)
	{
		mem = kmalloc(1UL << order, GFP_KERNEL);
		if (mem)
		{
			*list = kmalloc(sizeof(struct memlst), GFP_KERNEL);
			if (!*list)
			{
				kfree(mem);
				return -1;
			}
			(*list)->nxt = NULL;
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
		printk(KERN_INFO "Module km hit an error in finding maximum allocation order.");
		return list;
	}
	if (!order)
	{
		printk(KERN_INFO "Module km failed in finding the maximum allocation order.");
		return list;
	}

	printk(KERN_INFO "Module km attempting to allocate areas of size %ldKB.", 1UL << (order-10));

	while ((mem = kmalloc(1UL << order, GFP_KERNEL)))
	{
		++cnt;
		node = kmalloc(sizeof(struct memlst), GFP_KERNEL);
		if (!node)
		{
			kfree(mem);
			break;
		}
		node->mem = mem;
		node->nxt = list;
		list = node;
	}

	printk(KERN_INFO "Module km allocated %d areas, a total of %ldMB.\n", cnt, (cnt * (1UL << (order)) >> 20));

	return list;
}

static void deallocate(struct memlst* list)
{
	struct memlst* node;

	while (list)
	{
		node = list;
		list = list->nxt;
		kfree(node->mem);
		kfree(node);
	}
}

static int init_km(void)
{
	struct memlst* list;

	list = allocate();
	deallocate(list);

	printk(KERN_INFO "Module km initialized.\n");

	return 0;
}

static void exit_km(void)
{
	printk(KERN_INFO "Module km cleaned up.\n");
}

module_init(init_km)
module_exit(exit_km)

