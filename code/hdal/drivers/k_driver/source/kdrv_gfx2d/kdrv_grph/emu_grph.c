#include "grph_int.h"
#include "emu_grph.h"

#include "kdrv_gfx2d/kdrv_grph_lmt.h"

#define AUTOTEST_GRAPHIC	DISABLE
#define AUTOTEST_THREAD		DISABLE

#if (AUTOTEST_GRAPHIC == ENABLE)
// file system API wrapper
#if defined(__FREERTOS)
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

	printf("%s: close 0x%x\r\n", __func__, (unsigned int)file);

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

#else	// LINUX

#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include <kwrap/file.h>
#include <linux/delay.h>

static FST_FILE FileSys_OpenFile(char *filename, UINT32 flag)
{
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

	fp = vos_file_open(filename, fs_flg, 0);

	auto_msg(("file %s flg 0x%x flag 0x%x open result 0x%x\r\n",
				filename, fs_flg, flag, fp));

	if (-1 == fp)
		return NULL;
	else
		return (FST_FILE)fp;
}

static INT32 FileSys_CloseFile(FST_FILE file)
{
	INT32 ret;

	ret = vos_file_close((int)file);

	auto_msg(("%s: close 0x%p, result %d\r\n", __func__, file, ret));

	return ret;
}

static INT32 FileSys_ReadFile(FST_FILE file, UINT8 *p_buf, UINT32 *p_buf_size, UINT32 flag, INT32 *p_callbck)
{
	INT32 ret;
	INT32 size = *p_buf_size;

	auto_msg(("fd 0x%p read length %d, buf 0x%p\r\n",
			file, size, p_buf));

#if 0
	pbuffer = kmalloc(size, GFP_KERNEL);
	printk("kmalloc get 0x%p\r\n", pbuffer);
	if (pbuffer == NULL) {
		printk("kalloc %d fail\r\n", size);
		return -ENOMEM;
	}
#endif

	ret = vos_file_read((int)file, p_buf, size);

	auto_msg(("file read length %d\r\n", ret));

	if (ret < 0) {
	} else {
		*p_buf_size = ret;
		ret = 0;
//		memcpy(p_buf, pbuffer, size);
	}

	return ret;
}

#endif


#if (AUTOTEST_THREAD == ENABLE)
#define GRPH1_AOP0_FLGPTN	0x01
static FLGPTN     FLG_ID_GRPH1_AOP0;
THREAD_HANDLE m_grph1_tsk_aop0 = 0;
UINT32 grph1_aop1_addr = 0;
UINT32 grph1_aop1_size = 0;

static INT32 grph1_aop0_callback(KDRV_GRPH_EVENT_CB_INFO *p_cb_info,
				void *user_data)
{
	if (p_cb_info == NULL) {
		return -1;
	} else {
//		p_cb_info->timestamp;
//		p_cb_info->acc_result;
	}

	set_flg(FLG_ID_GRPH1_AOP0, GRPH1_AOP0_FLGPTN);

	return 0;
}

static THREAD_DECLARE(grph1_platform_tsk_aop0, arglist)
{
	UINT32 i;
        UINT32 uiImgAaddr, uiImgBaddr, uiImgCaddr, uiImgGoldAddr;
	UINT32 fsize;
        INT32 fstatus;
	GRPH_ID testGrphId;
        GRPH_REQUEST grphRequest;
        GRPH_IMG grphImageA = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG grphImageC = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
        FST_FILE fileHdl;
	char v_fname_buf[80];
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	auto_msg(("%s: base addr 0x%x\r\n", __func__, grph1_aop1_addr));

	OS_CONFIG_FLAG(FLG_ID_GRPH1_AOP0);

	callback.callback = (INT32 (*)(VOID *, VOID *))grph1_aop0_callback;


	uiImgAaddr = (grph1_aop1_addr);
        uiImgBaddr = (uiImgAaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgCaddr = (uiImgBaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgGoldAddr = (uiImgCaddr + IMG1_LOFF * IMG1_HEIGHT + 0);

	// Construct Input Image
        for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
                ((UINT8 *)uiImgAaddr)[i] = i & 0xFF;
                ((UINT8 *)uiImgBaddr)[i] = ~(i & 0xFF);
        }


	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP00.raw");
//      sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP00.raw");
        memset(&grphRequest, 0, sizeof(grphRequest));
        memset(&grphImageA, 0, sizeof(grphImageA));
        memset(&grphImageC, 0, sizeof(grphImageC));
        grphImageA.img_id = GRPH_IMG_ID_A;
        grphImageA.dram_addr = uiImgAaddr;
        grphImageA.lineoffset = IMG1_LOFF;
        grphImageA.width = IMG1_WIDTH;
        grphImageA.height = IMG1_HEIGHT;
        grphImageA.p_next = &grphImageC;
        grphImageC.img_id = GRPH_IMG_ID_C;
        grphImageC.dram_addr = uiImgCaddr;
        grphImageC.lineoffset = IMG1_LOFF;
        grphRequest.command = GRPH_CMD_A_COPY;
        grphRequest.format = GRPH_FORMAT_8BITS;
        grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
        fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
        if (fileHdl == NULL) {
                auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
                return AUTOTEST_RESULT_FAIL;
        }
        fsize = IMG1_LOFF * IMG1_HEIGHT;
        fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
        if (fstatus != 0) {
                auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
                return AUTOTEST_RESULT_FAIL;
        }
        FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
                memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//              hwmem_open();
//              hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//              hwmem_close();
                graph_open(testGrphId);
//		printk("%s: before trig\r\n", __func__);
		if (testGrphId == GRPH_ID_1) {
			kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0),
				(KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
				&callback, NULL);
		} else {
			kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0),
				(KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
				&callback, NULL);
		}
//                graph_request(testGrphId, &grphRequest);
//                graph_request(testGrphId, &grphRequest);
		wai_flg(&ptn, FLG_ID_GRPH1_AOP0, GRPH1_AOP0_FLGPTN,
			TWF_ORW | TWF_CLR);
