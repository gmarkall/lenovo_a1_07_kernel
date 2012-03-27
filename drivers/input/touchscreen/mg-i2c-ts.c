/* Morgan capacivite multi-touch device driver.
 *
 * Copyright(c) 2010 MorganTouch Inc.
 *
 * Author: Matt Hsu <matt@0xlab.org>
 *
 */

//#define DEBUG
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
//<-- LH_SWRD_Richard@20110520  add the head
#include <linux/timer.h>
#include <plat/gpio.h>
#include <linux/gpio.h>
#include <plat/mux.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/uaccess.h>


#include "mg-i2c-ts.h"
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>


#define IOCTL_ENDING    		0xD0
#define IOCTL_I2C_SLAVE			0xD1
#define IOCTL_READ_ID		  	0xD2
#define IOCTL_READ_VERSION  		0xD3
#define IOCTL_RESET  			0xD4
#define IOCTL_IAP_MODE			0xD5
#define IOCTL_CALIBRATION		0xD6
#define IOCTL_ACTION2			0xD7
#define IOCTL_DEFAULT			0x88

static int command_flag= 0;


#define MG_DRIVER_NAME 	"mg-i2c-mtouch"
#define BUF_SIZE 		15
//#define RANDY_DEBUG
static int home_flag = 0;
static int menu_flag = 0;
static int back_flag = 0;
u8 ver_buf[5]={0};
int ver_flag = 0;
int  id_flag = 0;
#define COORD_INTERPRET(MSB_BYTE, LSB_BYTE) \
		(MSB_BYTE << 8 | LSB_BYTE)

struct mg_data{
	__u16 	x, y, w, p, id;
	struct i2c_client *client;
	/* capacivite device*/
	struct input_dev *dev;
	/* digitizer */

	struct timer_list timer;

	struct input_dev *dig_dev;
	struct mutex lock;
	int irq;
	struct work_struct work;

struct early_suspend early_suspend;

	struct workqueue_struct *mg_wq;
	int (*power)(int on);
	int intr_gpio;
	int fw_ver;
    struct miscdevice firmware;
	#define IAP_MODE_ENABLE		1	/* TS is in IAP mode already */
	int iap_mode;		/* Firmware update mode or 0 for normal */

};

/* <-- LH_SWRD_CL1_Henry@2011.8.20 Add delayed work in touch initialization  */	
	struct delayed_work touch_init_work;	
	int init_work_flag=0;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mg_early_suspend(struct early_suspend *h);
static void mg_late_resume(struct early_suspend *h);
#endif
/*<-- LH_SWRD_CL1_richard@20110802   add to switch AC and DC mode of touch */
	static u_int8_t touch_list[3][5]={
					{ 0x81, 0x02, 0x00, 0x00, 0x00},
					{ 0x81, 0x02, 0x01, 0x00, 0x00},
					{ 0x97, 0x00, 0x00, 0x00, 0x00},
					
					};
	int ac_sw = 1;
	int dc_sw = 1;
/*LH_SWRD_CL1_richard@20110802   add to switch AC and DC mode of touch -->*/
/*<-- LH_SWRD_CL1_richard@20110822*/
static int ack_flag = 0;
static u8 read_buf[BUF_SIZE]={0};
/*LH_SWRD_CL1_richard@20110822 -->*/
static struct mg_data *private_ts;
int work_finish_yet=1;
int no_suspend_resume=0;
int irq_enabled_flag=0;
//int work_well_flag=0;
//int work_well_done=1;
int  set_tg_irq_status(unsigned int irq, bool enable);

int mg_iap_open(struct inode *inode, struct file *filp)
{ 

	struct mg_data *dev;

	#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_open\n");
	#endif
	dev = kmalloc(sizeof(struct mg_data), GFP_KERNEL);
	if (dev == NULL) {
		return -ENOMEM;
	}

	filp->private_data = dev;


		
	return 0;
}

int mg_iap_release(struct inode *inode, struct file *filp)
{    
	printk("[Driver]into mg_iap_release\n");
	
	struct mg_data *dev = filp->private_data;
	if (dev)
	{
		kfree(dev);
	}
	printk("[Driver]into mg_iap_release OVER\n");

	return 0;
}
ssize_t mg_iap_write(struct file *filp, const char *buff,    size_t count, loff_t *offp)
{  

    int ret;
    char *tmp;
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_write\n");
	#endif

 	if (count > 128)
       	count = 128;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;

    if (copy_from_user(tmp, buff, count)) {
        return -EFAULT;
    }
	#ifdef RANDY_DEBUG	
	int i = 0;
	printk("[Driver]Writing : ");
	for(i = 0; i < count; i++)
	{
		printk("%4x", tmp[i]);
	}
	printk("\n");
	#endif

	ret = i2c_master_send(private_ts->client, tmp, count);
    	if (!ret) 
		printk("[Driver] i2c_master_send fail, ret=%d \n", ret);
	

    	kfree(tmp);
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_write OVER\n");
	#endif
    return ret;

}

