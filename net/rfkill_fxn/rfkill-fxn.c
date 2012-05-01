#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include <linux/delay.h>
#include <linux/rfkill_2629.h>
#include <linux/fxn-rfkill.h>
//&*&*&*BC1_110513:add the wifi suspend wakelock to avoid wifi or system crash
#include <linux/wakelock.h>
//&*&*&*BC2_110513:add the wifi suspend wakelock to avoid wifi or system crash
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#define CONFIG_RFKILL_GPS_RESET
#define CONFIG_RFKILL_GPS_STANDBY

// Ellis+
#ifdef CONFIG_RFKILL_WIFI
//&*&*&*BC1_110513:add the wifi suspend wakelock to avoid wifi or system crash
static struct wake_lock wifi_lock;
//&*&*&*BC2_110513:add the wifi suspend wakelock to avoid wifi or system crash
static int toggled_on_wifi = 0;
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
struct early_suspend rfkill_early_suspend;
int rfkill_suspend_state=0;		//0: none; 1: suspend; 2:resume; 
#endif
	
static int fxn_rfkill_set_power_wifi(void *data, enum rfkill_state state)
{
	int _gpio = (int)data;
	// printk(">>> fxn_rfkill_set_power_bt, GPIO: %d, state: %d\n", nshutdown_gpio, state);
	//printk(">>> fxn_rfkill_set_power_wifi\n");
	
	//printk("  toggled_on_wifi to %d\n", (int)state);
        toggled_on_wifi = (int)state;
        
	switch (state) {
	case RFKILL_STATE_SOFT_BLOCKED:  // 0
	      //printk("  WL_RST_N to low\n");
           gpio_direction_output(_gpio, 0);
		break;
		
	case RFKILL_STATE_UNBLOCKED:  // 1
	      //printk("  WL_RST_N to high\n");
           gpio_direction_output(_gpio, 1);
		break;
		
	case RFKILL_STATE_HARD_BLOCKED:  // 2
	   //printk("  WL_BT_POW_EN to low\n");
           gpio_direction_output(FXN_GPIO_WL_BT_POW_EN, 0);
		break;
		
	default:
		printk(KERN_ERR "invalid rfkill state %d\n", state);
	}
	
	printk("<<< fxn_rfkill_set_power_wifi\n");
	return 0;
}
#endif
	
#ifdef CONFIG_RFKILL_BT
static int toggled_on_bt = 0;
	
static int fxn_rfkill_set_power_bt(void *data, enum rfkill_state state)
{
	// printk(">>> fxn_rfkill_set_power_bt, GPIO: %d, state: %d\n", _gpio, state);
	//printk(">>> fxn_rfkill_set_power_bt\n");
	
	//printk("  toggled_on_bt to %d\n", (int)state);
        toggled_on_bt = (int)state;
	
	switch (state) {
	case RFKILL_STATE_SOFT_BLOCKED:  // 0
	      //printk("  BT_RST_N to low\n");
           gpio_direction_output(FXN_GPIO_BT_RST_N, 0);
      	   mdelay(100);
           
		break;

	case RFKILL_STATE_UNBLOCKED:  // 1
	      //printk("  BT_RST_N to high\n");
           gpio_direction_output(FXN_GPIO_BT_RST_N, 1);
      	   mdelay(100);
           
		break;
		
	case RFKILL_STATE_HARD_BLOCKED:  // 2
	        //printk("  WL_BT_POW_EN to low\n");
           gpio_direction_output(FXN_GPIO_WL_BT_POW_EN, 0);
		break;		
		
	default:
		printk(KERN_ERR "invalid rfkill state %d\n", state);
	}
	
	//printk("<<< fxn_rfkill_set_power_bt\n");
	return 0;
}
#endif
	
#ifdef CONFIG_RFKILL_GPS
static int toggled_on_gps = 0;
	
