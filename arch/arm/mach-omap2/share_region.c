#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <asm/io.h>
#include <linux/crc32.h>
#include <linux/share_region.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

struct share_region_t *g_share_region = NULL;
extern int boot;

void get_share_region(void)
{
	unsigned int mem_size = (num_physpages >> (20 - PAGE_SHIFT));
	unsigned int __iomem *share_region_mem=NULL;
	unsigned int *pMme=NULL;
	int i;	

	g_share_region = (struct share_region_t *)kmalloc(SHARE_REGION_SIZE, GFP_KERNEL);
	memset(g_share_region, 0, SHARE_REGION_SIZE);
	
	if (mem_size < 256)
		return;
	
	if (mem_size > 256)
		share_region_mem = ioremap(SHARE_REGION_BASE, SHARE_REGION_SIZE);
	else   
		share_region_mem = ioremap(MEM_256MB_SHARE_REGION_BASE, SHARE_REGION_SIZE);

	if (share_region_mem == NULL) {
		printk("ioremap(SHARE_REGION_BASE, SHARE_REGION_SIZE) fail\n");
		return;
	}
	pMme = (unsigned int *)g_share_region;
	for (i=0 ; i<(SHARE_REGION_SIZE/sizeof(unsigned int)) ; i++) {
		pMme[i] = readl(share_region_mem+i);
	}
	iounmap(share_region_mem);

	printk("hw=%d,sw=%s\n", g_share_region->hardware_id, g_share_region->software_version);
}

EXPORT_SYMBOL(get_share_region);

int ep_get_hardware_id(void)
{
	return g_share_region->hardware_id;
}

EXPORT_SYMBOL(ep_get_hardware_id);

int ep_get_software_version(char *ver, const int len)
{
	if (!((g_share_region->software_version[0] == 'F' &&
	       g_share_region->software_version[1] == 'B')||
		  (g_share_region->software_version[0] == 'f' &&
	       g_share_region->software_version[1] == 'b')))
	{
		sprintf(ver, "FB00000000\0");
		return -1;	
	}
	memcpy(ver, g_share_region->software_version, len);
	ver[(len-1)] = '\0';

	return 0;
}

EXPORT_SYMBOL(ep_get_software_version);

int ep_get_debug_flag(void)
{
	return (g_share_region->debug_flag);
}

EXPORT_SYMBOL(ep_get_debug_flag);

int ep_get_3G_exist(void)
{
	   
	   return (g_share_region->status_3G_exist);
    
}

EXPORT_SYMBOL(ep_get_3G_exist);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct upgrade_mem_t *upgrade_mem = NULL;
//upgrade_mem_t * const upgrade_mem = (upgrade_mem_t *)UPGRADE_MEM_BASE; 
void clear_flags(void)
{
        upgrade_mem->flags.boot_flag = 0;
        upgrade_mem->flags.update_flag = 0;
}

static unsigned bg_crc32(unsigned char *buf, int len)
{
        return ~crc32_le(~0L, buf ,len);
}

static void save_upgrade_mem(void)
{
        if(upgrade_mem){
                 upgrade_mem->checksum = bg_crc32((u8 *)upgrade_mem, UPGRADE_MEM_SIZE - sizeof(unsigned long));
        }
}

static int check_upgrade_mem(void)
{
        int result = 0;

        if(upgrade_mem){
                unsigned long computed_residue, checksum;
                checksum = upgrade_mem->checksum;
                computed_residue = ~bg_crc32((unsigned char *) upgrade_mem, UPGRADE_MEM_SIZE);
                result = CRC32_RESIDUE == computed_residue;
        }
        if(result){
                printk("upgrade_mem: passed\n");
        }else{
                printk("upgrade_mem: failed\n");
        }
        return result;
}

static unsigned long get_kernel_update_flag(void)
{
	printk("%d\n", upgrade_mem->flags.update_flag);
        return upgrade_mem->flags.update_flag;
}

static void set_kernel_update_flag(unsigned long flag)
{
        if(flag > 4)
                return;
        upgrade_mem->flags.update_flag = flag;
		printk("***********update_flag:%d*************\n", upgrade_mem->flags.update_flag);
        save_upgrade_mem();
}

 static const flag_cb flag_proc[] = {
        {"update_flag", get_kernel_update_flag, set_kernel_update_flag},
};

static int proc_flag_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        const struct flag_cb *p = data;
        *eof = 1;
        if(p && p->flag_get){
                return snprintf(page, PAGE_SIZE, "%lu\n", p->flag_get());
        }else{
                return -EIO;
        }
}

static int proc_flag_write(struct file *file, const char __user *buffer, unsigned long count, void *data)
{       
	const struct flag_cb *p = data;
        char buf[16];
        if (count > sizeof(buf)-1)
                return -EINVAL;
        if(!count)
                return 0;
        if(copy_from_user(buf, buffer, count))
                return -EFAULT;
        buf[count] = '\0';
        if(p && p->flag_set){
                switch (buf[0]) {
                case '0':
                        p->flag_set(0x0);
                        break;
                case '1':
                        p->flag_set(0x1);
                        break;
                case '2':
                        p->flag_set(0x2);
                        break;
                case '3':
                        p->flag_set(0x3);
                        break;
				case '4':
						p->flag_set(0x4);
						break;
				default:
                        break;
                }
                return count;
        }else{
                return -EIO;
        }
}

void __init configure_upgrade_mem(void)
{
        upgrade_mem = ioremap(UPGRADE_MEM_BASE, UPGRADE_MEM_SIZE);

        if(!upgrade_mem){
                printk("upgrade_mem kernel: fail to reserve, quit!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
        }
        if(!check_upgrade_mem()){
                printk("upgrade_mem kernel: invalid ==|> clearing the flags!$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
                clear_flags();
        }
}
int __init upgrade_mem_init(void)
{
        unsigned i;
        struct proc_dir_entry *dir;

        configure_upgrade_mem();
		if(upgrade_mem->flags.update_flag != 0)
		{ 
		  boot=1;
		}
		//printk("andy *******************update_flag=%d,boot=%d\n",upgrade_mem->flags.update_flag,boot);
        dir = proc_mkdir("upgrade_mem", NULL);
        if(!dir){
                printk("could not create /porc/upgrade_mem\n");
                return 0;
        }

        for(i = 0; i < (sizeof (flag_proc) /sizeof(*flag_proc)); i++){
                struct proc_dir_entry *tmp;
                mode_t mode;
                mode = 0;
                if(flag_proc[i].flag_get){
                        mode |= S_IRUGO;
                }
                if(flag_proc[i].flag_set){
                        mode |= S_IWUGO;
                }

                tmp = create_proc_entry(flag_proc[i].path, mode ,dir);
                if(tmp){
                        tmp->data = (void *)&flag_proc[i];
                        tmp->write_proc = proc_flag_write;
                        tmp->read_proc = proc_flag_read;
                }
                else{
                                                                                                                                                                                                         printk(KERN_WARNING "share region: could not  create /proc/upgrade_mem/%s\n", flag_proc[i].path);
                }
        }
		printk("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<upgrade_mem_update_flage:%08x\n", upgrade_mem->flags.update_flag);
		printk("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<upgrade_mem_boot_flage:%08x\n", upgrade_mem->flags.boot_flag);
		printk("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<checksum:%08x\n", upgrade_mem->checksum);
        return 1;

}

late_initcall(upgrade_mem_init);