ssize_t mg_iap_read(struct file *filp, const char *buff,    size_t count, loff_t *offp){    

    char *tmp;
    int ret;  
    
	#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_read\n");
	#endif
    	if (count > 128)
        	count = 128;
/*<-- LH_SWRD_CL1_richard@20110822 add the read function for ap reading*/
	if(command_flag == 1)
	{
		printk("<<Waiting>>");
		return -1;
	}
	else
	{
		if(ack_flag == 1)
		{
			copy_to_user(buff, read_buf, 5);
				
			#ifdef RANDY_DEBUG
			int i = 0;
			printk("[Driver]Reading : ");
			for(i = 0; i < 5; i++)
			{
				printk("%4x", read_buf[i]);
			}
			printk("\n");
			#endif
			ack_flag = 0;
			return 5;
		}
	}
/*LH_SWRD_CL1_richard@20110822 add the read function for ap reading-->*/

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;

	ret = i2c_master_recv(private_ts->client, tmp, count);
    	if (ret >= 0)
        	copy_to_user(buff, tmp, count);
    
	#ifdef RANDY_DEBUG	
	int i = 0;
	printk("[Driver]Reading : ");
	for(i = 0; i < count; i++)
	{
		printk("%4x", tmp[i]);
	}
	printk("\n");
	#endif
    kfree(tmp);
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_read OVER\n");
	#endif

    return ret;
}
int mg_iap_ioctl(struct inode *inode, struct file *filp,    unsigned int cmd, unsigned long arg)
{
	u_int8_t ret = 0;

	command_flag = 1;

	#ifdef RANDY_DEBUG	
	printk("[Driver ]mg_iap_ioctl  Cmd =%x\n", cmd);

	#endif

	switch (cmd) {        
	  
		case IOCTL_DEFAULT:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_DEFAULT  \n");
			#endif
			break;   
			
		case IOCTL_I2C_SLAVE: 
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_I2C_SLAVE  \n");
			#endif
			
			gpio_set_value(TOUCH_RESET,1);
//			printk("  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));
/*<-- LH_SWRD_CL1_richard@20110822 */
			command_flag = 0;
/*LH_SWRD_CL1_richard@20110822 -->*/
			break;   

		case IOCTL_RESET:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_RESET  \n");
				#endif
			
		gpio_set_value(TOUCH_RESET,0);
//		printk("  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));
/*<-- LH_SWRD_CL1_richard@20110822 */
			command_flag = 0;
/*LH_SWRD_CL1_richard@20110822 -->*/
			break;
		case IOCTL_READ_ID:        
		#ifdef RANDY_DEBUG
			printk("[Driver ] IOCTL_READ_ID\n");
			#endif
			ret = i2c_master_send(private_ts->client, 
								command_list[3], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_READ_ID error!!!!!\n");
				return -1;
			}

			break;    
		case IOCTL_READ_VERSION:    
			#ifdef RANDY_DEBUG    
			printk("[Driver ] IOCTL_READ_VERSION\n");
			#endif
			ret = i2c_master_send(private_ts->client, 
								command_list[4], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
				return -1;
			}

			break;    
			
		case IOCTL_ENDING:        
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_ENDING\n");
			#endif
			

			ret = i2c_master_send(private_ts->client, 
								command_list[13], COMMAND_BIT);
			if(ret < 0)
			{
				printk("[Driver ]IOCTL_ENDING error!!!!!\n");
				return -1;
			}
/*<-- LH_SWRD_CL1_richard@20110822 */
			command_flag = 0;
/*LH_SWRD_CL1_richard@20110822 -->*/
			break;        
		case IOCTL_ACTION2:        
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_ACTION2\n");
			#endif

			ret = i2c_master_send(private_ts->client, 
								command_list[12], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_ACTION2 error!!!!!\n");
				return -1;
			}

			break;        
		case IOCTL_IAP_MODE:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_IAP_MODE\n");
			#endif
			
			ret = i2c_smbus_write_i2c_block_data(private_ts->client,
									0, COMMAND_BIT, command_list[10]);
			if(ret < 0)
			{
				printk("[Driver ]IOCTL_IAP_MODE error!!!!!\n");
				return -1;
			}

			break;
		case IOCTL_CALIBRATION:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_CALIBRATION1\n");
			#endif

			ret = i2c_smbus_write_i2c_block_data(private_ts->client,
									0, COMMAND_BIT, command_list[0]);
			if(ret < 0)
			{
				printk("[Driver ]Calibrate  Write error!!!!!\n");
				return -1;
			}

			break;
		default:            
			#ifdef RANDY_DEBUG	
			printk("[Driver ] default  \n");
			#endif
			break;   
	}     

	return 0;
}


