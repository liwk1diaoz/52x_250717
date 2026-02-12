#include "affine_reg.h"
#include "emu_affine.h"


#define AUTOTEST_AFFINE		ENABLE
#define AUTOTEST_THREAD		ENABLE

// file system API wrapper
#if defined(__FREERTOS)
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include <kwrap/cmdsys.h>

#include <malloc.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static FST_FILE FileSys_OpenFile(char *filename, UINT32 flag)
{
	int fd;
	int fs_flg = 0;
	char *str_buf;
	const int BUF_SIZE = 256;

	str_buf = (char *)malloc(BUF_SIZE);
	if (str_buf == NULL)
		return NULL;

        if (flag & FST_OPEN_READ) {
                fs_flg |=  O_RDONLY;
        }
        if (flag & FST_OPEN_WRITE) {
                fs_flg |= O_WRONLY;
        }
        if (flag & FST_OPEN_ALWAYS) {
                fs_flg |= O_CREAT;
        }

	snprintf(str_buf, BUF_SIZE, "/mnt/sd/%s", filename);
	printf("%s: path: %s\r\n", __func__, str_buf);

	fd = open(str_buf, flag, 666);

	printf("file %s flg 0x%x flag 0x%x open result 0x%x\r\n",
			filename, fs_flg, (unsigned int)flag, fd);

	free(str_buf);

	if (fd == -1)
		return NULL;
	else
		return (FST_FILE)fd;
}

static INT32 FileSys_CloseFile(FST_FILE file)
{
	INT32 ret;

	ret = close((int)file);

	printf("%s: result %d\r\n", __func__, (int)ret);

	return ret;
}


static INT32 FileSys_ReadFile(FST_FILE file, UINT8 *p_buf, UINT32 *p_buf_size, UINT32 flag, INT32 *p_callbck)
{
	INT32 ret;
	INT32 size = *p_buf_size;

	auto_msg(("fd 0x%p read length %d, buf 0x%p\r\n",
				file, (int)size, p_buf));

	ret = read((int)file, p_buf, size);

	auto_msg(("file read length %d\r\n", (int)ret));

	if (ret < 0) {
	} else {
		*p_buf_size = ret;
		ret = 0;
	}

	return ret;
}

#if (AUTOTEST_THREAD == ENABLE)
static UINT32 FileSys_GetFileLen(char *filename)
{
#if 0
	char *str_buf;
	const int BUF_SIZE = 256;

	struct stat  static;

	snprintf(str_buf, BUF_SIZE, "/mnt/sd/%s", filename);
	printf("%s: path: %s\r\n", __func__, str_buf);

	if(stat(filename, &static) < 0)
		return 0;

	return (UINT32)static.st_size;
#endif
	return 0;
}
#endif

#else	// LINUX
// file system API wrapper
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include <kwrap/file.h>
#include <linux/delay.h>
//#include <math.h>
#include <linux/string.h>
#include <linux/kernel.h>

static FST_FILE FileSys_OpenFile(char *filename, UINT32 flag)
{
	int fd;
	int fs_flg = 0;

	if (flag & FST_OPEN_READ) {
		fs_flg |= O_RDONLY;
	}
	if (flag & FST_OPEN_WRITE) {
		fs_flg |= O_WRONLY;
	}
	if (flag & FST_OPEN_ALWAYS) {
		fs_flg |= O_CREAT;
	}
	if (flag & FST_CREATE_ALWAYS) {
		fs_flg |= O_CREAT;
		fs_flg |= O_TRUNC;
	}


	fd = vos_file_open(filename, fs_flg, 0);

	auto_msg(("file %s flg 0x%x flag 0x%x open result 0x%x\r\n",
				filename, (unsigned int)fs_flg, (unsigned int)flag, fd));

	if (-1 == fd)
		return NULL;
	else
		return (FST_FILE)fd;
}

static INT32 FileSys_CloseFile(FST_FILE file)
{
	INT32 ret;

	ret = vos_file_close((int)file);

	auto_msg(("%s: close 0x%p, result %d\r\n", __func__, file, (int)ret));

	return ret;
}

#if (READ_CORRECT == 0)
static INT32 FileSys_WriteFile(FST_FILE file, UINT8 *p_buf, UINT32 *p_buf_size, UINT32 flag, INT32 *p_callbck, UINT32 StartOfs)
{
	INT32 ret;
	INT32 size = *p_buf_size;
	int write_size;

	auto_msg(("fd 0x%p write length %d, buf 0x%p\r\n",
			file, (int)size, p_buf));

#if 0
	pbuffer = kmalloc(size, GFP_KERNEL);
	printk("kmalloc get 0x%p\r\n", pbuffer);
        if (pbuffer == NULL) {
		printk("kalloc %d fail\r\n", size);
                return -ENOMEM;
        }
#endif

	write_size = vos_file_write((int)file, p_buf, size);

	auto_msg(("file write length %d\r\n", (int)ret));

	if (ret < 0) {
	} else {
		*p_buf_size = ret;
		ret = 0;
//		memcpy(pBuf, pbuffer, size);
	}

//	kfree(pbuffer);

	return ret;
}

