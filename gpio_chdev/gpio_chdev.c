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
#include <linux/hrtimer.h>
#include <asm/io.h>
#include <mach/platform.h>

#define uchar unsigned char
#define uint  unsigned int


#define IN_LASER	22
#define OUT_LASER 	23

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

static inline int bcm2835_gpio_lev(int pin)
{
	return ((s_pGpioRegisters->GPLEV[pin / 32]) & (1 << (pin % 32))) ? 1: 0;
}


static void bcm2835_gpio_set(int pin)
{
	s_pGpioRegisters->GPSET[pin / 32] = (1 << (pin % 32));
	__tudelay(5);
	s_pGpioRegisters->GPSET[pin / 32] = (1 << (pin % 32));
}

static void bcm2835_gpio_clr(int pin)
{
	s_pGpioRegisters->GPCLR[pin / 32] = (1 << (pin % 32));
	__tudelay(5);
	s_pGpioRegisters->GPCLR[pin / 32] = (1 << (pin % 32));
}


#define BITWIDTH        12
static unsigned char r_buffer[1024];

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

#define TSC unsigned long
static TSC *s_pTSCL = 0;

static inline TSC getTSC(void)
{
	return *s_pTSCL;
}

static inline TSC __gotoTSC(TSC t)
{
	TSC c;
	while(1)
	{
		c = *s_pTSCL;
		if(c >= t)
			return c;
		else
			__tudelay(1);
	}
}

static inline TSC gotoTSC(TSC t, unsigned long d)
{
	return __gotoTSC(t+d);
}

static TSC udelayTSC(unsigned long d)
{
	return __gotoTSC(getTSC()+d);
}

static ssize_t dlaser_read(struct file *filp, char __user * buf,
			     size_t len, loff_t * off)
{
	int tag;
	int i,j,k,ret;
	char c,d;
	TSC ct;
	ret = len < 1024 ? len : 1024;
	local_irq_disable();

	/*
	for(i = 0; i < 20;i++)
	{
		while(1) {
			if (0 == bcm2835_gpio_lev(IN_LASER))
				break;
			udelayTSC(1);
		}
		while(1) {
			if (1 == bcm2835_gpio_lev(IN_LASER))
				break;
			udelayTSC(1);
		}
		ct = getTSC();
		j = 0;
		for(j = 0; j < 100000;j++)
		{
			gotoTSC(ct, 2+j*8);
			if(0 == bcm2835_gpio_lev(IN_LASER))
				break;
			
			gotoTSC(ct, 4+j*8);
			if(1 == bcm2835_gpio_lev(IN_LASER))
				break;
		}
		printk("llll %d\n", j);
	}
	*/
	for (i = 0; i < ret; i++) {
		tag = 0;
		while (1) {
			while(1) {
				if (0 == bcm2835_gpio_lev(IN_LASER))
					break;
				udelayTSC(1);
			}
			while(1) {
				if (1 == bcm2835_gpio_lev(IN_LASER))
					break;
				udelayTSC(1);
			}
			while(1) {
				ct = getTSC();
				
				k = 0;
				for(k = 1; k < 22; k++)
				{
					gotoTSC(ct,k);
					if (0 == bcm2835_gpio_lev(IN_LASER))
						break;
				}
				if(k == 22) {
					tag = 1;
					break;
				}
			}
			if (1 == tag)
				break;
		}
		c = 0;
		d = 1;
		for (j = 1; j <= 8; j++) {
			gotoTSC(ct,18+j*12);

			if (1 == bcm2835_gpio_lev(IN_LASER)) {
				c |= d;
			} 
			d = d << 1;
		}
		r_buffer[i] = c;
	}
//	*/
	i = copy_to_user(buf, r_buffer, ret);
	local_irq_enable();
	return 0;
}

static struct file_operations dlaser_cdev_fops = {
	.owner = THIS_MODULE,
	.open = dlaser_open,
	.read = dlaser_read,
	.release = dlaser_release,
};

static int dlaser_init(void)
{
	int i,j,ret;
	TSC ct1,ct2;
	TSC t1[100],t2[100];
	int high, low;
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

	s_pTSCL = (TSC *)((unsigned long)__io_address(GPIO_BASE) - (0x200000-0x3004));//不考虑溢出情况
	
	bcm2835_gpio_fsel(IN_LASER, 0);
	bcm2835_gpio_fsel(OUT_LASER, 1);

	//gpio configure
	printk("dlaser init successfully s_pGpioRegisters=%p, s_pTSCL=%p\n", s_pGpioRegisters, s_pTSCL);
/*	
	low=high=0;
	ct1 = getTSC();
	for(i =0; i< 1000000; i++)
	{
		if(0==bcm2835_gpio_lev(IN_LASER))
			low++;
		else
			high++;
	}
	ct2 = getTSC();
	printk("high=%u, low=%u, ct1=%u ct2=%u, ct2-ct1=%u\n", high, low, ct1, ct2, ct2-ct1);
	
	ct1 = getTSC();
	for(i =0; i< 1000000; i++)
	{
		if(i%2)
			bcm2835_gpio_set(OUT_LASER);
		else
			bcm2835_gpio_clr(OUT_LASER);

	}
	ct2 = getTSC();
	printk("ct1=%d ct2=%d, ct2-ct1=%d\n", ct1, ct2, ct2-ct1);
	

	for(i = 0; i < 10; i++)
	{
		ct1 = getTSC();
		bcm2835_gpio_clr(OUT_LASER);
		gotoTSC(ct1,10);
		
		ct1 = getTSC();
		bcm2835_gpio_set(OUT_LASER);

		if(1 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 1 read 1 0\n");
			continue;
		}
		gotoTSC(ct1,1);
		if(1 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 1 read 1 1\n");
			continue;
		}
		gotoTSC(ct1,2);
		if(1 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 1 read 1 2\n");
			continue;
		}
		gotoTSC(ct1,4);
		if(1 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 1 read 1 4\n");
			continue;
		}
	}
	
	for(i = 0; i < 10; i++)
	{
		ct1 = getTSC();
		bcm2835_gpio_set(OUT_LASER);
		gotoTSC(ct1,10);
		
		ct1 = getTSC();
		bcm2835_gpio_clr(OUT_LASER);

		if(0 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 0 read 0 0\n");
			continue;
		}
		gotoTSC(ct1,1);
		if(0 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 0 read 0 1\n");
			continue;
		}
		gotoTSC(ct1,2);
		if(0 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 0 read 0 2\n");
			continue;
		}
		gotoTSC(ct1,4);
		if(0 == bcm2835_gpio_lev(IN_LASER)) {
			printk("write 0 read 0 4\n");
			continue;
		}
	}

	ct1=getTSC();
	bcm2835_gpio_clr(OUT_LASER);
	gotoTSC(ct1,10);
	for(j = 0;j < 50;j++)
	{
		t1[j*2]=getTSC();
		t2[j*2]=0;
		bcm2835_gpio_set(OUT_LASER);
		
		for(i=0;i<10;i++) {
			if(1 == bcm2835_gpio_lev(IN_LASER)) {
				t2[j*2]=getTSC();
				break;
			}
		}
		
		t1[j*2+1]=getTSC();
		t2[j*2+1]=0;
		bcm2835_gpio_clr(OUT_LASER);
		for(i=0;i<10;i++) {
			if(0 == bcm2835_gpio_lev(IN_LASER)) {
				t2[j*2+1]=getTSC();
				break;
			}
		}

	}
	for(j=0;j<100;j++)
		printk("write p read p %u %u %u\n",t2[j], t1[j], t2[j]-t1[j]);
*/	
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
