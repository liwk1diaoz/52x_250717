#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <asm/signal.h>

#include "test_drv.h"
#include "test_main.h"
#include "test_dbg.h"
#if 1

#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include <mach/fmem.h>
#include "kwrap/task.h"

#include "kdrv_videoout/kdrv_vdoout.h"

#if 0
UINT32 engine = KDRV_VDOOUT_ENGINE0;
#else
static unsigned int mode = VDDO_DEV_PANEL;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "video id. default is 15\r\n");
UINT32 engine = KDRV_VDOOUT_ENGINE0;
#endif
THREAD_HANDLE m_ide_tsk_2 = 0;
THREAD_HANDLE m_ide_tsk_3 = 0;
THREAD_HANDLE m_ide_tsk_4 = 0;
THREAD_HANDLE m_ide_tsk_5 = 0;

#define ENABLE_CALLBACK 1

#define IDE_THREAD5_FLGPTN	0x01
static FLGPTN     FLG_ID_IDE_THREAD5;


#if ENABLE_CALLBACK
static INT32 ide_thread5_callback(KDRV_VDDO_EVENT_CB_INFO *p_cb_info,
                                void *user_data)
{
        if (p_cb_info == NULL) {
                return -1;
        } else {
//                p_cb_info->timestamp;
//                p_cb_info->acc_result;
        }

	printk("%s: set flg\r\n", __func__);
        set_flg(FLG_ID_IDE_THREAD5, IDE_THREAD5_FLGPTN);

        return 0;
}
#endif

int nvt_test_thread5(void * __test)
{
	//KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	UINT32 handler = KDRV_DEV_ID(0, engine, 0);
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	OS_CONFIG_FLAG(FLG_ID_IDE_THREAD5);

    callback.callback = (INT32 (*)(VOID *, VOID *))ide_thread5_callback;

	while(!THREAD_SHOULD_STOP) {
		msleep(50);
		DBG_DUMP("thread5 load\r\n");
#if ENABLE_CALLBACK
		kdrv_vddo_trigger(handler, &callback);
		wai_flg(&ptn, FLG_ID_IDE_THREAD5, IDE_THREAD5_FLGPTN, TWF_ORW | TWF_CLR);
#else
		kdrv_vddo_trigger(handler, NULL);
#endif
		//kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END, &kdrv_disp_ctrl);
		DBG_DUMP("thread5 load done\r\n");
		cond_resched();
	}

	return 0;
}

#define IDE_THREAD4_FLGPTN	0x01
static FLGPTN     FLG_ID_IDE_THREAD4;
#if ENABLE_CALLBACK
static INT32 ide_thread4_callback(KDRV_VDDO_EVENT_CB_INFO *p_cb_info,
                                void *user_data)
{
        if (p_cb_info == NULL) {
                return -1;
        } else {
//                p_cb_info->timestamp;
//                p_cb_info->acc_result;
        }

	printk("%s: set flg\r\n", __func__);
        set_flg(FLG_ID_IDE_THREAD4, IDE_THREAD4_FLGPTN);

        return 0;
}
#endif


int nvt_test_thread4(void * __test)
{
	//KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	UINT32 handler = KDRV_DEV_ID(0, engine, 0);
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	OS_CONFIG_FLAG(FLG_ID_IDE_THREAD4);

    callback.callback = (INT32 (*)(VOID *, VOID *))ide_thread4_callback;

	while(!THREAD_SHOULD_STOP) {
		msleep(50);
		DBG_DUMP("thread4 load\r\n");
#if ENABLE_CALLBACK
		kdrv_vddo_trigger(handler, &callback);
		wai_flg(&ptn, FLG_ID_IDE_THREAD4, IDE_THREAD4_FLGPTN, TWF_ORW | TWF_CLR);
#else
		kdrv_vddo_trigger(handler, NULL);
#endif
		//kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END, &kdrv_disp_ctrl);
		DBG_DUMP("thread4 load done\r\n");
		cond_resched();
	}

	return 0;
}

#define IDE_THREAD3_FLGPTN	0x01
static FLGPTN     FLG_ID_IDE_THREAD3;
#if ENABLE_CALLBACK
static INT32 ide_thread3_callback(KDRV_VDDO_EVENT_CB_INFO *p_cb_info,
                                void *user_data)
{
        if (p_cb_info == NULL) {
                return -1;
        } else {
//                p_cb_info->timestamp;
//                p_cb_info->acc_result;
        }

	printk("%s: set flg\r\n", __func__);
        set_flg(FLG_ID_IDE_THREAD3, IDE_THREAD3_FLGPTN);

        return 0;
}
#endif