//		printk("%s: after wait\r\n", __func__);
                graph_close(testGrphId);
#if (READ_CORRECT == 0)
#else
//        FileSys_CloseFile(fileHdl);
                if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
                        auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
                        return AUTOTEST_RESULT_FAIL;
                }
#endif
	}

	printk("%s: enter loop\r\n", __func__);

	kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH0);
	kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH1);
	i = 0;
	while (!THREAD_SHOULD_STOP) {
		UINT32 id;

                memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
		if (i & 0x01)
			id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);
		else
			id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
//		printk("%s: before trig 0x%x\r\n", __func__, id);
#if 0
		kdrv_grph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
				NULL, NULL);
#else
		kdrv_grph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
			&callback, NULL);
		wai_flg(&ptn, FLG_ID_GRPH1_AOP0, GRPH1_AOP0_FLGPTN,
			TWF_ORW | TWF_CLR);
#endif
//		printk("%s: after wait\r\n", __func__);
                if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
                        auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			while (1);
                        return AUTOTEST_RESULT_FAIL;
                }

//#include <linux/delay.h>
		cond_resched();
		i++;
	}

	THREAD_RETURN(0);
}

#define GRPH1_GOP8_FLGPTN	0x01
static FLGPTN     FLG_ID_GRPH1_GOP8;
THREAD_HANDLE m_grph1_tsk_gop8 = 0;
UINT32 grph1_gop8_addr = 0;
UINT32 grph1_gop8_size = 0;

static INT32 grph1_gop8_callback(KDRV_GRPH_EVENT_CB_INFO *p_cb_info,
				void *user_data)
{
	if (p_cb_info == NULL) {
	} else {
//		p_cb_info->timestamp;
//		p_cb_info->acc_result;
	}

	set_flg(FLG_ID_GRPH1_GOP8, GRPH1_GOP8_FLGPTN);

	return 0;
}

static THREAD_DECLARE(grph1_platform_tsk_gop8, arglist)
{
	UINT32 i;
        UINT32 uiImgAaddr, uiImgBaddr, uiImgCaddr, uiImgGoldAddr;
	UINT32 fsize;
        INT32 fstatus;
	GRPH_ID testGrphId;
        GRPH_REQUEST grphRequest;
        GRPH_IMG grphImageA = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG grphImageB = {GRPH_IMG_ID_B, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG grphImageC = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
	GRPH_PROPERTY grphProperty[7];
        GRPH_QUAD_DESC grphQuadDesc = {0};
        FST_FILE fileHdl;
	char v_fname_buf[80];
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	printk("%s: base addr 0x%x\r\n", __func__, grph1_gop8_addr);

	OS_CONFIG_FLAG(FLG_ID_GRPH1_GOP8);

	callback.callback = (INT32 (*)(VOID *, VOID *))grph1_gop8_callback;


	uiImgAaddr = (grph1_gop8_addr);
        uiImgBaddr = (uiImgAaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgCaddr = (uiImgBaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgGoldAddr = (uiImgCaddr + IMG1_LOFF * IMG1_HEIGHT + 0);

	printk("%s: gold addr 0x%x\r\n", __func__, uiImgGoldAddr);

	// Construct Input Image
        for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
                ((UINT8 *)uiImgAaddr)[i] = i & 0xFF;
                ((UINT8 *)uiImgBaddr)[i] = ~(i & 0xFF);
        }


	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/GOP08.raw");
//      sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP08.raw");
        memset(&grphRequest, 0, sizeof(grphRequest));
        memset(&grphImageA, 0, sizeof(grphImageA));
        memset(&grphImageB, 0, sizeof(grphImageB));
        memset(&grphImageC, 0, sizeof(grphImageC));
        grphImageA.img_id = GRPH_IMG_ID_A;
        grphImageA.dram_addr = uiImgAaddr;
        grphImageA.lineoffset = IMG1_LOFF;
        grphImageA.width = IMG1_WIDTH;
        grphImageA.height = IMG1_HEIGHT;
        grphImageA.p_next = &grphImageB;
        grphImageB.img_id = GRPH_IMG_ID_B;
        grphImageB.dram_addr = uiImgAaddr;
        grphImageB.lineoffset = 256;
        grphImageB.width = IMG1_WIDTH / 128;
        grphImageB.height = IMG1_HEIGHT;
        grphImageB.p_next = &grphImageC;
        grphImageC.img_id = GRPH_IMG_ID_C;
        grphImageC.dram_addr = uiImgCaddr;
        grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_VCOV;
        grphRequest.format = GRPH_FORMAT_16BITS;
        grphRequest.p_images = &grphImageA;

        grphProperty[0].id = GRPH_PROPERTY_ID_YUVFMT;
        grphProperty[0].property = GRPH_YUV_422;
        grphProperty[0].p_next = &(grphProperty[1]);
        grphProperty[1].id = GRPH_PROPERTY_ID_QUAD_PTR;
        grphProperty[1].property = (UINT32)&grphQuadDesc;
        grphProperty[1].p_next = NULL;
        grphRequest.p_property = &(grphProperty[0]);

	grphQuadDesc.blend_en = TRUE;
        grphQuadDesc.alpha = 0x20;
        grphQuadDesc.top_left.x = 10;
        grphQuadDesc.top_left.y = 0;
        grphQuadDesc.top_right.x = 2048;
        grphQuadDesc.top_right.y = 20;
        grphQuadDesc.bottom_right.x = 2000;
        grphQuadDesc.bottom_right.y = 40;
        grphQuadDesc.bottom_left.x = 0;
        grphQuadDesc.bottom_left.y = 16;
        grphQuadDesc.mosaic_width = 128;
        grphQuadDesc.mosaic_height = 64;

#if (READ_CORRECT == 1)
        fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
        if (fileHdl == NULL) {
                auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
                return AUTOTEST_RESULT_FAIL;
        }
        fsize = IMG1_LOFF * IMG1_HEIGHT;
        fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
        if (fstatus != 0) {
                auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
                FileSys_CloseFile(fileHdl);
                return AUTOTEST_RESULT_FAIL;
        }
	FileSys_CloseFile(fileHdl);
#endif


	testGrphId = GRPH_ID_2;
	graph_open(testGrphId);
	graph_request(testGrphId, &grphRequest);
	printk("%s: P1\r\n", __func__);
	graph_close(testGrphId);
#if (READ_CORRECT == 0)
#else
	if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
		auto_msg(("%s: file %s CMP fail\r\n", __func__, v_fname_buf));
		while (1);
		return AUTOTEST_RESULT_FAIL;
	}

#endif

	printk("%s: enter loop\r\n", __func__);

	kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH1);
	i = 0;
	while (!THREAD_SHOULD_STOP) {
		UINT32 id;

                memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
//		printk("%s: before trig\r\n", __func__);
		kdrv_grph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
				&callback, NULL);
		wai_flg(&ptn, FLG_ID_GRPH1_GOP8, GRPH1_GOP8_FLGPTN,
			TWF_ORW | TWF_CLR);
//		printk("%s: after wait\r\n", __func__);
//		graph_request(testGrphId, &grphRequest);

		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			while (1);
			return AUTOTEST_RESULT_FAIL;
		}

		cond_resched();
		i++;
	}

	THREAD_RETURN(0);
}

