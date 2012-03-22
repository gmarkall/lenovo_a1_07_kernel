/*
  Author: Brian Xu
  Date: 2009/09/14
  File: adxl345_sysfs.c

  Modify: Aimar Liu
  Date: 2009/12/30
  File: adxl345_sysfs.c
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <mach/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */


#include "adxl345.h"


#if 0
	#define ADXL_DEBUG
#else
	#ifdef ADXL_DEBUG
	#undef ADXL_DEBUG
	#endif
#endif

#ifdef ADXL_DEBUG
	#define dprintk(x, args...)	do { printk(x, ##args); } while(0)
#else
	#define dprintk(x, args...)	do { } while(0)
#endif

/* ADXL345/6 Register Map */
#define DEVID		    0x00	/* R   Device ID */
#define THRESH_TAP	0x1D	/* R/W Tap threshold */
#define OFSX		    0x1E	/* R/W X-axis offset */
#define OFSY		    0x1F	/* R/W Y-axis offset */
#define OFSZ		    0x20	/* R/W Z-axis offset */
#define DUR		      0x21	/* R/W Tap duration */
#define LATENT		  0x22	/* R/W Tap latency */
#define WINDOW		  0x23	/* R/W Tap window */
#define THRESH_ACT	  0x24	/* R/W Activity threshold */
#define THRESH_INACT	0x25	/* R/W Inactivity threshold */
#define TIME_INACT   	0x26	/* R/W Inactivity time */
#define ACT_INACT_CTL	0x27	/* R/W Axis enable control for activity and inactivity detection */
#define THRESH_FF	      0x28	/* R/W Free-fall threshold */
#define TIME_FF		      0x29	/* R/W Free-fall time */
#define TAP_AXES	      0x2A	/* R/W Axis control for tap/double tap */
#define ACT_TAP_STATUS	0x2B	/* R   Source of tap/double tap */
#define BW_RATE		  0x2C	/* R/W Data rate and power mode control */
#define POWER_CTL	  0x2D	/* R/W Power saving features control */
#define INT_ENABLE	0x2E	/* R/W Interrupt enable control */
#define INT_MAP		  0x2F	/* R/W Interrupt mapping control */
#define INT_SOURCE	0x30	/* R   Source of interrupts */
#define DATA_FORMAT	0x31	/* R/W Data format control */
#define DATAX0		  0x32	/* R   X-Axis Data 0 */
#define DATAX1		  0x33	/* R   X-Axis Data 1 */
#define DATAY0		  0x34	/* R   Y-Axis Data 0 */
#define DATAY1		  0x35	/* R   Y-Axis Data 1 */
#define DATAZ0		  0x36  /* R   Z-Axis Data 0 */
#define DATAZ1		  0x37	/* R   Z-Axis Data 1 */
#define FIFO_CTL	  0x38	/* R/W FIFO control  */
#define FIFO_STATUS	0x39	/* R   FIFO status   */

/* DEVIDs */
#define ID_ADXL345	0xE5

/* INT_ENABLE/INT_MAP/INT_SOURCE Bits */
#define DATA_READY	(1 << 7)
#define SINGLE_TAP	(1 << 6)
#define DOUBLE_TAP	(1 << 5)
#define ACTIVITY	  (1 << 4)
#define INACTIVITY	(1 << 3)
#define FREE_FALL	  (1 << 2)
#define WATERMARK	  (1 << 1)
#define OVERRUN		  (1 << 0)

/* ACT_INACT_CONTROL Bits */
#define ACT_ACDC	  (1 << 7)
#define ACT_X_EN	  (1 << 6)
#define ACT_Y_EN	  (1 << 5)
#define ACT_Z_EN	  (1 << 4)
#define INACT_ACDC	(1 << 3)
#define INACT_X_EN	(1 << 2)
#define INACT_Y_EN	(1 << 1)
#define INACT_Z_EN	(1 << 0)

/* TAP_AXES Bits */
#define SUPPRESS	(1 << 3)
#define TAP_X_EN	(1 << 2)
#define TAP_Y_EN	(1 << 1)
#define TAP_Z_EN	(1 << 0)

/* ACT_TAP_STATUS Bits */
#define ACT_X_SRC	(1 << 6)
#define ACT_Y_SRC	(1 << 5)
#define ACT_Z_SRC	(1 << 4)
#define ASLEEP		(1 << 3)
#define TAP_X_SRC	(1 << 2)
#define TAP_Y_SRC	(1 << 1)
#define TAP_Z_SRC	(1 << 0)

/* BW_RATE Bits */
#define LOW_POWER	(1 << 4)
#define RATE(x)		((x) & 0xF)

/* POWER_CTL Bits */
#define PCTL_LINK	      (1 << 5)
#define PCTL_AUTO_SLEEP (1 << 4)
#define PCTL_MEASURE	  (1 << 3)
#define PCTL_SLEEP	    (1 << 2)
#define PCTL_WAKEUP(x)	((x) & 0x3)

/* DATA_FORMAT Bits */
#define SELF_TEST	  (1 << 7)
#define SPI		      (1 << 6)
#define INT_INVERT	(1 << 5)
#define FULL_RES	  (1 << 3)
#define JUSTIFY		  (1 << 2)
#define RANGE(x)	  ((x) & 0x3)
#define RANGE_PM_2g	  0
#define RANGE_PM_4g	  1
#define RANGE_PM_8g	  2
#define RANGE_PM_16g	3

