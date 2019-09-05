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
#include <linux/slab.h>
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

/*@brief set the gpio function input/output
*@parameter pin The selected gpio pin 
*@parameter functionCode 
*/
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
	return ((s_pGpioRegisters->
		 GPLEV[pin / 32]) & (1 << (pin % 32))) ? 1 : 0;
}

static void bcm2835_gpio_set(int pin)
{
	s_pGpioRegisters->GPSET[pin / 32] = (1 << (pin % 32));
}

static void bcm2835_gpio_clr(int pin)
{
	s_pGpioRegisters->GPCLR[pin / 32] = (1 << (pin % 32));
}

static unsigned char r_buffer[1024];
static unsigned char *w_buffer;

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
	while (1) {
		c = *s_pTSCL;
		if (c >= t)
			return c;
		else
			__tudelay(1);
	}
}

static inline TSC gotoTSC(TSC t, unsigned long d)
{
	return __gotoTSC(t + d);
}

static TSC udelayTSC(unsigned long d)
{
	return __gotoTSC(getTSC() + d);
}

static ssize_t dlaser_read(struct file *filp, char __user * buf,
			   size_t len, loff_t * off)
{
	int tag;
	int i, j, ret;
	char c, d;
	TSC ct;

	ret = len < 1024 ? len : 1024;
	local_irq_disable();
	for (i = 0; i < ret; i++) {
		tag = 0;
		while (1) {
			while (1) {
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;
				udelayTSC(1);
			}
			while (1) {
				if (1 == bcm2835_gpio_lev(pinILASER))
					break;
				udelayTSC(1);
			}
			while (1) {
				ct = getTSC();
				gotoTSC(ct, 2);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 4);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 6);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 8);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 10);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 12);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 14);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 16);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 18);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				gotoTSC(ct, 20);
				if (0 == bcm2835_gpio_lev(pinILASER))
					break;

				tag = 1;
				break;
			}
			if (1 == tag)
				break;
		}
		c = 0;
		d = 1;
		for (j = 1; j <= 8; j++) {
			gotoTSC(ct, 20 + j * 12);

			if (1 == bcm2835_gpio_lev(pinILASER)) {
				c |= d;
			} else {
			}
			d = d << 1;
		}
		r_buffer[i] = c;
		udelayTSC(4);
	}
	i = copy_to_user(buf, r_buffer, ret);
	local_irq_enable();
	return 0;
}

static int wmode = 0;
static ssize_t dlaser_write(struct file *filp, const char __user * buf,
			    size_t len, loff_t * off)
{
	int i, j;
	unsigned char d;
	TSC ct;
	wmode = 2;
	local_irq_disable();
	if (copy_from_user(w_buffer, buf, len)) {
		local_irq_enable();
		return -EFAULT;
	}
	if (wmode == 0)		//低电平使能
	{
		for (i = 0; i < len; i++) {
			ct = getTSC();
			bcm2835_gpio_clr(pinOLASER);
			gotoTSC(ct, 24);

			d = 1;
			for (j = 0; j < 8; j++) {
				if (w_buffer[i] & d) {
					bcm2835_gpio_set(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 4);
					bcm2835_gpio_clr(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 8);
				} else {
					bcm2835_gpio_clr(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 4);
					bcm2835_gpio_set(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 8);
				}
				d = d << 1;
			}
			bcm2835_gpio_set(pinOLASER);
			//125us一个位，采样率8000
			gotoTSC(ct, 24 + 8 * 12 + 4);
		}
	}
	if (wmode == 1)		//gao电平使能
	{
		for (i = 0; i < len; i++) {
			ct = getTSC();
			bcm2835_gpio_set(pinOLASER);
			gotoTSC(ct, 24);

			d = 1;
			for (j = 0; j < 8; j++) {
				if (w_buffer[i] & d) {
					bcm2835_gpio_clr(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 1);
					bcm2835_gpio_set(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 10);
					bcm2835_gpio_clr(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 1);
				} else {
					bcm2835_gpio_set(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 1);
					bcm2835_gpio_clr(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 10);
					bcm2835_gpio_set(pinOLASER);
					gotoTSC(ct, 24 + j * 12 + 1);
				}
				d = d << 1;
			}
			bcm2835_gpio_clr(pinOLASER);
			//125us一个位，采样率8000
			gotoTSC(ct, 24 + 8 * 12 + 4);
		}
	}
	if (wmode == 2)
	{
		ct = getTSC();
		for(i = 0; i < 10000000; i++)
		{
			bcm2835_gpio_clr(pinOLASER);
			gotoTSC(ct, i*12+6);
			
			bcm2835_gpio_set(pinOLASER);
			gotoTSC(ct, (i+1)*12);
		}
	}
	local_irq_enable();
	return len;
}

static struct file_operations dlaser_cdev_fops = {
	.owner = THIS_MODULE,
	.open = dlaser_open,
	.read = dlaser_read,
	.write = dlaser_write,
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

	s_pTSCL = (TSC *) ((unsigned long)__io_address(GPIO_BASE) + 0x3004 -0x200000);	//不考虑溢出情况

	//gpio configure
	bcm2835_gpio_fsel(pinILASER, 0);
	w_buffer = (unsigned char *)kmalloc(1024*2000, GFP_ATOMIC);
	
	printk("dlaser init successfull s_pTSCL=%p s_pGpioRegisters=%p w_buffer=%p\n", s_pGpioRegisters, s_pTSCL, w_buffer);
	return 0;
}

void dlaser_exit(void)
{
	device_destroy(dlaser_class, dlaser_dev_id);
	class_destroy(dlaser_class);
	unregister_chrdev_region(dlaser_dev_id, 1);
	if(w_buffer)
		kfree(w_buffer);
	printk("dlaser exit successfulln");
}

module_init(dlaser_init);
module_exit(dlaser_exit);

MODULE_DESCRIPTION("Raspberry dlaser Driver");
MODULE_AUTHOR("Jack<wang_kejie@foxmail.com>");
MODULE_LICENSE("GPL");