#define GRPH1_AOP13_FLGPTN	0x01
static FLGPTN     FLG_ID_GRPH1_AOP13;
THREAD_HANDLE m_grph1_tsk_aop13 = 0;
UINT32 grph1_aop13_addr = 0;
UINT32 grph1_aop13_size = 0;

static INT32 grph1_aop13_callback(KDRV_GRPH_EVENT_CB_INFO *p_cb_info,
                                void *user_data)
{
        if (p_cb_info == NULL) {
                return -1;
        } else {
//                p_cb_info->timestamp;
//                p_cb_info->acc_result;
        }

//	printk("%s: set flg\r\n", __func__);
        set_flg(FLG_ID_GRPH1_AOP13, GRPH1_AOP13_FLGPTN);

        return 0;
}

static THREAD_DECLARE(grph1_platform_tsk_aop13, arglist)
{
        UINT32 i;
        UINT32 uiImgAaddr, uiImgBaddr, uiImgCaddr, uiImgGoldAddr;
        UINT32 fsize;
        INT32 fstatus;
        GRPH_ID testGrphId;
        GRPH_REQUEST grphRequest;
        GRPH_IMG grphImageA = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG grphImageB = {GRPH_IMG_ID_B, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG grphImageC = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
	GRPH_INOUTOP grphInOutOpA_U;
	GRPH_INOUTOP grphInOutOpB_U;
	GRPH_INOUTOP grphInOutOpC_U;
        GRPH_PROPERTY grphProperty[7];
//        GRPH_QUAD_DESC grphQuadDesc = {0};
        FST_FILE fileHdl;
        char v_fname_buf[80];
        FLGPTN              ptn;
        KDRV_CALLBACK_FUNC callback = {0};

        printk("%s: base addr 0x%x\r\n", __func__, grph1_aop13_addr);

	OS_CONFIG_FLAG(FLG_ID_GRPH1_AOP13);

        callback.callback = (INT32 (*)(VOID *, VOID *))grph1_aop13_callback;


        uiImgAaddr = (grph1_aop13_addr);
        uiImgBaddr = (uiImgAaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgCaddr = (uiImgBaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
        uiImgGoldAddr = (uiImgCaddr + IMG1_LOFF * IMG1_HEIGHT + 0);

        printk("%s: gold addr 0x%x\r\n", __func__, uiImgGoldAddr);

        // Construct Input Image
        for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
                ((UINT8 *)uiImgAaddr)[i] = i & 0xFF;
                ((UINT8 *)uiImgBaddr)[i] = ~(i & 0xFF);
        }

	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP13.raw");
//      sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP13.raw");
        memset(&grphRequest, 0, sizeof(grphRequest));
        memset(&grphImageA, 0, sizeof(grphImageA));
        memset(&grphImageB, 0, sizeof(grphImageB));
        memset(&grphImageC, 0, sizeof(grphImageC));
        memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
        memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
        memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
        memset(grphProperty, 0, sizeof(grphProperty));
        grphImageA.img_id = GRPH_IMG_ID_A;
        grphImageA.dram_addr = uiImgAaddr;
        grphImageA.lineoffset = IMG1_LOFF;
        grphImageA.width = IMG1_WIDTH;
        grphImageA.height = IMG1_HEIGHT;
        grphImageA.p_next = &grphImageB;
        grphImageB.img_id = GRPH_IMG_ID_B;
        grphImageB.dram_addr = uiImgBaddr;
        grphImageB.lineoffset = IMG1_LOFF;
        grphImageB.p_next = &grphImageC;
        grphImageC.img_id = GRPH_IMG_ID_C;
        grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
        grphProperty[0].property = GRPH_BLD_WA_WB_THR(30, 226, 250);
        grphProperty[0].p_next = NULL;
        grphRequest.p_property = &(grphProperty[0]);

        grphRequest.command = GRPH_CMD_BLENDING;
        grphRequest.format = GRPH_FORMAT_8BITS;
        grphRequest.p_images = &grphImageA;

#if (READ_CORRECT == 1)
        fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
        if (fileHdl == NULL) {
                auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
                return AUTOTEST_RESULT_FAIL;
        }
        fsize = IMG1_LOFF * IMG1_HEIGHT;
        fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
        if (fstatus != 0) {
                auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
                return AUTOTEST_RESULT_FAIL;
        }
        FileSys_CloseFile(fileHdl);
#endif

	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
                memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//              hwmem_open();
//              hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//              hwmem_close();
		printk("%s: before request 0x%x\r\n", __func__, testGrphId);
                graph_open(testGrphId);
                graph_request(testGrphId, &grphRequest);
                graph_close(testGrphId);
		printk("%s: after request 0x%x\r\n", __func__, testGrphId);
#if (READ_CORRECT == 0)
#else
		//        FileSys_CloseFile(fileHdl);
                if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
                        auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
                        return AUTOTEST_RESULT_FAIL;
                }
#endif
	}

	kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH0);
	kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH1);
	i = 0;
	while (!THREAD_SHOULD_STOP) {
		UINT32 id;

		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);

		if (i & 0x01)
                        id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);
                else
                        id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
		printk("%s: before trig 0x%x\r\n", __func__, id);
