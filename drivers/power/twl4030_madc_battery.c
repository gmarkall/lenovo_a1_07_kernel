#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/twl4030_madc_battery.h>
#include <linux/slab.h>

/* Step size and prescaler ratio */
#define BK_VOLT_STEP_SIZE	441
#define BK_VOLT_PSR_R		100

#define VOLT_STEP_SIZE		588
#define VOLT_PSR_R			100

struct twl4030_madc_battery_device_info 
{	
	struct device		*dev;
	int			voltage_uV;
	int			bk_voltage_uV;
	int         voltage_soc;
	struct power_supply	bat;
	struct power_supply	bk_bat;
	struct delayed_work	twl4030_battery_madc_monitor_work;
	struct delayed_work	twl4030_bk_madc_monitor_work;
	struct twl4030_madc_battery_platform_data *twl4030_madc_pdata;
};

static enum power_supply_property twl4030_madc_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_HEALTH,
};

static enum power_supply_property twl4030_bk_madc_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

extern int twl4030_madc_conversion(struct twl4030_madc_request *req);

struct twl4030_madc_battery_device_info *g_madc_battery_di = NULL;

/*
 * Return battery capacity
 * Or < 0 on failure.
 */
int twl4030battery_capacity(void)
{
	int ret = 0;

	if(g_madc_battery_di == NULL)
		return 50;
	/*
	 * need to get the correct percentage value per the
	 * battery characteristics. Approx values for now.
	 */
	if (g_madc_battery_di->voltage_uV < 3430) {
		ret = 5;
	} else if (g_madc_battery_di->voltage_uV < 3630 && g_madc_battery_di->voltage_uV > 3430){
		ret = 10;
	} else if (g_madc_battery_di->voltage_uV < 3740 && g_madc_battery_di->voltage_uV > 3630){
		ret = 20;
	} else if (g_madc_battery_di->voltage_uV < 3800 && g_madc_battery_di->voltage_uV > 3740){
		ret = 35;
	} else if (g_madc_battery_di->voltage_uV < 3890 && g_madc_battery_di->voltage_uV > 3800){
		ret = 55;
	} else if (g_madc_battery_di->voltage_uV < 4000 && g_madc_battery_di->voltage_uV > 3890){
		ret = 75;
	} else if (g_madc_battery_di->voltage_uV < 4100 && g_madc_battery_di->voltage_uV > 4000){
		ret = 90;
	} else if (g_madc_battery_di->voltage_uV < 4130 && g_madc_battery_di->voltage_uV > 4100){
		ret = 95;
	} else if (g_madc_battery_di->voltage_uV > 4130){
		ret = 99;
		if(g_madc_battery_di->twl4030_madc_pdata->battery_is_charging && g_madc_battery_di->twl4030_madc_pdata->charger_is_online)
		{
			if(g_madc_battery_di->twl4030_madc_pdata->charger_is_online())
			{
				if(!g_madc_battery_di->twl4030_madc_pdata->battery_is_charging())
					ret = 100;
			}
		}
	}
	
	g_madc_battery_di->voltage_soc = ret;
	return ret;
}
EXPORT_SYMBOL(twl4030battery_capacity);

/*
 * Return the battery backup voltage
 * Or < 0 on failure.
 */
static int twl4030backupbatt_voltage(void)
{
	struct twl4030_madc_request req;
	int temp;

	req.channels = (1 << 9);
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	twl4030_madc_conversion(&req);
	temp = (u16)req.rbuf[9];

	return  (temp * BK_VOLT_STEP_SIZE) / BK_VOLT_PSR_R;
}

/*
 * Return battery voltage
 * Or < 0 on failure.
 */
static int twl4030battery_voltage(void)
{
	int volt;
	struct twl4030_madc_request req;

	/* No charger present.
	* BCI registers is not automatically updated.
	* Request MADC for information - 'SW1 software conversion req'
	*/
	req.channels = (1 << 12);
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	twl4030_madc_conversion(&req);
	volt = (u16)req.rbuf[12];
	volt = (volt * VOLT_STEP_SIZE) / VOLT_PSR_R;
	return volt;
}

