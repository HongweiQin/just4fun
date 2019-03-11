#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/uuid.h>
#include <linux/cpumask.h>
#include <linux/delay.h>

#define AVS_NR_THREADS (8)
#define USE_ATOMIC

static struct task_struct *avs_ts[AVS_NR_THREADS];
static unsigned long long avs_count[AVS_NR_THREADS];

static struct work_struct test_ws;
static struct delayed_work	test_delayws;


#ifdef USE_ATOMIC
atomic_t a_value;
#else
spinlock_t value_lock;
int s_value;
#endif



static int avs_thread_fn(void *data)
{
	long th_id = (long) data;
	unsigned long long *countp = &avs_count[th_id];

	while (!kthread_should_stop()) {
#ifdef USE_ATOMIC
		atomic_set(&a_value, th_id + 10086);
#else
		spin_lock(&value_lock);
		s_value = th_id + 10086;
		spin_unlock(&value_lock);
#endif
		(*countp)++;
		//schedule();
	}
	return 0;
}

static void lets_go(void)
{
	int i;
	for (i = 0; i < AVS_NR_THREADS; i++)
		wake_up_process(avs_ts[i]);
}

static void test_stop(void)
{
	int i;
	for (i = 0; i < AVS_NR_THREADS; i++)
		kthread_stop(avs_ts[i]);
}

static void test_delay_ws_handler(struct work_struct *work)
{
	unsigned long long totalcount;
	long i;

	test_stop();
	totalcount = 0;
	for(i = 0; i < AVS_NR_THREADS; i++) {
		totalcount += avs_count[i];
		pr_notice("[%ld], %llu\n", i, avs_count[i]);
	}
	pr_notice("Total: %llu\n", totalcount);
}

static void test_ws_handler(struct work_struct *work)
{
	char tsname[64];
	long i;

#ifdef USE_ATOMIC
	atomic_set(&a_value, 0);
#else
	spin_lock_init(&value_lock);
	s_value = 0;
#endif
	INIT_DELAYED_WORK(&test_delayws, test_delay_ws_handler);
	for (i=0;i<AVS_NR_THREADS;i++) {
		avs_count[i] = 0;
		sprintf(tsname, "avs_%ld", i);
		avs_ts[i] = kthread_create(avs_thread_fn, (void *)i, tsname);
		if (IS_ERR(avs_ts[i])) {
			pr_err("no space for avs_ts\n");
			return;
		}
	}
	
	lets_go();
	schedule_delayed_work(&test_delayws,__msecs_to_jiffies(5000));

}


static int __init avs_init(void)
{
	INIT_WORK(&test_ws, test_ws_handler);

	schedule_work(&test_ws);
	pr_notice("avs init\n");
	return 0;
}

static void avs_exit(void)
{
	pr_notice("avs exit\n");
}

module_init(avs_init);
module_exit(avs_exit);
MODULE_LICENSE("GPL");




