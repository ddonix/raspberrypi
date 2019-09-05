#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <mach/platform.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <asm/io.h>
#include <mach/platform.h>

#define uchar unsigned char
#define uint  unsigned int

#define pinILASER	22
#define pinOLASER 	23

#define DRIVER_NAME "dlaser"

int major;
struct gpio_chip *gpiochip;
static dev_t dlaser_dev_id;	//device id
static struct class *dlaser_class;	//device class
static struct cdev dlaser_cdev;	//character device

static int state = 0;		//0->closed 1->open
struct GpioRegisters {
	uint32_t GPFSEL[6];
	uint32_t Reserved1;
	uint32_t GPSET[2];
	uint32_t Reserved2;
	uint32_t GPCLR[2];
	uint32_t Reserved3;
	uint32_t GPLEV[2];
};

struct GpioRegisters *s_pGpioRegisters;

static void bcm2835_gpio_fsel(int pin, int functionCode)
{
	int registerIndex = pin / 10;
	int bit = (pin % 10) * 3;

	unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
	unsigned mask = 0b111 << bit;
	printk("Changing function of GPIO%d from %x to %x\n",
	       pin, (oldValue >> bit) & 0b111, functionCode);

	s_pGpioRegisters->GPFSEL[registerIndex] =
	    (oldValue & ~mask) | ((functionCode << bit) & mask);
}

static int bcm2835_gpio_lev(int pin)
{
	return ((s_pGpioRegisters->GPLEV[pin / 32]) & (1 << (pin % 32))) ? 1: 0;
}

/*@brief set the gpio function input/output
*@parameter pin The selected gpio pin 
*@parameter functionCode 
*/

static int dlaser_open(struct inode *inode, struct file *flip)
{
	printk("Open the dlaser device!\n");
	if (state != 0) {
		printk("The file is opened!\n");
		return -1;
	}
	state++;
	printk("Open dlaser successfully!\n");
	return 0;
}

static int dlaser_release(struct inode *inode, struct file *flip)
{
	printk("Close the dlaser device!\n");
	if (state == 1) {
		state = 0;
		printk("Close the file successfully!\n");
		return 0;
	} else {
		printk("The file has closed!\n");
		return -1;
	}
}


#define BITWIDTH        12
static char r_buffer[1024];
static struct semaphore sem1;
static struct hrtimer timer;
static ktime_t kt0;
static ktime_t kt1;
static ktime_t kt5;
static ktime_t kt12;

static int r_cur = 0;
static int r_cur_bit = 0;
static int r_t = 0;		//0:读低电平,1:读到高电平，>1:读到高电平时间
static unsigned char c = 0;
static unsigned char d = 1;
static int r_want = 0;

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	//读取数据
	switch(r_want)
	{
	case 0:	//读取字节头
		if (0 == bcm2835_gpio_lev(pinILASER))
		{
			r_t = 1;
			hrtimer_forward(timer, timer->base->get_time(), kt1);
			return HRTIMER_RESTART;
		}
		else
		{
			if (0 == r_t)
			{
				hrtimer_forward(timer, timer->base->get_time(), kt1);
				return HRTIMER_RESTART;
			}
			else if((r_t >= 1) && (r_t < 21))
			{
				r_t += 5;
				hrtimer_forward(timer, timer->base->get_time(), kt5);
				return HRTIMER_RESTART;
			}
			else //r_t == 21,读到了完整的头
			{
				r_want = 1;//读位
				r_cur_bit = 0;
				c = 0;
				d = 1;
				hrtimer_forward(timer, timer->base->get_time(), kt12);
				return HRTIMER_RESTART;
			}
		}
	break;

	default://读取位数据
		if(1 == bcm2835_gpio_lev(pinILASER))
			c |= d;
		d = d<<1;
		r_cur_bit++;
		if(8 == r_cur_bit)//读满8位
		{
			r_buffer[r_cur] = c;
			r_cur++;
			if (1024 == r_cur)//读满1024字节
			{
				up(&sem1);
				return HRTIMER_NORESTART;
			}
			else
			{
				r_want = 0;
				r_t = 0;
				hrtimer_forward(timer, timer->base->get_time(), kt1);
				return HRTIMER_RESTART;
			}
		}
		else
		{
			hrtimer_forward(timer, timer->base->get_time(), kt12);
			return HRTIMER_RESTART;
		}
	break;
	}
}



static ssize_t dlaser_read(struct file *filp, char __user * buf,
			     size_t len, loff_t * off)
{
	int i;

	r_cur = 0;
	r_cur_bit = 0;
	r_t = 0;
	r_want = 0;
	c = 0;
	d = 1;
	
	hrtimer_start(&timer, kt0, HRTIMER_MODE_REL);
	down(&sem1);
	hrtimer_cancel(&timer);
	
	i = copy_to_user(buf, r_buffer, len);
	return len;
}

static struct file_operations dlaser_cdev_fops = {
	.owner = THIS_MODULE,
	.open = dlaser_open,
	.read = dlaser_read,
	.release = dlaser_release,
};

static int dlaser_init(void)
{
	int ret;

	dlaser_dev_id = MKDEV(major, 0);
	if (major)		//static allocate 
		ret = register_chrdev_region(dlaser_dev_id, 1, DRIVER_NAME);
	else			//dynamic allocate
	{
		ret = alloc_chrdev_region(&dlaser_dev_id, 0, 1, DRIVER_NAME);
		major = MAJOR(dlaser_dev_id);
	}

	if (ret < 0)
		return ret;

	cdev_init(&dlaser_cdev, &dlaser_cdev_fops);	//initialize character dev
	cdev_add(&dlaser_cdev, dlaser_dev_id, 1);	//register character device
	dlaser_class = class_create(THIS_MODULE, DRIVER_NAME);	//create a class
	device_create(dlaser_class, NULL, dlaser_dev_id, NULL, DRIVER_NAME);	//create a dev

	s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);

	printk("address = %x\n", (int)__io_address(GPIO_BASE));

	//gpio configure
	bcm2835_gpio_fsel(pinILASER, 0);
	
	kt0 = ktime_set(0, 1);		/* 1ns */
	kt1 = ktime_set(0, 1000);	/* 1us = 1000 nsec */
	kt5 = ktime_set(0, 5000);
	kt12 = ktime_set(0, 12000);	
	sema_init(&sem1, 0);
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = hrtimer_handler;

	printk("dlaser init successfully");
	return 0;
}

void dlaser_exit(void)
{
	device_destroy(dlaser_class, dlaser_dev_id);
	class_destroy(dlaser_class);
	unregister_chrdev_region(dlaser_dev_id, 1);
	printk("dlaser exit successfully\n");
}

module_init(dlaser_init);
module_exit(dlaser_exit);

MODULE_DESCRIPTION("Raspberry dlaser Driver");
MODULE_AUTHOR("Jack<wang_kejie@foxmail.com>");
MODULE_LICENSE("GPL");