//                kdrv_grph_trigger(id, &grphRequest, NULL, NULL);
                kdrv_grph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)&grphRequest,
				&callback, NULL);
                wai_flg(&ptn, FLG_ID_GRPH1_AOP13, GRPH1_AOP13_FLGPTN,
                        TWF_ORW | TWF_CLR);
//              printk("%s: after wait\r\n", __func__);
                if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
                        auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
                        while (1);
                        return AUTOTEST_RESULT_FAIL;
                }


		cond_resched();
                i++;
	}

	THREAD_RETURN(0);
}

static AUTOTEST_RESULT test_multi_task(UINT32 addr, UINT32 size)
{
	grph1_aop1_addr = addr;
	grph1_aop1_size = IMG1_LOFF * (IMG1_HEIGHT + 8) * 8;
#if 1
	THREAD_CREATE(m_grph1_tsk_aop0, grph1_platform_tsk_aop0,
			NULL, "grph1_tsk_aop0");
        THREAD_RESUME(m_grph1_tsk_aop0);
#endif

	grph1_gop8_addr = grph1_aop1_addr + grph1_aop1_size;
	grph1_gop8_size = IMG1_LOFF * (IMG1_HEIGHT + 8) * 4;

	printk("%s: input addr 0x%x, size 0x%x\r\n", __func__, addr, size);
	printk("%s: gop8 addr 0x%x, size 0x%x\r\n", __func__,
			grph1_aop1_addr + grph1_aop1_size, grph1_gop8_size);
	grph1_gop8_size = IMG1_LOFF * (IMG1_HEIGHT + 8) * 4;
#if 1
	THREAD_CREATE(m_grph1_tsk_gop8, grph1_platform_tsk_gop8,
			NULL, "grph2_tsk_gop8");
        THREAD_RESUME(m_grph1_tsk_gop8);
#endif

//	grph1_aop13_addr = addr;
	grph1_aop13_addr = grph1_gop8_addr + grph1_gop8_size;
        grph1_aop13_size = IMG1_LOFF * (IMG1_HEIGHT + 8) * 4;

        printk("%s: input addr 0x%x, size 0x%x\r\n", __func__, addr, size);
        printk("%s: aop13 addr 0x%x, size 0x%x\r\n", __func__,
                        grph1_aop13_addr, grph1_aop13_size);
#if 1
        THREAD_CREATE(m_grph1_tsk_aop13, grph1_platform_tsk_aop13,
                        NULL, "grph1_tsk_aop13");
        THREAD_RESUME(m_grph1_tsk_aop13);
#endif


	while (1) {
		cond_resched();
	}

	return AUTOTEST_RESULT_OK;
}
#endif

static char v_fname_buf[80];

/*
    Graphic Auto test program.

    Graphic Auto test program.

    @param[in]  uiAddr          Buffer address
    @param[in]  uiAddr          Buffer size
    @return Test status
	- @b AUTOTEST_RESULT_OK     :   Test OK
	- @b AUTOTEST_RESULT_FAIL   :   Test Fail
*/
GRPH_PROPERTY grphProperty[7];
GRPH_INOUTOP grphInOutOpA_U, grphInOutOpA_V;
GRPH_INOUTOP grphInOutOpB_U, grphInOutOpB_V;
GRPH_INOUTOP grphInOutOpC_U, grphInOutOpC_V;
UINT8 vGrphLut[289];

AUTOTEST_RESULT emu_graphic_auto(UINT32 uiAddr, UINT32 uiSize)
{
	UINT32 i;
	UINT32 uiImgAaddr, uiImgBaddr, uiImgCaddr, uiImgGoldAddr;
	UINT32 fsize;
	UINT32 freq1 = 0, freq2 = 0;
	INT32 fstatus;
	GRPH_ID testGrphId;
	GRPH_REQUEST grphRequest;
	GRPH_IMG grphImageA = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, NULL};
	GRPH_IMG grphImageB = {GRPH_IMG_ID_B, 0, 0, 0, 0, NULL, NULL};
	GRPH_IMG grphImageC = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
	GRPH_QUAD_DESC grphQuadDesc = {0};
	FST_FILE fileHdl;

	// Check input buffer size (3*image, 1*golden data)
	if (uiSize < AUTOTEST_BUFSIZE_GRAPHIC) {
		auto_msg(("%s: input buf size 0x%x, require 0x%x\r\n", __func__, (unsigned int)uiSize,
			AUTOTEST_BUFSIZE_GRAPHIC));
		return AUTOTEST_RESULT_FAIL;
	}
	uiImgAaddr = (uiAddr);
	uiImgBaddr = (uiImgAaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
	uiImgCaddr = (uiImgBaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
	uiImgGoldAddr = (uiImgCaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
//	uiImgAaddr = dma_getCacheAddr(uiAddr);
//	uiImgBaddr = dma_getCacheAddr(uiImgAaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
//	uiImgCaddr = dma_getCacheAddr(uiImgBaddr + IMG1_LOFF * IMG1_HEIGHT + 0);
//	uiImgGoldAddr = dma_getCacheAddr(uiImgCaddr + IMG1_LOFF * IMG1_HEIGHT + 0);

	kdrv_grph_get(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0),
			KDRV_GRPH_PARAM_FREQ, &freq1);
	kdrv_grph_get(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0),
			KDRV_GRPH_PARAM_FREQ, &freq2);
//	graph_get_config(GRPH_ID_1, GRPH_CONFIG_ID_FREQ, &freq1);
//	graph_get_config(GRPH_ID_2, GRPH_CONFIG_ID_FREQ, &freq2);
	auto_msg(("%s: initial clk %d and %d\r\n", __func__, (int)freq1, (int)freq2));
	freq1 = 420;
	kdrv_grph_set(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0),
			KDRV_GRPH_PARAM_FREQ, &freq1);
	freq2 = 420;
	kdrv_grph_set(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0),
			KDRV_GRPH_PARAM_FREQ, &freq2);
