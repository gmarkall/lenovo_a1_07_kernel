/*
 * IOC F668 driver
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/slab.h>

#include <linux/gpio.h>
#include <linux/io.h>

#include <asm/io.h>
#include <mach/hardware.h>

#include "ioc_f668.h"
#include "Elan_Update668FW_func.h"
#include "Elan_Initial_f668.h"

#include <linux/cdev.h>
#include <asm/system.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#define SLAVE_ADDR      0x33

#define RSTATUS_READY   0x04 

/* Register Address of Real Time Clock */
#define RTIME_YEAR 			0x40
#define RTIME_MONTH     0x41
#define RTIME_DATE 			0x42
#define RTIME_DAY 			0x43
#define RTIME_HOUR 			0x44
#define RTIME_MIN 			0x45
#define RTIME_SEC				0x46

#define RMSG_EVENT      0x6A
#define RSYSTEM_STATE 	0xFD

#define RRESET          0x90
#define RMODE_CONTROL   0x91
#define RUPGRADE_FW		 	0x92 //RUPGRADE_MODE
#define RBOARDID_VERIFY 0x93

#define DRIVER_VERSION			"0.0.0"
#define I2C_M_WR				0

static struct class *ioc_class;

enum ioc_type
{
	ioc=0,
};

struct ioc_dev_t
{
	struct cdev cdev; 
};
struct ioc_dev_t ioc_dev;

enum ioc_cmd
{
	CM_UPDATE_FW,
	IOCSIM_REVISION,	//device id
	IOCSIM_GETSERIAL,
	IOCSIM_ENCRYPT,
	IOCSIM_DECRYPT,
	IOCSIM_GETDRMKEY,
	IOCSIM_RESETDRMKEY,
	IOCSIM_AUTH,
	IOCSIM_CHANGEPIN,
	IOCSIM_CHKAUTH,
	IOCSIM_RESETUSERKEY,
	WRITE_KEY,
	FINALIZE,
	CM_IOC_RESET,	
	CM_BAT_CAP,
	CM_UPDATE_EEPROM,
	
	CMD_END
};

struct iocf668_device_info;
struct iocf668_access_methods {
	int (*read)(unsigned char addr, u8 reg, unsigned char *val);
	int (*multi_read)(unsigned char addr, u8 start_reg, unsigned char *val_buf, int num_of_data);
	int (*write)(unsigned char addr, u8 reg, unsigned char val);
	int (*multi_write)(unsigned char addr, u8 start_reg, unsigned char *val, int num_of_data);
};

struct iocf668_device_info {
	struct device 		*dev;
	struct iocf668_access_methods	*bus;
	struct i2c_client	*client;

	struct workqueue_struct *wqueue;
};

struct iocf668_device_info *gp_ioc;

unsigned short fw_a_area[8453] = {0};

extern unsigned short Elan_Update668FW_func(unsigned short Elan_FUNCTIONS);
static void ioc_update(void);
static int ioc_reset(void);
static int iocf668_get_serial(unsigned char *serial);

static unsigned char get_checksum(char *wbuf, int num)
{
	int i ;
	char *buf = wbuf ;
	unsigned char checksum = 0;
	for(i = 0 ; i <= num ; i++)
	{
		checksum ^= buf[i];
	}
	checksum ^= 0xff;
	return checksum;
}

#if 0
static int iocf668_read(unsigned char addr, u8 reg, unsigned char *val)
{
	int ret;
	ret = gp_ioc->bus->read(addr, reg, val);
//	*rt_value = be16_to_cpu(*rt_value);
	return ret;
}
#endif

static int iocf668_multi_read(unsigned char addr, u8 start_reg, unsigned char *val_buf, int num_of_data)
{
	int ret;
	ret = gp_ioc->bus->multi_read(addr, start_reg, val_buf, num_of_data);
//	*rt_value = be16_to_cpu(*rt_value);
	return ret;
}