/*
 * Maximum value our axis may get in full res mode for the input device
 * (signed 13 bits)
 */
#define ADXL_FULLRES_MAX_VAL 4096

/*
 * Maximum value our axis may get in fixed res mode for the input device
 * (signed 10 bits)
 */
#define ADXL_FIXEDRES_MAX_VAL 512

/* FIFO_CTL Bits */
#define FIFO_MODE(x)	(((x) & 0x3) << 6)
#define FIFO_BYPASS	  0
#define FIFO_FIFO	    1
#define FIFO_STREAM	  2
#define FIFO_TRIGGER	3
#define TRIGGER		(1 << 5)
#define SAMPLES(x)	((x) & 0x1F)

/* FIFO_STATUS Bits */
#define FIFO_TRIG	(1 << 7)
#define ENTRIES(x)	((x) & 0x3F)

#define AC_READ(ac, reg, buf)	  ( (ac)->read ( (ac)->client, reg, buf ) )
#define AC_WRITE(ac, reg, val)	( (ac)->write ( (ac)->client, reg, val ) )

#define USE_FIFO_MODE
#define USE_POLLING_MODE
#define ADXL345_IRQ_PIN 100

#ifdef CONFIG_HAS_EARLYSUSPEND
static void adxl345_early_suspend(struct early_suspend *handler);
static void adxl345_late_resume(struct early_suspend *handler);
#endif /* CONFIG_HAS_EARLYSUSPEND */


static const struct adxl345_platform_data adxl345_default_init = {
  	.tap_threshold = 64, //62.5mg LSB, reg 0x1d
  	.x_axis_offset = 0,  //15.6mg LSB, reg 0x1e
	.y_axis_offset = 0,  //15.6mg LSB, reg 0x1f
	.z_axis_offset = 0,  //15.6mg LSB, reg 0x20
	.tap_duration  = 0x10, //0.625ms LSB, reg 0x21,should not be less than 5.
	.tap_latency   = 0x60, //1.25ms LSB, reg 0x22
	.tap_window    = 0x30, //1.25ms LSB, reg 0x23 
	.activity_threshold   = 6, //62.5mg LSB, reg 0x24
	.inactivity_threshold = 4, //62.5mg LSB, reg 0x25
	.inactivity_time = 2,      //1s LSB, reg 0x26
	.act_axis_control = 0xFF,  //ACT_ACDC,ACT_X_EN,ACT_Y_EN,ACT_Z_EN, reg 0x27
	.free_fall_threshold = 8,  //62.5mg LSB, reg 0x28, 300~600mg recommended
	.free_fall_time = 0x20,      //5ms LSB, reg 0x29, 100~350ms recommended 
	//.tap_axis_control = TAP_X_EN | TAP_Y_EN | TAP_Z_EN, // | SUPPRESS,//reg 0x2a
	.data_rate = 7, //3200HZ/(2^(15-x)), reg 0x2c, data rate bigger, more sensitive.
	.data_format = FULL_RES, //reg 0x31

	.ev_type = EV_ABS,  // EV_ABS=0x03
	.ev_code_x = ABS_X,	/* EV_ABS ABS_X=0x01*/
	.ev_code_y = ABS_Y,	/* EV_ABS ABS_Y=0x02*/
	.ev_code_z = ABS_Z,	/* EV_ABS ABS_Z=0x03*/
   
	//.ev_code_tap_x = KEY_X,	/* EV_KEY KEY_X=45*/
	//.ev_code_tap_y = KEY_Y,	/* EV_KEY KEY_Y=21*/
	//.ev_code_tap_z = KEY_Z,	/* EV_KEY KEY_Z=44*/
	//.ev_code_ff    = KEY_F, /* EV_KEY KEY_F=33*/
	.ev_code_activity   = KEY_A,   /* EV_KEY KEY_A=30 */
	.ev_code_inactivity = KEY_F12,
	
	.low_power_mode = 0,
	.power_mode = ADXL_AUTO_SLEEP | ADXL_LINK,  //reg 0x2d
#ifdef USE_FIFO_MODE
	.fifo_mode = FIFO_STREAM,  //reg 0x38  FIFO_BYPASS FIFO_STREAM
	.watermark = 1,  //reg 0x2e, INT_ENABLE
#else
	.fifo_mode = FIFO_BYPASS,  //reg 0x38  FIFO_BYPASS FIFO_STREAM
	.watermark = 0,  //reg 0x2e, INT_ENABLE
#endif
};

static int adxl34x_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char * buf);
static int adxl34x_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val);
static void adxl345_enable(struct adxl345 *ac);
static void adxl345_disable(struct adxl345 *ac);

static void adxl345_report_key_single(struct input_dev *input, int code)
{
	input_report_key(input, code, 1);
	input_sync(input);
	input_report_key(input, code, 0);
	input_sync(input);
}

static void adxl345_report_key_double(struct input_dev *input, int code)
{
  	input_report_key(input, code, 1);
	input_sync(input);
	input_report_key(input, code, 0);
	input_sync(input);
	input_report_key(input, code, 1);
	input_sync(input);
	input_report_key(input, code, 0);
	input_sync(input);
}