struct file_operations mg_touch_fops = {    
        open:       mg_iap_open,    
        write:      mg_iap_write,    
        read:	    mg_iap_read,    
        release:    mg_iap_release,    
        ioctl:      mg_iap_ioctl,  
 };
 
 


static ssize_t mg_fw_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int err;
/*<-- LH_SWRD_CL1_richard@20110817  change the follow of reading the touch firmware*/
  	#ifdef RANDY_DEBUG
	printk("[Driver ] IOCTL_READ_VERSION\n");
	printk("[Driver ]DISABLE !!!!!!!\n");
	#endif
	err = i2c_master_send(private_ts->client, 
						command_list[2], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]DISABLE  Error !!!!!!!\n");
		return -1;
	}
	#ifdef RANDY_DEBUG
	printk("[Driver ]ENABLE !!!!!!!\n");
	#endif
	err = i2c_master_send(private_ts->client, 
						command_list[1], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]ENABLE  Error !!!!!!!\n");
		return -1;
	}
	mdelay(50);
	ver_flag = 1;
	command_flag = 1;
	err = i2c_master_send(private_ts->client, 
						command_list[4], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
		return -1;
	}	
	
	mdelay(100);

	//return sprintf(buf,"%x,%x,%x,%x,%x\n",  ver_buf[0], ver_buf[1],ver_buf[2],ver_buf[3],ver_buf[4]);
	if (ver_flag == 0)
		return sprintf(buf,"%x.%x%c \n",  ver_buf[3], ver_buf[4], ver_buf[2]);
	else
	{
		return sprintf(buf,"couldn't read firmware version \n");
	}
/*LH_SWRD_CL1_richard@20110817  change the follow of reading the touch firmware -->*/
}

/* <-- LH_SWRD_CL1_Henry@2011.8.20 Add delayed work in touch initialization  */	
static int mg_try_resume(struct i2c_client *client);
static int mg_resume(struct i2c_client *client);
void delayed_work_start(void)
{
		cancel_delayed_work(&touch_init_work);	
		schedule_delayed_work(&touch_init_work, msecs_to_jiffies(8000));
}
static void touch_init_once(struct work_struct *data)
{
       if (init_work_flag == 0)
      	{
       	init_work_flag = 1;
	mg_try_resume(private_ts->client);	
      	}   
}

/* <-- LH_SWRD_CL1_Henry@2011.8.5 check whether touch work well or not  */	
static irqreturn_t mg_irq(int irq, void *_mg);
static ssize_t mg_work_well(struct i2c_client *client)
{	
#if 0
	int err;

	err = i2c_master_send(private_ts->client, 
					command_list[4], COMMAND_BIT);

	if(err < 0)
		{
			printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
			return -1;
		}
	ver_flag = 1;
	
	command_flag = 1;
	int counter=0;
       while (ver_flag == 1)
       	{       		
		msleep(10);
		counter++;
		if (counter > 30)
			break;
       	}

	if (ver_flag == 0)
		{
		if ((ver_buf[0][0] == 0xde) && (ver_buf[0][1] == 0xee))
			{
			return 1;
			}
		else
			{
			free_irq(private_ts->irq, private_ts);
			request_irq(private_ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, private_ts);					
			return -1;			
			}
		}
	else
		{
		free_irq(private_ts->irq, private_ts);
		request_irq(private_ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, private_ts);					
		ver_flag = 0;
		command_flag = 0;
		printk("couldn't read firmware version\n");
		return -1;
		}	
#endif
}
/* LH_SWRD_CL1_Henry@2011.8.5 check whether touch work well or not  -->*/	