//	graph_set_config(GRPH_ID_1, GRPH_CONFIG_ID_FREQ, 420);
//	graph_set_config(GRPH_ID_2, GRPH_CONFIG_ID_FREQ, 420);
	graph_get_config(GRPH_ID_1, GRPH_CONFIG_ID_FREQ, &freq1);
	graph_get_config(GRPH_ID_2, GRPH_CONFIG_ID_FREQ, &freq2);
	auto_msg(("%s: mod clk %d and %d\r\n", __func__, (int)freq1, (int)freq2));

	//
	// 0. Prepare source image
	//
	// Construct Input Image
	for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
		((UINT8 *)uiImgAaddr)[i] = i & 0xFF;
		((UINT8 *)uiImgBaddr)[i] = ~(i & 0xFF);
		if (i < sizeof(vGrphLut) / sizeof(vGrphLut[0])) {
			vGrphLut[i] = i;
		}
	}

	//
	// 1. Test GOP0 (GRPH_CMD_ROT_90)
	//
#if 0
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP00.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_HEIGHT;
	grphRequest.command = GRPH_CMD_ROT_90;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		graph_close(GRPH_ID_1);
		graph_close(GRPH_ID_2);
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_1) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 2. Test GOP1 (GRPH_CMD_ROT_270)
	//
#if 0
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP01.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_HEIGHT;
	grphRequest.command = GRPH_CMD_ROT_270;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_1) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 3. Test GOP2 (GRPH_CMD_ROT_180)
	//
#if 0
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP02.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_ROT_180;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_1) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 4. Test GOP3 (GRPH_CMD_HRZ_FLIP)
	//
#if 0
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP03.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_HRZ_FLIP;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_1) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 5. Test GOP4 (GRPH_CMD_VTC_FLIP)
	//
#if 0
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP04.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_VTC_FLIP;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_1) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 6. Test GOP8 (GRPH_CMD_VCOV)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/GOP08.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\GOP08.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgAaddr;
	grphImageB.lineoffset = 256;
	grphImageB.width = IMG1_WIDTH / 128;
	grphImageB.height = IMG1_HEIGHT;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_VCOV;
	grphRequest.format = GRPH_FORMAT_16BITS;
	grphRequest.p_images = &grphImageA;

	grphProperty[0].id = GRPH_PROPERTY_ID_YUVFMT;
	grphProperty[0].property = GRPH_YUV_422;
	grphProperty[0].p_next = &(grphProperty[1]);
	grphProperty[1].id = GRPH_PROPERTY_ID_QUAD_PTR;
	grphProperty[1].property = (UINT32)&grphQuadDesc;
	grphProperty[1].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphQuadDesc.blend_en = TRUE;
	grphQuadDesc.alpha = 0x20;
	grphQuadDesc.top_left.x = 10;
	grphQuadDesc.top_left.y = 0;
	grphQuadDesc.top_right.x = 2048;
	grphQuadDesc.top_right.y = 20;
	grphQuadDesc.bottom_right.x = 2000;
	grphQuadDesc.bottom_right.y = 40;
	grphQuadDesc.bottom_left.x = 0;
	grphQuadDesc.bottom_left.y = 16;
	grphQuadDesc.mosaic_width = 128;
	grphQuadDesc.mosaic_height = 64;

#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_2; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		auto_msg(("%s: P1\r\n", __func__));
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		// save image C
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		if (testGrphId == GRPH_ID_2) {
			FileSys_CloseFile(fileHdl);
		}
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 6. Test AOP0 (GRPH_CMD_A_COPY)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP00.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP00.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageC, 0, sizeof(grphImageC));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphRequest.command = GRPH_CMD_A_COPY;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
//        FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 7. Test AOP1 (GRPH_CMD_PLUS_SHF)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP01.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP01.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	if (1) { /* support IN/OUT op */
		grphInOutOpA_U.id = GRPH_INOUT_ID_IN_A;
		grphInOutOpA_U.op = GRPH_INOP_INVERT;
		grphInOutOpA_U.p_next = NULL;
		grphImageA.p_inoutop = &grphInOutOpA_U;

		grphInOutOpB_U.id = GRPH_INOUT_ID_IN_B;
		grphInOutOpB_U.op = GRPH_INOP_SHIFTR_ADD;
		grphInOutOpB_U.shifts = 2;
		grphInOutOpB_U.constant = 1;
		grphInOutOpB_U.p_next = NULL;
		grphImageB.p_inoutop = &grphInOutOpB_U;

		grphInOutOpC_U.id = GRPH_INOUT_ID_OUT_C;
		grphInOutOpC_U.op = GRPH_OUTOP_SHF;
		grphInOutOpC_U.shifts = 3;
		grphInOutOpC_U.constant = 0;
		grphInOutOpC_U.p_next = NULL;
		grphImageC.p_inoutop = &grphInOutOpC_U;
	}
	grphRequest.command = GRPH_CMD_PLUS_SHF;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif
	//
	// 8. Test AOP2 (GRPH_CMD_MINUS_SHF)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP02.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP02.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	if (1) { /* support IN/OUT op */
		grphInOutOpA_U.id = GRPH_INOUT_ID_IN_A;
		grphInOutOpA_U.op = GRPH_INOP_SHIFTL_ADD;
		grphInOutOpA_U.shifts = 1;
		grphInOutOpA_U.constant = 2;
		grphInOutOpA_U.p_next = NULL;
		grphImageA.p_inoutop = &grphInOutOpA_U;

		grphInOutOpB_U.id = GRPH_INOUT_ID_IN_B;
		grphInOutOpB_U.op = GRPH_INOP_SHIFTL_SUB;
		grphInOutOpB_U.shifts = 3;
		grphInOutOpB_U.constant = 4;
		grphInOutOpB_U.p_next = NULL;
		grphImageB.p_inoutop = &grphInOutOpB_U;

		grphInOutOpC_U.id = GRPH_INOUT_ID_OUT_C;
		grphInOutOpC_U.op = GRPH_OUTOP_INVERT;
		grphInOutOpC_U.shifts = 0;
		grphInOutOpC_U.constant = 0;
		grphInOutOpC_U.p_next = NULL;
		grphImageC.p_inoutop = &grphInOutOpC_U;
	}
	grphRequest.command = GRPH_CMD_MINUS_SHF;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 9. Test AOP3 (GRPH_CMD_COLOR_EQ)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP03.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP03.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_COLOR_KEY_PROPTY(0x60);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_COLOR_EQ;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