#ifdef _ADXL345_AUTO_ROTATION_
static ssize_t adxl345_auto_rotation_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	return sprintf(buf, "auto rotation is %u\n", ac->auto_rotation);
}
static ssize_t adxl345_auto_rotation_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;
		
	if (val)
	{
		ac->auto_rotation = val;
		adxl345_disable(ac);
		mod_timer(&ac->adx_auto_rotation_timeout, jiffies + ac->auto_rotation*HZ);
	}
	else
	{
		ac->auto_rotation = 0;
		del_timer_sync(&ac->adx_auto_rotation_timeout);
		adxl345_enable(ac);
	}

	return count;
}
static DEVICE_ATTR(auto_rotation, S_IRUGO|S_IWUSR|S_IWGRP, adxl345_auto_rotation_show, adxl345_auto_rotation_store);

static void adxl345_auto_rotation_timeout(unsigned long arg)
{
	struct adxl345 *ac = (struct adxl345 *)arg;
	struct adxl345_platform_data *pdata = &ac->pdata;
	struct axis_triple axis;
	int i;
	//Landscape
	if(!ac->rotation){
		axis.x = 0x00FD;
		axis.y = 0x000F;
		axis.z = 0x007C;

		for(i=0; i<10; i++)
		{
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_x, axis.x);
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_y, axis.y);
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_z, axis.z);
			input_sync(ac->input);
			mdelay(1);
		}
		/* Send key event to avoid enter suspend */
		adxl345_report_key_single(ac->input, pdata->ev_code_inactivity);
		mod_timer(&ac->adx_auto_rotation_timeout, jiffies + ac->auto_rotation*HZ);
		ac->rotation = 1;
	}
	//Protrait
	else{
		axis.x = 0x0018;
		axis.y = 0x00E3;
		axis.z = 0x0090;

		for(i=0; i<10; i++)
		{
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_x, axis.x);
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_y, axis.y);
			input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_z, axis.z);
			mdelay(1);
		}
		/* Send key event to avoid enter suspend */
		adxl345_report_key_single(ac->input, pdata->ev_code_inactivity);
		mod_timer(&ac->adx_auto_rotation_timeout, jiffies + ac->auto_rotation*HZ);
		ac->rotation = 0;
	}
}
#endif

static void adxl345_get_triple(struct adxl345 *ac, struct axis_triple *axis)
{
	unsigned char buf[6];
	ac->read_block(ac->client, DATAX0, 6, (unsigned char *)buf);
	//mutex_lock(&ac->mutex);
	
	/* filter to 10bit (include +/- symbol) */
	buf[1] &= 0x07;
	buf[3] &= 0x07;
	buf[5] &= 0x07;

	axis->x = (buf[1]<<8) + buf[0];
	axis->y = (buf[3]<<8) + buf[2];
	axis->z = (buf[5]<<8) + buf[4];
	
	/* Translate the twos complement to true binary code for +/- (LSB/g) */
	//dprintk(KERN_ALERT "[ADXL345 G-Sensor]:Enter %s, dataX=0x%04x, dataY=0x%04x, dataZ=0x%04x\n",__func__, axis->x, axis->y, axis->z);
			
	if (axis->x > 0x400)
		axis->x -= 0x800;

	if (axis->y > 0x400)
		axis->y -= 0x800;
	
	if (axis->z > 0x400)
		axis->z -= 0x800;

	//mutex_unlock(&ac->mutex);
}

static void adxl345_service_ev_fifo(struct adxl345 *ac)
{
	struct adxl345_platform_data *pdata = &ac->pdata;
	struct axis_triple axis;
	short int x,y,z;
	
	adxl345_get_triple(ac, &axis);
	/* (mg/LSB unit * acceleration of gravity) */
//#if defined(CONFIG_PANEL_LANDSCAPE_REVERSED)   //Landscape reversed for EP10
//	x = (short int)((axis.y*9800)>>8);

//	y = (short int)((axis.x*9800)>>8);
//	z = (short int)((axis.z*9800)>>8)*-1;
//#elif defined(CONFIG_PANEL_LANDSCAPE)  //Landscape for EP10
//	x = (short int)((axis.y*9800)>>8)*-1;
//	y = (short int)((axis.x*9800)>>8)*-1;
//	z = (short int)((axis.z*9800)>>8)*-1;
//#elif defined(CONFIG_PANEL_PORTRAIT_REVERSED) //Protrait reversed for EP10
//	x = (short int)((axis.x*9800)>>8);
//	y = (short int)((axis.y*9800)>>8)*-1;
//	z = (short int)((axis.z*9800)>>8)*-1;
//#else //Protrait  for EP10
//	x = (short int)((axis.x*9800)>>8)*-1;
//	y = (short int)((axis.y*9800)>>8);
//	z = (short int)((axis.z*9800)>>8)*-1;
//#endif

//&*&*&*JJ1
	x = (short int)((axis.y*9800)>>8)*-1;
	y = (short int)((axis.x*9800)>>8);
	z = (short int)((axis.z*9800)>>8);
//&*&*&*JJ2
	input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_x, x);
	input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_y, y);
	input_event(ac->input, ac->pdata.ev_type, pdata->ev_code_z, z);
	input_sync(ac->input);
}

#ifdef USE_INTRRUPT_MODE
static int act_count = 0;
static int inact_count = 0;
static int ff_count = 0;
static int tap_count = 0;
#endif