static int fxn_rfkill_set_gps_reset(void *data, enum rfkill_state state)
{
	//printk(">>> fxn_rfkill_set_reset_gps\n");
	
	printk("  toggled_on_gps to %d\n", (int)state);
        toggled_on_gps = (int)state;
	
	switch (state) {
	case RFKILL_STATE_SOFT_BLOCKED:  // 0
           printk("  GPS_RST 0 \n");
	    gpio_direction_output(FXN_GPIO_GPS_RST, 0);
	    msleep(20);
	    break;
		
	case RFKILL_STATE_UNBLOCKED:  // 1
            printk("  GPS_ON_OFF 0 -> 1\n");
  	     gpio_direction_output(FXN_GPIO_GPS_RST, 0);
	     msleep(20);
	     gpio_direction_output(FXN_GPIO_GPS_RST, 1);
	     break;
		
	default:
		printk(KERN_ERR "invalid rfkill state %d\n", state);
	}
	
	printk("<<< fxn_rfkill_set_reset_gps\n");
	return 0;
}


static int fxn_rfkill_set_gps_standby(void *data, enum rfkill_state state)
{
	printk(">>> fxn_rfkill_set_standby_gps\n");
	
	printk("  toggled_on_gps to %d\n", (int)state);
        toggled_on_gps = (int)state;
	
	switch (state) {
	case RFKILL_STATE_SOFT_BLOCKED:  // 0
            printk("  GPS_ON_OFF 0 \n");
	     gpio_direction_output(FXN_GPIO_GPS_ON_OFF, 0);
	     msleep(20);
	     break;
		
	case RFKILL_STATE_UNBLOCKED:  // 1
            printk("  GPS_ON_OFF 1 \n");
	     gpio_direction_output(FXN_GPIO_GPS_ON_OFF, 1);
	     msleep(20);
	     break;
		
	default:
		printk(KERN_ERR "invalid rfkill state %d\n", state);
	}
	
	printk("<<< fxn_rfkill_set_standby_gps\n");
	return 0;
}

#endif
// Ellis-

static int fxn_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct fxn_rfkill_platform_data *pdata = pdev->dev.platform_data;
	enum rfkill_state default_state = RFKILL_STATE_UNBLOCKED;

	//printk(">>> fxn_rfkill_probe\n");
	
	// WiFi
#ifdef CONFIG_RFKILL_WIFI
	printk("<RFKILL_WIFI>\n");
	
	if (pdata->wifi_reset_gpio >= 0) {
        	rfkill_set_default(RFKILL_TYPE_WLAN, RFKILL_STATE_UNBLOCKED);
		
		pdata->rfkill[FXN_WIFI] = rfkill_allocate(&pdev->dev, RFKILL_TYPE_WLAN);
		if (unlikely(!pdata->rfkill[FXN_WIFI]))
		{
			return -ENOMEM;
		}
		
		// printk("  WL_RST_N GPIO %d to HIGH\n", (void *)pdata->wifi_reset_gpio);
		fxn_rfkill_set_power_wifi((void *)pdata->wifi_reset_gpio, RFKILL_STATE_UNBLOCKED);
           
		pdata->rfkill[FXN_WIFI]->name = "fxn_wifi";
		pdata->rfkill[FXN_WIFI]->state = default_state;
		/* userspace cannot take exclusive control */
		pdata->rfkill[FXN_WIFI]->user_claim_unsupported = 1;
		pdata->rfkill[FXN_WIFI]->user_claim = 0;
		pdata->rfkill[FXN_WIFI]->data = (void *)pdata->wifi_reset_gpio;
		pdata->rfkill[FXN_WIFI]->toggle_radio = fxn_rfkill_set_power_wifi;

		rc = rfkill_register(pdata->rfkill[FXN_WIFI]);
		if (unlikely(rc)) {
			rfkill_free(pdata->rfkill[FXN_WIFI]);
			return rc;
		}
	}
