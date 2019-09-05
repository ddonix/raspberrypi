#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include "bcm2835.h"
    
// Blinks on RPi Plug P1 pin 11 (which is GPIO pin 17)  
#define PIN RPI_GPIO_P1_11

void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
{
    /* Function selects are 10 pins per 32 bit word, 3 bits per pin */
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFSEL0/4 + (pin/10);
    uint8_t   shift = (pin % 10) * 3;
    uint32_t  mask = BCM2835_GPIO_FSEL_MASK << shift;
    uint32_t  value = mode << shift;
    bcm2835_peri_set_bits(paddr, value, mask);
}

/* Set output pin */
void bcm2835_gpio_set(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPSET0/4 + pin/32;
    uint8_t shift = pin % 32;
    bcm2835_peri_write(paddr, 1 << shift);
}

/* Clear output pin */
void bcm2835_gpio_clr(uint8_t pin)
{
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPCLR0/4 + pin/32;
    uint8_t shift = pin % 32;
    bcm2835_peri_write(paddr, 1 << shift);
}











int open_state = 0;		//文件打开状态  

static int leds_open(struct inode *inode, struct file *filp)
{
	if (open_state == 0) {
		open_state = 1;
		printk("Open file suc!\n");
		return 0;
	} else {
		printk("The file has opened!\n");
		return -1;
	}
}

static int leds_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case 0:
		bcm2835_gpio_clr(PIN);
		printk("LED OFF!\n");
		break;
	case 1:
		bcm2835_gpio_set(PIN);
		printk("LED ON!\n");
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int leds_release(struct inode *inode, struct file *filp)
{
	if (open_state == 1) {
		open_state = 0;
		printk("close file suc!\n");
		return 0;
	} else {
		printk("The file has closed!\n");
		return -1;
	}
}

static const struct file_operations leds_fops = {
	.owner = THIS_MODULE,
	.open = leds_open,
	.unlocked_ioctl = leds_ioctl,
	.release = leds_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "my_leds",
	.fops = &leds_fops,
};

static int __init leds_init(void)
{
	int ret;

	//注册混杂设备  
	ret = misc_register(&misc);

	//配置功能选择寄存器为输出  
	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_OUTP);

	//设置输出电平为高电平，LED亮  
	bcm2835_gpio_set(PIN);

	printk("ledsinit.\n");
	return ret;
}

static void leds_exit(void)
{
	//LED灭  
	bcm2835_gpio_clr(PIN);

	misc_deregister(&misc);

	printk("leds_exit\n");
}

module_init(leds_init);
module_exit(leds_exit);

MODULE_AUTHOR("Hu Chunxu");
MODULE_LICENSE("GPL");
