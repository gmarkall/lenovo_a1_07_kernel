#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <mach/gpio.h>

#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
#include <linux/share_region.h>
#include <linux/wakelock.h>

int modem_PW = 0, PHY_reset = 0, modem_resume_state = 0;
static int use_3g = 0;
static struct platform_device *modem;
static struct wake_lock modem_wake_lock;
static int wake_lock_state = 0;

#define USB_PHY_RESET_GPIO      23
#define USB_PHY_RESET_GPIO_NAME      "usb_phy_reset"

#define MODEM_PWR_EN_GPIO       36
#define MODEM_PWR_EN_GPIO_NAME      "modem_pow_en"

#define MODEM_RESET_GPIO        35//35(EVT2)//43(EVT1)
#define MODEM_RESET_GPIO_NAME       "modem_reset" 
/*<--LH_SWRD_CL1_Sam@20110910,Add for 3G Board*/
#if defined(CONFIG_CL1_3G_BOARD )
#define    MODEM_W_DISABLE_GPIO 	(4) //4(DVT3)//21(EVT2)//44(EVT1)
#else
#define    MODEM_W_DISABLE_GPIO 	(21)  //4(DVT3)//21(EVT2)//44(EVT1)
#endif
/*LH_SWRD_CL1_Sam@20110910,Add for 3G Board-->*/
#define MODEM_W_DISABLE_GPIO_NAME   "modem_w_disable"

extern int PPPorDirectIPMode;
void modem_hsdpa_pwr (int on);

static int modem_suspend(struct platform_device  *pdev, pm_message_t state);
static int modem_resume(struct platform_device  *pdev);

static ssize_t show_modem_power_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_modem_power_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count);
static ssize_t show_PHY_reset_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_PHY_reset_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count);
static ssize_t show_resume_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_resume_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count);
static ssize_t show_connect_mode_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_wake_lock_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count);
static ssize_t show_wake_lock_status(struct device *dev, struct device_attribute *attr, char *buf);

MODULE_LICENSE("GPL");

#define MODEM_MAJOR     232
#define MODEM_DRIVER	"Modem_driver"

static struct device_attribute modem_power_attr =
    __ATTR(ModemPower, 0660, show_modem_power_status, store_modem_power_status);

static struct device_attribute PHY_reset_attr =
    __ATTR(PHYreset, 0660, show_PHY_reset_status, store_PHY_reset_status);

static struct device_attribute resume_attr =
    __ATTR(Resume, 0660, show_resume_status, store_resume_status);

static struct device_attribute mode_attr =
    __ATTR(Mode, 0660, show_connect_mode_status, NULL);

static struct device_attribute wakelock_attr =
    __ATTR(Wakelock, 0660, show_wake_lock_status, store_wake_lock_status);

static struct attribute *modem_attrs[] = {
    &modem_power_attr.attr,
    &PHY_reset_attr.attr,
    &resume_attr.attr,
    &mode_attr.attr,
    &wakelock_attr.attr,
    NULL,
};

static struct attribute_group modem_attr_grp = {
    .name = "control",
    .attrs = modem_attrs,
};

static struct file_operations modem_file_operations = 
{
    .owner = THIS_MODULE,
};

void modem_hsdpa_pwr (int on)
{
    printk("3G Modem %s\n", on ? "on" : "off");

    gpio_direction_output(MODEM_W_DISABLE_GPIO, on);
    gpio_direction_output(MODEM_RESET_GPIO, on);
    gpio_direction_output(MODEM_PWR_EN_GPIO, on);

    modem_PW=on;
}

static int modem_probe(struct platform_device *pdev)
{
    use_3g = ep_get_3G_exist();
    printk(" >>> modem_probe %s\n", use_3g ? "have modem module" : "no modem module");
    gpio_direction_output(USB_PHY_RESET_GPIO, 0);
    modem_hsdpa_pwr(0);
    return 0;
}

#ifdef CONFIG_PM
static int modem_suspend(struct platform_device  *pdev, pm_message_t state)
{
    printk(">>> modem_suspend\n");
    return 0;
}

static int modem_resume(struct platform_device  *pdev)
{
    printk(">>> modem_resume\n");
    if(use_3g){
        modem_resume_state = 1;
    }
    return 0;
}
#endif

EXPORT_SYMBOL(modem_PW);
EXPORT_SYMBOL(PHY_reset);
EXPORT_SYMBOL(modem_resume_state);