void adxl345_work(struct work_struct *work)
{
	struct adxl345 *ac = container_of(work, struct adxl345, work.work);
	struct adxl345_platform_data *pdata = (struct adxl345_platform_data *)&ac->pdata;
#ifdef USE_INTRRUPT_MODE
	unsigned char int_stat, tap_stat;
#endif
#ifdef USE_FIFO_MODE
	unsigned char fifo_state;
	int samples;
#endif

#ifdef USE_INTRRUPT_MODE
	/*ACT_TAP_STATUS should be read before clearing the interrupt
	 * Avoid reading ACT_TAP_STATUS in case TAP detection is disabled*/
	if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))  // | SUPPRESS))
		AC_READ(ac, ACT_TAP_STATUS, &tap_stat);
	else
		tap_stat = 0;

  	//read interrpt source
	AC_READ(ac, INT_SOURCE, &int_stat);

  	//if free fall
	if (int_stat & FREE_FALL)
	{
		dprintk(KERN_ALERT "\n%s: Free Fall %d\n",__func__,++ff_count);
	}
  	//if overrun
	if (int_stat & OVERRUN)
	{
		dprintk(KERN_ALERT "%s OVERRUN\n",__func__);
	}
 	//if single tap
	if (int_stat & SINGLE_TAP) 
	{	
		if (tap_stat & TAP_X_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapX\n",__func__);
		}
		if (tap_stat & TAP_Y_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapY\n",__func__);
		}
		if (tap_stat & TAP_Z_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Single TapZ\n",__func__);
		}
		dprintk(KERN_ALERT "\n%s: Single Tap  %d\n",__func__,++tap_count);
	}
  	//if double tap
	if (int_stat & DOUBLE_TAP) 
	{
		if (tap_stat & TAP_X_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapX\n",__func__);
		}
		if (tap_stat & TAP_Y_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapY\n",__func__);
		}
		if (tap_stat & TAP_Z_SRC)
		{
			dprintk(KERN_ALERT "\n%s: Double TapZ\n",__func__);
		}
		dprintk(KERN_ALERT "\n%s: Double Tap  %d\n",__func__,++tap_count);
	}
  	//if activity or inactivity
	if (pdata->ev_code_activity || pdata->ev_code_inactivity) 
	{	
		if (int_stat & ACTIVITY)
		{
			if(tap_stat & ACT_X_SRC)
			{
				dprintk(KERN_ALERT "\n%s: Activity ActX\n",__func__);
			}
			if(tap_stat & ACT_Y_SRC)
			{
				dprintk(KERN_ALERT "\n%s: Activity ActY\n",__func__);
			}
			if(tap_stat & ACT_Z_SRC)
			{
				dprintk(KERN_ALERT "\n%s: Activity ActZ\n",__func__);
			}
			dprintk(KERN_ALERT "\n%s: Activity %d\n",__func__,++act_count);
		}
		if (int_stat & INACTIVITY)
		{
			dprintk(KERN_ALERT "\n%s: Inactivity %d\n",__func__, ++inact_count);
		}
	}

	//if data ready | watermark
	if (int_stat & (DATA_READY | WATERMARK)) 
	{
#endif

#ifdef USE_FIFO_MODE
		if (pdata->fifo_mode)
		{
			AC_READ(ac, FIFO_STATUS, &fifo_state);
			//dprintk(KERN_ALERT "[ADXL345 G-Sensor]:Enter %s, FIFO Status 0x%02x\n",__func__, fifo_state);
			samples = ENTRIES(fifo_state) + 1;
		}
		else
			samples = 1;

		for (; samples>0; samples--) 
		{
			adxl345_service_ev_fifo(ac);
			/* To ensure that the FIFO has
			 * completely popped, there must be at least 5 us between
			 * the end of reading the data registers, signified by the
			 * transition to register 0x38 from 0x37 or the CS pin
			 * going high, and the start of new reads of the FIFO or
			 * reading the FIFO_STATUS register. For SPI operation at
			 * 1.5 MHz or lower, the register addressing portion of the
			 * transmission is sufficient delay to ensure the FIFO has
			 * completely popped. It is necessary for SPI operation
			 * greater than 1.5 MHz to de-assert the CS pin to ensure a
			 * total of 5 us, which is at most 3.4 us at 5 MHz
			 * operation.*/
			if (ac->fifo_delay && (samples>1))
				udelay(3);
		}
#endif

#ifdef USE_INTRRUPT_MODE
	}
#endif
    //&*&*&*AL1_20100927, prevrnt enter sleep when rotation
	adxl345_report_key_single(ac->input, pdata->ev_code_inactivity);
	//&*&*&*AL2_20100927, prevrnt enter sleep when rotation
	//enable_irq(ac->client->irq); /*20101022, Jimmy Su, avoid for warring message*/
#ifdef USE_INTRRUPT_MODE
	local_irq_enable();
#else
	schedule_delayed_work(&ac->work, ac->delaytime);
#endif

}



static ssize_t adxl345_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	struct adxl345 *ac = dev_get_drvdata(dev);
	printk("[ADXL345 G-Sensor]:Enter %s, delaytime=%d\n", __FUNCTION__, ac->delaytime);
    return sprintf(buf, "%d\n", ac->delaytime);
}

static ssize_t adxl345_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;
	
	error = strict_strtoul(buf, 10, &val);
	ac->delaytime = val;
	
	printk("[ADXL345 G-Sensor]:Enter %s, delaytime=%d\n", __FUNCTION__, ac->delaytime);
    return count;
}
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP, adxl345_delay_show, adxl345_delay_store);