#else
static INT32 FileSys_ReadFile(FST_FILE file, UINT8 *p_buf, UINT32 *p_buf_size, UINT32 flag, INT32 *p_callbck)
{
	INT32 ret;
	INT32 size = *p_buf_size;

	auto_msg(("fd 0x%p read length %d, buf 0x%p\r\n",
			file, (int)size, p_buf));

#if 0
	pbuffer = kmalloc(size, GFP_KERNEL);
	printk("kmalloc get 0x%p\r\n", pbuffer);
	if (pbuffer == NULL) {
		printk("kalloc %d fail\r\n", size);
		return -ENOMEM;
	}
#endif

	ret = vos_file_read((int)file, p_buf, size);

	auto_msg(("file read length %d\r\n", (int)ret));

	if (ret < 0) {
	} else {
		*p_buf_size = ret;
		ret = 0;
//		memcpy(p_buf, pbuffer, size);
	}

//	kfree(pbuffer);

//	affine_platform_dma_flush_dev2mem((UINT32)p_buf, *p_buf_size);

	return ret;
}

#if (AUTOTEST_THREAD == ENABLE)
static UINT32 FileSys_GetFileLen(char *filename)
{
	struct kstat  stat = {0};

	if(vfs_stat(filename, &stat) < 0)
		return 0;

	return (UINT32)stat.size;
}
#endif

#endif
#endif
static UINT32 LoadImage(char *Fn, UINT8 *Dest, UINT32 fsize)
{
	INT32 status;
	FST_FILE fileHdl;

	fileHdl = FileSys_OpenFile(Fn, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, Fn));
		return 0;
	}
	status = FileSys_ReadFile(fileHdl, (UINT8 *)Dest, &fsize, AFFINE_AUTO_FS_MODE, NULL);
	FileSys_CloseFile(fileHdl);

	if (status != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, Fn));
		return 0;
	}

	return fsize;
}

#if (AUTOTEST_AFFINE == ENABLE)

#if (AUTOTEST_THREAD == ENABLE)
typedef struct  affreg_t {
	UINT32      ctrl_00;
	UINT32      config_04;
	UINT32      inten_08;
	UINT32      intsts_0C;
	UINT32      coeff_a_10;
	UINT32      coeff_b_14;
	UINT32      coeff_c_18;
	UINT32      coeff_d_1C;
	UINT32      coeff_e_20;
	UINT32      coeff_f_24;
	UINT32      subblk1_28;
	UINT32      subblk2_2C;
	UINT32      src_imgaddr_30;
	UINT32      src_off_34;
	UINT32      imgheight_38;
	UINT32      imgwidth_3C;
	UINT32      dst_imgaddr_40;
	UINT32      dst_off_44;
	UINT32      src_fetchaddr_1000;

	UINT32      TestId;
} AFF_REG;

