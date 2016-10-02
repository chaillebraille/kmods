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

static struct memlst* allocate(void)
{
	struct memlst* list = NULL;
	struct memlst* node;
	void* mem = NULL;
	unsigned int cnt = 0;

	while ((mem = kmalloc(1 << 24, GFP_KERNEL)))
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

	printk(KERN_INFO "Module km allocated %d large areas.\n", cnt);

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