//        FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif
	//
	// 10. Test AOP4 (GRPH_CMD_COLOR_LE)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP04.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP04.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_COLOR_KEY_PROPTY(0x41);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_COLOR_LE;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
//        FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 11. Test AOP5 (GRPH_CMD_A_AND_B)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP05.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP05.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphRequest.command = GRPH_CMD_A_AND_B;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif
	//
	// 12. Test AOP6 (GRPH_CMD_A_OR_B)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP06.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP06.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphRequest.command = GRPH_CMD_A_OR_B;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 13. Test AOP7 (GRPH_CMD_A_XOR_B)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP07.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP07.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphRequest.command = GRPH_CMD_A_XOR_B;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
//        auto_msg(("Test done\r\n"));
#endif
	}
#endif

	//
	// 14. Test AOP8 (GRPH_CMD_TEXT_COPY)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP08.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP08.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphImageC.width = IMG1_WIDTH;
	grphImageC.height = IMG1_HEIGHT;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = 0xFF00A55A;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_COPY;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageC;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
//        FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 15. Test AOP9 (GRPH_CMD_TEXT_AND_A)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP09.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP09.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = 0xFFCCAA03;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_AND_A;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 16. Test AOP10 (GRPH_CMD_TEXT_OR_A)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP10.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP10.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = 0x01042080;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_OR_A;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 17. Test AOP11 (GRPH_CMD_TEXT_XOR_A)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP11.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP11.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = 0x11442288;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_XOR_A;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 18. Test AOP12 (GRPH_CMD_TEXT_AND_AB)
	//
#if 1
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP12.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP12.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = 0xFFF00F3C;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_AND_AB;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif
	//
	// 18. Test AOP13 (GRPH_CMD_BLENDING)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP13.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP13.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_BLD_WA_WB_THR(30, 226, 250);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_BLENDING;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
	FileSys_CloseFile(fileHdl);
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_2; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
//        FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 19. Test AOP14 (GRPH_CMD_ACC)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP14.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP14.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_ACC_PROPTY(TRUE, 0x000);
	grphProperty[0].p_next = &(grphProperty[1]);
	grphProperty[1].id = GRPH_PROPERTY_ID_ACC_SKIPCTRL;
	grphProperty[1].property = GRPH_ACC_SKIP_NONE;
	grphProperty[1].p_next = &(grphProperty[2]);
	grphProperty[2].id = GRPH_PROPERTY_ID_ACC_FULL_FLAG;
	grphProperty[2].property = 0;
	grphProperty[2].p_next = &(grphProperty[3]);
	grphProperty[3].id = GRPH_PROPERTY_ID_PIXEL_CNT;
	grphProperty[3].property = 0;
	grphProperty[3].p_next = &(grphProperty[4]);
	grphProperty[4].id = GRPH_PROPERTY_ID_VALID_PIXEL_CNT;
	grphProperty[4].property = 0;
	grphProperty[4].p_next = &(grphProperty[5]);
	grphProperty[5].id = GRPH_PROPERTY_ID_ACC_RESULT;
	grphProperty[5].property = 0;
	grphProperty[5].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_ACC;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = 0x20;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
#if 1
		*((UINT32 *)(uiImgCaddr + 0)) = grphProperty[2].property;
		*((UINT32 *)(uiImgCaddr + 4)) = grphProperty[3].property;
		*((UINT32 *)(uiImgCaddr + 8)) = grphProperty[4].property;
#endif
		*((UINT32 *)(uiImgCaddr + 0xC)) = grphProperty[5].property;
		fsize = 0x10;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)uiImgCaddr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