static void RegParser(CHAR *pBuf, AFF_REG *gpm, UINT32 uiBufSize)
{
	CHAR *pData;
	UINT32 tmpData;
	UINT32 uiOfs = 0;

#if defined(__FREERTOS)
	for (pData = strtok(pBuf, "= \t\n"); pData; pData = strtok(NULL, "= \t\n")) {
		if (sscanf(pData, "%04x%08x", (unsigned int*)&uiOfs, (unsigned int*)&tmpData) != 2) {
			continue;
		}
#else
	CHAR s_tmpData[] = "00000000";
	CHAR s_Ofs[] = "0000";

	for(pData = strsep(&pBuf, "= \t\n"); pData != NULL; pData = strsep(&pBuf, "\t\n")) {
		if (sscanf(pData, "%04s%08s", s_Ofs, s_tmpData) != 2) {
			continue;
		}

		tmpData = 0;
		kstrtoul(s_Ofs, 16, (unsigned long*)&uiOfs);
		kstrtoul(s_tmpData, 16, (unsigned long*)&tmpData);
#endif

		switch (uiOfs) {
		case 0x00:
			gpm->ctrl_00 = tmpData;             // unused
			break;
		case 0x04:
			gpm->config_04 = tmpData;
			break;
		case 0x08:
			gpm->inten_08 = tmpData;            // unused
			break;
		case 0x0C:
			gpm->intsts_0C = tmpData;           // unused
			break;
		case 0x10:
			gpm->coeff_a_10 = tmpData;
			break;
		case 0x14:
			gpm->coeff_b_14 = tmpData;
			break;
		case 0x18:
			gpm->coeff_c_18 = tmpData;
			break;
		case 0x1C:
			gpm->coeff_d_1C = tmpData;
			break;
		case 0x20:
			gpm->coeff_e_20 = tmpData;
			break;
		case 0x24:
			gpm->coeff_f_24 = tmpData;
			break;
		case 0x28:
			gpm->subblk1_28 = tmpData;
			break;
		case 0x2C:
			gpm->subblk2_2C = tmpData;
			break;
		case 0x30:
			gpm->src_imgaddr_30 = tmpData;
			break;
		case 0x34:
			gpm->src_off_34 = tmpData;
			break;
		case 0x38:
			gpm->imgheight_38 = tmpData;
			break;
		case 0x3C:
			gpm->imgwidth_3C = tmpData;
			break;
		case 0x40:
			gpm->dst_imgaddr_40 = tmpData;
			break;
		case 0x44:
			gpm->dst_off_44 = tmpData;
			break;
		case 0x1000:
			gpm->src_fetchaddr_1000 = tmpData;
			break;
		default:
			break;
		}
		//auto_msg(("pData:%s\r\n", pData));
		//auto_msg(("uiOfs:%04x  tmpData:%08x\r\n", uiOfs, tmpData));
	}

}

UINT32 uiGrphBaseBuf;
UINT32 affine_0_addr = 0;
UINT32 affine_0_size = 0;
static INT32 RunAffTest(AFF_REG *affReg, KDRV_CALLBACK_FUNC *p_cb_func)
{
#if 1
	char in1[80], out[80], err[80];
	AFFINE_ID test_affine_id = AFFINE_ID_1;
	UINT32 ADDR, temp1;
	UINT32 fsize = 0;
	UINT32 uiGrphBaseBuf = (affine_0_addr + 0xFF) & ~0xFF;
	UINT32 /*uiSrcSize,*/ uiDstSize, uiMaxSize, uiSrcPrimeSize;
	UINT32 uiDstSrcAddrDiff;
	UINT32 uiSrcAddrByteOfs = 0;
	INT32  iSrcAddrDiff;

	sprintf(in1, "/mnt/sd/Pattern/Affine/%03d_in.raw", (INT) affReg->TestId);
	sprintf(out, "/mnt/sd/Pattern/Affine/%03dout.raw", (INT) affReg->TestId);
	sprintf(err, "/mnt/sd/Pattern/Affine/%03d_err.raw", (INT) affReg->TestId);

	if (affReg->src_imgaddr_30 > affReg->src_fetchaddr_1000) {
		iSrcAddrDiff = affReg->src_imgaddr_30 - affReg->src_fetchaddr_1000;
	} else {
		iSrcAddrDiff = - (affReg->src_fetchaddr_1000 - affReg->src_imgaddr_30);
	}
	uiDstSrcAddrDiff = affReg->dst_imgaddr_40 - affReg->src_imgaddr_30;
	uiSrcAddrByteOfs = affReg->src_imgaddr_30 & 0x03;

	uiSrcPrimeSize = FileSys_GetFileLen(in1);
	//uiSrcSize = affReg->src_off_34 * (affReg->imgheight_38+1) * 8;
	uiDstSize = affReg->dst_off_44 * (affReg->imgheight_38 + 1) * 8;
	uiMaxSize = (uiDstSrcAddrDiff > uiDstSize) ? uiDstSrcAddrDiff : uiDstSize;
	if (uiSrcPrimeSize > uiMaxSize) {
		uiMaxSize = uiSrcPrimeSize;
	}
//    uiMaxSize = (uiSrcPrimeSize > uiDstSize) ? uiSrcPrimeSize : uiDstSize;
	uiMaxSize = ((uiMaxSize + 31) / 32) * 32;

	affReg->src_fetchaddr_1000 = (affReg->src_fetchaddr_1000 & 0xFF) + uiGrphBaseBuf + 4 - uiSrcAddrByteOfs;
//    affReg->src_fetchaddr_1000 = (affReg->src_fetchaddr_1000&0xFF)+uiGrphBaseBuf + affReg->src_off_34*8;
	affReg->src_imgaddr_30 = affReg->src_fetchaddr_1000 + iSrcAddrDiff;
//    affReg->src_imgaddr_30 = (affReg->src_imgaddr_30&0xFF)+uiGrphBaseBuf;
//    affReg->dst_imgaddr_40 = affReg->src_imgaddr_30 + uiDstSrcAddrDiff;
	affReg->dst_imgaddr_40 = (affReg->dst_imgaddr_40 & 0xFC) + uiMaxSize + affReg->src_imgaddr_30 + 32;

	memset((UINT32 *)uiGrphBaseBuf, 0, ((affReg->dst_imgaddr_40 + uiDstSize) - uiGrphBaseBuf));

	temp1 = affReg->src_fetchaddr_1000;
	fsize = LoadImage(in1, (UINT8 *)temp1, uiSrcPrimeSize);
	if (fsize == 0) {
		return AUTOTEST_RESULT_FAIL;
	}

	if (1) {
		AFFINE_REQUEST affReq = {0};
		AFFINE_IMAGE srcImg = {0};
		AFFINE_IMAGE dstImg = {0};
		AFFINE_COEFF coeffs = {0};
		UINT32 *IMG1, *IMG3;
		UINT32 i = 0;

		if (affReg->config_04 == 0) {
			affReq.format = AFFINE_FORMAT_FORMAT_8BITS;
		} else {
			affReq.format = AFFINE_FORMAT_FORMAT_16BITS_UVPACK;
		}
		affReq.width = (affReg->imgwidth_3C + 1) * 16;
		affReq.height = (affReg->imgheight_38 + 1) * 8;
		srcImg.uiImageAddr = affReg->src_imgaddr_30;
		srcImg.uiLineOffset = affReg->src_off_34;
		dstImg.uiImageAddr = affReg->dst_imgaddr_40;
		dstImg.uiLineOffset = affReg->dst_off_44;
		affReq.p_src_img = &srcImg;
		affReq.p_dst_img = &dstImg;
		affReq.p_coefficient = &coeffs;

		auto_msg(("SrcImg[0x%x] DstImg[0x%x] SrcRes[%d*%d] DstRes[%d*%d]\r\n", (unsigned int)srcImg.uiImageAddr, (unsigned int)dstImg.uiImageAddr,
					(int)srcImg.uiLineOffset, (int)affReq.height, (int)dstImg.uiLineOffset, (int)affReq.height));

		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_A, affReg->coeff_a_10);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_B, affReg->coeff_b_14);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_C, affReg->coeff_c_18);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_D, affReg->coeff_d_1C);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_E, affReg->coeff_e_20);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_COEFF_F, affReg->coeff_f_24);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_SUBBLK1, affReg->subblk1_28);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_SUBBLK2, affReg->subblk2_2C);
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_DIRECT_COEFF, TRUE);

		//auto_msg(("coeff_a_10[0x%08x] coeff_b_14[0x%08x] coeff_c_18[0x%08x] coeff_d_1C[0x%08x] \r\n",
		//		affReg->coeff_a_10, affReg->coeff_b_14, affReg->coeff_c_18, affReg->coeff_d_1C));
		//auto_msg(("coeff_e_20[0x%08x] coeff_f_24[0x%08x] subblk1_28[0x%08x] subblk2_2C[0x%08x] \r\n",
		//		affReg->coeff_e_20, affReg->coeff_f_24, affReg->subblk1_28, affReg->subblk2_2C));

		//affine_request(test_affine_id, &affReq);
		kdrv_affine_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_AFFINE, 0), (KDRV_AFFINE_TRIGGER_PARAM*)&affReq, NULL, NULL);

		// Rollback AFFINE_CONFIG_ID_DIRECT_COEFF as FALSE
		affine_setconfig(test_affine_id, AFFINE_CONFIG_ID_DIRECT_COEFF, FALSE);

		// Compare pattern
		temp1 = affReg->src_fetchaddr_1000 & 0xFFFFFFFC;
		IMG1 = (UINT32 *) temp1;
		fsize = FileSys_GetFileLen(out);
		fsize = LoadImage(out, (UINT8 *) temp1, fsize);
		if (fsize == 0) {
			return AUTOTEST_RESULT_FAIL;
		}

		temp1 = affReg->dst_imgaddr_40 & 0x3;
		ADDR = affReg->dst_imgaddr_40 - temp1;
		IMG3 = (UINT32 *) ADDR;

		for (i = 0; i < affReg->dst_off_44 * affReq.height >> 2 ; i++) {
			if (IMG1[i] != IMG3[i]) { // IMG1: golden, IMG3: h/w output
				auto_msg(("ID: %d, Data compare error!!\r\n", (int)affReg->TestId));
				auto_msg(("Correct = 0x%08X   But = 0x%08X  Position = %d\r\n", (unsigned int)IMG1[i], (unsigned int)IMG3[i], (int)i));
				auto_msg(("Expect DRAM addr 0x%x\r\n", (unsigned int)&IMG1[i]));
				auto_msg(("Affine output addr 0x%x\r\n", (unsigned int)&IMG3[i]));
				return AUTOTEST_RESULT_FAIL;
			}
		}
		auto_msg(("ID: %d, Data compare pass!!\r\n", (int)affReg->TestId));
	}