#if 0
static ssize_t mg_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	
  printk("[Touch]:(%s) touch_ID_version = %s \n",__FUNCTION__, ver_buf[1]);
	
	return sprintf(buf, "%x, %x, %x, %x, %x\n", ver_buf[1][0], ver_buf[1][1], ver_buf[1][2], ver_buf[1][3], ver_buf[1][4]);
}
#endif
static DEVICE_ATTR(firmware_version, S_IRUGO|S_IWUSR|S_IWGRP, mg_fw_show, NULL);
//static DEVICE_ATTR(id_version, S_IRUGO|S_IWUSR|S_IWGRP, mg_id_show, NULL);

static struct attribute *mg_attributes[] = {
	&dev_attr_firmware_version.attr,
//	&dev_attr_id_version.attr,
	NULL
};

static const struct attribute_group mg_attr_group = {
	.attrs = mg_attributes,
};


static void time_led_off(unsigned long _data)
{
//	printk("=******RANDY stfu pls*******\n");
	light_touch_LED(TOUCH_LED, LED_OFF);
}

static struct i2c_driver mg_driver;

/*<-- LH_SWRD_CL1_richard@20110802   add to switch AC and DC mode of touch */
static void mg_ac_dc_sw(struct mg_data *mg)
{
	int ret=0;
	int switch_ad = 1;
	switch_ad = gpio_get_value(51);
	//if( ( touch_ac_dc == 3 ) || (touch_ac_dc == 1) || (touch_ac_dc == 2))
	if( !switch_ad )
	{
		
		dc_sw = 1;
		if(ac_sw == 1)
		{
			printk("mg_ad_dc mode  gpio_get_value =  %d\n", switch_ad);
			command_flag = 1;
			//mdelay(10);
			//disable_irq_nosync(mg->client->irq);
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[1]);
			if (ret < 0)
			{
				 printk("mg get the AC mode fail!\n"); 
			}
			else
			{
				 printk("mg get the AC mode success!\n"); 
			}
		
			msleep(30);
			/*<-- LH_SWRD_CL1_richard@20110822 modify the ack_flag*/
			ack_flag = 0;
			/*LH_SWRD_CL1_richard@20110822 modify the ack_flag-->*/
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if (ret < 0)
			{
				 printk("mg report message again fail!\n"); 
			}
			else
			{
				printk("mg report message again success!\n");
			}
		
			ac_sw = 0;
			//set_tg_irq_status(mg->client->irq, true);
		
		}
	}
	else
	{
	
		ac_sw = 1;
		if(dc_sw == 1)
		{
			command_flag = 1;
			printk("mg_ad_dc mode  gpio_get_value =  %d\n", switch_ad);
			//mdelay(10);
			//disable_irq_nosync(mg->client->irq);
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[0]);
			if (ret < 0)
			{
				 printk("mg get the DC mode fail!\n"); 
			}
			else
			{
				printk("mg get the DC mode success!\n");
			}
		
			msleep(30);
			/*<-- LH_SWRD_CL1_richard@20110822 modify the ack_flag*/
			ack_flag = 0;
			/*LH_SWRD_CL1_richard@20110822 modify the ack_flag-->*/
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if (ret < 0)
			{
				 printk("mg report message again fail!\n"); 
			}
			else
			{
				printk("mg report message again success!\n");
			}
		
			dc_sw = 0;
			//set_tg_irq_status(mg->client->irq, true);
		
		}
	}
		
}

/*LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch -->*/

static irqreturn_t mg_irq(int irq, void *_mg)
{
	struct mg_data *mg = _mg;

      	init_work_flag = 1;
	if (work_finish_yet == 0)
		return IRQ_HANDLED;
	else
		work_finish_yet=0;
	//if (work_well_done == 0)
	//	return IRQ_HANDLED;	
	disable_irq_nosync(irq);
       irq_enabled_flag = 0;
	schedule_work(&mg->work);
	return IRQ_HANDLED;
}

static inline void mg_mtreport(struct mg_data *mg)
{
if((mg->x > CAP_Y_CORD)||(mg->y > CAP_X_CORD))	
{
	#ifdef RANDY_DEBUG	
		printk("Reporting  the X and Y Value is error!" ); 

	#endif
	input_report_abs(mg->dev, ABS_MT_TRACKING_ID, 0);
	input_report_abs(mg->dev, ABS_MT_TOUCH_MAJOR, 0);
	input_report_abs(mg->dev, ABS_MT_WIDTH_MAJOR, 0);
	input_report_abs(mg->dev, ABS_MT_POSITION_X, 0 );
	input_report_abs(mg->dev, ABS_MT_POSITION_Y, 0 );
	input_mt_sync(mg->dev);
}
else
{


	input_report_abs(mg->dev, ABS_MT_TRACKING_ID, mg->id);
	input_report_abs(mg->dev, ABS_MT_TOUCH_MAJOR, mg->w);
	input_report_abs(mg->dev, ABS_MT_WIDTH_MAJOR, 0);
	input_report_abs(mg->dev, ABS_MT_POSITION_X, (mg->x ) );
	input_report_abs(mg->dev, ABS_MT_POSITION_Y, (mg->y ) );
	input_mt_sync(mg->dev);
}

}