#if 0
static int iocf668_write(unsigned char addr, u8 reg, unsigned char val)
{	
	int ret;
	ret = gp_ioc->bus->write(addr, reg, val);
	return ret;
}

static int iocf668_multi_write(unsigned char addr, u8 start_reg, unsigned char *val_buf, int num_of_data)
{	
	int ret;
	ret = gp_ioc->bus->multi_write(addr, start_reg, val_buf, num_of_data);
	return ret;
}
#endif

#define to_iocf668_device_info(x) container_of((x), \
				struct iocf668_device_info, bat);

int ioc_read_reg(unsigned char addr, unsigned char reg, unsigned char *val)
{
	struct i2c_msg msg;
	unsigned char regaddr[1];

	msg.addr = addr & 0xff ;
	msg.flags = I2C_M_WR;	//Write
	msg.len = 1;
	regaddr[0] = reg; 
	msg.buf = regaddr; 
	
	if ((i2c_transfer(gp_ioc->client->adapter, &msg, 1)) != 1)
	{
		printk("write -EIO\n");
		return -EIO;
	}
	
	/// now read data from I2C device
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = (unsigned char *)val; //note: here var is a pointer, the value of reg will be read to *val

	if ((i2c_transfer(gp_ioc->client->adapter, &msg, 1)) != 1) {
		printk("%s -EIO\n", __func__);
		return -EIO;
	}	
		
	//else the value of reg will be read to *val
	return msg.len;
}

EXPORT_SYMBOL(ioc_read_reg) ;

/****************************************************************************
 * 
 * FUNCTION NAME:  IOC_read_multi_reg   
 *  
 *
 * DESCRIPTION:   read from ioc regs  
 *
 *
 * ARGUMENTS:        
 * 
 * ARGUMENT         TYPE            I/O     DESCRIPTION 
 * --------         ----            ----    ----------- 
 *  @addr        unsigned char              client slave address
 *	@start_reg   u8                         start reg 
 *	@val_buf		(unsigned char *)           start address of store-value buf              
 *	@num_of_data int                        length of data in bytes 
 * 
 * RETURNS: num of bytes read from ioc regs or else EIO when error
 * 
 ***************************************************************************/
int ioc_read_multi_reg(unsigned char addr, unsigned char start_reg, unsigned char *val_buf, int num_of_data)
{
	struct i2c_msg msg[2];
	unsigned char regaddr[1];

	msg[0].addr = addr & 0xff;
	msg[0].flags = I2C_M_WR;	//Write
	msg[0].len = 1;
	regaddr[0] = start_reg; 
	msg[0].buf = regaddr; //buf pointer to the start address of regs
	
	/// now read data from I2C device
	msg[1].addr = addr & 0xff;
	msg[1].flags = I2C_M_RD;
	msg[1].len = num_of_data;
	msg[1].buf = (unsigned char *)val_buf; //Note: here val_buf is a pointer, the value of reg will be read to val_buf

	if ((i2c_transfer(gp_ioc->client->adapter, msg, 2)) != 2) {
		printk("%s -EIO\n", __func__);
		return -EIO;
	}	
	//else the value of reg will be read to val_buf
	return msg[1].len;
}

EXPORT_SYMBOL(ioc_read_multi_reg) ;

int ioc_write_reg(unsigned char addr, unsigned char reg, unsigned char val)
{
	struct i2c_msg msg;
	unsigned char buf[2];

	msg.addr = addr & 0xff;
	msg.flags = I2C_M_WR;	//Write
	buf[0] = reg;
	buf[1] = val ;
	msg.buf = buf;
	msg.len = 2 ;// one reg, one val(data)
	
	if ((i2c_transfer(gp_ioc->client->adapter, &msg, 1)) != 1)
		return -EIO;

	return msg.len;
}

EXPORT_SYMBOL(ioc_write_reg) ;