#endif
	return AUTOTEST_RESULT_OK;
}


#define AFFINE_0_FLGPTN	0x01
#define AFFINE_REG_FILE_BUFSIZE (0x1000)
#if defined(__FREERTOS)
static ID FLG_ID_AFFINE_0;
#else
static FLGPTN FLG_ID_AFFINE_0;
#endif
THREAD_HANDLE m_affine_tsk_0 = 0;

static INT32 affine_0_callback(KDRV_AFFINE_EVENT_CB_INFO *p_cb_info,
				void *user_data)
{
	if (p_cb_info == NULL) {
		return -1;
	} else {
	//printk("%s: Enter callback function! %d\r\n", __func__, p_cb_info->timestamp);
//		p_cb_info->handle;
//		p_cb_info->timestamp;
	}

	set_flg(FLG_ID_AFFINE_0, AFFINE_0_FLGPTN);

	return 0;
}

static THREAD_DECLARE(affine_platform_tsk_0, arglist)
{
	UINT32 img_reg_addr;
	UINT32 fsize;
	INT32 fstatus;
	FST_FILE file_hdl;
	char v_fname_buf[80];
	KDRV_CALLBACK_FUNC callback = {0};
	UINT32 uiStartID = 1011;
	UINT32 uiEndID = 1040;
	UINT32 uiTestID = uiStartID;
	AFF_REG gpm;

	auto_msg(("%s: base addr 0x%x\r\n", __func__, (unsigned int)affine_0_addr));

	OS_CONFIG_FLAG(FLG_ID_AFFINE_0);

	callback.callback = (INT32 (*)(VOID *, VOID *))affine_0_callback;

	img_reg_addr = affine_0_addr;

	//
	// 1. Test 0
	//
	auto_msg(("\nTest 0 (CMD_ROT)\n\n"));

	// Affine open
	kdrv_affine_open(KDRV_CHIP0, KDRV_GFX2D_AFFINE);

	while (1) {
		if (uiStartID == 0 || uiTestID > uiEndID)
				break;

		// Open xxx_in.reg file
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Affine/%03d_in.reg", (INT)uiTestID);
		file_hdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
		if (file_hdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			break;
		}

		// Read xxx_in.reg file
		fsize = AFFINE_REG_FILE_BUFSIZE;
	    fstatus = FileSys_ReadFile(file_hdl, (UINT8 *)img_reg_addr, &fsize, AFFINE_AUTO_FS_MODE, NULL);
		FileSys_CloseFile(file_hdl);
		if (fstatus != 0) {
			auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
			break;
		}

		// Parse content of xxx_in.reg file into pgm structure
		memset(&gpm, 0, sizeof(gpm));
		RegParser((char *)img_reg_addr, &gpm, fsize);

		// Run affine test
		auto_msg(("Start running %d emulation !\r\n", (int)uiTestID));
		gpm.TestId = uiTestID;
		if (RunAffTest(&gpm, &callback) != AUTOTEST_RESULT_OK)
			break;

		uiTestID++;
		vos_task_delay_ms(100); //msleep(100);
	}

	// Affine close
	kdrv_affine_close(KDRV_CHIP0, KDRV_GFX2D_AFFINE);

	THREAD_RETURN(0);
}