static void mg_i2c_work(struct work_struct *work)
{
	int i = 0;
	struct mg_data *mg =
			container_of(work, struct mg_data, work);
	u_int8_t ret = 0;
//	u8 read_buf[BUF_SIZE];	/* buffer for first point of multitouch */

	struct i2c_client *client=mg->client;
	struct mg_data *ts = i2c_get_clientdata(client);

	for(i = 0; i < BUF_SIZE; i ++)	
			read_buf[i] = 0;


	mutex_lock(&mg->lock);
if(command_flag == 1)
{
		ret = i2c_master_recv(mg->client, read_buf, 5);
		if(ret < 0)
		{
			printk("Read error!!!!!\n");
		mutex_unlock(&mg->lock);
		if (irq_enabled_flag == 0)
			{
			set_tg_irq_status(client->irq, true);			
		       irq_enabled_flag = 1;			
			}
		work_finish_yet=1;
			return;
		}
		#ifdef RANDY_DEBUG	
		printk("\nRecieve Command ACK : "); 
		for(i = 0; i < 5; i ++) 	
			printk("%4x", read_buf[i]);
		printk("\n");
		#endif

////////version ++
//
/*<-- LH_SWRD_CL1_richard@20110822 change to read fw-*/	
		if(ver_flag == 1)
		{
//			printk("[mg-capacitive] ::   read fw start !\n "); 
			for(i = 0; i < 5; i ++)
				ver_buf[i] = read_buf[i];
			
			mdelay(20);
			ret =i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if(ret<0)
			{
				printk("Wistron==>show f/w!!\n");
				mutex_unlock(&mg->lock);
				if (irq_enabled_flag == 0)
					{
					set_tg_irq_status(client->irq, true);			
				       irq_enabled_flag = 1;			
					}
				work_finish_yet=1;
				
				return;
			}
		
		ver_flag = 0;
//		printk("[mg-capacitive] ::   read fw endding !\n "); 
		}
/*LH_SWRD_CL1_richard@20110822 change to read fw-->*/	
////////version --
	/*<-- LH_SWRD_CL1_richard@20110822 change the flag of ack*/
			ack_flag = 1;
	/*LH_SWRD_CL1_richard@20110822 change the flag of ack-->*/
		command_flag = 0;
		mutex_unlock(&mg->lock);
		if (irq_enabled_flag == 0)
			{
			set_tg_irq_status(client->irq, true);
		       irq_enabled_flag = 1;			
			}
		work_finish_yet=1;
		return;
	}

	ret = i2c_smbus_read_i2c_block_data(mg->client,
							0x0, BUF_SIZE, read_buf);
	if(ret < 0)
	{
		printk("Read error!!!!!\n");
		mutex_unlock(&mg->lock);
		if (irq_enabled_flag == 0)
			{
			set_tg_irq_status(client->irq, true);			
		       irq_enabled_flag = 1;			
			}
		work_finish_yet=1;
		return;
	}

			#ifdef RANDY_DEBUG	
				printk("\nRead: "); 
				for(i = 0; i < BUF_SIZE; i ++) 	
					printk("%4x", read_buf[i]);
			#endif

	if(read_buf[MG_MODE]  == 2)
	{
	
		switch(read_buf[TOUCH_KEY_CODE])
		{
		case 1:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
//				light_touch_LED(TOUCH_LED, LED_ON);

			home_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_HOME, 1);
			#ifdef RANDY_DEBUG	
				printk("Home\n"); 
			#endif
                     
			break;
		case 2:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
//				light_touch_LED(TOUCH_LED, LED_ON);

			back_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_BACK, 1);
   	                  #ifdef RANDY_DEBUG	
			printk("back\n"); 
			#endif
			break;
		case 3:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
//				light_touch_LED(TOUCH_LED, LED_ON);

			menu_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_MENU, 1);
                     	#ifdef RANDY_DEBUG	
			printk("menu\n"); 
			#endif
			break;
		case 0:
			if(home_flag == 1)
			{
				home_flag = 0;
				//input_report_key(mg->dig_dev, BTN_TOUCH, 0);
				input_event(mg->dev, EV_KEY, KEY_HOME, 0);
			#ifdef RANDY_DEBUG	
				printk("home0\n"); 
			#endif
			}
			else if(back_flag == 1)
			{
				back_flag = 0;
				input_event(mg->dev, EV_KEY, KEY_BACK, 0);
			#ifdef RANDY_DEBUG	
				printk("back0\n"); 
			#endif
			}
			else
			{
				menu_flag = 0;
				input_event(mg->dev, EV_KEY, KEY_MENU, 0);
			#ifdef RANDY_DEBUG	
				printk("menu0\n"); 
			#endif
			}
			
			#ifdef RANDY_DEBUG	
				printk("BTN_TOUCH00\n"); 
			#endif
			input_report_key(mg->dev, BTN_TOUCH, 0);
			#ifdef RANDY_DEBUG	
				printk("BTN_TOUCH11\n"); 
			#endif
			input_sync(mg->dev);
			#ifdef RANDY_DEBUG	
				printk("sync\n"); 
			#endif
			break;
		default:
			break;
		}
		//mod_timer(&mg->timer, jiffies + msecs_to_jiffies(2000));
	}
	else
	{
		if(read_buf[ACTUAL_TOUCH_POINTS] >= 1)
		{

			mg->y = COORD_INTERPRET(read_buf[MG_POS_X_HI], read_buf[MG_POS_X_LOW]);
			mg->x = COORD_INTERPRET(read_buf[MG_POS_Y_HI], read_buf[MG_POS_Y_LOW]);
			mg->w = read_buf[MG_STATUS];
			mg->id = read_buf[MG_CONTACT_ID];

			mg_mtreport(mg);
		}
		if(read_buf[ACTUAL_TOUCH_POINTS] == 2) 
		{
			mg->y =	COORD_INTERPRET(read_buf[MG_POS_X_HI2], read_buf[MG_POS_X_LOW2]);
			mg->x = COORD_INTERPRET(read_buf[MG_POS_Y_HI2], read_buf[MG_POS_Y_LOW2]);
			mg->w = read_buf[MG_STATUS2];
			mg->id = read_buf[MG_CONTACT_ID2];

			mg_mtreport(mg);
		}
		input_sync(mg->dev);
	}


