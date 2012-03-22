/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/share_region.h>
#include <linux/delay.h>

static int android_factory_default_reboot_call(struct notifier_block *this,
			unsigned long code, void *cmd)
{
#if 0		
	struct file *filp;
	mm_segment_t fs;
	char *data = "CMD:recovery";
	
	if ((cmd != NULL) && (code == SYS_RESTART)) {
		/* set cold reset */
		if (strncmp(cmd, "recovery", 8) == 0) {
			printk("[android_factory_default_reboot_call]\n");
			sys_chmod("/dev/block/mmcblk0", 0666);
			filp = filp_open("/dev/block/mmcblk0", O_RDWR | O_SYNC | O_TRUNC, 0666);
			if(IS_ERR(filp)) {
				printk("filp_open error...\n");
      	return NOTIFY_DONE;
   		}
   		printk("Write CMD to eMMC ++\n");
			fs=get_fs();
  		set_fs(KERNEL_DS);
  		filp->f_op->llseek(filp, 0x2E00200, SEEK_SET);
  		filp->f_op->write(filp, data, strlen(data),&filp->f_pos);
  		set_fs(fs);
  		filp_close(filp,NULL);
  		printk("Write CMD to eMMC --\n");
		}
	}
#else
	if ((code == SYS_RESTART) && (cmd)) {
		/* set cold reset */
		if (strncmp(cmd, "recovery", 8) == 0) {
			printk("reboot to recovery ...\n");
//			set_recovery_flag();			
			mdelay(100);
		}
	}
#endif
	return NOTIFY_DONE;
}

static struct notifier_block android_factory_default_reboot_notifier = {
	.notifier_call = android_factory_default_reboot_call,
};

void android_factory_default_init(void)
{
	register_reboot_notifier(&android_factory_default_reboot_notifier);
}