#if 1
		if (*((UINT32 *)(uiImgGoldAddr + 0)) != grphProperty[2].property) {
			auto_msg(("%s: full flag not match\r\n", __func__));
			return AUTOTEST_RESULT_FAIL;
		}
		if (*((UINT32 *)(uiImgGoldAddr + 4)) != grphProperty[3].property) {
			auto_msg(("%s: pixel cnt not match\r\n", __func__));
			return AUTOTEST_RESULT_FAIL;
		}
		if (*((UINT32 *)(uiImgGoldAddr + 8)) != grphProperty[4].property) {
			auto_msg(("%s: valid pixel not match\r\n", __func__));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
		if (*((UINT32 *)(uiImgGoldAddr + 0xC)) != grphProperty[5].property) {
			auto_msg(("%s: acc result not match: 0x%x :: 0x%x\r\n",
				__func__,
				(int)*((UINT32 *)(uiImgGoldAddr + 0xC)),
				(int)grphProperty[5].property));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 20. Test AOP15 (GRPH_CMD_MULTIPLY_DIV)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP15.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP15.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	if (1) { /* support IN/OUT op */
		grphInOutOpA_U.id = GRPH_INOUT_ID_IN_A;
		grphInOutOpA_U.op = GRPH_INOP_SHIFTL_SUB;
		grphInOutOpA_U.shifts = 0;
		grphInOutOpA_U.constant = 1;
		grphInOutOpA_U.p_next = NULL;
		grphImageA.p_inoutop = &grphInOutOpA_U;

		grphInOutOpB_U.id = GRPH_INOUT_ID_IN_B;
		grphInOutOpB_U.op = GRPH_INOP_NONE;
		grphInOutOpB_U.shifts = 0;
		grphInOutOpB_U.constant = 0;
		grphInOutOpB_U.p_next = NULL;
		grphImageB.p_inoutop = &grphInOutOpB_U;

		grphInOutOpC_U.id = GRPH_INOUT_ID_OUT_C;
		grphInOutOpC_U.op = GRPH_OUTOP_ADD;
		grphInOutOpC_U.shifts = 0;
		grphInOutOpC_U.constant = 4;
		grphInOutOpC_U.p_next = NULL;
		grphImageC.p_inoutop = &grphInOutOpC_U;
	}
	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_MULT_PROPTY(FALSE, 0x0, 0x0, 0x2);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_MULTIPLY_DIV;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 21. Test AOP16 (GRPH_CMD_PACKING)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP16.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP16.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH / 2;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphRequest.command = GRPH_CMD_PACKING;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 22. Test AOP17 (GRPH_CMD_DEPACKING)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP17_C.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP17_C.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphRequest.command = GRPH_CMD_DEPACKING;
	grphRequest.format = GRPH_FORMAT_16BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageB.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageB.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		// Save image C first
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		// Save image B
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP17_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP17_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageB.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP17_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP17_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		// Read golden B to image A buffer
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgAaddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file AOP17_C.raw compare fail\r\n", __func__));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgAaddr, (void *)grphImageB.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file AOP17_B.raw compare fail\r\n", __func__));
			return AUTOTEST_RESULT_FAIL;
		}

		// Recover contents on image A
		for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
			((UINT8 *)uiImgAaddr)[i] = i & 0xFF;
		}

#endif
	}

	//
	// 23. Test AOP18 (GRPH_CMD_TEXT_MUL)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP18.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP18.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphImageC.width = IMG1_WIDTH;
	grphImageC.height = IMG1_HEIGHT;
	if (1) { /* support IN/OUT op */
		grphInOutOpA_U.id = GRPH_INOUT_ID_IN_A;
		grphInOutOpA_U.op = GRPH_INOP_SHIFTR_ADD;
		grphInOutOpA_U.shifts = 1;
		grphInOutOpA_U.constant = 2;
		grphInOutOpA_U.p_next = NULL;
		grphImageA.p_inoutop = &grphInOutOpA_U;

		grphInOutOpC_U.id = GRPH_INOUT_ID_OUT_C;
		grphInOutOpC_U.op = GRPH_OUTOP_SUB;
		grphInOutOpC_U.shifts = 2;
		grphInOutOpC_U.constant = 1;
		grphInOutOpC_U.p_next = NULL;
		grphImageC.p_inoutop = &grphInOutOpC_U;
	}
	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_TEXT_MULT_PROPTY(GRPH_TEXTMUL_BYTE, GRPH_TEXTMUL_SIGNED, 0x9, 0x1);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_TEXT_MUL;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 24. Test AOP19 (GRPH_CMD_PLANE_BLENDING)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP19.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP19.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphImageC.width = IMG1_WIDTH;
	grphImageC.height = IMG1_HEIGHT;

	grphRequest.command = GRPH_CMD_PLANE_BLENDING;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	// Prepare data on image C
	for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
		((UINT8 *)uiImgCaddr)[i] = (i ^ 0xFF) & 0xFF;
	}

	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset((void*)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

#if 0
	//
	// 25. Test AOP20 (GRPH_CMD_ALPHA_SWITCH)
	//
	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP20.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;
	grphImageC.width = IMG1_WIDTH;
	grphImageC.height = IMG1_HEIGHT;

	grphRequest.command = GRPH_CMD_ALPHA_SWITCH;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	// Prepare data on image C
	for (i = 0; i < IMG1_LOFF * IMG1_HEIGHT; i++) {
		((UINT8 *)uiImgCaddr)[i] = (i ^ 0xFF) & 0xFF;
	}

	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		hwmem_open();
		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 26. Test AOP21 (GRPH_CMD_HWORD_TO_BYTE)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP21.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP21.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[0].property = GRPH_HWORD2BYTE_PROPTY(GRPH_HWORD_TO_BYTE_SIGNED, 0x3);
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_HWORD_TO_BYTE;
	grphRequest.format = GRPH_FORMAT_16BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}
#endif

	//
	// 27. Test AOP22 (GRPH_CMD_1D_LUT)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP22.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP22.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_LUT_BUF;
	grphProperty[0].property = (UINT32)vGrphLut;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_1D_LUT;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 28. Test AOP23 (GRPH_CMD_2D_LUT)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP23.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP23.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF;

	grphProperty[0].id = GRPH_PROPERTY_ID_LUT_BUF;
	grphProperty[0].property = (UINT32)vGrphLut;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_2D_LUT;
	grphRequest.format = GRPH_FORMAT_8BITS;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);
		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
#endif
	}

	//
	// 29. Test AOP24 (GRPH_CMD_RGBYUV_BLEND)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP24_C.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP24_C.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;    // image A: ARGB4444: 2 byte per pixel
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;       // image B: Y: 1 byte per pixel
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF / 2;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;       // image C: UV downsample: avg 1 byte per pixel
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF / 2;

	grphProperty[0].id = GRPH_PROPERTY_ID_YUVFMT;
	grphProperty[0].property = GRPH_YUV_422;
	grphProperty[0].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_RGBYUV_BLEND;
	grphRequest.format = GRPH_FORMAT_16BITS_ARGB4444_RGB;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	// read image C
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		// save image C
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		// save image A
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP24_A.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP24_A.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		// save image B
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP24_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP24_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageB.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		// compare image C
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT / 2) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}

		// compare image B
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP24_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP24_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		// Read golden B to image A buffer
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageB.dram_addr, IMG1_LOFF * IMG1_HEIGHT / 2) != 0) {
			auto_msg(("%s: file AOP24_B.raw compare fail\r\n", __func__));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		FileSys_CloseFile(fileHdl);
#endif
	}

	//
	// 30. Test AOP25 (GRPH_CMD_RGBYUV_COLORKEY)
	//
	sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP25_C.raw");