/***************************************************************************************
 * 
 * FUNCTION NAME:  ioc_write_multi_reg   
 *  
 *
 * DESCRIPTION:   write to ioc regs  
 *
 *
 * ARGUMENTS:        
 * 
 * ARGUMENT         TYPE            I/O     DESCRIPTION 
 * --------         ----            ----    ----------- 
 *  @addr        unsigned char              client slave address
 *	@start_reg   u8                         start reg 
 *	@var				 (unsigned char *)          start address of buf(data stored in first)              
 *	@num_of_data int                        length of data in bytes 
 * 
 * RETURNS: num of bytes write to ioc regs or else EIO when error
 * 
 ***************************************************************************************/
int ioc_write_multi_reg(unsigned char addr, unsigned char start_reg, unsigned char *val, int num_of_data)
{
	struct i2c_msg msg;
	unsigned char *tmp ;
	unsigned int i ;
	
	tmp = (unsigned char *)kzalloc(sizeof(char) * (num_of_data + 1), GFP_KERNEL) ;
	if (!tmp ) {
		printk("kzalloc failed\n");
		return -ENOMEM;
	}

	msg.addr = addr & 0xff;
	msg.flags = I2C_M_WR;	//Write
	tmp[0] = start_reg;
	for(i = 0 ; i < num_of_data ; i++)
	{
		tmp[i+1] = *(val+i) ;
	} 
	msg.buf = tmp;
	msg.len = num_of_data + 1 ;// one reg, one val(data)
	
	if ((i2c_transfer(gp_ioc->client->adapter, &msg, 1)) != 1)
		return -EIO;
	kfree(tmp);
	
	return msg.len;
}

EXPORT_SYMBOL(ioc_write_multi_reg) ;

/*
 * ***************************** IOC ioct support******************************************
 */

int ioc_open(struct inode *inode, struct file *filep)
{
  filep->private_data = &ioc_dev.cdev;
  
	return 0;
}

int ioc_release(struct inode *inode, struct file *filep)
{
	return 0;
}

static int ioc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long reset_result[1];

	switch(cmd)
	{
		case CM_UPDATE_FW:
			//sdcard -> fw_a_area
			printk("%s::CM_UPDATE_FW\n", __FUNCTION__);
			printk("========== Copy user data to kernel. ==========\n");
			if( copy_from_user((unsigned long *)fw_a_area, (unsigned long *)arg, sizeof(fw_a_area) ) != 0 )
					return -EFAULT;
			ioc_update();
			break ;
		case CM_IOC_RESET:
			printk("%s::CM_IOC_RESET\n", __FUNCTION__);
			reset_result[0] = (unsigned long)ioc_reset();
			if( copy_to_user( (unsigned long *)arg, (unsigned long *)reset_result, sizeof(reset_result) ) != 0 )
	  			return -EFAULT;
			break ;
		default:
			return -EINVAL;
	}
	return 0;
}

static const struct file_operations ioc_fops = {
	.owner 		= THIS_MODULE,
	.open		  = ioc_open,
	.release	= ioc_release,
	.ioctl		= ioc_ioctl,
};

// show IOC ELAN id
static ssize_t iocf668_show_ioc_id(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char buffer[4];
  int len;
  
	memset(buffer, 0x0, sizeof(buffer));
	len = iocf668_multi_read(gp_ioc->client->addr, 0x00, buffer, sizeof(buffer));
	if (len < 0)
	{
		printk("read ioc_id err!\n");
	}
	return sprintf(buf, "0x%02X%02X\n", buffer[1], buffer[0]);
}

// show IOC revision
static ssize_t iocf668_show_ioc_revision(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char buffer[4];
  int len;
  
	memset(buffer, 0x0, sizeof(buffer));
	len = iocf668_multi_read(gp_ioc->client->addr, 0x00, buffer, sizeof(buffer));
	if (len < 0)
	{
		printk("read ioc_revision err!\n");
	}
	return sprintf(buf, "V%02x.%02x\n", buffer[3], buffer[2]);
}

