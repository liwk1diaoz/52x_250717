#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include "nvt_sdk_ver.h"

static struct proc_dir_entry *sdkproc, *sdkverproc;


static int sdk_ver_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m,"SDK version: %01x.%02x.%03x \r\n", ((NVT_SDK_VER & 0xF00000) >> 20), ((NVT_SDK_VER & 0xFF000) >> 12), (NVT_SDK_VER & 0xFFF));
	return 0;
}

static int sdk_ver_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, sdk_ver_proc_show, NULL);
}
static const struct file_operations sdk_ver_proc_ops = {
	.owner = THIS_MODULE,
	.open = sdk_ver_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};


extern int nvtmpp_module_init(void);
extern int nvtmpp_module_exit(void);
extern int isf_flow_module_init(void);
extern int isf_flow_module_exit(void);
extern int nvt_gximage_module_init(void);
extern int nvt_gximage_module_exit(void);
extern int fastboot_module_init(void);
extern int fastboot_module_exit(void);

int __init kflow_common_init(void)
{
	int ret;

	printk("SDK version: %01x.%02x.%03x \r\n", ((NVT_SDK_VER & 0x00F00000) >> 20), ((NVT_SDK_VER & 0x000FF000) >> 12), (NVT_SDK_VER & 0x00000FFF));

	// create /proc/SDK/version node
	sdkproc = proc_mkdir("SDK", NULL);
	if (sdkproc == NULL) {
			return -EIO;
	}
	sdkverproc = proc_create("version", 0, sdkproc, &sdk_ver_proc_ops);
	if (sdkverproc == NULL) {
			return -EIO;
	}

	if ((ret=nvtmpp_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=isf_flow_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_gximage_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=fastboot_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kflow_common_exit(void)
{
	if (sdkverproc != 0) {
		remove_proc_entry("version", sdkproc);
	}
	if (sdkproc != 0) {
		remove_proc_entry("SDK", NULL);
	}
	fastboot_module_exit();
	nvt_gximage_module_exit();
	isf_flow_module_exit();
	nvtmpp_module_exit();
}

module_init(kflow_common_init);
module_exit(kflow_common_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kflow_common driver");
MODULE_LICENSE("GPL");

#else
#endif