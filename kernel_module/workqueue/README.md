# WorkQueue
For more info, see: linux/Documentation/core-api/workqueue.rst

## Definition

Workqueue is a kernel infrastructure to allow kernel modules or subsystems to delay the execution of a task(usually a function).

To maximize performance, the queues are managed in a per-cpu manner. In other words, by default, each cpu has a queue and a kernel thread to iterate this queue to perform queued functions.

Actually there are more than one queue-thread pairs for each CPU. For one CPU, multiple queues are used. As shown in `include/linux/workqueue.h', kernel defines several workqueues. This not only enables diffent configurations such as thread priority but also avoids contentions between tasks inside one queue.

Using shared workqueues enables kernel to save PIDs. It also makes kernel development easier. If kernel developers feels necessary, they can also define their own workqueues.

## Key Data Structures

```
struct workqueue_struct *wq;
```

This is a workqueue. BUT, one `struct workqueue_struct' contains a set of queues, each corresponds to one cpu.

For example, if we have 4 cpus, one `struct workqueue_struct' actually means 4 queues.


```
struct work_struct ws;
struct delayed_work delay_ws;
```

These are works that can be inserted into a workqueue. Differences are APIs, see codes for more info.