static const char on_string[] = "on";
static const char off_string[] = "off";
static const char ppp_string[] = "PPP";
static const char directip_string[] = "DirectIP";

static ssize_t show_modem_power_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *p = off_string;

    if (modem_PW)        p = on_string;

    return sprintf(buf, "%s\n", p);
}

static ssize_t store_modem_power_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
    const char *p = off_string;

    if(use_3g){
        if(!memcmp(buf, on_string, 2) && !modem_PW){
            p = on_string;
            modem_hsdpa_pwr(1);
            return sprintf(buf, "%s\n", p);
        }else if(!memcmp(buf, off_string, 3) && modem_PW){
            p = off_string;
            modem_hsdpa_pwr(0);
            return sprintf(buf, "%s\n", p);
        }
    }
    return off_string;
}

static ssize_t show_PHY_reset_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *p = off_string;

    if (PHY_reset)        p = on_string;

    return sprintf(buf, "%s\n", p);
}

static ssize_t store_PHY_reset_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
    const char *p = off_string;

    if(use_3g){
        if(!memcmp(buf, on_string, 2) && !PHY_reset){
            PHY_reset = 1;
            p = on_string;
            gpio_direction_output(USB_PHY_RESET_GPIO, 1);
            return sprintf(buf, "%s\n", p);
        }else if(!memcmp(buf, off_string, 3) && PHY_reset){
            PHY_reset = 0;
            p = off_string;
            gpio_direction_output(USB_PHY_RESET_GPIO, 0);
            return sprintf(buf, "%s\n", p);
        }
    }
    return off_string;
}

static ssize_t show_resume_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *p = off_string;

    if (modem_resume_state)        p = on_string;

    return sprintf(buf, "%s\n", p);
}

static ssize_t store_resume_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
    const char *p = off_string;

    if(use_3g){
        if(!memcmp(buf, off_string, 3) && modem_resume_state){
            modem_resume_state = 0;
            p = off_string;
            return sprintf(buf, "%s\n", p);
        }
    }
    return off_string;
}

static ssize_t show_connect_mode_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *p = directip_string;

    if (PPPorDirectIPMode)        p = ppp_string;

    return sprintf(buf, "%s\n", p);
}

static ssize_t show_wake_lock_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    const char *p = off_string;

    if (wake_lock_state)        p = on_string;

    return sprintf(buf, "%s\n", p);
}

static ssize_t store_wake_lock_status(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
    const char *p = off_string;

    if(use_3g){
        if(!memcmp(buf, on_string, 2) && !wake_lock_state){
            wake_lock_state = 1;
            p = on_string;
            wake_lock(&modem_wake_lock);
            return sprintf(buf, "%s\n", p);
        }else if(!memcmp(buf, off_string, 3) && wake_lock_state){
            wake_lock_state = 0;
            p = off_string;
            wake_unlock(&modem_wake_lock);
            return sprintf(buf, "%s\n", p);
        }
    }
    return off_string;
}

static struct platform_driver modem_driver = {
	.probe		= modem_probe,
#ifdef CONFIG_PM
	.suspend	= modem_suspend,
	.resume		= modem_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MODEM_DRIVER
	},
};

static int modem_init(void)
{
	int retval;

	printk(">>> modem_init\n");

	retval = platform_driver_register(&modem_driver);
	if (retval < 0)
	  return retval;

    modem = platform_device_register_simple(MODEM_DRIVER, -1, NULL, 0);	  
    if (IS_ERR(modem))
        return -1;

    sysfs_create_group(&modem->dev.kobj, &modem_attr_grp);
	retval = register_chrdev(MODEM_MAJOR, MODEM_DRIVER, &modem_file_operations);
	if (retval < 0)
	{
		printk(KERN_WARNING "Can't get major %d\n", MODEM_MAJOR);
		return retval;
	}

    wake_lock_init(&modem_wake_lock, WAKE_LOCK_SUSPEND, "Modem_event");
	printk("<<< modem_init\n");
	return 0;
}

static void modem_exit(void)
{
    printk("Modem_exit...");
    sysfs_remove_group(&modem->dev.kobj, &modem_attr_grp);
    unregister_chrdev(MODEM_MAJOR, MODEM_DRIVER);
}

module_init(modem_init);
module_exit(modem_exit);

