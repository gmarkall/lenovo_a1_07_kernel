///////////////////////////////////////////////////////////////////////////////
/* linux/arch/arm/mach-s5pv210/pix_i2c.c
 * Copyright (c) 2010 PIXTREE, Inc.
 * http://www.pixtree.com
 * S5PV210 - I2C driver via 
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
///////////////////////////////////////////////////////////////////////////////

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
//#include <linux/input.h>
//#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/delay.h>
//#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <plat/control.h>

#define ERROR_CODE_TRUE			0
#define ERROR_CODE_FALSE		1
#define ERROR_CODE_WRITE_ADDR	10
#define ERROR_CODE_WRITE_DATA	20
#define ERROR_CODE_READ_ADDR	30

static int nack_flag = 0;
void OMAP_GPIO_168(void)
{
	u32 i2c_2_scl;
	
			i2c_2_scl = __raw_readw(OMAP2_L4_IO_ADDRESS(0x480021be));
			i2c_2_scl = i2c_2_scl & ~(0x7 << 0);
		//	i2c_2_scl = i2c_2_scl |(1<<8)|(1<<4)|(1<<3)|(4<<0);
			i2c_2_scl = i2c_2_scl |(1<<8)|(4<<0);
			__raw_writew(i2c_2_scl,OMAP2_L4_IO_ADDRESS(0x480021be));
}

void OMAP_GPIO_183(void)
{
	u32 i2c_2_sda;
	
			i2c_2_sda = __raw_readw(OMAP2_L4_IO_ADDRESS(0x480021c0));
			i2c_2_sda = i2c_2_sda & ~(0x7 << 0);
		//	i2c_2_sda = i2c_2_sda |(1<<8)|(1<<4)|(1<<3)|(4<<0);
			i2c_2_sda = i2c_2_sda |(1<<8)|(4<<0);
			__raw_writew(i2c_2_sda,OMAP2_L4_IO_ADDRESS(0x480021c0));
}

//---> Modify this lines to fit your gpio setting.
#define PIN_SDA				183
#define PIN_SCL				168
//#define SECRET_EN			S5PV210_GPH1(2)
//#define PIX_I2C_MINOR	222
//<---

#define SDA_IN			gpio_direction_input(PIN_SDA)
//#define SDA_IN			gpio_input_sda()
#define SDA_OUT			gpio_direction_output(PIN_SDA,1)
//#define SDA_LOW			gpio_direction_output(PIN_SDA, 0)
//#define SDA_HIGH		gpio_direction_output(PIN_SDA, 1)
#define SDA_LOW			gpio_set_value(PIN_SDA, 0)
#define SDA_HIGH		gpio_set_value(PIN_SDA, 1)
#define SDA_DETECT		gpio_get_value(PIN_SDA)
//#define SDA_DETECT		(__raw_readw(OMAP2_L4_IO_ADDRESS(0x49058038))& (1<<23))?1:0

//#define SCL_LOW			gpio_direction_output(PIN_SCL, 0)
//#define SCL_HIGH		gpio_direction_output(PIN_SCL, 1)
#define SCL_LOW			gpio_set_value(PIN_SCL, 0)
#define SCL_HIGH		gpio_set_value(PIN_SCL, 1)
#define SCL_OUT			gpio_direction_output(PIN_SCL,1)

#define I2C_DELAY		udelay(5)
#define I2C_DELAY_LONG	udelay(100)

int InitGPIO(void)
{
		OMAP_GPIO_168();
		OMAP_GPIO_183();
//	s3c_gpio_setpull(PIN_SDA, S3C_GPIO_PULL_UP);
//    s3c_gpio_setpull(PIN_SCL, S3C_GPIO_PULL_UP);
//    s3c_gpio_setpull(SECRET_EN, S3C_GPIO_PULL_NONE);

    gpio_direction_output(PIN_SCL,1);
    gpio_direction_output(PIN_SDA,1);
//    gpio_direction_output(SECRET_EN,0);
    return 0;
}

void ReleaseGPIO(void)
{
    return;
}

void I2c_start(void)
{
    SDA_HIGH;
    I2C_DELAY;
    I2C_DELAY;
    SCL_HIGH;
    I2C_DELAY;
    SDA_LOW;
    I2C_DELAY;
    SCL_LOW;
    I2C_DELAY;
}

void I2c_nack_send(void)
{
		SDA_OUT;
    SDA_HIGH;
    I2C_DELAY;
    SCL_HIGH;
    I2C_DELAY;
    SCL_LOW;
    I2C_DELAY;
    SDA_LOW;
    I2C_DELAY;
}

void I2c_stop(void)
{
    SDA_LOW;
    I2C_DELAY;
    SCL_HIGH;
    I2C_DELAY;
    SDA_HIGH;
    I2C_DELAY_LONG;
}

unsigned char I2c_ack_detect(void)
{
	int count = 0;
    SDA_IN;	
    I2C_DELAY;
//    I2C_DELAY;
 //   I2C_DELAY;
//    I2C_DELAY;
//    udelay(300);
    while(count < 500)
    {
    	udelay(2);
    	count++;
    	if(!SDA_DETECT)
    		break;
    }
    if(count >= 500)
    	{
    		count = 0;
    		SDA_OUT;
    		return ERROR_CODE_FALSE; 
    	}
    SCL_HIGH;
    I2C_DELAY;
    if (SDA_DETECT)
    {
        SDA_OUT;
        return ERROR_CODE_FALSE; 
    }
    I2C_DELAY;
    SCL_LOW;
    SDA_OUT;
    return ERROR_CODE_TRUE; 
}

void I2c_ack_send(void)
{
    SDA_OUT;
    SDA_LOW;
    I2C_DELAY;
    SCL_HIGH;
    I2C_DELAY;
    SCL_LOW;
    I2C_DELAY;
}

unsigned char I2c_write_byte(unsigned char data)
{
    int i;
//		printk("===========data = 0x%x\n",data);
    for(i = 0; i< 8; i++)
    {
        if( (data << i) & 0x80) SDA_HIGH;
        else SDA_LOW;
        I2C_DELAY;
        SCL_HIGH;
        I2C_DELAY;
        SCL_LOW;
        I2C_DELAY;
    }
	
    if(I2c_ack_detect()!=ERROR_CODE_TRUE) {
        return ERROR_CODE_FALSE;
    }
    return ERROR_CODE_TRUE;
}

unsigned char I2c_read_byte(int byteno)
{
    int i;
    unsigned char data;

    data = 0;
    SDA_IN;
     udelay(350);
    for(i = 0; i< 8; i++){
        data <<= 1;
        I2C_DELAY;
        SCL_HIGH;
        I2C_DELAY;
        if (SDA_DETECT) data |= 0x01;
        SCL_LOW;
        I2C_DELAY;
    }
    if(byteno == nack_flag)
    	I2c_nack_send();
    else
    	I2c_ack_send();
    return data;
}

unsigned char I2c_write(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int i;

    I2c_start();
    I2C_DELAY;
    if(I2c_write_byte(device_addr)) {
        I2c_stop();
        return ERROR_CODE_WRITE_ADDR;
    }
    if(I2c_write_byte(sub_addr)) {
        I2c_stop();
        return ERROR_CODE_WRITE_ADDR;
    }
    for(i = 0; i<ByteNo; i++) {
        if(I2c_write_byte(buff[i])) {
            I2c_stop();
            return ERROR_CODE_WRITE_DATA;
        }
    }
    I2C_DELAY;
    I2c_stop();
    I2C_DELAY_LONG;
    return ERROR_CODE_TRUE;
}

unsigned char I2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
    int i;
    nack_flag = ByteNo - 1;
    I2c_start();
    I2C_DELAY;
    if(I2c_write_byte(device_addr)) {
        I2c_stop();
        printk("Fail to write device addr\n");
        return ERROR_CODE_READ_ADDR;
    }
    if(I2c_write_byte(sub_addr)) {
        I2c_stop();
        printk("Fail to write sub addr\n");
        return ERROR_CODE_READ_ADDR;
    }
    #if 1
//    printk("=========i2c_start===\n");
    I2c_start();
    I2C_DELAY;
    if(I2c_write_byte(device_addr+1)) {
        I2c_stop();
        printk("Fail to write device addr+1\n");
        return ERROR_CODE_READ_ADDR;
    }
    for(i = 0; i<ByteNo; i++) buff[i] = I2c_read_byte(i);
    I2C_DELAY;
    I2C_DELAY_LONG;
    I2c_stop();
    I2C_DELAY_LONG;
   #endif
   printk("buff[0] = %d,buff[1] = %d\n",buff[0],buff[1]);
    return ERROR_CODE_TRUE;
}

int PixI2cWrite(unsigned char device_addr, unsigned char sub_addr, void *ptr_data, unsigned int ui_data_length)
{
    unsigned char uc_return;
    if((uc_return = I2c_write(device_addr, sub_addr, (unsigned char*)ptr_data, ui_data_length))!=ERROR_CODE_TRUE)
    {
        printk("[Write]error code = %d\n",uc_return);
        return -1;
    }
    return 0;
}

int PixI2cRead(unsigned char device_addr, unsigned char sub_addr, void *ptr_data, unsigned int ui_data_length)
{
    unsigned char uc_return;
    if((uc_return=I2c_read(device_addr, sub_addr, (unsigned char*)ptr_data, ui_data_length))!=ERROR_CODE_TRUE)
    {
        printk("[Read]error code = %d\n",uc_return);
        return -1;
    }
    return 0;
}

void i2c2_init(void)
{
	  gpio_request(PIN_SDA,"SDA");
    gpio_request(PIN_SCL,"SCL");
   
		InitGPIO();
}