int nvt_test_thread3(void * __test)
{
	//KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	UINT32 handler = KDRV_DEV_ID(0, engine, 0);
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	OS_CONFIG_FLAG(FLG_ID_IDE_THREAD3);

    callback.callback = (INT32 (*)(VOID *, VOID *))ide_thread3_callback;

	while(!THREAD_SHOULD_STOP) {
		msleep(50);
		DBG_DUMP("thread3 load\r\n");
#if ENABLE_CALLBACK
		kdrv_vddo_trigger(handler, &callback);
		wai_flg(&ptn, FLG_ID_IDE_THREAD3, IDE_THREAD3_FLGPTN, TWF_ORW | TWF_CLR);
#else
		kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END, NULL);
		//kdrv_vddo_trigger(handler, NULL);
#endif
		DBG_DUMP("thread3 load done\r\n");
		cond_resched();
	}


	return 0;
}

#define IDE_THREAD2_FLGPTN	0x01
static FLGPTN     FLG_ID_IDE_THREAD2;
#if ENABLE_CALLBACK
static INT32 ide_thread2_callback(KDRV_VDDO_EVENT_CB_INFO *p_cb_info,
                                void *user_data)
{
        if (p_cb_info == NULL) {
                return -1;
        } else {
//                p_cb_info->timestamp;
//                p_cb_info->acc_result;
        }

	printk("%s: set flg\r\n", __func__);
        set_flg(FLG_ID_IDE_THREAD2, IDE_THREAD2_FLGPTN);

        return 0;
}
#endif

int nvt_test_thread2(void * __test)
{
	//KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	UINT32 handler = KDRV_DEV_ID(0, engine, 0);
	FLGPTN              ptn;
	KDRV_CALLBACK_FUNC callback = {0};

	OS_CONFIG_FLAG(FLG_ID_IDE_THREAD2);

    callback.callback = (INT32 (*)(VOID *, VOID *))ide_thread2_callback;

	while(!THREAD_SHOULD_STOP) {
		msleep(50);
		DBG_DUMP("thread2 load\r\n");
#if ENABLE_CALLBACK
		kdrv_vddo_trigger(handler, &callback);
		wai_flg(&ptn, FLG_ID_IDE_THREAD2, IDE_THREAD2_FLGPTN, TWF_ORW | TWF_CLR);
#else
		//kdrv_vddo_trigger(handler, NULL);
		kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END, NULL);
#endif
		DBG_DUMP("thread2 load done\r\n");
		cond_resched();
	}


	return 0;
}