// show IOC get serial number
static ssize_t iocf668_show_ioc_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char buffer[21];

	memset(buffer, 0x0, sizeof(buffer));
	iocf668_get_serial(buffer);
	msleep(5);

	return sprintf(buf, "%s\n", buffer);
}

static ssize_t iocf668_show_ioc_reset(struct device *dev, struct device_attribute *attr, char *buf)
{
	ioc_reset();
	return sprintf(buf, "IOC_RESET_OK\n");
}

static DEVICE_ATTR(ioc_id, S_IRUGO, iocf668_show_ioc_id , NULL);
static DEVICE_ATTR(ioc_version, S_IRUGO, iocf668_show_ioc_revision, NULL);
static DEVICE_ATTR(ioc_serial, S_IRUGO, iocf668_show_ioc_serial, NULL);
static DEVICE_ATTR(ioc_reset, S_IRUGO, iocf668_show_ioc_reset, NULL);

static struct attribute *iocf668_attributes[] = {
	&dev_attr_ioc_id.attr,
	&dev_attr_ioc_version.attr,
	&dev_attr_ioc_serial.attr,
	&dev_attr_ioc_reset.attr,
	NULL
};

static const struct attribute_group iocf668_attr_group = {
	.attrs = iocf668_attributes,
};

static int iocf668_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int iocf668_resume(struct i2c_client *client)
{
	return 0;
}

static int iocf668_get_serial(unsigned char *serial)
{
	int len;
	unsigned char val[2];

	val[0] = IOCSIM_GETSERIAL;
	val[1] = get_checksum(val, 0); 

	len = ioc_write_multi_reg(gp_ioc->client->addr, 0x2E, val, 2);
	if (len < 0)
	{
		printk("write 0x2E err!\n");
		return -EFAULT;
	}
	mdelay(1);
	len = iocf668_multi_read(gp_ioc->client->addr, 0x2E, serial, 20);
	if (len < 0)
	{
		printk("read 0x2E err!\n");
		return -EFAULT;
	}
	
	return 0;
} 

static int ioc_reset(void)
{
	gpio_set_value(IOC_RST_GPIO, 1);
	msleep(200);
	gpio_set_value(IOC_RST_GPIO, 0);
	msleep(200);
	printk("ioc_reset\n");
	
	return 0;
}

static void ioc_update(void)
{
	/************** Begining Update IOC Firmware ***************/
	Elan_Initial_f668(fw_a_area);
	mdelay(100);
//	Elan_Update668FW_func(UPDATE_MODE_EEPROM_ONLY);
  Elan_Update668FW_func(UPDATE_MODE_ROM_EEPROM_CODEOPTION);
	mdelay(100);
	/***************** End Update IOC Firmware *****************/	
}

static int iocf668_probe(struct i2c_client *client, const struct i2c_device_id *id)
{	
	int retval = 0;
	struct iocf668_device_info *di = NULL;
	struct iocf668_access_methods *bus = NULL;

	printk("IOC %s\n", __FUNCTION__);
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto failed_1;
	}

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate access method data\n");
		retval = -ENOMEM;
		goto failed_2;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	bus->read = &ioc_read_reg;
	bus->multi_read = &ioc_read_multi_reg;
	bus->write = &ioc_write_reg;
	bus->multi_write = &ioc_write_multi_reg;
	di->bus = bus;
	di->client = client;
	gp_ioc = di;

	/* Register sysfs hooks */
	retval = sysfs_create_group(&client->dev.kobj, &iocf668_attr_group);
  if (retval) {
  	printk(KERN_ERR "iocf668_probe: sysfs_create_group failed\n");
		goto failed_3;
	}
	
	retval = register_chrdev (IOC_MAJOR, IOC_DEVICE_NAME, &ioc_fops);
	if (retval < 0) { 
		printk(KERN_ERR "iocf668_probe: Register chrdev region fail.\n");
		goto failed_3;
	}

	ioc_class = class_create(THIS_MODULE, IOC_DEVICE_NAME);
	device_create(ioc_class, NULL, MKDEV(IOC_MAJOR, 0), NULL, IOC_DEVICE_NAME);

  return 0;

