#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/share_region.h>

static int modem_3g_proc_ep_show(struct seq_file *m, void *v)
{
	unsigned int value = ep_get_3G_exist();
	
	seq_printf(m, "%d", value);
	
	return 0;
}

static int modem_3g_proc_ep_open(struct inode *inode, struct file *file)
{
	return single_open(file, modem_3g_proc_ep_show, NULL);
}

static const struct file_operations modem_3g_proc_ep_fops = {
	.open		= modem_3g_proc_ep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_modem_3g_ep_init(void)
{
	proc_create("ep_3g_modem", 0, NULL, &modem_3g_proc_ep_fops);
	return 0;
}
module_init(proc_modem_3g_ep_init);
