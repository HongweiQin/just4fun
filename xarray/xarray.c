#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/bio.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/crc32.h>
#include <linux/uuid.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>

struct xarray test_xa;

static void my_xa_dump_node(const struct xa_node *node)
{
	unsigned i, j;

	if (!node)
		return;
	if ((unsigned long)node & 3) {
		pr_cont("node %px\n", node);
		return;
	}

	pr_cont("node %px %s %d parent %px shift %d count %d values %d "
		"array %px list %px %px marks",
		node, node->parent ? "offset" : "max", node->offset,
		node->parent, node->shift, node->count, node->nr_values,
		node->array, node->private_list.prev, node->private_list.next);
	for (i = 0; i < XA_MAX_MARKS; i++)
		for (j = 0; j < XA_MARK_LONGS; j++)
			pr_cont(" %lx", node->marks[i][j]);
	pr_cont("\n");
}

static void my_xa_dump_index(unsigned long index, unsigned int shift)
{
	if (!shift)
		pr_info("%lu: ", index);
	else if (shift >= BITS_PER_LONG)
		pr_info("0-%lu: ", ~0UL);
	else
		pr_info("%lu-%lu: ", index, index | ((1UL << shift) - 1));
}

static void xa_dump_entry(const void *entry, unsigned long index, unsigned long shift)
{
	if (!entry)
		return;

	my_xa_dump_index(index, shift);

	if (xa_is_node(entry)) {
		if (shift == 0) {
			pr_cont("%px\n", entry);
		} else {
			unsigned long i;
			struct xa_node *node = xa_to_node(entry);
			my_xa_dump_node(node);
			for (i = 0; i < XA_CHUNK_SIZE; i++)
				xa_dump_entry(node->slots[i],
				      index + (i << node->shift), node->shift);
		}
	} else if (xa_is_value(entry))
		pr_cont("value %ld (0x%lx) [%px]\n", xa_to_value(entry),
						xa_to_value(entry), entry);
	else if (!xa_is_internal(entry))
		pr_cont("%px\n", entry);
	else if (xa_is_retry(entry))
		pr_cont("retry (%ld)\n", xa_to_internal(entry));
	else if (xa_is_sibling(entry))
		pr_cont("sibling (slot %ld)\n", xa_to_sibling(entry));
	else if (xa_is_zero(entry))
		pr_cont("zero (%ld)\n", xa_to_internal(entry));
	else
		pr_cont("UNKNOWN ENTRY (%px)\n", entry);
}

static void myxa_dump(const struct xarray *xa)
{
	void *entry = xa->xa_head;
	unsigned int shift = 0;

	pr_info("xarray: %px head %px flags %x marks %d %d %d\n", xa, entry,
			xa->xa_flags, xa_marked(xa, XA_MARK_0),
			xa_marked(xa, XA_MARK_1), xa_marked(xa, XA_MARK_2));
	if (xa_is_node(entry))
		shift = xa_to_node(entry)->shift + XA_CHUNK_SHIFT;
	xa_dump_entry(entry, 0, shift);
}

static ssize_t xa_test_write(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char usrCommand[512];
	int ret;
	unsigned long key, value;
	void *pvalue;

	ret = copy_from_user(usrCommand, buffer, count);
	switch (usrCommand[0]) {
	case 'a':
		pr_notice("xa init\n");
		xa_init_flags(&test_xa, XA_FLAGS_LOCK_IRQ | XA_FLAGS_ACCOUNT);
		break;
	case 'b':
		sscanf(&usrCommand[1], "%lu%lu", &key, &value);
		pr_notice("store, key=%lu, value=%lu\n", key, value);
		xa_store(&test_xa, key, xa_mk_value(value), GFP_KERNEL);
		break;
	case 'c':
		sscanf(&usrCommand[1], "%lu", &key);
		value = xa_to_value(xa_load(&test_xa, key));
		pr_notice("load, key=%lu, value=%lu\n", key, value);
		break;
	case 'd':
		//WARNING: O(n.logn), xas for each is more efficient.
		xa_for_each(&test_xa, key, pvalue) {
			pr_notice("Iterate, key=%lu, value=%lu\n", key, xa_to_value(pvalue));
		}
		break;
	case 'e':
		myxa_dump(&test_xa);
		break;
	case 'z':
		pr_notice("xa destroy\n");
		xa_destroy(&test_xa);
		break;
	}
	return count;
}

static const struct proc_ops xa_proc_ops = {
	.proc_write	= xa_test_write,
};

static int __init xa_practice_init(void)
{
	proc_create("xa_test", 0, NULL, &xa_proc_ops);
	pr_notice("xa practice init\n");
	return 0;
}

static void xa_practice_exit(void)
{
	remove_proc_entry("xa_test", NULL);
	pr_notice("xa practice exit\n");
}

module_init(xa_practice_init);
module_exit(xa_practice_exit);
MODULE_LICENSE("GPL");