failed_3:
	kfree(bus);
failed_2:
	kfree(di);
failed_1:

	return retval;
}

static int iocf668_remove(struct i2c_client *client)
{
	return 0;
}

/* I2C Device Driver */
static struct i2c_device_id iocf668_idtable[] = {
    {IOCF668_I2C_DEVICE_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, iocf668_idtable);

static struct i2c_driver iocf668_driver = {
    .driver = {
        .name       = IOCF668_I2C_DEVICE_NAME,
        .owner      = THIS_MODULE,
    },
    .id_table       = iocf668_idtable,
    .probe          = iocf668_probe,
    .remove         = iocf668_remove,
    .suspend        = iocf668_suspend,
    .resume         = iocf668_resume,
};

static struct init_ioc_gpios init_gpio_config[] = {
	{ .gpio_num = IOC_POWER_GPIO,    .gpio_dir = IOC_GPIO_OUTPUT, .gpio_val = 1, .gpio_name = "IOC_POWER" },
	{ .gpio_num = IOC_RST_GPIO,      .gpio_dir = IOC_GPIO_OUTPUT, .gpio_val = 1, .gpio_name = "IOC_RST" },
	{ .gpio_num = IOC_I2C_REQ_GPIO,  .gpio_dir = IOC_GPIO_INPUT,  .gpio_val = 0, .gpio_name = "IOC_I2C_REQ" },
	{ .gpio_num = UP_IOC_DATA_GPIO,  .gpio_dir = IOC_GPIO_INPUT,  .gpio_val = 0, .gpio_name = "UP_IOC_DATA" },
	{ .gpio_num = UP_IOC_CLK_GPIO,   .gpio_dir = IOC_GPIO_INPUT,  .gpio_val = 0, .gpio_name = "UP_IOC_CLK" },
	{ .gpio_num = UP_IOC_LEVEL_GPIO, .gpio_dir = IOC_GPIO_OUTPUT, .gpio_val = 1, .gpio_name = "UP_IOC_LEVEL" },
	};

static int __init iocf668_init(void)
{
	int ret, i;

	printk("%s ++\n", __func__);
	
	for ( i = 0 ; i < ARRAY_SIZE(init_gpio_config) ; i++ ) {
		ret = gpio_request(init_gpio_config[i].gpio_num, init_gpio_config[i].gpio_name);
		if (ret < 0) {
			printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__, init_gpio_config[i].gpio_num);
			continue;
		}

		if (init_gpio_config[i].gpio_dir == IOC_GPIO_OUTPUT) 
			gpio_direction_output(init_gpio_config[i].gpio_num, init_gpio_config[i].gpio_val);
		else
			gpio_direction_input(init_gpio_config[i].gpio_num);
	};;
	
	/* give a reset signal to IOC when power on */
	msleep(200);
	gpio_set_value(IOC_RST_GPIO, 0);

  ret = i2c_add_driver(&iocf668_driver);
	if (ret)
		printk(KERN_ERR "Unable to register IOC F668 driver\n");

	printk("%s --\n", __func__);
	
	return ret;
}

module_init(iocf668_init);

static void __exit iocf668_exit(void)
{
	i2c_del_driver(&iocf668_driver);
	device_destroy(ioc_class, MKDEV(IOC_MAJOR, 0));
	class_destroy(ioc_class);
	unregister_chrdev (IOC_MAJOR, IOC_DEVICE_NAME);
}
module_exit(iocf668_exit);
MODULE_AUTHOR("TMSBG-CDPG");
MODULE_DESCRIPTION("iocf668 driver");
MODULE_LICENSE("GPL");