/*<-- LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch*/

		mg_ac_dc_sw(mg);

/*LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch -->*/
	mutex_unlock(&mg->lock);			
	if (irq_enabled_flag == 0)
			{
			set_tg_irq_status(client->irq, true);			
		       irq_enabled_flag = 1;			
			}
	work_finish_yet=1;


}

static int
mg_probe(struct i2c_client *client, const struct i2c_device_id *ids)
{
	struct mg_data *mg;
	struct input_dev *input_dev;
	int err = 0;

	/* allocate mg data */
	mg = kzalloc(sizeof(struct mg_data), GFP_KERNEL);
	if (!mg)
		return -ENOMEM;

	mg->client = client;
	dev_info(&mg->client->dev, "device probing\n");
	i2c_set_clientdata(client, mg);
	mutex_init(&mg->lock);


      mg->firmware.minor = MISC_DYNAMIC_MINOR;
      mg->firmware.name = "MG_Update";
      mg->firmware.fops = &mg_touch_fops;
      mg->firmware.mode = S_IRWXUGO; 
      if (misc_register(&mg->firmware) < 0)
         printk("[mg]misc_register failed!!");
      else
         printk("[mg]misc_register finished!!"); 

	/* allocate input device for capacitive */
	input_dev = input_allocate_device();

	if (!input_dev) {
		dev_err(&mg->client->dev, "failed to allocate input device \n");
		goto exit_kfree;
	}

	input_dev->name = "mg-capacitive";
//<-- LH_SWRD_Richard@20110523 add the info for morgan touch
	input_dev->phys = "I2C";
	input_dev->id.bustype = BUS_I2C;
	input_dev->id.vendor = 0x0EAF;
	input_dev->id.product = 0x1020;
//<-- LH_SWRD_Richard@20110523 add the info for morgan touch
	mg->dev = input_dev;
	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) |
					BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	set_bit(KEY_BACK, input_dev->keybit);
        set_bit(KEY_MENU, input_dev->keybit);
        set_bit(KEY_HOME, input_dev->keybit);

//&*&*&*JJ1	
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 8, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 8, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, CAP_MAX_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, CAP_MAX_X, 0, 0);
	
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);



//&*&*&*JJ2
	err = input_register_device(input_dev);
	if (err)
		goto exit_input;

	gpio_direction_output(TOUCH_RESET,1);