#if 0
#define AFFINE_1_FLGPTN	0x01
static FLGPTN     FLG_ID_AFFINE_1;
THREAD_HANDLE m_affine_tsk_1 = 0;
UINT32 affine_1_addr = 0;
UINT32 affine_1_size = 0;

static INT32 affine_1_callback(KDRV_AFFINE_EVENT_CB_INFO *p_cb_info,
				void *user_data)
{
	if (p_cb_info == NULL) {
		return -1;
	} else {
//		p_cb_info->handle;
//		p_cb_info->timestamp;
	}

	set_flg(FLG_ID_AFFINE_1, AFFINE_1_FLGPTN);

	return 0;
}

static THREAD_DECLARE(affine_platform_tsk_1, arglist)
{
	KDRV_CALLBACK_FUNC callback = {0};

	callback.callback = (INT32 (*)(VOID *, VOID *))affine_1_callback;

	THREAD_RETURN(0);
}
#endif

static AUTOTEST_RESULT test_multi_task(UINT32 addr, UINT32 size)
{
	affine_0_addr = addr;
	affine_0_size = size/2;
#if 1
	THREAD_CREATE(m_affine_tsk_0, affine_platform_tsk_0,
			NULL, "affine_tsk_0");
	THREAD_RESUME(m_affine_tsk_0);
#endif

	//affine_1_addr = addr + size/2;
	//affine_1_size = size/2;
#if 0
        THREAD_CREATE(m_affine_tsk_1, affine_platform_tsk_1,
                        NULL, "affine_tsk_1");
        THREAD_RESUME(m_affine_tsk_1);
#endif


	//while (1) {
	if (1){
		vos_task_delay_ms(1); //cond_resched();
	}

	return AUTOTEST_RESULT_OK;
}
#endif