/* Iniatial status for application registered g-snesor listener (The data not be used by sensormanager) */
static ssize_t adxl345_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct adxl345 *ac = dev_get_drvdata(dev);
    int rt;

	printk("[ADXL345 G-Sensor]:Enter %s, status attr=%d\n", __FUNCTION__, ac->opened);
    rt = sprintf(buf, "%d\n", ac->opened);

    return rt;
}
static DEVICE_ATTR(status, S_IRUGO|S_IWUSR|S_IWGRP, adxl345_status_show, NULL);

/* Used for mSensorService binder Died */
static ssize_t adxl345_wake_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);
	if(val)
	{
		ac->bstoppolling = true;
	}
	else
	{
		ac->bstoppolling = false;
	}
	printk("[ADXL345 G-Sensor]:Enter %s, bstoppolling=%d\n", __FUNCTION__, ac->bstoppolling);
 
  	return count;
}

static DEVICE_ATTR(wake, S_IRUGO|S_IWUSR|S_IWGRP, NULL, adxl345_wake_store);

/* Iniatial data for application registered g-snesor listener (The data not be used by sensormanager) */
static ssize_t adxl345_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned char data[10];
	short int data_x,data_y,data_z;
	short int x,y,z;
	
	ac->read_block(ac->client, DATAX0, 6, data);
  	data_x = (data[1]<<8) + data[0];
  	data_y = (data[3]<<8) + data[2];
  	data_z = (data[5]<<8) + data[4];
	
	// Translate the twos complement to true binary code for +/-
	if (data_x > 0x400)
		data_x -= 0x800;

	if (data_y > 0x400)
		data_y -= 0x800;
	
	if (data_z > 0x400)
		data_z -= 0x800;
	
//#if defined(CONFIG_PANEL_LANDSCAPE_REVERSED)   //Landscape reversed for EP10
//	x = (short int)((data_y*9800)>>8);

//	y = (short int)((data_x*9800)>>8);
//	z = (short int)((data_z*9800)>>8)*-1;
//#elif defined(CONFIG_PANEL_LANDSCAPE)  //Landscape for EP10

//	x = (short int)((data_y*9800)>>8)*-1;
//	y = (short int)((data_x*9800)>>8)*-1;
//	z = (short int)((data_z*9800)>>8)*-1;
//#elif defined(CONFIG_PANEL_PORTRAIT_REVERSED) //Protrait reversed for EP10
//	x = (short int)((data_x*9800)>>8);
//	y = (short int)((data_y*9800)>>8)*-1;
//	z = (short int)((data_z*9800)>>8)*-1;
//#else //Protrait  for EP10
//	x = (short int)((data_x*9800)>>8)*-1;
//	y = (short int)((data_y*9800)>>8);
//	z = (short int)((data_z*9800)>>8)*-1;
//#endif

//&*&*&*JJ1
	x = (short int)((data_y*9800)>>8)*-1;
	y = (short int)((data_x*9800)>>8);
	z = (short int)((data_z*9800)>>8);
//&*&*&*JJ2
	
    printk("[ADXL345 G-Sensor]:Enter %s, x=%d y=%d z=%d \n",__FUNCTION__, x, y, z);
	
	return sprintf(buf, "%d %d %d\n", x, y, z);
}
static DEVICE_ATTR(data, S_IRUGO|S_IWUSR|S_IWGRP, adxl345_data_show, NULL);

static ssize_t adxl345_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned enable;
	if(ac->enabled)
		enable = 1;
	else
		enable = 0;
		
	printk("[ADXL345 G-Sensor]:Enter %s, enable attr=%d\n", __FUNCTION__, enable);
	return sprintf(buf, "%u\n", enable);
}

static ssize_t adxl345_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;
		
	if (val)
	{
		ac->cancel_work = 1;
		adxl345_enable(ac);
	}
	else
	{
		adxl345_disable(ac);
		#ifdef _ADXL345_AUTO_ROTATION_
		if(ac->auto_rotation > 0)
			del_timer_sync(&ac->adx_auto_rotation_timeout);
		#endif
	}
	printk("[ADXL345 G-Sensor]:Enter %s, val=%d\n",__FUNCTION__, (int)val);
	return count;
}
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP, adxl345_enable_show, adxl345_enable_store);

#ifdef ADXL_DEBUG
static ssize_t adxl345_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct adxl345 *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;
	/* This allows basic ADXL register write access for debug purposes.*/
	mutex_lock(&ac->mutex);
	error = strict_strtoul(buf, 16, &val);
	if (error)
		return error;
	AC_WRITE(ac, val >> 8, val & 0xFF); //high 8bit is reg, low 8bit is value
	mutex_unlock(&ac->mutex);
	return count;
}
static DEVICE_ATTR(write, S_IRUGO|S_IWUSR|S_IWGRP, NULL, adxl345_write_store);
#endif

static struct attribute *adxl345_attributes[] = {
	&dev_attr_data.attr,
	&dev_attr_enable.attr,
    &dev_attr_delay.attr,
    &dev_attr_status.attr,
    &dev_attr_wake.attr,
#ifdef ADXL_DEBUG
	&dev_attr_write.attr,
#endif
#ifdef _ADXL345_AUTO_ROTATION_
	&dev_attr_auto_rotation.attr,
#endif
	NULL
};

static const struct attribute_group adxl345_attr_group = {
	.attrs = adxl345_attributes,
};