/* <-- LH_SWRD_CL1_Henry@2011.9.1 reset touch in touch initialization  */	
	gpio_set_value(TOUCH_RESET,0);
	msleep(50);	
	gpio_set_value(TOUCH_RESET,1);
	msleep(50);
	//printk("&*&*&*&*&*&*&*&*&**&*&*&*&*&*&**&*&*&*&*&*&*&*  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));

	err = sysfs_create_group(&client->dev.kobj, &mg_attr_group);
	if (err)
		printk("[Touch](%s)failed to create flash sysfs files\n",__FUNCTION__);

	setup_timer(&mg->timer, time_led_off, (unsigned long)mg);

	INIT_WORK(&mg->work, mg_i2c_work);
/* <-- LH_SWRD_CL1_Henry@2011.8.20 Add delayed work in touch initialization  */	
	INIT_DELAYED_WORK(&touch_init_work, touch_init_once);

	/* request IRQ resouce */
	if (client->irq < 0) {
		dev_err(&mg->client->dev,
			"No irq allocated in client resources!\n");
		goto exit_input;
	}

	mg->irq = client->irq;
	err = request_irq(mg->irq, mg_irq,
		IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, mg);


#ifdef CONFIG_HAS_EARLYSUSPEND
		mg->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		mg->early_suspend.suspend = mg_early_suspend;
		mg->early_suspend.resume = mg_late_resume;
		register_early_suspend(&mg->early_suspend);
#endif
#ifdef RANDY_DEBUG	
				printk("\nregister_early_suspend  OK !!!!\n"); 
#endif

	private_ts = mg;
	//mg_work_well(client);
	//work_well_done = 1;
	irq_enabled_flag=1;	
	set_tg_irq_status(client->irq, true);

/* <-- LH_SWRD_CL1_Henry@2011.8.20 Add delayed work in touch initialization  */	
	delayed_work_start();

	return 0;

exit_input:
	input_unregister_device(mg->dev);
exit_kfree:
	kfree(mg);
	return err;
}

static int __devexit mg_remove(struct i2c_client *client)
{
	struct mg_data *mg = i2c_get_clientdata(client);

	free_irq(mg->irq, mg);
	input_unregister_device(mg->dev);
	del_timer_sync(&mg->timer);//addby richard 20110704
	work_finish_yet = 1;
	cancel_work_sync(&mg->work);
	kfree(mg);
	return 0;
}
static int mg_firstpart_suspend(struct i2c_client *client, pm_message_t state)
{
	struct mg_data *ts = i2c_get_clientdata(client);
	time_led_off((unsigned long)ts);
	return 0;
}
static int mg_suspend(struct i2c_client *client, pm_message_t state)
{
#ifdef RANDY_DEBUG	
printk("\nmg_suspend11111111\n"); 
#endif

	int ret;
	int counter=0;
	struct mg_data *ts = i2c_get_clientdata(client);
	
/* <-- LH_SWRD_CL1_Henry@2011.8.1 touch not enter sleep mode when device couldn't sleep  */	
//	if (__gpio_get_value(51) == 1) {
//		printk("mg_suspend exit as USB exist\n");
//		no_suspend_resume = 1;
//		return;
//	}
	no_suspend_resume = 0;
      	init_work_flag = 1;

	while (work_finish_yet == 0)
		{
		msleep(10);
		counter++;
		if (counter > 20)
			break;
		}
	//if (ts->irq)
	//{
	//	disable_irq(client->irq);			
	//}
/*  LH_SWRD_CL1_Henry@2011.8.1 touch not enter sleep mode when device couldn't sleep -->*/
	//free_irq(ts->irq, ts);
	//work_well_flag = 0;
	set_tg_irq_status(client->irq, false);

	work_finish_yet = 1;
	ret = cancel_work_sync(&ts->work);
	
	/******************    randy add for suspend/ resume*****************/
	int i;
	for (i=0; i < 10; i++)
		{
		ret = i2c_smbus_write_i2c_block_data(ts->client, 0, COMMAND_BIT, command_list[5]);
		if (ret < 0)
			{
			if (i == 9)
				{
				printk(KERN_ERR "mg_suspend: i2c_smbus_write_i2c_block_data failed\n");
				}
			}
			else 
			{
			break;
			}			
		msleep(10);
		}			
	/******************    randy add for suspend/ resume*****************/

	time_led_off((unsigned long)ts);
	del_timer_sync(&ts->timer);
/*<-- LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch*/
	//ac_sw = 1;
	//dc_sw = 1;
/*LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch -->*/	
	return 0;
}

