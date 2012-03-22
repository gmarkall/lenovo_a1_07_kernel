#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/io.h>

#include <linux/svnversion.h>

static int svnversion_show(struct seq_file *m, void *v)
{
if(strlen(SVNVERSION)==4)
    seq_printf(m, "%s\n", SVNVERSION);
else
    seq_printf(m, "0%s\n", SVNVERSION);
	
	return 0;
}

static int svnversion_open(struct inode *inode, struct file *file)
{
	return single_open(file, svnversion_show, NULL);
}

static const struct file_operations svnversion_proc_fops = {
	.open		= svnversion_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_svnversion_init(void)
{
	proc_create("svnversion", 0, NULL, &svnversion_proc_fops);
	return 0;
}
module_init(proc_svnversion_init);