static void twl4030_bk_madc_battery_read_status(struct twl4030_madc_battery_device_info *di)
{	
	di->bk_voltage_uV = twl4030backupbatt_voltage();

	printk("********** twl4030 MADC Battery Report ***********\n");
    printk("***        Voltage is %d              ***\n", di->voltage_uV);
	printk("***        BackupVoltage is %d        ***\n", di->bk_voltage_uV);
	printk("***        Capacity is %d                 ***\n", di->voltage_soc);
	printk("**********         END           *************\n");
}

static void twl4030_madc_battery_update_status(struct twl4030_madc_battery_device_info *di)
{	
	di->voltage_uV = twl4030battery_voltage();
	/*	 
	* Since Charger interrupt only happens for AC plug-in
	* and not for usb plug-in, we use the next update	
	* cycle to update the status of the power_supply	 
	* change to user space.	 
	*/	
	power_supply_changed(&di->bat);
}

static void twl4030_bk_madc_battery_work(struct work_struct *work)
{	
	struct twl4030_madc_battery_device_info *di = container_of(work, struct twl4030_madc_battery_device_info,	twl4030_bk_madc_monitor_work.work);
	twl4030_bk_madc_battery_read_status(di);
	schedule_delayed_work(&di->twl4030_bk_madc_monitor_work, 1000);
}

static void twl4030_madc_battery_work(struct work_struct *work)
{	
	struct twl4030_madc_battery_device_info *di = container_of(work,	struct twl4030_madc_battery_device_info, twl4030_battery_madc_monitor_work.work);
	twl4030_madc_battery_update_status(di);
	schedule_delayed_work(&di->twl4030_battery_madc_monitor_work, 1000);
}

#define to_twl4030_madc_battery_device_info(x) container_of((x), \
	struct twl4030_madc_battery_device_info, bat);

#if 0
static void twl4030_madc_battery_external_power_changed(struct power_supply *psy)
{	
	struct twl4030_madc_battery_device_info *di = to_twl4030_madc_battery_device_info(psy);
	cancel_delayed_work(&di->twl4030_battery_madc_monitor_work);	
	schedule_delayed_work(&di->twl4030_battery_madc_monitor_work, 0);
}
#endif

#define to_twl4030_bk_madc_battery_device_info(x) container_of((x), \
	struct twl4030_madc_battery_device_info, bk_bat);

static int twl4030_bk_madc_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{	
	struct twl4030_madc_battery_device_info *di = to_twl4030_bk_madc_battery_device_info(psy);
	switch (psp) {	
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:		
			val->intval = di->bk_voltage_uV;		
			break;	
		default:
			return -EINVAL;
	}	
	return 0;
}

static int twl4030_madc_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{	
	struct twl4030_madc_battery_device_info *di;
	di = to_twl4030_madc_battery_device_info(psy);

	switch (psp) {	
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:	
			val->intval = di->voltage_uV;	
			printk("hugo: twl4030_bci_battery_get_property,POWER_SUPPLY_PROP_VOLTAGE_NOW = %d \n",val->intval);
			break;	
		case POWER_SUPPLY_PROP_CAPACITY:	
			/*		 
			* need to get the correct percentage value per the
			* battery characteristics. Approx values for now.	
			*/	
			val->intval = twl4030battery_capacity();
			break;	
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		default:		
			return -EINVAL;
	}	
	return 0;
}

#if 0
static char *twl4030_madc_battery_supplied_to[] = {	
	"twl4030_madc_battery",
};
#endif