#ifdef USE_INTRRUPT_MODE
static irqreturn_t adxl345_irq(int irq, void *handle)
{
	struct adxl345 *ac = handle;
 	//disable_irq_nosync(irq);
	local_irq_disable();
	schedule_delayed_work(&ac->work, 1);
	return IRQ_HANDLED;
}
#endif


static int __devinit adxl345_initialize(struct i2c_client *client, struct adxl345 *ac)
{
	struct input_dev *input_dev;
	struct adxl345_platform_data *devpd = client->dev.platform_data;
	struct adxl345_platform_data *pdata;
	int err;
	unsigned char revid;
	int range;
 	
 	dprintk(KERN_ALERT "[ADXL345 G-Sensor]Enter %s \n", __FUNCTION__);
	
	if (!client->irq) 
	{
		dprintk(KERN_ALERT "%s no IRQ?\n",__func__);
		return -ENODEV;
	}

	if (!devpd) 
	{
		dprintk(KERN_ALERT "%s No platfrom data: Using default initialization\n",__func__);
		devpd = (struct adxl345_platform_data *)&adxl345_default_init;
	}

	memcpy(&ac->pdata, devpd, sizeof(ac->pdata));
	pdata = &ac->pdata;

	ac->enabled = 0;
	ac->opened = 1;
//	ac->delaytime = 200;
ac->delaytime = 50;
	
	INIT_DELAYED_WORK(&ac->work, adxl345_work);
	mutex_init(&ac->mutex);

	AC_READ(ac, DEVID, &revid);

	switch (revid) 
	{    
		case ID_ADXL345:
				ac->model = 0x345;
				dprintk(KERN_ALERT "************************************************************\n");
				dprintk(KERN_ALERT "%s: ADXL345 ID is 0x%x.\n",__func__,ac->model);
				dprintk(KERN_ALERT "************************************************************\n");
				break;
		default:
				dprintk(KERN_ALERT "%s Failed to probe, no find sensor device.\n", __func__);
				err = -ENODEV;
				goto err_free_mem;
	}

	/* Create input device and configuration */
	input_dev = input_allocate_device();
	if (!input_dev)
	{
		return -ENOMEM;
	}
	strcpy(ac->phys, "adxl345-accelerometer/input3\0");
	ac->input = input_dev;
	input_dev->name = "accelerometer";//"ADXL345 accelerometer";
	input_dev->phys = ac->phys;
	input_dev->dev.parent = &client->dev;
	input_dev->id.product = ac->model;
	input_dev->id.bustype = BUS_I2C;
	input_set_drvdata(input_dev, ac);//input_dev->dev.driver_data = ac;

	__set_bit(ac->pdata.ev_type, input_dev->evbit);
	if (ac->pdata.ev_type == EV_REL) 
	{
		__set_bit(REL_X, input_dev->relbit);
		__set_bit(REL_Y, input_dev->relbit);
		__set_bit(REL_Z, input_dev->relbit);
	} 
	else /* EV_ABS */
	{
		__set_bit(ABS_X, input_dev->absbit);
		__set_bit(ABS_Y, input_dev->absbit);
		__set_bit(ABS_Z, input_dev->absbit);

		if (pdata->data_format & FULL_RES)
			range = ADXL_FULLRES_MAX_VAL;	  /* Signed 13-bit */
		else
			range = ADXL_FIXEDRES_MAX_VAL;	/* Signed 10-bit */

		input_set_abs_params(input_dev, ABS_X, -range, range, 3, 3);
		input_set_abs_params(input_dev, ABS_Y, -range, range, 3, 3);
		input_set_abs_params(input_dev, ABS_Z, -range, range, 3, 3);
	}
	
	/* The follow functions not be actived (20101220 by aimar) */
	if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))
	{
		ac->int_mask |= SINGLE_TAP | DOUBLE_TAP;
		__set_bit(EV_KEY, input_dev->evbit);
		__set_bit(pdata->ev_code_tap_x, input_dev->keybit);
		__set_bit(pdata->ev_code_tap_y, input_dev->keybit);
		__set_bit(pdata->ev_code_tap_z, input_dev->keybit);
	}
	if (pdata->ev_code_ff) 
	{
		ac->int_mask |= FREE_FALL;
		__set_bit(pdata->ev_code_ff, input_dev->keybit);
	}

	if (pdata->ev_code_activity)
	{
		ac->int_mask |= ACTIVITY;
		__set_bit(pdata->ev_code_activity, input_dev->keybit);
	}
	
	if (pdata->ev_code_inactivity)
	{
		ac->int_mask |= INACTIVITY;
		__set_bit(pdata->ev_code_inactivity, input_dev->keybit);
	}

	/* FIFO mode, enable watermark */
	if (pdata->watermark) 
	{
		ac->int_mask |= WATERMARK;
		if (!FIFO_MODE(pdata->fifo_mode))
			pdata->fifo_mode |= FIFO_STREAM;
	} 
	if (FIFO_MODE(pdata->fifo_mode) == FIFO_BYPASS)
		ac->fifo_delay = 0;

	AC_WRITE(ac, POWER_CTL, 0);

	err = input_register_device(input_dev);
	if (err)
		goto err_free_mem;

	err = sysfs_create_group(&input_dev->dev.kobj, &adxl345_attr_group);
	if (err)
		goto err_remove_input;
	
	
