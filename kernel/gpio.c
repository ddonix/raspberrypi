#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/cdev.h>

//定义命令   
#define LED_ON      _IOW('h',0x01,unsigned long)	//LED开的命令
#define LED_OFF     _IOW('h',0x02,unsigned long)	//LED关的命令

int open_state = 0;		//1为打开，0为关闭   

/*----------------------------------------------------------------------------- 
函数名:      led_open 
参数:        struct inode *inode,struct file *filp 
返回值:      int 
描述:        open对应的驱动函数 
*-----------------------------------------------------------------------------*/
int led_open(struct inode *inode, struct file *filp)
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

/*----------------------------------------------------------------------------- 
函数名:      led_release 
参数:        struct inode *inode,struct file *filp 
返回值:      int 
描述:        open对应的驱动函数 
*-----------------------------------------------------------------------------*/
int led_release(struct inode *inode, struct file *filp)
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

/*----------------------------------------------------------------------------- 
函数名:      led_ioctl 
参数:        struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg 
返回值:      int 
描述:        ioctl对应的驱动函数 
*-----------------------------------------------------------------------------*/
int led_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
	      unsigned long arg)
{
	switch (cmd) {
	case LED_ON:
		printk("ON!\n");
//              at91_set_gpio_value(AT91_PIN_PC0, 1);   //灯亮起来   
		break;

	case LED_OFF:
		printk("OFF\n");
//              at91_set_gpio_value(AT91_PIN_PC0, 0);   //灯灭掉   
		break;

	default:
		printk("Error command!\n");
	}
	return 0;
}

const struct file_operations led_fop = {
	.owner = THIS_MODULE,
	.open = led_open,
	.unlocked_ioctl = led_ioctl,
	.release = led_release,
};

struct miscdevice misc = {
	.minor = 30,
	.fops = &led_fop,
	.name = "led3"
};

/*----------------------------------------------------------------------------- 
函数名:      gpio_init 
参数:        void 
返回值:      int 
描述:        模块初始化函数，在安装模块时候执行 
*-----------------------------------------------------------------------------*/
static int __init gpio_init(void)
{
	struct device *dev;
	int major;		//自动分配主设备号
	major = alloc_chrdev_region(0, 0, 1, "");
	//register_chrdev 注册字符设备使系统知道有LED这个模块在.

	cdev_init(0,0);
	major = cdev_add(0, 0, 1);
	//注册class
//	pi_led_class = class_create(THIS_MODULE, "");

//	dev = device_create(pi_led_class, NULL, pi_led_devno, NULL, DRIVER_NAME);

//	gpiochip = gpiochip_find("bcm2708_gpio", is_right_chip);
//	gpiochip ->direction_output(gpiochip, led_pin, 1);
//	gpiochip ->set(gpiochip, led_pin, 0);
	printk("pi led init ok!\n");
	return 0;

}

/*----------------------------------------------------------------------------- 
函数名:      gpio_exit 
参数:        void 
返回值:      void 
描述:        模块卸载函数，在卸载模块时候执行 
*-----------------------------------------------------------------------------*/
static void __exit gpio_exit(void)
{
	misc_deregister(&misc);
	open_state = 0;
	printk("GPIO test End\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