//&*&*&*BC1_110513:add the wifi suspend wakelock to avoid wifi or system crash
	wake_lock_init(&wifi_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");
//&*&*&*BC2_110513:add the wifi suspend wakelock to avoid wifi or system crash
#endif
	
	// Bluetooth
#ifdef CONFIG_RFKILL_BT
	printk("<RFKILL_BT>\n");
	
	if (pdata->bt_reset_gpio >= 0) {
		rfkill_set_default(RFKILL_TYPE_BLUETOOTH, RFKILL_STATE_SOFT_BLOCKED);
		
		// printk("  BT_RST_N to LOW\n");
		fxn_rfkill_set_power_bt((void *)pdata->bt_reset_gpio, RFKILL_STATE_SOFT_BLOCKED);
		
		pdata->rfkill[FXN_BT] = rfkill_allocate(&pdev->dev, RFKILL_TYPE_BLUETOOTH);
		if (unlikely(!pdata->rfkill[FXN_BT]))
		{
			return -ENOMEM;
		}
           
		pdata->rfkill[FXN_BT]->name = "fxn_bt";
		pdata->rfkill[FXN_BT]->state = default_state;
		/* userspace cannot take exclusive control */
		pdata->rfkill[FXN_BT]->user_claim_unsupported = 1;
		pdata->rfkill[FXN_BT]->user_claim = 0;
		pdata->rfkill[FXN_BT]->data = (void *)pdata->bt_reset_gpio;
		pdata->rfkill[FXN_BT]->toggle_radio = fxn_rfkill_set_power_bt;

		rc = rfkill_register(pdata->rfkill[FXN_BT]);
		if (unlikely(rc)) {
			rfkill_free(pdata->rfkill[FXN_BT]);
			return rc;
		}
	}
#endif
	
	// GPS
#ifdef CONFIG_RFKILL_GPS
	printk("<RFKILL_GPS>\n");
	
	rc = 0;
	
		
	#ifdef FXN_GPIO_GPS_PWR_EN
	 if (pdata->gps_pwr_en_gpio >= 0) 
       // GPS_PWR_EN
           printk("  GPS_PWR_EN to high\n");
           gpio_direction_output(FXN_GPIO_GPS_PWR_EN, 1);
	    udelay(100);
	#endif
	
	#ifdef CONFIG_UART_SW	
	     printk("  UART1_SW to low\n");
	     gpio_direction_output(FXN_GPIO_UART1_SW, 0);
             msleep(20);
	#endif
	 // GPS_RST
             printk("  GPS_RST to high\n");
	     gpio_direction_output(FXN_GPIO_GPS_RST, 1);
	     msleep(20);
	     
      	// GPS_ON_OFF
            printk("  GPS_ON_OFF to low\n");
	     gpio_direction_output(FXN_GPIO_GPS_ON_OFF, 0);



	#ifdef CONFIG_RFKILL_GPS_RESET

	if (pdata->gps_reset_gpio >= 0) {
	    rfkill_set_default(RFKILL_TYPE_GPS_RESET, RFKILL_STATE_UNBLOCKED);
		 
	     
		pdata->rfkill[FXN_GPS_RESET] = rfkill_allocate(&pdev->dev, RFKILL_TYPE_GPS_RESET);
		if (unlikely(!pdata->rfkill[FXN_GPS_RESET]))
		{
			return -ENOMEM;
		}
		
	     // set BLOCKED as default
		fxn_rfkill_set_gps_reset((void *)pdata->gps_reset_gpio, RFKILL_STATE_SOFT_BLOCKED);
           
		pdata->rfkill[FXN_GPS_RESET]->name = "fxn_gps_reset";
		pdata->rfkill[FXN_GPS_RESET]->state = default_state;
		/* userspace cannot take exclusive control */
		pdata->rfkill[FXN_GPS_RESET]->user_claim_unsupported = 1;
		pdata->rfkill[FXN_GPS_RESET]->user_claim = 0;
		pdata->rfkill[FXN_GPS_RESET]->data = (void *)pdata->gps_reset_gpio;
		pdata->rfkill[FXN_GPS_RESET]->toggle_radio = fxn_rfkill_set_gps_reset;

		rc = rfkill_register(pdata->rfkill[FXN_GPS_RESET]);
		if (unlikely(rc)) {
			rfkill_free(pdata->rfkill[FXN_GPS_RESET]);
			return rc;
		}
	}
	#endif

	#ifdef CONFIG_RFKILL_GPS_STANDBY

	if (pdata->gps_reset_gpio >= 0) {
	    rfkill_set_default(RFKILL_TYPE_GPS_RESET, RFKILL_STATE_UNBLOCKED);
		 
	     
		pdata->rfkill[FXN_GPS_STANDBY] = rfkill_allocate(&pdev->dev, RFKILL_TYPE_GPS_STANDBY);
		if (unlikely(!pdata->rfkill[FXN_GPS_STANDBY]))
		{
			return -ENOMEM;
		}
		
	     // set BLOCKED as default
		fxn_rfkill_set_gps_standby((void *)pdata->gps_on_off_gpio, RFKILL_STATE_SOFT_BLOCKED);
           
		pdata->rfkill[FXN_GPS_STANDBY]->name = "fxn_gps_standby";
		pdata->rfkill[FXN_GPS_STANDBY]->state = default_state;
		/* userspace cannot take exclusive control */
		pdata->rfkill[FXN_GPS_STANDBY]->user_claim_unsupported = 1;
		pdata->rfkill[FXN_GPS_STANDBY]->user_claim = 0;
		pdata->rfkill[FXN_GPS_STANDBY]->data = (void *)pdata->gps_on_off_gpio;
		pdata->rfkill[FXN_GPS_STANDBY]->toggle_radio = fxn_rfkill_set_gps_standby;

		rc = rfkill_register(pdata->rfkill[FXN_GPS_STANDBY]);
		if (unlikely(rc)) {
			rfkill_free(pdata->rfkill[FXN_GPS_STANDBY]);
			return rc;
		}
	}
	#endif
#endif
	
	//printk("<<< fxn_rfkill_probe\n");
	return 0;
}

