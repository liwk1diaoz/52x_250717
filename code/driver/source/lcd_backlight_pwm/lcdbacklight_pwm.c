#include <linux/delay.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <kwrap/dev.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include "comm/pwm.h"
#include "kwrap/util.h"

#define __MODULE__			lcd_backlight_pwm
#define __DBGLVL__			1      // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__			"*"    //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#define IOC_LCD_BACKLIGHT_ON              	_IOWR('w', 1, unsigned int)
#define IOC_LCD_BACKLIGHT_OFF              	_IOWR('w', 2, unsigned int)
#define IOC_LCD_BACKLIGHT_HIGH             	_IOWR('w', 3, unsigned int) //duty cycle 100
#define IOC_LCD_BACKLIGHT_MID             	_IOWR('w', 4, unsigned int) //duty cycle 60
#define IOC_LCD_BACKLIGHT_LOW             	_IOWR('w', 5, unsigned int) //duty cycle 20

static PWM_CFG g_LCDBacklightPWMInfo = {100, 0, 100, 0, 0};

static long lcdpwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int level = 0;

	switch (cmd) {
	case IOC_LCD_BACKLIGHT_ON:
	     ret = copy_from_user(&level, (void *)arg, sizeof(unsigned int));
	     if (ret == 0)
	     {
	     	printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_LEVEL_0\r\n");
		g_LCDBacklightPWMInfo.ui_rise = 0;
		g_LCDBacklightPWMInfo.ui_fall = level;
		pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
		pwm_pwm_reload(PWMID_3);
	     }
	     else
	     {
		printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_ON failed\r\n");
	     }
	     break;

	case IOC_LCD_BACKLIGHT_OFF:
	     printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_OFF\r\n");
	     g_LCDBacklightPWMInfo.ui_rise = 100;
	     g_LCDBacklightPWMInfo.ui_fall = 0;
	     pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
	     pwm_pwm_reload(PWMID_3);
	     break;

	case IOC_LCD_BACKLIGHT_HIGH:
	     printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_HIGH\r\n");
	     g_LCDBacklightPWMInfo.ui_rise = 0;
	     g_LCDBacklightPWMInfo.ui_fall = 100;
	     pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
	     pwm_pwm_reload(PWMID_3);
	     break;

	case IOC_LCD_BACKLIGHT_MID:
	     printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_MID\r\n");
	     g_LCDBacklightPWMInfo.ui_rise = 40;
	     g_LCDBacklightPWMInfo.ui_fall = 100;
	     pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
	     pwm_pwm_reload(PWMID_3);
	     break;

	case IOC_LCD_BACKLIGHT_LOW:
	     printk("lcdpwm_ioctl: IOC_LCD_BACKLIGHT_LOW\r\n");
	     g_LCDBacklightPWMInfo.ui_rise = 80;
	     g_LCDBacklightPWMInfo.ui_fall = 100;
	     pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
	     pwm_pwm_reload(PWMID_3);
	     break;

	default:
	     printk("lcdpwm_ioctl: wrong operate\r\n");
	     break;
	}

	return 0;
}

static const struct file_operations lcdpwm_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl	= lcdpwm_ioctl,
};

static struct miscdevice lcdpwm_miscdev = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "lcdpwm",
	.fops           = &lcdpwm_fops,
};

static int __init lcdpwm_module_init(void)
{
	int rt;
	struct device_node* dt_node = of_find_node_by_path("/logo");
	u32 u32Enable = 0;

	//printk("lcdpwm_module_init\r\n");

	if (dt_node) {
		of_property_read_u32(dt_node, "enable", &u32Enable);
        printk("lcdpwm_module_init: value = %d\r\n", u32Enable);
	}

	rt = misc_register(&lcdpwm_miscdev);
    if (rt) {
        DBG_ERR("misc_register fail, ret = %d\n", rt);
        return rt;
    }

    pwm_open(PWMID_3);
    if (u32Enable == 0) {
        pwm_pwm_config_clock_div(PWM0_3_CLKDIV, 99); //120MHz/(1+div)
        pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
        pwm_pwm_enable(PWMID_3);
    } else {
        pwm_pwm_reload_config(PWMID_3, 0,  50,  100);
	    pwm_pwm_reload(PWMID_3);
    }

#if 0 //only for test
    vos_util_delay_ms(1000);

    g_LCDBacklightPWMInfo.ui_fall = 30;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);

    vos_util_delay_ms(1000);

    g_LCDBacklightPWMInfo.ui_fall = 50;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);

    vos_util_delay_ms(1000);

    g_LCDBacklightPWMInfo.ui_fall = 70;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);

    vos_util_delay_ms(1000);

    g_LCDBacklightPWMInfo.ui_fall = 90;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);

    vos_util_delay_ms(1000);

    g_LCDBacklightPWMInfo.ui_rise = 100;
    g_LCDBacklightPWMInfo.ui_fall = 0;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);

    g_LCDBacklightPWMInfo.ui_rise = 0;
    g_LCDBacklightPWMInfo.ui_fall = 50;
    pwm_pwm_config(PWMID_3, &g_LCDBacklightPWMInfo);
    pwm_pwm_reload(PWMID_3);
#endif

    return 0;
}

static void __exit lcdpwm_module_exit(void)
{
	DBG_IND("\n");

    pwm_pwm_disable(PWMID_3);
    pwm_close(PWMID_3, TRUE);

    misc_deregister(&lcdpwm_miscdev);
}

module_init(lcdpwm_module_init);
module_exit(lcdpwm_module_exit);

MODULE_DESCRIPTION("pwm ctrl lcd backlight");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
