#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
static struct hrtimer timer;
ktime_t kt;
static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	printk(" ------ I am in hrtimer -----\n");
	hrtimer_forward(timer, timer->base->get_time(), kt);
	
	
	return HRTIMER_RESTART;
}

static int __init test_init(void)
{
	pr_info("timer resolution: %lu\n", TICK_NSEC);
	kt = ktime_set(0, 1000);	/* 1 sec, 10 nsec */
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = hrtimer_handler;
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL);
	printk("\n-------- test start ---------\n");
	return 0;
}

static void __exit test_exit(void)
{
	hrtimer_cancel(&timer);
	printk("-------- test over ----------\n");
	return;
}

MODULE_LICENSE("GPL");
module_init(test_init);
module_exit(test_exit);
