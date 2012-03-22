/*
 * Generic low battery process
 * This version variable for low battery process calling
 *
 * Copyright (C) 1999 Hewlett-Packard Co.
 * Copyright (C) 1999 Stephane Eranian <eranian@hpl.hp.com>
 */ 
  #include <plat/io.h>
 
 int get_polling_check_low_battery_flag_android(void);
 int get_polling_check_low_battery_flag_myself(void);
 int set_polling_check_low_battery_flag_android(int value);
 int set_polling_check_low_battery_flag_myself(int value);
 
 int get_alarm_time_for_low_battery(void);
 
extern int i2c_read(char chip, uint addr, int alen, char * buffer, int len);
extern int i2c_write(char chip, uint addr, int alen, char * buffer, int len);
extern void i2c_init(int speed, int slaveadd);
extern int select_bus(int bus, int speed);
 
 #ifndef _OMAP34XX_I2C_H_
#define _OMAP34XX_I2C_H_

/* Get the i2c base addresses */

//#define I2C_BASE1 OMAP2_L4_IO_ADDRESS(0x48070000)
//#define I2C_BASE2 OMAP2_L4_IO_ADDRESS(0x48072000)
//#define I2C_BASE3 OMAP2_L4_IO_ADDRESS(0x48060000)

#define I2C_BASE1 (0x48070000)
#define I2C_BASE2 (0x48072000)
#define I2C_BASE3 (0x48060000)

#define CFG_I2C_SLAVE 0x1
#define CFG_I2C_SLAVE_2 0x2
#define CFG_I2C_SPEED 100
#define CFG_I2C_SPEED_2 400

#define I2C_DEFAULT_BASE I2C_BASE1

#define I2C_REV                 (0x00)
#define I2C_IE                  (0x04)
#define I2C_STAT                (0x08)
#define I2C_IV                  (0x0c)
#define I2C_SYSS                (0x10)
#define I2C_BUF                 (0x14)
#define I2C_CNT                 (0x18)
#define I2C_DATA                (0x1c)
#define I2C_SYSC                (0x20) 
#define I2C_CON                 (0x24)
#define I2C_OA                  (0x28)
#define I2C_OA_1                (0x44)
#define I2C_SA                  (0x2c)
#define I2C_PSC                 (0x30)
#define I2C_SCLL                (0x34)
#define I2C_SCLH                (0x38)
#define I2C_SYSTEST             (0x3c)

/* I2C masks */

/* I2C Interrupt Enable Register (I2C_IE): */
#define I2C_IE_GC_IE    (1 << 5)
#define I2C_IE_XRDY_IE  (1 << 4)        /* Transmit data ready interrupt enable */
#define I2C_IE_RRDY_IE  (1 << 3)        /* Receive data ready interrupt enable */
#define I2C_IE_ARDY_IE  (1 << 2)        /* Register access ready interrupt enable */
#define I2C_IE_NACK_IE  (1 << 1)        /* No acknowledgment interrupt enable */
#define I2C_IE_AL_IE    (1 << 0)        /* Arbitration lost interrupt enable */

/* I2C Status Register (I2C_STAT): */

#define I2C_STAT_SBD    (1 << 15)       /* Single byte data */
#define I2C_STAT_BB     (1 << 12)       /* Bus busy */
#define I2C_STAT_ROVR   (1 << 11)       /* Receive overrun */
#define I2C_STAT_XUDF   (1 << 10)       /* Transmit underflow */
#define I2C_STAT_AAS    (1 << 9)        /* Address as slave */
#define I2C_STAT_GC     (1 << 5)
#define I2C_STAT_XRDY   (1 << 4)        /* Transmit data ready */
#define I2C_STAT_RRDY   (1 << 3)        /* Receive data ready */
#define I2C_STAT_ARDY   (1 << 2)        /* Register access ready */
#define I2C_STAT_NACK   (1 << 1)        /* No acknowledgment interrupt enable */
#define I2C_STAT_AL     (1 << 0)        /* Arbitration lost interrupt enable */


/* I2C Interrupt Code Register (I2C_INTCODE): */

#define I2C_INTCODE_MASK        7
#define I2C_INTCODE_NONE        0
#define I2C_INTCODE_AL          1       /* Arbitration lost */
#define I2C_INTCODE_NAK         2       /* No acknowledgement/general call */
#define I2C_INTCODE_ARDY        3       /* Register access ready */
#define I2C_INTCODE_RRDY        4       /* Rcv data ready */
#define I2C_INTCODE_XRDY        5       /* Xmit data ready */