int nvt_test_thread(void)
{
	KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	KDRV_VDDO_DISPDEV_PARAM kdrv_disp_dev = {0};
	KDRV_VDDO_DISPDEV_PARAM kdrv_disp_dev_temp = {0};
	KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
	UINT32 handler = KDRV_DEV_ID(0, engine, 0);
    UINT32 ii = 0;
	UINT32 size = 0;
	int ret = 0;
	static void *handle_y = NULL;
	static void *handle_uv = NULL;
	

	struct nvt_fmem_mem_info_t      buf_info_y = {0};
	struct nvt_fmem_mem_info_t      buf_info_cbcr = {0};

/*
	void __iomem *top_addr;
	UINT32 temp_reg;

	top_addr = ioremap_nocache(0xf0010000, 0x200);

	temp_reg = ioread32(top_addr + 0x8);
	iowrite32((0x4 | temp_reg), top_addr + 0x8);

	temp_reg = ioread32(top_addr + 0xb8);
	iowrite32((temp_reg & 0xfffff000), top_addr + 0xb8);
*/
	kdrv_vddo_open(0, engine);

	if (mode == VDDO_DEV_PANEL) {
		kdrv_disp_dev_temp.SEL.KDRV_VDDO_REG_IF.lcd_ctrl = VDDO_DISPDEV_LCDCTRL_SIF;
		kdrv_disp_dev_temp.SEL.KDRV_VDDO_REG_IF.ui_sif_ch = VDDO_SIF_CH2;
		kdrv_disp_dev_temp.SEL.KDRV_VDDO_REG_IF.ui_gpio_sen = L_GPIO(22);
		kdrv_disp_dev_temp.SEL.KDRV_VDDO_REG_IF.ui_gpio_clk = L_GPIO(23);
		kdrv_disp_dev_temp.SEL.KDRV_VDDO_REG_IF.ui_gpio_data = L_GPIO(24);
		kdrv_vddo_set(handler, VDDO_DISPDEV_REG_IF, &kdrv_disp_dev_temp);
	}

	if (mode == VDDO_DEV_HDMI) {
        kdrv_disp_dev.SEL.KDRV_VDDO_HDMIMODE.video_id = VDDO_HDMI_1920X1080P60;
        kdrv_disp_dev.SEL.KDRV_VDDO_HDMIMODE.audio_id = VDDO_HDMI_AUDIO_NO_CHANGE;
        kdrv_vddo_set(handler, VDDO_DISPDEV_HDMIMODE, &kdrv_disp_dev);
	}

	kdrv_disp_dev_temp.SEL.KDRV_VDDO_OPEN_DEVICE.dev_id = mode;

	if (kdrv_vddo_set(handler, VDDO_DISPDEV_OPEN_DEVICE, &kdrv_disp_dev_temp) != 0) {
		DBG_ERR("open device err\r\n");
		return 0;
	}

    if (mode == VDDO_DEV_HDMI) {
        kdrv_vddo_get(handler, VDDO_DISPDEV_HDMI_ABI, &kdrv_disp_dev_temp);
        for (ii = 0; ii < kdrv_disp_dev_temp.SEL.KDRV_VDDO_HDMI_ABILITY.len; ii++) {
          DBG_DUMP("HDMI Video Ability %d: %d\r\n", ii, (UINT32)kdrv_disp_dev_temp.SEL.KDRV_VDDO_HDMI_ABILITY.video_abi[ii].video_id);
        }
    }

	kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.dev_id = mode;
	kdrv_vddo_get(handler, VDDO_DISPDEV_DISPSIZE, &kdrv_disp_dev);
	DBG_DUMP("win w %d\r\n", kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_width);
	DBG_DUMP("win h %d\r\n", kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_height);
	DBG_DUMP("buf w %d\r\n", kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width);
	DBG_DUMP("buf h %d\r\n", kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height);

	size = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width * kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height * 2;

	ret = nvt_fmem_mem_info_init(&buf_info_y, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		handle_y = fmem_alloc_from_cma(&buf_info_y, 0);
	}

	ret = nvt_fmem_mem_info_init(&buf_info_cbcr, NVT_FMEM_ALLOC_CACHE, size, NULL);
	if (ret >= 0) {
		handle_uv = fmem_alloc_from_cma(&buf_info_cbcr, 0);
	}

	memset(buf_info_y.vaddr, 0xff, buf_info_y.size);
	memset(buf_info_cbcr.vaddr, 0x00, buf_info_cbcr.size);

	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = TRUE;
	kdrv_vddo_set(handler, VDDO_DISPCTRL_ENABLE, &kdrv_disp_ctrl);


	kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_y = 0x4d;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cb = 0x55;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cr = 0xff;
	kdrv_vddo_set(handler, VDDO_DISPCTRL_BACKGROUND, &kdrv_disp_ctrl);

	kdrv_vddo_trigger(handler, NULL);

	kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END, NULL);

	msleep(5000);

	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = (UINT32)buf_info_y.vaddr;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = (UINT32)buf_info_cbcr.vaddr;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO1;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);

	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = VDDO_DISPBUFFORMAT_YUV422PACK;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_width;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_height;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = 80;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = 0;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_VDO1;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);

	kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_VDO1;
	kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = TRUE;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);

	kdrv_vddo_trigger(handler, NULL);

	msleep(5000);

	kdrv_disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = VDDO_DISPLAYER_VDO1 | VDDO_DISPLAYER_VDO2 | VDDO_DISPLAYER_OSD1;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = FALSE;
	kdrv_vddo_set(handler, VDDO_DISPCTRL_ALL_LYR_EN, &kdrv_disp_ctrl);

	kdrv_vddo_trigger(handler, NULL);

	if (engine == KDRV_VDOOUT_ENGINE0) {
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = (UINT32)buf_info_cbcr.vaddr;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = (UINT32)buf_info_y.vaddr;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO2;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);

		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = VDDO_DISPBUFFORMAT_YUV422PACK;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_width;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_height;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = 80;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = 80;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_VDO2;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);

		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_VDO2;
		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = TRUE;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);

		kdrv_vddo_trigger(handler, NULL);
	}
	msleep(5000);

	kdrv_disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = VDDO_DISPLAYER_VDO1 | VDDO_DISPLAYER_VDO2 | VDDO_DISPLAYER_OSD1;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = FALSE;
	kdrv_vddo_set(handler, VDDO_DISPCTRL_ALL_LYR_EN, &kdrv_disp_ctrl);

	kdrv_vddo_trigger(handler, NULL);

	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = fmem_lookup_pa((UINT32)buf_info_cbcr.vaddr);
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = fmem_lookup_pa((UINT32)buf_info_y.vaddr);
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_OSD1;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);

	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = VDDO_DISPBUFFORMAT_ARGB8565;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width * 2;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_width;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_height;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = 0;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = 80;
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_OSD1;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);

	kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_OSD1;
	kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = TRUE;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);

	kdrv_vddo_trigger(handler, NULL);

	msleep(5000);

	kdrv_disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = VDDO_DISPLAYER_VDO1 | VDDO_DISPLAYER_VDO2 | VDDO_DISPLAYER_OSD1;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = TRUE;
	kdrv_vddo_set(handler, VDDO_DISPCTRL_ALL_LYR_EN, &kdrv_disp_ctrl);

	kdrv_vddo_trigger(handler, NULL);

	msleep(5000);

	kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.type = VDDO_DISPCTRL_BLEND_TYPE_GLOBAL;
	kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.global_alpha = 0x0;
	kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.layer = VDDO_DISPLAYER_OSD1;
	kdrv_vddo_set(handler, VDDO_DISPLAYER_BLEND, &kdrv_disp_layer);

	kdrv_vddo_trigger(handler, NULL);

	msleep(5000);

	if (engine == KDRV_VDOOUT_ENGINE0) {
		kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.type = VDDO_DISPCTRL_BLEND_TYPE_GLOBAL;
		kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.global_alpha = 0x0;
		kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.layer = VDDO_DISPLAYER_VDO2;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_BLEND, &kdrv_disp_layer);

		kdrv_vddo_trigger(handler, NULL);

		msleep(5000);

		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_num = VDDO_DISPFD_NUM0;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_x = 0;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_y = 0;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_w = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_width;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_h = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.win_height;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_bord_w = 20;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_bord_h = 20;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_color_y = 35;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_color_cb = 212;
		kdrv_disp_layer.SEL.KDRV_VDDO_FDSIZE.fd_color_cr = 114;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_FD_CONFIG, &kdrv_disp_layer);

		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_FD;
		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.fd_num = VDDO_DISPFD_NUM0;
		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = TRUE;
		kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);

		kdrv_vddo_trigger(handler, NULL);
	}
	kdrv_vddo_get(handler, VDDO_DISPDEV_OPEN_DEVICE, &kdrv_disp_ctrl);
	DBG_DUMP("open device %d\r\n", kdrv_disp_dev.SEL.KDRV_VDDO_OPEN_DEVICE.dev_id);

	kdrv_vddo_get(handler, VDDO_DISPCTRL_BACKGROUND, &kdrv_disp_ctrl);
	DBG_DUMP("background y cb cr %x %x %x\r\n", kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_y, kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cb, kdrv_disp_ctrl.SEL.KDRV_VDDO_BACKGROUND.color_cr);