static int fxn_rfkill_remove(struct platform_device *pdev)
{
	struct fxn_rfkill_platform_data *pdata = pdev->dev.platform_data;
	
	//printk(">>> fxn_rfkill_remove\n");

#ifdef CONFIG_RFKILL_WIFI
	if (pdata->wifi_reset_gpio >= 0) {
		rfkill_unregister(pdata->rfkill[FXN_WIFI]);
		rfkill_free(pdata->rfkill[FXN_WIFI]);
		gpio_free(pdata->wifi_reset_gpio);
	}
//&*&*&*BC1_110513:add the wifi suspend wakelock to avoid wifi or system crash
	wake_lock_destroy(&wifi_lock);
//&*&*&*BC2_110513:add the wifi suspend wakelock to avoid wifi or system crash	
#endif
	
	// BT
#ifdef CONFIG_RFKILL_BT
	printk("<RFKILL_BT>\n");
	
	if (pdata->bt_reset_gpio >= 0) {
		rfkill_unregister(pdata->rfkill[FXN_BT]);
		rfkill_free(pdata->rfkill[FXN_BT]);
		gpio_free(pdata->bt_reset_gpio);
	}
#endif
	
	// GPS
#ifdef CONFIG_RFKILL_GPS
	printk("<RFKILL_GPS>\n");
	
	if (pdata->gps_pwr_en_gpio >= 0) {
		rfkill_unregister(pdata->rfkill[FXN_GPS_RESET]);
		rfkill_unregister(pdata->rfkill[FXN_GPS_STANDBY]);
		rfkill_free(pdata->rfkill[FXN_GPS_RESET]);
		rfkill_free(pdata->rfkill[FXN_GPS_STANDBY]);
		gpio_free(pdata->gps_reset_gpio);
		gpio_free(pdata->gps_on_off_gpio);
	}
#endif
	
	//printk("<<< fxn_rfkill_remove\n");
	return 0;
}

#ifdef CONFIG_PM
static int fxn_rfkill_suspend(struct platform_device  *pdev, pm_message_t state)
{
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
    if (rfkill_suspend_state == 1)	//suspend
		return 0;
#endif
    printk(">>> fxn_rfkill_suspend, state.event= %d\n", state.event);
	gpio_direction_output(FXN_GPIO_GPS_RST, 0);
	     
#ifdef CONFIG_RFKILL_WIFI
    if (toggled_on_wifi == 1)
    {
	      printk("  WL_RST_N to low\n");
           gpio_direction_output(FXN_GPIO_WL_RST_N, 0);
    }
#endif
    
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
    rfkill_suspend_state = 1;
#endif
    //printk("<<< fxn_rfkill_suspend\n");
    return 0;
}