/* I2C Buffer Configuration Register (I2C_BUF): */

#define I2C_BUF_RDMA_EN         (1 << 15)       /* Receive DMA channel enable */
#define I2C_BUF_XDMA_EN         (1 << 7)        /* Transmit DMA channel enable */

/* I2C Configuration Register (I2C_CON): */

#define I2C_CON_EN      (1 << 15)       /* I2C module enable */
#define I2C_CON_BE      (1 << 14)       /* Big endian mode */
#define I2C_CON_STB     (1 << 11)       /* Start byte mode (master mode only) */
#define I2C_CON_MST     (1 << 10)       /* Master/slave mode */
#define I2C_CON_TRX     (1 << 9)        /* Transmitter/receiver mode (master mode only) */
#define I2C_CON_XA      (1 << 8)        /* Expand address */
#define I2C_CON_STP     (1 << 1)        /* Stop condition (master mode only) */
#define I2C_CON_STT     (1 << 0)        /* Start condition (master mode only) */

/* I2C System Test Register (I2C_SYSTEST): */

#define I2C_SYSTEST_ST_EN       (1 << 15)       /* System test enable */
#define I2C_SYSTEST_FREE        (1 << 14)       /* Free running mode (on breakpoint) */
#define I2C_SYSTEST_TMODE_MASK  (3 << 12)       /* Test mode select */
#define I2C_SYSTEST_TMODE_SHIFT (12)            /* Test mode select */
#define I2C_SYSTEST_SCL_I       (1 << 3)        /* SCL line sense input value */
#define I2C_SYSTEST_SCL_O       (1 << 2)        /* SCL line drive output value */
#define I2C_SYSTEST_SDA_I       (1 << 1)        /* SDA line sense input value */
#define I2C_SYSTEST_SDA_O       (1 << 0)        /* SDA line drive output value */

/* I2C System Control Register (I2C_SYSC): */

#define I2C_SYSC_SRST           (1 << 1)        /* Software Reset */

/* I2C System Status Register (I2C_SYSS): */

#define I2C_SYSS_RDONE          (1 << 0)        /* Internel reset monitoring */


#define I2C_SCLL_SCLL        (0)
#define I2C_SCLL_SCLL_M      (0xFF)
#define I2C_SCLL_HSSCLL      (8)
#define I2C_SCLH_HSSCLL_M    (0xFF)
#define I2C_SCLH_SCLH        (0)
#define I2C_SCLH_SCLH_M      (0xFF)
#define I2C_SCLH_HSSCLH      (8)
#define I2C_SCLH_HSSCLH_M    (0xFF)

#define OMAP_I2C_STANDARD          100
#define OMAP_I2C_FAST_MODE         400
#define OMAP_I2C_HIGH_SPEED        3400

#define SYSTEM_CLOCK_12       12000
#define SYSTEM_CLOCK_13       13000
#define SYSTEM_CLOCK_192      19200
#define SYSTEM_CLOCK_96       96000

#define I2C_IP_CLK SYSTEM_CLOCK_96

#ifndef I2C_IP_CLK
#define I2C_IP_CLK			SYSTEM_CLOCK_96
#endif

/* Boards may want to define I2C_INTERNAL_SAMPLING_CLK */

/* These are the trim values for standard and fast speed */
#ifndef I2C_FASTSPEED_SCLL_TRIM
#define I2C_FASTSPEED_SCLL_TRIM			7
#endif
#ifndef I2C_FASTSPEED_SCLH_TRIM
#define I2C_FASTSPEED_SCLH_TRIM			5
#endif

/* These are the trim values for high speed */
#ifndef I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM
#define I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM	I2C_FASTSPEED_SCLL_TRIM
#endif
#ifndef I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM
#define I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM	I2C_FASTSPEED_SCLH_TRIM
#endif
#ifndef I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM
#define I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM	I2C_FASTSPEED_SCLL_TRIM
#endif
#ifndef I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM
#define I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM	I2C_FASTSPEED_SCLH_TRIM
#endif

#define I2C_PSC_MAX          (0x0f)
#define I2C_PSC_MIN          (0x00)

#endif

extern int i2c_read(char chip, uint addr, int alen, char * buffer, int len);
extern int i2c_write(char chip, uint addr, int alen, char * buffer, int len);
extern void i2c_init(int speed, int slaveadd);

int compare_android_alarm_to_myself_alarm(void);

 void i2c2_init(void);
 unsigned char I2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo);

 