#define AFFINE_EMU_0_FLGPTN	0x01
#if defined(__FREERTOS)
static ID FLG_ID_AFFINE_EMU_0;
#else
static FLGPTN FLG_ID_AFFINE_EMU_0;
#endif
static INT32 emu_affine_callback(KDRV_AFFINE_EVENT_CB_INFO *p_cb_info,
				void *user_data)
{
	if (p_cb_info == NULL) {
		return -1;
	} else {
	//auto_msg(("%s: Enter callback function! %d\r\n", __func__, p_cb_info->timestamp));
//		p_cb_info->handle;
//		p_cb_info->timestamp;
	}

	set_flg(FLG_ID_AFFINE_EMU_0, AFFINE_EMU_0_FLGPTN);

	return 0;
}

/*
    Affine Auto test program.

    Affine Auto test program.

    @param[in]  addr          Buffer address
    @param[in]  addr          Buffer size
    @return Test status
	- @b AUTOTEST_RESULT_OK     :   Test OK
	- @b AUTOTEST_RESULT_FAIL   :   Test Fail
*/
#define AFFINE_AUTO_WIDTH       1920
#define AFFINE_AUTO_LINEOFFSET  1920
#define AFFINE_AUTO_HEIGHT      32

static char vAffineAutoLineBuf[80];

AUTOTEST_RESULT emu_affine_auto(UINT32 uiAddr, UINT32 size)
{
	UINT32 i;
	UINT32 uiBaseAddr, uiImgSrcAddr, uiImgDstAddr, uiImgGoldAddr;
	UINT32 fsize;
	INT32 fstatus;
	INT32 addr;
	FLOAT fA, fB, fC, fD, fE, fF;
	FLOAT x, y;
	//const FLOAT fAngel = 15.0;
#if 0
	AFFINE_ID test_affine_id = AFFINE_ID_1;
	AFFINE_REQUEST request = {0};
#else //kdrv version
	KDRV_AFFINE_TRIGGER_PARAM request = {0};
#endif
	AFFINE_IMAGE srcImg = {0};
	AFFINE_IMAGE dstImg = {0};
	AFFINE_COEFF coeffs = {0};
	FST_FILE fileHdl;
	KDRV_CALLBACK_FUNC callback = {0};
	FLGPTN ptn;

	OS_CONFIG_FLAG(FLG_ID_AFFINE_EMU_0);

	// Check input buffer size
	if (size < AUTOTEST_BUFSIZE_AFFINE) {
		auto_msg(("%s: input buf size 0x%x, require 0x%x\r\n", __func__, (unsigned int)size, AUTOTEST_BUFSIZE_AFFINE));
		return AUTOTEST_RESULT_FAIL;
	}

	//
	// 1. Test 8 bit Y plane
	//
	fA = 0.96592582/*cos(fAngel * AFFINE_PI / 180.0)*/;
	fB = -0.25881904/*-sin(fAngel * AFFINE_PI / 180.0)*/;
	fC = 0;
	fD = 0.25881904/*sin(fAngel * AFFINE_PI / 180.0)*/;
	fE = 0.96592582/*cos(fAngel * AFFINE_PI / 180.0)*/;
	fF = 0;
#if 0
	// Left, Top
	x = fC;
	y = fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: left/top %f/%f, addr %d\r\n", __func__, x, y, addr);

	// Right, Top
	x = fA * (AFFINE_AUTO_WIDTH - 1) + fB * 0 + fC;
	y = fD * (AFFINE_AUTO_WIDTH - 1) + fE * 0 + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: right/top %f/%f, addr %d\r\n", __func__, x, y, addr);

	// Left, Bottom
	x = fA * 0 + fB * (AFFINE_AUTO_HEIGHT - 1) + fC;
	y = fD * 0 + fE * (AFFINE_AUTO_HEIGHT - 1) + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: left/bottom %f/%f, addr %d\r\n", __func__, x, y, addr);
#endif
	// Right, Bottom
	x = fA * (AFFINE_AUTO_WIDTH - 1) + fB * (AFFINE_AUTO_HEIGHT - 1) + fC;
	y = fD * (AFFINE_AUTO_WIDTH - 1) + fE * (AFFINE_AUTO_HEIGHT - 1) + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: right/bottom %f/%f, addr %d\r\n", __func__, x, y, addr);

	uiBaseAddr = uiAddr;
	uiImgSrcAddr = uiAddr + AFFINE_AUTO_LINEOFFSET;
	uiImgDstAddr = ((uiImgSrcAddr + addr + 16 * 8) / 32) * 32;
	uiImgGoldAddr = uiImgDstAddr + AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;

//    auto_msg("%s: base addr 0x%x\r\n", __func__, uiBaseAddr);
//    auto_msg("%s: src addr 0x%x\r\n", __func__, uiImgSrcAddr);
//    auto_msg("%s: dst addr 0x%x\r\n", __func__, uiImgDstAddr);
//    auto_msg("%s: gold addr 0x%x\r\n", __func__, uiImgGoldAddr);

	memset((void *)uiBaseAddr, 0, uiImgDstAddr - uiBaseAddr);

	// Construct Input Image
	for (i = 0; i < AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT; i++) {
		((UINT8 *)uiImgSrcAddr)[i] = i & 0xFF;
	}

#if (READ_CORRECT == 0)
	// Construct output Image
	for (i = 0; i < AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT; i++) {
		((UINT8 *)uiImgDstAddr)[i] = 0xFF;
	}
#else
	// Construct output Image
	for (i = 0; i < AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT; i++) {
		((UINT8 *)uiImgDstAddr)[i] = 0x5A;
	}
#endif


	//
	// Test affine rotate degree +15
	//;
	sprintf(vAffineAutoLineBuf, "/mnt/sd/Pattern/Affine/PTN00.raw");
	memset(&request, 0, sizeof(request));
	memset(&srcImg, 0, sizeof(srcImg));
	memset(&dstImg, 0, sizeof(dstImg));
	memset(&coeffs, 0, sizeof(coeffs));
	request.format = AFFINE_FORMAT_FORMAT_8BITS;
	request.uiWidth = AFFINE_AUTO_WIDTH;
	request.uiHeight = AFFINE_AUTO_HEIGHT;
	srcImg.uiImageAddr = uiImgSrcAddr;
	srcImg.uiLineOffset = AFFINE_AUTO_LINEOFFSET;
	dstImg.uiImageAddr = uiImgDstAddr;
	dstImg.uiLineOffset = AFFINE_AUTO_LINEOFFSET;
	request.pSrcImg = &srcImg;
	request.pDstImg = &dstImg;
	coeffs.fCoeffA = fA;
	coeffs.fCoeffB = fB;
	coeffs.fCoeffC = fC;
	coeffs.fCoeffD = fD;
	coeffs.fCoeffE = fE;
	coeffs.fCoeffF = fF;
	request.pCoefficient = &coeffs;
	request.pNext = NULL;

#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(vAffineAutoLineBuf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, AFFINE_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail, sts %d\r\n", __func__, vAffineAutoLineBuf, (int)fstatus));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif

#if 0
	affine_open(test_affine_id);
	affine_request(test_affine_id, &request);
	affine_close(test_affine_id);
#else //kdrv version
	callback.callback = (INT32 (*)(VOID *, VOID *))emu_affine_callback;
	kdrv_affine_open(KDRV_CHIP0, KDRV_GFX2D_AFFINE);
	kdrv_affine_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_AFFINE, 0), &request, &callback, NULL);
	wai_flg(&ptn, FLG_ID_AFFINE_EMU_0, AFFINE_EMU_0_FLGPTN, TWF_ORW | TWF_CLR);
	kdrv_affine_close(KDRV_CHIP0, KDRV_GFX2D_AFFINE);
