#include "wq.h"

static struct work_struct qhw_ws;
static struct delayed_work	qhw_delayws;

static struct workqueue_struct *qhw_wq;

static void qhw_ws_handler(struct work_struct *work)
{
	int cpuid;

	cpuid = get_cpu();
	put_cpu();

	pr_notice("%s, cpuid=%d\n",
			__func__, cpuid);
	dump_stack();
}

static void qhw_delay_ws_handler(struct work_struct *work)
{
	int cpuid;

	cpuid = smp_processor_id();

	pr_notice("%s, cpuid=%d\n",
			__func__, cpuid);
	dump_stack();
}


static ssize_t wq_test_write(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char usrCommand[512];
	int ret;
	int cpuid;
	int totalcpus;

	ret = copy_from_user(usrCommand, buffer,count);
	switch (usrCommand[0]) {
	case 'a':
		pr_notice("command: schedule work_struct\n");
		schedule_work(&qhw_ws);//Actually, this func is to queue work at system_wq
		break;
	case 'b':
		pr_notice("command: queue_work\n");
		queue_work(qhw_wq,&qhw_ws);
		break;
	case 'c':
		pr_notice("command: queue_work_on\n");
		cpuid = smp_processor_id();
		totalcpus = num_possible_cpus();
		cpuid++;
		if (cpuid == totalcpus)
			cpuid = 0;
		queue_work_on(cpuid, qhw_wq,&qhw_ws);
		break;
	case 'd':
		pr_notice("command: mod_delayed_work\n");
		//see comments on mod_delayed_work_on
		mod_delayed_work(qhw_wq,&qhw_delayws,__msecs_to_jiffies(2000));
		break;
	case 'e':
		pr_notice("command: mod_delayed_work_on\n");
		cpuid = smp_processor_id();
		totalcpus = num_possible_cpus();
		cpuid++;
		if (cpuid == totalcpus)
			cpuid = 0;
		mod_delayed_work_on(cpuid, qhw_wq,&qhw_delayws,__msecs_to_jiffies(4000));
		break;
	case 'f':
		pr_notice("command: queue_delayed_work\n");
		queue_delayed_work(qhw_wq, &qhw_delayws, __msecs_to_jiffies(6000));
		break;
	case 'g':
		pr_notice("command: queue_delayed_work_on\n");
		cpuid = smp_processor_id();
		totalcpus = num_possible_cpus();
		cpuid++;
		if (cpuid == totalcpus)
			cpuid = 0;
		queue_delayed_work_on(cpuid, qhw_wq,&qhw_delayws,__msecs_to_jiffies(8000));
		break;
	}
	
	return count;
}



static const struct file_operations wq_test_fops = {
  .owner = THIS_MODULE,
  .write = wq_test_write,
};


static int __init wq_module_init(void)
{
	int ret = 0;


	proc_create("wq_test", 0, NULL, &wq_test_fops);

	// This is struct work_struct
	INIT_WORK(&qhw_ws, qhw_ws_handler);
	INIT_DELAYED_WORK(&qhw_delayws, qhw_delay_ws_handler);

	// int alloc_workqueue(char *name, unsigned int flags, int max_active);
	qhw_wq = alloc_workqueue("qhw_wq",
					    WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
	if (!qhw_wq) {
		pr_err("error initializing wq\n");
		ret = -ENOMEM;
		goto err_out;
	}

	pr_notice("wq init\n");
	return 0;

err_out:
	return ret;

}

static void wq_module_exit(void)
{
	remove_proc_entry("wq_test", NULL);
	pr_notice("wq exit\n");
}

module_init(wq_module_init);
module_exit(wq_module_exit);
MODULE_LICENSE("GPL");

