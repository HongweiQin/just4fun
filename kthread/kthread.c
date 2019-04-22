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


#define KT_NORMAL (0)
#define KT_INTR_SLEEP (1)
#define KT_UNINTR_SLEEP (2)
#define KT_IDLE_SLEEP (3)

static struct task_struct *ts;
static atomic_t kt_status;

static int kthreadTest_ts(void *data)
{

	while (!kthread_should_stop()) {
		if (kthread_should_park()) 
			kthread_parkme();
		else {
			switch (atomic_read(&kt_status)) {
			case KT_NORMAL:
				schedule();
				break;
			case KT_INTR_SLEEP:
				set_current_state(TASK_INTERRUPTIBLE);
				schedule();
				break;
			case KT_UNINTR_SLEEP:
				set_current_state(TASK_UNINTERRUPTIBLE);
				schedule();
				break;
			case KT_IDLE_SLEEP:
				set_current_state(TASK_IDLE);
				schedule();
				break;
			}
		}
	}
	return 0;
}


static void kthreadTest_park(void)
{
	kthread_park(ts);
	pr_notice("kthread park\n");
}

static void kthreadTest_unpark(void)
{
	kthread_unpark(ts);
	pr_notice("kthread unpark\n");
}

static void kthreadTest_fs(void)
{
	kthread_stop(ts);
	pr_notice("kthread stopped\n");
}

static void kthreadTest_fc(void)
{
	ts = kthread_create(kthreadTest_ts, NULL, "kthread_test");
	if (IS_ERR(ts)) {
		pr_err("kthread create error %p\n", ts);
	} else {
		pr_notice("kthread create succeed\n");
	}
}

static void kthreadTest_fw(void)
{
	atomic_set(&kt_status, KT_NORMAL);
	wake_up_process(ts);
	pr_notice("wakeup kthread\n");
}

static void kthreadTest_fp(void)
{
	if(ts) {
		pr_notice("task_state: %ld\n", READ_ONCE(ts->state));
		pr_notice("task usage [%d]\n", atomic_read(&(ts)->usage));
	}
}

static ssize_t kt_test_write(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char usrCommand[512];
	int ret;

	ret = copy_from_user(usrCommand, buffer,count);
	switch (usrCommand[0]) {
	case 'c':
		kthreadTest_fc();
		break;
	case 's':
		kthreadTest_fs();
		break;
	case 'w':
		kthreadTest_fw();
		break;
	case 'p':
		kthreadTest_fp();
		break;
	case 'a' :
		kthreadTest_park();
		break;
	case 'b' :
		kthreadTest_unpark();
		break;
	case 'i':
		atomic_set(&kt_status, KT_INTR_SLEEP);
		pr_notice("set interruptable sleep\n");
		break;
	case 'u':
		atomic_set(&kt_status, KT_UNINTR_SLEEP);
		pr_notice("set uninterruptable sleep\n");
		break;
	case 'z':
		atomic_set(&kt_status, KT_IDLE_SLEEP);
		pr_notice("set idle sleep\n");
		break;
	}
	
	return count;
}



static const struct file_operations kthread_test_fops = {
  .owner = THIS_MODULE,
  .write = kt_test_write,
};


static int __init kthread_module_init(void)
{
	ts = NULL;
	atomic_set(&kt_status, 0);
	proc_create("kthread_test", 0, NULL, &kthread_test_fops);

	pr_notice("kthreadTest init\n");
	return 0;

}

static void kthread_module_exit(void)
{
	remove_proc_entry("kthread_test", NULL);
	pr_notice("kthreadTest exit\n");
}

module_init(kthread_module_init);
module_exit(kthread_module_exit);
MODULE_LICENSE("GPL");

