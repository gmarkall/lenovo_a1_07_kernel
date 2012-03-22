#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/share_region.h>

static int version_proc_ep_show(struct seq_file *m, void *v)
{
	char version[50];
	
	ep_get_software_version(version, sizeof(version));
	version[48] = '\0';
	seq_printf(m, "%s", version);
	
	return 0;
}

static int version_proc_ep_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_proc_ep_show, NULL);
}

static const struct file_operations version_proc_ep_fops = {
	.open		= version_proc_ep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_version_ep_init(void)
{
	proc_create("ep_sw_version", 0, NULL, &version_proc_ep_fops);
	return 0;
}
module_init(proc_version_ep_init);