#endif

#if (READ_CORRECT == 0)
	fileHdl = FileSys_OpenFile(vAffineAutoLineBuf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
	//fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
	//if (fstatus != 0) {
	//	auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, vAffineAutoLineBuf, 0));
	//	return AUTOTEST_RESULT_FAIL;
	//}
	fsize = AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;
	fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)dstImg.uiImageAddr, &fsize, 0, NULL, 0);
	if (fstatus != 0) {
		auto_msg(("%s: write file %s fail\r\n", __func__, vAffineAutoLineBuf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
	auto_msg(("%s: ptn %s is generated\r\n", __func__, vAffineAutoLineBuf));
#else
	FileSys_CloseFile(fileHdl);
	if (memcmp((void *)uiImgGoldAddr, (void *)dstImg.uiImageAddr, AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT) != 0) {
		auto_msg(("%s: file %s compare fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif

	//
	// 2. Test 16 bit UV plane
	//
//	fA = fA;
	fB = fB / 2;
	fC = fC / 2;
	fD = fD * 2;
//	fE = fE;
//	fF = fF;
#if 0
	// Left, Top
	x = fC;
	y = fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: left/top %f/%f, addr %d\r\n", __func__, x, y, addr);

	// Right, Top
	x = fA * (AFFINE_AUTO_WIDTH - 1) + fB * 0 + fC;
	y = fD * (AFFINE_AUTO_WIDTH - 1) + fE * 0 + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: right/top %f/%f, addr %d\r\n", __func__, x, y, addr);

	// Left, Bottom
	x = fA * 0 + fB * (AFFINE_AUTO_HEIGHT - 1) + fC;
	y = fD * 0 + fE * (AFFINE_AUTO_HEIGHT - 1) + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: left/bottom %f/%f, addr %d\r\n", __func__, x, y, addr);
#endif
	// Right, Bottom
	x = fA * (AFFINE_AUTO_WIDTH - 1) + fB * (AFFINE_AUTO_HEIGHT - 1) + fC;
	y = fD * (AFFINE_AUTO_WIDTH - 1) + fE * (AFFINE_AUTO_HEIGHT - 1) + fF;
	addr = y * AFFINE_AUTO_LINEOFFSET + x;
//    auto_msg("%s: right/bottom %f/%f, addr %d\r\n", __func__, x, y, addr);

	uiBaseAddr = uiAddr;
	uiImgSrcAddr = uiAddr + AFFINE_AUTO_LINEOFFSET;
	uiImgDstAddr = ((uiImgSrcAddr + addr + 16 * 8) / 32) * 32;
	uiImgGoldAddr = uiImgDstAddr + AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;

//    auto_msg("%s: base addr 0x%x\r\n", __func__, uiBaseAddr);
//    auto_msg("%s: src addr 0x%x\r\n", __func__, uiImgSrcAddr);
//    auto_msg("%s: dst addr 0x%x\r\n", __func__, uiImgDstAddr);
//    auto_msg("%s: gold addr 0x%x\r\n", __func__, uiImgGoldAddr);

	memset((void *)uiBaseAddr, 0, uiImgDstAddr - uiBaseAddr);

	// Construct Input Image
	for (i = 0; i < AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT; i++) {
		((UINT8 *)uiImgSrcAddr)[i] = i & 0xFF;
	}

	//
	// Test affine rotate degree +15
	//
	sprintf(vAffineAutoLineBuf, "/mnt/sd/Pattern/Affine/PTN01.raw");
	memset(&request, 0, sizeof(request));
	memset(&srcImg, 0, sizeof(srcImg));
	memset(&dstImg, 0, sizeof(dstImg));
	memset(&coeffs, 0, sizeof(coeffs));
	request.format = AFFINE_FORMAT_FORMAT_16BITS_UVPACK;
	request.uiWidth = AFFINE_AUTO_WIDTH;
	request.uiHeight = AFFINE_AUTO_HEIGHT;
	srcImg.uiImageAddr = uiImgSrcAddr;
	srcImg.uiLineOffset = AFFINE_AUTO_LINEOFFSET;
	dstImg.uiImageAddr = uiImgDstAddr;
	dstImg.uiLineOffset = AFFINE_AUTO_LINEOFFSET;
	request.pSrcImg = &srcImg;
	request.pDstImg = &dstImg;
	coeffs.fCoeffA = fA;
	coeffs.fCoeffB = fB;
	coeffs.fCoeffC = fC;
	coeffs.fCoeffD = fD;
	coeffs.fCoeffE = fE;
	coeffs.fCoeffF = fF;
	request.pCoefficient = &coeffs;
	request.pNext = NULL;

#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(vAffineAutoLineBuf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, AFFINE_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail, sts %d\r\n", __func__, vAffineAutoLineBuf, (int)fstatus));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif

#if 0
	affine_open(test_affine_id);
	affine_request(test_affine_id, &request);
	affine_close(test_affine_id);
#else //kdrv version
	callback.callback = (INT32 (*)(VOID *, VOID *))emu_affine_callback;
	kdrv_affine_open(KDRV_CHIP0, KDRV_GFX2D_AFFINE);
	kdrv_affine_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_AFFINE, 0), &request, &callback, NULL);
	wai_flg(&ptn, FLG_ID_AFFINE_EMU_0, AFFINE_EMU_0_FLGPTN, TWF_ORW | TWF_CLR);
	kdrv_affine_close(KDRV_CHIP0, KDRV_GFX2D_AFFINE);
#endif

#if (READ_CORRECT == 0)
	fileHdl = FileSys_OpenFile(vAffineAutoLineBuf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
	//fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
	//if (fstatus != 0) {
	//	auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, vAffineAutoLineBuf, 0));
	//	return AUTOTEST_RESULT_FAIL;
	//}
	fsize = AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT;
	fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)dstImg.uiImageAddr, &fsize, 0, NULL, 0);
	if (fstatus != 0) {
		auto_msg(("%s: write file %s fail\r\n", __func__, vAffineAutoLineBuf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
	auto_msg(("%s: ptn %s is generated\r\n", __func__, vAffineAutoLineBuf));
#else
	FileSys_CloseFile(fileHdl);
	if (memcmp((void *)uiImgGoldAddr, (void *)dstImg.uiImageAddr, AFFINE_AUTO_WIDTH * AFFINE_AUTO_HEIGHT) != 0) {
		auto_msg(("%s: file %s compare fail\r\n", __func__, vAffineAutoLineBuf));
		return AUTOTEST_RESULT_FAIL;
	}
	else
		auto_msg(("%s: file %s compare pass\r\n", __func__, vAffineAutoLineBuf));
#endif

#if (AUTOTEST_THREAD == ENABLE)
		test_multi_task(uiAddr, size);
#endif


	return AUTOTEST_RESULT_OK;
}
#endif