static int __init twl4030_madc_battery_probe(struct platform_device *pdev)
{
	struct twl4030_madc_battery_platform_data *pdata = pdev->dev.platform_data;
	struct twl4030_madc_battery_device_info *di;
	int ret;
    printk("[madc battery]Enter %s \n", __FUNCTION__);
	
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;
	di->twl4030_madc_pdata = pdata;
	
	di->bat.name = "twl4030_madc_battery";
	//di->bat.supplied_to = twl4030_madc_battery_supplied_to;
	//di->bat.num_supplicants = ARRAY_SIZE(twl4030_madc_battery_supplied_to);
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = twl4030_madc_battery_props;
	di->bat.num_properties = ARRAY_SIZE(twl4030_madc_battery_props);
	di->bat.get_property = twl4030_madc_battery_get_property;
	//di->bat.external_power_changed =
			//twl4030_madc_battery_external_power_changed;

	di->bk_bat.name = "twl4030_madc_bk_battery";
	di->bk_bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bk_bat.properties = twl4030_bk_madc_battery_props;
	di->bk_bat.num_properties = ARRAY_SIZE(twl4030_bk_madc_battery_props);
	di->bk_bat.get_property = twl4030_bk_madc_battery_get_property;
	di->bk_bat.external_power_changed = NULL;

	platform_set_drvdata(pdev, di);

	g_madc_battery_di = di;

	ret = power_supply_register(&pdev->dev, &di->bat);
	if (ret) {
		dev_dbg(&pdev->dev, "failed to register main battery\n");
		goto batt_failed;
	}

	INIT_DELAYED_WORK_DEFERRABLE(&di->twl4030_battery_madc_monitor_work,
				twl4030_madc_battery_work);
	schedule_delayed_work(&di->twl4030_battery_madc_monitor_work, 0);

	ret = power_supply_register(&pdev->dev, &di->bk_bat);
	if (ret) {
		dev_dbg(&pdev->dev, "failed to register backup battery\n");
		goto bk_batt_failed;
	}

	INIT_DELAYED_WORK_DEFERRABLE(&di->twl4030_bk_madc_monitor_work,
				twl4030_bk_madc_battery_work);
	schedule_delayed_work(&di->twl4030_bk_madc_monitor_work, 500);

	return 0;

bk_batt_failed:
	power_supply_unregister(&di->bat);
batt_failed:
	kfree(di);
	return ret;
}

static int __exit twl4030_madc_battery_remove(struct platform_device *pdev)
{
	struct twl4030_madc_battery_device_info *di = platform_get_drvdata(pdev);
	
	flush_scheduled_work();
	power_supply_unregister(&di->bat);
	power_supply_unregister(&di->bk_bat);
	platform_set_drvdata(pdev, NULL);
	kfree(di);

	return 0;
}

#ifdef CONFIG_PM
static int twl4030_madc_battery_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct twl4030_madc_battery_device_info *di = platform_get_drvdata(pdev);

	cancel_delayed_work(&di->twl4030_battery_madc_monitor_work);
	cancel_delayed_work(&di->twl4030_bk_madc_monitor_work);
	return 0;
}

static int twl4030_madc_battery_resume(struct platform_device *pdev)
{
	struct twl4030_madc_battery_device_info *di = platform_get_drvdata(pdev);

	schedule_delayed_work(&di->twl4030_battery_madc_monitor_work, 0);
	schedule_delayed_work(&di->twl4030_bk_madc_monitor_work, 5000);
	return 0;
}
#else
#define twl4030_madc_battery_suspend	NULL
#define twl4030_madc_battery_resume	NULL
#endif /* CONFIG_PM */

static struct platform_driver twl4030_madc_battery_driver = {
	.probe		= twl4030_madc_battery_probe,
	.remove		= __exit_p(twl4030_madc_battery_remove),
	.suspend	= twl4030_madc_battery_suspend,
	.resume		= twl4030_madc_battery_resume,
	.driver		= {
		.name	= "twl4030_madc_battery",
	},
};

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:twl4030_madc_battery");
MODULE_AUTHOR("Foxconn Inc.");

static int __init twl4030_battery_init(void)
{
	return platform_driver_register(&twl4030_madc_battery_driver);
}
module_init(twl4030_battery_init);

static void __exit twl4030_battery_exit(void)
{
	platform_driver_unregister(&twl4030_madc_battery_driver);
}
module_exit(twl4030_battery_exit);