#ifdef USE_INTRRUPT_MODE
		err = request_irq(client->irq, adxl345_irq, IRQF_TRIGGER_FALLING, client->dev.driver->name, ac);
		if (err) 
		{
			dev_err(&client->dev, "irq %d busy?\n", client->irq);
			goto err_remove_input;
		}
#endif

	/* **** Setting G-Sensor via I2C ***** */
	AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
	AC_WRITE(ac, OFSX, pdata->x_axis_offset);
	AC_WRITE(ac, OFSY, pdata->y_axis_offset);
	AC_WRITE(ac, OFSZ, pdata->z_axis_offset);
	ac->hwcal.x = pdata->x_axis_offset;
	ac->hwcal.y = pdata->y_axis_offset;
	ac->hwcal.z = pdata->z_axis_offset;
	AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
	AC_WRITE(ac, DUR, pdata->tap_duration);
	AC_WRITE(ac, LATENT, pdata->tap_latency);
	AC_WRITE(ac, WINDOW, pdata->tap_window);
	AC_WRITE(ac, THRESH_ACT, pdata->activity_threshold);
	AC_WRITE(ac, THRESH_INACT, pdata->inactivity_threshold);
	AC_WRITE(ac, TIME_INACT, pdata->inactivity_time);
	AC_WRITE(ac, THRESH_FF, pdata->free_fall_threshold);
	AC_WRITE(ac, TIME_FF, pdata->free_fall_time);
	AC_WRITE(ac, TAP_AXES, pdata->tap_axis_control);
	AC_WRITE(ac, ACT_INACT_CTL, pdata->act_axis_control);
	AC_WRITE(ac, BW_RATE, RATE(ac->pdata.data_rate) | (pdata->low_power_mode ? LOW_POWER : 0));
	AC_WRITE(ac, DATA_FORMAT, pdata->data_format | INT_INVERT );
	AC_WRITE(ac, FIFO_CTL, FIFO_MODE(pdata->fifo_mode) | SAMPLES(pdata->watermark));
	AC_WRITE(ac, INT_MAP, 0x00);	/* Map all INTs to INT1 */
	AC_WRITE(ac, INT_ENABLE, ac->int_mask); 

	pdata->power_mode &= (PCTL_AUTO_SLEEP | PCTL_LINK);

	return 0;

#ifdef USE_INTRRUPT_MODE
 err_free_irq:
	free_irq(client->irq, ac);
#endif
 err_remove_input:
 	input_free_device(input_dev);
 err_free_mem:
	return err;
}

static void adxl345_disable(struct adxl345 *ac)
{
	mutex_lock(&ac->mutex);
	if (ac->enabled && ac->opened) 
	{
		ac->enabled = 0;
		ac->cancel_work = cancel_delayed_work(&ac->work);

		/*A '0' places the ADXL345 into standby mode with minimum power consumption.*/
		AC_WRITE(ac, POWER_CTL, 0);
		printk("[ADXL345 G-Sensor]Enter %s, cancel work is %d, and adxl345-sensor enter suspend\n", __FUNCTION__, ac->cancel_work);
	}
	mutex_unlock(&ac->mutex);
}

static void adxl345_enable(struct adxl345 *ac)
{
	mutex_lock(&ac->mutex);
	if (!ac->enabled && ac->opened) 
	{
		AC_WRITE(ac, POWER_CTL, ac->pdata.power_mode | PCTL_MEASURE);
		ac->enabled = 1;
#ifdef USE_INTRRUPT_MODE
		if(ac->cancel_work) /* work_queue be canceled before execution */
		{
			ac->cancel_work = 0;
			schedule_delayed_work(&ac->work, 1);
#else
			schedule_delayed_work(&ac->work, ac->delaytime); /* parameter 2  is msec  */
#endif
			printk("[ADXL345 G-Sensor]Enter %s, reschedule work queue( or enable irq).\n", __FUNCTION__);
#ifdef USE_INTRRUPT_MODE
		}
#endif
	}
	mutex_unlock(&ac->mutex);
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void adxl345_early_suspend(struct early_suspend *handler)
{
		struct adxl345 *ac = container_of(handler, struct adxl345, early_suspend);//dev_get_drvdata(&client->dev);
	adxl345_disable(ac);
	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation > 0)
		del_timer_sync(&ac->adx_auto_rotation_timeout);
	#endif
	return 0;

}

static void adxl345_late_resume(struct early_suspend *handler)
{
	struct adxl345 *ac = container_of(handler, struct adxl345, early_suspend);

	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation == 0)
	#endif
		adxl345_enable(ac);
	
	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation > 0)
		mod_timer(&ac->adx_auto_rotation_timeout, jiffies+ac->auto_rotation*HZ);
	#endif

	return 0;
}
#endif  /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_PM
static int adxl345_suspend(struct i2c_client *client, pm_message_t message)
{
	struct adxl345 *ac = dev_get_drvdata(&client->dev);
	adxl345_disable(ac);
	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation > 0)
		del_timer_sync(&ac->adx_auto_rotation_timeout);
	#endif
	return 0;
}

static int adxl345_resume(struct i2c_client *client)
{
	struct adxl345 *ac = dev_get_drvdata(&client->dev);

	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation == 0)
	#endif
		adxl345_enable(ac);
	
	#ifdef _ADXL345_AUTO_ROTATION_
	if(ac->auto_rotation > 0)
		mod_timer(&ac->adx_auto_rotation_timeout, jiffies+ac->auto_rotation*HZ);
	#endif
	return 0;
}
#else
#define adxl345_suspend NULL
#define adxl345_resume  NULL
#endif