/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
int dhd_fxn_rfkill_suspend(void)
{
    if (rfkill_suspend_state == 1)	//suspend
		return 0;
	     
#ifdef CONFIG_RFKILL_WIFI
    if (toggled_on_wifi == 1)
    {
	      printk("  WL_RST_N to low\n");
           gpio_direction_output(FXN_GPIO_WL_RST_N, 0);
    }
#endif
    
    rfkill_suspend_state = 1;
    //printk("<<< fxn_rfkill_suspend\n");
    return 0;
}
EXPORT_SYMBOL_GPL(dhd_fxn_rfkill_suspend);

int dhd_fxn_rfkill_resume(void)
{
    
#ifdef CONFIG_RFKILL_WIFI
    if (toggled_on_wifi == 1)
    {
	      printk("  WL_RST_N to high\n");
           gpio_direction_output(FXN_GPIO_WL_RST_N, 1);
    }
#endif
    
    rfkill_suspend_state = 2;
    //printk("<<< fxn_rfkill_resume\n");
    return 0;
}
EXPORT_SYMBOL_GPL(dhd_fxn_rfkill_resume);
#endif
static int fxn_rfkill_resume(struct platform_device  *pdev)
{
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
       if (rfkill_suspend_state == 2)	//resume
		return 0;	
#endif
    //printk(">>> fxn_rfkill_resume\n");
	gpio_direction_output(FXN_GPIO_GPS_RST, 0);
	msleep(20);
	gpio_direction_output(FXN_GPIO_GPS_RST, 1);
    
#ifdef CONFIG_RFKILL_WIFI
    if (toggled_on_wifi == 1)
    {
	      printk("  WL_RST_N to high\n");
           gpio_direction_output(FXN_GPIO_WL_RST_N, 1);
//&*&*&*BC1_110513:add the wifi suspend wakelock to avoid wifi or system crash		   
	    wake_lock_timeout(&wifi_lock, HZ * 6);   
//&*&*&*BC2_110513:add the wifi suspend wakelock to avoid wifi or system crash		
    }
#endif
    
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
    rfkill_suspend_state = 2;
#endif
    //printk("<<< fxn_rfkill_resume\n");
    return 0;
}
#endif

/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
static void rfkill_earlysuspend(struct early_suspend *h)
{
}

static void rfkill_late_resume(struct early_suspend *h)
{
		dhd_fxn_rfkill_resume();
}
#endif
static struct platform_driver fxn_rfkill_platform_driver = {
	.probe = fxn_rfkill_probe,
	.remove = fxn_rfkill_remove,
#ifdef CONFIG_PM
	.suspend	= fxn_rfkill_suspend,
	.resume		= fxn_rfkill_resume,
#endif
	.driver = {
		   .name = "rfkill-fxn",
		   .owner = THIS_MODULE,
		   },
};

static int __init fxn_rfkill_init(void)
{
	printk("fxn_rfkill_init ENTER\n");
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
	rfkill_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	rfkill_early_suspend.suspend = rfkill_earlysuspend;
	rfkill_early_suspend.resume = rfkill_late_resume;
	register_early_suspend(&rfkill_early_suspend);
#endif	
	return platform_driver_register(&fxn_rfkill_platform_driver);
}

static void __exit fxn_rfkill_exit(void)
{
/* <-- LH_SWRD_CL1_Henry@2011.8.21 Add rfkill early_suspend for adaptor plug-in  */	
#ifdef CONFIG_WIRELESS_EARLYSUSPEND
	unregister_early_suspend(&rfkill_early_suspend);
#endif
	//printk("fxn_rfkill_exit ENTER\n");
	platform_driver_unregister(&fxn_rfkill_platform_driver);
}

module_init(fxn_rfkill_init);
module_exit(fxn_rfkill_exit);

MODULE_ALIAS("platform: TI OMAP 3622");
MODULE_DESCRIPTION("rfkill-fxn");
MODULE_LICENSE("GPL");