static int mg_resume(struct i2c_client *client)
{
#ifdef RANDY_DEBUG	
printk("\nmg_resume11111111111\n"); 
#endif

	int ret;
	
	struct mg_data *ts = i2c_get_clientdata(client);

/* <-- LH_SWRD_CL1_Henry@2011.8.1 touch not enter sleep mode when device couldn't sleep  */	
	if (no_suspend_resume == 1) {
		//printk("mg_resume exit as wake_lock exist\n");
		return 0;
	}
      	init_work_flag = 1;
/*  LH_SWRD_CL1_Henry@2011.8.1 touch not enter sleep mode when device couldn't sleep -->*/
/* <-- LH_SWRD_CL1_Henry@2011.7.24 make sure touch resume command finished properly*/	

#if 0
	ret = request_irq(ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, ts);
	if (ret < 0)
	{
		printk(KERN_ERR "mg_resume: request_irq  failed\n");
	}
	else
	{
		printk(KERN_ERR "mg_resume: request_irq  success\n");
	}
#endif

	int i;
	for (i=0; i < 10; i++)
		{
			ret = i2c_smbus_write_i2c_block_data(ts->client, 0, 5, command_list[7]);
			if (ret < 0)
				{
				if (i == 9)
					{
					printk(KERN_ERR "mg_resume: i2c_smbus_write_i2c_block_data   failed\n");
					//free_irq(ts->irq, ts);
					//ret = request_irq(ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, ts);					
					//work_well_flag = 0;
					gpio_set_value(TOUCH_RESET,0);					
					mdelay(2);
					gpio_set_value(TOUCH_RESET,1);
					}
				}
			else 
				{
				printk("mg_resume: i2c_smbus_write_i2c_block_data  success i=%d\n",i);
				//work_well_flag = 1;
				break;
				}
/* <-- LH_SWRD_CL1_Henry@2011.9.1 leave a interval in i2c transfer  */	
			msleep(10);
		}		

/* LH_SWRD_CL1_Henry@2011.7.24 make sure touch resume command finished properly --> */	
	//printk("<>><><><><><><><><>mg_resume: setup_timer\n");
	//setup_timer(&ts->timer, time_led_off, (unsigned long)ts);


/*<-- LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch*/
//	printk("<>><><><><><><><><>mg_resume: mg_ac_dc_sw\n");
//		mg_ac_dc_sw(ts);

/*LH_SWRD_CL1_richard@20110802  add to switch AC and DC mode of touch -->*/

	//mg_work_well(client);
	//work_well_done = 1;
       irq_enabled_flag = 1;	
	set_tg_irq_status(client->irq, true);
	return 0;
}
static int mg_try_resume(struct i2c_client *client)
{

	
	int ret;
	
	struct mg_data *ts = i2c_get_clientdata(client);

	
	if (no_suspend_resume == 1) {

		return 0;
	}
      	init_work_flag = 1;
	irq_enabled_flag = 1;	
	set_tg_irq_status(client->irq, true);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mg_early_suspend(struct early_suspend *h)
{
#ifdef RANDY_DEBUG	
	printk("\nmg_early_suspend111111111111\n"); 
#endif

	struct mg_data *ts;
	ts = container_of(h, struct mg_data, early_suspend);
	mg_suspend(ts->client, PMSG_SUSPEND);
	//mg_firstpart_suspend(ts->client, PMSG_SUSPEND);
}

static void mg_late_resume(struct early_suspend *h)
{
#ifdef RANDY_DEBUG	
	printk("\nmg_late_resume111111111111\n"); 
#endif
	struct mg_data *ts;
	ts = container_of(h, struct mg_data, early_suspend);
	mg_resume(ts->client);
}

#else
#define mg_suspend NULL
#define mg_resume NULL
#endif

static struct i2c_device_id mg_id_table[] = {
    /* the slave address is passed by i2c_boardinfo */
    {MG_DRIVER_NAME, },
    {/* end of list */}
};

static struct i2c_driver mg_driver = {
	.driver = {
		.name	 = MG_DRIVER_NAME,

	},
	.id_table 	= mg_id_table,
	.probe 		= mg_probe,
	.remove 	= mg_remove,
	//.suspend = mg_suspend,	
};

static int __init mg_init(void)
{
	return i2c_add_driver(&mg_driver);
}

static void mg_exit(void)
{
	i2c_del_driver(&mg_driver);
}
module_init(mg_init);
module_exit(mg_exit);

MODULE_AUTHOR("Randy pan <panhuangduan@morgan-touch.com>");
