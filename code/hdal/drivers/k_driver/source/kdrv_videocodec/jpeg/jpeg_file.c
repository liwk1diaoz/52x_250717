#if defined __KERNEL__
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <kwrap/file.h>
#endif
#include "jpeg_file.h"


#if 1   // file system API wrapper

FST_FILE filesys_openfile(char *filename, UINT32 flag)
{
	//struct file *fp;
	int fp;
	int fs_flg = 0;

	if (flag & FST_OPEN_READ) {
		fs_flg |=  O_RDONLY;
	}
	if (flag & FST_OPEN_WRITE) {
		fs_flg |= O_WRONLY;
	}
	if (flag & FST_OPEN_ALWAYS) {
		fs_flg |= O_CREAT;
	}

	//fp = filp_open(filename, fs_flg, 0);
	fp = vos_file_open(filename, fs_flg, 0);

	//printk("file %s flg 0x%x flag 0x%x open result 0x%p\r\n",
	//          filename, fs_flg, flag, fp);

	//if (IS_ERR_OR_NULL(fp)) {
	if (-1 == fp) {
		return NULL;
	} else {
		return (FST_FILE)fp;
	}

}

INT32 filesys_closefile(FST_FILE pfile)
{
	INT32 ret = 0;

	//ret = filp_close((struct file *)pfile, NULL);
	vos_file_close((int)pfile);

	//printk("%s: close 0x%p, result %d\r\n", __func__, pFile, ret);

	return ret;
}

INT32 filesys_readfile(FST_FILE pfile, UINT8 *pbuf, UINT32 *pbufsize, UINT32 flag, INT32 *CB)
{
	mm_segment_t old_fs;
	INT32 ret = 0;
	INT32 size = *pbufsize;
	//loff_t offset = 0;

	//printk("fd 0x%p read length %d, buf 0x%p\r\n",
	//      pFile, size, pBuf);

#if 0
	pbuffer = kmalloc(size, GFP_KERNEL);
	printk("kmalloc get 0x%p\r\n", pbuffer);
	if (pbuffer == NULL) {
		printk("kalloc %d fail\r\n", (int)(size));
		return -ENOMEM;
	}
#endif
	old_fs = get_fs();
	set_fs(get_ds());

	//ret = vfs_read((struct file *)pfile, pbuf, size, &offset);
	size = vos_file_read((int)pfile, pbuf, size);

	set_fs(old_fs);

	//printk("file read length %d\r\n", (int)(ret));

	if (ret < 0) {
	} else {
		*pbufsize = ret;
		ret = 0;
//		memcpy(pBuf, pbuffer, size);
	}

//	kfree(pbuffer);

	return ret;
}


INT32 filesys_readcontfile(FST_FILE pfile, UINT8 *pbuf, UINT32 *pbufsize, UINT32 flag, INT32 *CB, loff_t offset)
{
	mm_segment_t old_fs;
	INT32 ret=0;
	INT32 size = *pbufsize;
	//loff_t offset;

	//printk("fd 0x%p read length %d, buf 0x%p\r\n",
	//      pFile, size, pBuf);

#if 0
	pbuffer = kmalloc(size, GFP_KERNEL);
	printk("kmalloc get 0x%p\r\n", pbuffer);
	if (pbuffer == NULL) {
		printk("kalloc %d fail\r\n", (int)(size));
		return -ENOMEM;
	}
#endif
	old_fs = get_fs();
	set_fs(get_ds());

	//ret = vfs_read((struct file *)pfile, pbuf, size, &offset);
	size = vos_file_read((int)pfile, pbuf, size);

	set_fs(old_fs);

	//printk("file read length %d\r\n", (int)(ret));

	if (ret < 0) {
	} else {
		*pbufsize = ret;
		ret = 0;
//		memcpy(pBuf, pbuffer, size);
	}

//	kfree(pbuffer);

	return ret;
}

#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(filesys_openfile);
EXPORT_SYMBOL(filesys_closefile);
EXPORT_SYMBOL(filesys_readfile);
EXPORT_SYMBOL(filesys_readcontfile);
#endif