//	sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP25_C.raw");
	memset(&grphRequest, 0, sizeof(grphRequest));
	memset(&grphImageA, 0, sizeof(grphImageA));
	memset(&grphImageB, 0, sizeof(grphImageB));
	memset(&grphImageC, 0, sizeof(grphImageC));
	memset(&grphInOutOpA_U, 0, sizeof(grphInOutOpA_U));
	memset(&grphInOutOpB_U, 0, sizeof(grphInOutOpB_U));
	memset(&grphInOutOpC_U, 0, sizeof(grphInOutOpC_U));
	memset(grphProperty, 0, sizeof(grphProperty));
	grphImageA.img_id = GRPH_IMG_ID_A;
	grphImageA.dram_addr = uiImgAaddr;
	grphImageA.lineoffset = IMG1_LOFF;    // image A: ARGB4444: 2 byte per pixel
	grphImageA.width = IMG1_WIDTH;
	grphImageA.height = IMG1_HEIGHT;
	grphImageA.p_next = &grphImageB;
	grphImageB.img_id = GRPH_IMG_ID_B;       // image B: Y: 1 byte per pixel
	grphImageB.dram_addr = uiImgBaddr;
	grphImageB.lineoffset = IMG1_LOFF / 2;
	grphImageB.p_next = &grphImageC;
	grphImageC.img_id = GRPH_IMG_ID_C;       // image C: UV downsample: avg 1 byte per pixel
	grphImageC.dram_addr = uiImgCaddr;
	grphImageC.lineoffset = IMG1_LOFF / 2;

	grphProperty[0].id = GRPH_PROPERTY_ID_YUVFMT;
	grphProperty[0].property = GRPH_YUV_422;
	grphProperty[0].p_next = &(grphProperty[1]);
	grphProperty[1].id = GRPH_PROPERTY_ID_NORMAL;
	grphProperty[1].property = 200;
	grphProperty[1].p_next = &(grphProperty[2]);
	grphProperty[2].id = GRPH_PROPERTY_ID_R;
	grphProperty[2].property = 2;
	grphProperty[2].p_next = &(grphProperty[3]);
	grphProperty[3].id = GRPH_PROPERTY_ID_G;
	grphProperty[3].property = 63;
	grphProperty[3].p_next = &(grphProperty[4]);
	grphProperty[4].id = GRPH_PROPERTY_ID_B;
	grphProperty[4].property = 1;
	grphProperty[4].p_next = NULL;
	grphRequest.p_property = &(grphProperty[0]);

	grphRequest.command = GRPH_CMD_RGBYUV_COLORKEY;
	grphRequest.format = GRPH_FORMAT_16BITS_RGB565;
	grphRequest.p_images = &grphImageA;
#if (READ_CORRECT == 1)
	// read image C
	fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
	if (fileHdl == NULL) {
		auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
		return AUTOTEST_RESULT_FAIL;
	}
	fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
	fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
	if (fstatus != 0) {
		auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
		FileSys_CloseFile(fileHdl);
		return AUTOTEST_RESULT_FAIL;
	}
#endif
	for (testGrphId = GRPH_ID_1; testGrphId <= GRPH_ID_1; testGrphId++) {
		memset((void *)grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_open();
//		hwmem_memset(grphImageC.dram_addr, 0, IMG1_LOFF * IMG1_HEIGHT);
//		hwmem_close();
		graph_open(testGrphId);
		graph_request(testGrphId, &grphRequest);
		graph_close(testGrphId);
#if (READ_CORRECT == 0)
		// save image C
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		// save image A
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP25_A.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP25_A.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageC.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		// save image B
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP25_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP25_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		fstatus = FileSys_SeekFile(fileHdl, 0, FST_SEEK_SET);
		if (fstatus != 0) {
			auto_msg(("%s: seek file %s to pos %d fail\r\n", __func__, v_fname_buf, 0));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_WriteFile(fileHdl, (UINT8 *)grphImageB.dram_addr, &fsize, 0, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: write file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}
		FileSys_CloseFile(fileHdl);

		auto_msg(("%s: ptn %s is generated\r\n", __func__, v_fname_buf));
		break;
#else
		// compare image C
		FileSys_CloseFile(fileHdl);
		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageC.dram_addr, IMG1_LOFF * IMG1_HEIGHT / 2) != 0) {
			auto_msg(("%s: file %s compare fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}

		// compare image B
		sprintf(v_fname_buf, "/mnt/sd/Pattern/Graphic/AOP25_B.raw");
//		sprintf(v_fname_buf, "A:\\Pattern\\Graphic\\AOP25_B.raw");
		fileHdl = FileSys_OpenFile(v_fname_buf, FST_OPEN_READ);
		if (fileHdl == NULL) {
			auto_msg(("%s: open file %s fail\r\n", __func__, v_fname_buf));
			return AUTOTEST_RESULT_FAIL;
		}
		// Read golden B to image A buffer
		fsize = IMG1_LOFF * IMG1_HEIGHT / 2;
		fstatus = FileSys_ReadFile(fileHdl, (UINT8 *)uiImgGoldAddr, &fsize, GRPH_AUTO_FS_MODE, NULL);
		if (fstatus != 0) {
			auto_msg(("%s: read file %s fail\r\n", __func__, v_fname_buf));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		if (memcmp((void *)uiImgGoldAddr, (void *)grphImageB.dram_addr, IMG1_LOFF * IMG1_HEIGHT / 2) != 0) {
			auto_msg(("%s: file AOP25_B.raw compare fail\r\n", __func__));
			FileSys_CloseFile(fileHdl);
			return AUTOTEST_RESULT_FAIL;
		}

		FileSys_CloseFile(fileHdl);
#endif
	}

#if (AUTOTEST_THREAD == ENABLE)
	test_multi_task(uiAddr, uiSize);
#endif

	return AUTOTEST_RESULT_OK;
}
#endif