static int adxl34x_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char * buf)
{
  int ret;
  struct i2c_msg send_msg = { client->addr, 0, 1, &reg };
  struct i2c_msg recv_msg = { client->addr, I2C_M_RD, 1, buf };
  
  ret = i2c_transfer(client->adapter, &send_msg, 1);
  if (ret < 0) 
  {
    dprintk(KERN_ALERT "%s transfer sendmsg Error.\n",__func__);
    return -EIO;
  }
  ret = i2c_transfer(client->adapter, &recv_msg, 1);
  if (ret < 0)
  {
    dprintk(KERN_ALERT "%s receive reg_data error.\n",__func__);	
    return -EIO;
  }
  return 0;     	
}


static int adxl34x_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val)
{
  int ret;
  unsigned char buf[2];
  struct i2c_msg msg = { client->addr, 0, 2, buf };
	
  buf[0] = reg;
  buf[1] = val; 

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret < 0)
  {
  	dprintk(KERN_ALERT "%s write reg Error.\n",__func__);
  	return -EIO;
  }
  return 0;
}

static int adxl345_i2c_read_block_data(struct i2c_client *client, unsigned char reg, int count,unsigned char *buf)
{
	int ret;
  struct i2c_msg send_msg = { client->addr, 0, 1, &reg };
  struct i2c_msg recv_msg = { client->addr, I2C_M_RD, count, buf };
  
  ret = i2c_transfer(client->adapter, &send_msg, 1);
  if (ret < 0) 
  {
    dprintk(KERN_ALERT "%s transfer sendmsg Error.\n",__func__);
    return -EIO;
  }
  ret = i2c_transfer(client->adapter, &recv_msg, 1);
  if (ret < 0)
  {
    dprintk(KERN_ALERT "%s receive reg_data error.\n",__func__);	
    return -EIO;
  }
  return 0;     	
}

static int __devinit adxl345_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct adxl345 *ac;
	int error;
	
	printk("[ADXL345 G-Sensor]Enter %s \n", __FUNCTION__);
	
	if(gpio_request(ADXL345_IRQ_PIN, "adxl345_irq") < 0)
	{
		printk(KERN_ERR "Failed to request GPIO%d for adxl345 IRQ\n",
			ADXL345_IRQ_PIN);
		return -1;
	}
	gpio_direction_input(ADXL345_IRQ_PIN);


	ac = kzalloc(sizeof(struct adxl345), GFP_KERNEL);
	if (!ac)
		return -ENOMEM;

	i2c_set_clientdata(client, ac);//client->dev.driver_data = ac;

	ac->client = client;
	ac->read_block = adxl345_i2c_read_block_data;
	ac->read = adxl34x_i2c_read;
	ac->write = adxl34x_i2c_write;

	error = adxl345_initialize(client, ac);
	if (error) 
	{
		i2c_set_clientdata(client, NULL);
		kfree(ac);
	}
	#ifdef CONFIG_HAS_EARLYSUSPEND
		ac->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		ac->early_suspend.suspend = adxl345_early_suspend;
		ac->early_suspend.resume = adxl345_late_resume;
		register_early_suspend(&ac->early_suspend);
#endif
	#ifdef _ADXL345_AUTO_ROTATION_
	init_timer(&ac->adx_auto_rotation_timeout);
	ac->adx_auto_rotation_timeout.function = adxl345_auto_rotation_timeout;
	ac->adx_auto_rotation_timeout.data = (unsigned long)ac;	
	#endif

	return 0;
}

static int __devexit adxl345_cleanup(struct i2c_client *client, struct adxl345 *ac)
{
	adxl345_disable(ac);
	sysfs_remove_group(&ac->input->dev.kobj, &adxl345_attr_group);
#ifdef USE_INTRRUPT_MODE
	free_irq(ac->client->irq, ac);
#endif
	input_unregister_device(ac->input);
	dprintk(KERN_ALERT "%s unregistered accelerometer\n",__func__);
	return 0;
}

static int __devexit adxl345_i2c_remove(struct i2c_client *client)
{
	struct adxl345 *ac = dev_get_drvdata(&client->dev);
	adxl345_cleanup(client, ac);
	i2c_set_clientdata(client, NULL);
	kfree(ac);
	return 0;
}

static const struct i2c_device_id adxl345_id[] = {
	{ "adxl345", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, adxl345_id);

static struct i2c_driver adxl345_driver = {
	.driver = {
		.name = "adxl345",
		.owner = THIS_MODULE,
	},
	.probe    = adxl345_i2c_probe,
	.remove   = __devexit_p(adxl345_i2c_remove),
//	.suspend  = adxl345_suspend,
//	.resume   = adxl345_resume,
	.id_table = adxl345_id,
};

static int __init adxl345_i2c_init(void)
{
	printk("[ADXL345 G-Sensor]Enter %s \n", __FUNCTION__);
	return i2c_add_driver(&adxl345_driver);
}
module_init(adxl345_i2c_init);

static void __exit adxl345_i2c_exit(void)
{
	i2c_del_driver(&adxl345_driver);
}
module_exit(adxl345_i2c_exit);

MODULE_AUTHOR("Brian Xu <xuhuanting929@yahoo.com.cn>");
MODULE_DESCRIPTION("ADXL345 Three-Axis Digital Accelerometer Driver");
MODULE_LICENSE("GPL");
