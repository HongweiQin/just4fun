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

static ssize_t xa_test_write(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char usrCommand[512];
	int ret;
	unsigned long key, value;

	ret = copy_from_user(usrCommand, buffer, count);
	switch (usrCommand[0]) {
	case 'a':
		pr_notice("xa init\n");
		xa_init_flags(&test_xa, XA_FLAGS_LOCK_IRQ | XA_FLAGS_ACCOUNT);
		break;
	case 'b':
		sscanf(&usrCommand[1], "%lu%lu", &key, &value);
		pr_notice("insert, key=%lu, value=%lu\n", key, value);
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