/*
	THREAD_CREATE(m_ide_tsk_2, nvt_test_thread2, NULL, "nvt_test_thread2");
    THREAD_RESUME(m_ide_tsk_2);
	THREAD_SET_PRIORITY(m_ide_tsk_2, 30);

	THREAD_CREATE(m_ide_tsk_3, nvt_test_thread3, NULL, "nvt_test_thread3");
    THREAD_RESUME(m_ide_tsk_3);
	THREAD_SET_PRIORITY(m_ide_tsk_3, 30);

	THREAD_CREATE(m_ide_tsk_4, nvt_test_thread4, NULL, "nvt_test_thread4");
    THREAD_RESUME(m_ide_tsk_4);
	THREAD_SET_PRIORITY(m_ide_tsk_4, 30);

	THREAD_CREATE(m_ide_tsk_5, nvt_test_thread5, NULL, "nvt_test_thread5");
    THREAD_RESUME(m_ide_tsk_5);
	THREAD_SET_PRIORITY(m_ide_tsk_5, 30);


	kthread_run(nvt_test_thread2, NULL, "nvt_test_thread2");
	DBG_DUMP("start thread2\r\n");
	kthread_run(nvt_test_thread3, NULL, "nvt_test_thread3");
	DBG_DUMP("start thread3\r\n");
	kthread_run(nvt_test_thread4, NULL, "nvt_test_thread4");
	DBG_DUMP("start thread4\r\n");
	kthread_run(nvt_test_thread5, NULL, "nvt_test_thread5");
	DBG_DUMP("start thread5\r\n");

	while(1){
		cond_resched();
	}
*/
	return 0;
}

#else

static unsigned int mode = 0;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0 is xxx. 1 is xxx");


int nvt_test_thread(void * __test)
{

	while(1) {
		DBG_DUMP("nvt_test_thread running\r\n");
		msleep(2000);
	}


	return 0;
}



#endif
