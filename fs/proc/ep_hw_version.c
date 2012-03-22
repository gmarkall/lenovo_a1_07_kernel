#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/share_region.h>

static int hardware_version_proc_ep_show(struct seq_file *m, void *v)
{
	int board_id;

	board_id = ep_get_hardware_id();
	switch(board_id)
	{ 
	/* <--LH_SWRD_CL1_Mervins@2011.06.28:for hardware version. */
		case BOARD_ID_PRO:	seq_printf(m, "PRO");  break; 
		case BOARD_ID_EVT:	seq_printf(m, "EVT"); break;
		case BOARD_ID_DVT1: seq_printf(m, "DVT1"); break;
		case BOARD_ID_WIFI_DVT2: seq_printf(m, "WIFI_DVT2"); break;
		case BOARD_ID_WIFI_PVT: seq_printf(m, "WIFI_PVT"); break;
		case BOARD_ID_WIFI_MP: seq_printf(m, "WIFI_MP"); break;
		case BOARD_ID_3G_DVT2: seq_printf(m, "3G_DVT2"); break;
		case BOARD_ID_3G_PVT:  seq_printf(m, "3G_PVT");  break;
		case BOARD_ID_3G_MP:   seq_printf(m, "3G_MP");   break; 
	/* LH_SWRD_CL1_Mervins@2011.06.28--> */
		default:
		case BOARD_VERSION_UNKNOWN:
			seq_printf(m, "UNKNOWN");
			break;
	}	
	return 0;
}

static int hardware_version_proc_ep_open(struct inode *inode, struct file *file)
{
	return single_open(file, hardware_version_proc_ep_show, NULL);
}

static const struct file_operations hardware_version_proc_ep_fops = {
	.open		= hardware_version_proc_ep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_hardware_version_ep_init(void)
{
	proc_create("ep_hw_version", 0, NULL, &hardware_version_proc_ep_fops);
	return 0;
}
module_init(proc_hardware_version_ep_init);
