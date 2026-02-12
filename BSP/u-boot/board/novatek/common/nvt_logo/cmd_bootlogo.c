
#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <part.h>
#include <asm/hardware.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/shm_info.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>
#include "cmd_bootlogo.h"
#include "nvt_gpio.h"
#include "nvt_display_common.h"
#include <asm/arch/display.h>
#include <asm/arch/top.h>
#include <linux/libfdt.h>
#include <asm/arch/gpio.h>
#include <asm/arch/IOAddress.h>
#include <pwm.h>
#include "logo.dat"   //jpg bitstream binary


UINT32 dma_getDramBaseAddr(DMA_ID id);
UINT32 dma_getDramCapacity(DMA_ID id);

static UINT32 chip_id = 0x0;

static int do_bootlogo(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	PDISP_OBJ       p_emu_disp_obj;
	DISPCTRL_PARAM  emu_disp_ctrl;
	DISPDEV_PARAM   emu_disp_dev;
	DISPLAYER_PARAM emu_disp_lyr;
	ulong logo_addr, logo_size;
	unsigned int img_width, img_height;
	int ret=0;
	int nodeoffset; /* next node offset from libfdt */
	const u32 *nodep; /* property node pointer */
	int len;
	int clock_source = 1;
	int clock_freq = 300000000;
	REGVALUE reg_data;
	UINT32 ui_reg_offset;
    int logo_fdt_offset = 0;
    int memcfg_fdt_offset = 0;
    uint8_t *nvt_orig_fdt_buffer = NULL;

	_W_LOG("do_bootlogo\r\n");

    memcfg_fdt_offset = fdt_path_offset(nvt_fdt_buffer, "/nvt_memory_cfg/fdt");
    if (memcfg_fdt_offset < 0) {
        printf("nvt_memory_cfg node isn't existed\r\n");
    } else {
        nodep = (u32 *)fdt_getprop(nvt_fdt_buffer, memcfg_fdt_offset, "reg", NULL);
        if (nodep > 0) {
            nvt_orig_fdt_buffer = ( uint8_t *)be32_to_cpu(nodep[0]);
            if (nvt_orig_fdt_buffer > 0) {
                printf("do_bootlogo: nvt_orig_fdt_buffer = 0x%08x\r\n", (uint32_t)nvt_orig_fdt_buffer);
                logo_fdt_offset = fdt_path_offset(nvt_orig_fdt_buffer, "/logo");
                if (logo_fdt_offset < 0) {
                    printf("mem-tbi.dtsi fdt logo node isn't existed\r\n");
                } else {
                    fdt_setprop_u32(nvt_orig_fdt_buffer, logo_fdt_offset, "enable", 0x1); // 0x1 -> enable logo  display
                }
            }
        }
    }

    logo_fdt_offset = fdt_path_offset(nvt_fdt_buffer, "/logo");
    if (logo_fdt_offset < 0) {
        printf("logo node isn't existed\r\n");
    } else {
        fdt_setprop_u32(nvt_fdt_buffer, logo_fdt_offset, "enable", 0x1); // 0x1 -> enable logo  display
    }

	nvt_pinmux_probe();

    nvt_gpio_setDir(S_GPIO(12), GPIO_DIR_OUTPUT),
    nvt_gpio_setPin(S_GPIO(12));
    mdelay(2);
    nvt_gpio_clearPin(S_GPIO(12));
    mdelay(5);
    nvt_gpio_setPin(S_GPIO(12));

    pwm_init(3, 0, 0);
    pwm_config(3, 1000000, 500000);
    pwm_enable(3);

    nvt_display_dsi_reset();//only for DSI LCD

	p_emu_disp_obj = disp_get_display_object(DISP_1);
	p_emu_disp_obj->open();

	nodeoffset = fdt_path_offset((const void *)nvt_fdt_buffer, "/ide@f0800000");
	if (nodeoffset < 0) {
		printf("no find node ide@f0800000\n");
	} else {
	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "clock-source", &len);
		if (len > 0) {
        	clock_source = be32_to_cpu(nodep[0]);
		} else {
			printf("no ide clock_source info, used default PLL\n");
		}

		nodep = fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "source-frequency", &len);
		if (len > 0) {
			clock_freq  = be32_to_cpu(nodep[0]);
		} else {
        	printf("no ide source-frequency info, used default \n");
		}

		//printf("lcd clk src:%d freq:%d : 0x%x : %d\n", clock_source, clock_freq, *nodep, len);
	}

	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_PANEL;
	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = dispdev_get_lcd1_dev_obj();
	p_emu_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &emu_disp_dev);

	pll_set_ideclock_rate(clock_source, clock_freq);

	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = (DISPCTRL_SRCCLK)clock_source;//DISPCTRL_SRCCLK_PLL6;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_SRCCLK, &emu_disp_ctrl);

	////////////////////////////DISP-1//////////////////////////////////////////////////
	emu_disp_dev.SEL.SET_REG_IF.lcd_ctrl     = DISPDEV_LCDCTRL_GPIO;
	emu_disp_dev.SEL.SET_REG_IF.ui_sif_ch     = SIF_CH2;
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_sen   = L_GPIO(22);
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_clk   = L_GPIO(23);
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_data  = L_GPIO(24);
	p_emu_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &emu_disp_dev);


	p_emu_disp_obj->dev_ctrl(DISPDEV_CLOSE_DEVICE, NULL);
	emu_disp_dev.SEL.GET_PREDISPSIZE.dev_id = DISPDEV_ID_PANEL;
	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_PREDISPSIZE, &emu_disp_dev);
	_Y_LOG("Pre Get Size =%d, %d\r\n", (int)(emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_width), (int)(emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_height));

	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_LCDMODE, &emu_disp_dev);
	_Y_LOG("LCD mode =%d\r\n", (int)(emu_disp_dev.SEL.GET_LCDMODE.mode));

	emu_disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_PANEL;
	p_emu_disp_obj->dev_ctrl(DISPDEV_OPEN_DEVICE, &emu_disp_dev);;

	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_DISPSIZE, &emu_disp_dev);

	emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width     = emu_disp_dev.SEL.GET_DISPSIZE.ui_buf_width;;
	emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height   	= emu_disp_dev.SEL.GET_DISPSIZE.ui_buf_height;
	emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x      = 0;
	emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y      = 0;
	p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);
	_W_LOG(fmt,args...)("DISPLAYER_OP_SET_WINSIZE %d %d\r\n",emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width,emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height);

	ret=nvt_getfdt_logo_addr_size((ulong)nvt_fdt_buffer, &logo_addr, &logo_size);
	if(ret!=0) {
		printf("err:%d\r\n",ret);
		return -1;
	}
	_Y_LOG("logo_addr %x logo_size %x\r\n",logo_addr,logo_size);

	{
		_Y_LOG("start JPEG decode size: %x\n",sizeof(inbuf));
		jpeg_setfmt(1);
		jpeg_decode(inbuf, (unsigned char*)logo_addr);
		jpeg_getdim(&img_width, &img_height);
		_Y_LOG("image size: %d x %d\n", img_width, img_height);
		if(logo_size < img_width*img_height*2) {
			printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,logo_size);
			return -1;
		}
		flush_dcache_range((unsigned long)logo_addr,(unsigned long)(logo_addr+logo_size));
	}
    printf("do_bootlogo: img_width = %d\r\n", img_width);
    printf("do_bootlogo: img_height = %d\r\n", img_height);
	{
		UINT32 buf_w = img_width;
		UINT32 buf_h = img_height;
		UINT32 uiVDO_YAddr = logo_addr;
		UINT32 VDO_BUF_SIZE = buf_w*buf_h;
		UINT32 uiVDO_UVAddr = uiVDO_YAddr+VDO_BUF_SIZE;
        UINT32 i,j, lineoffset = img_width;
        UINT32 pixeloffset = 0;
		_W_LOG("show logo %x %x\r\n",uiVDO_YAddr,uiVDO_UVAddr);

		emu_disp_lyr.SEL.SET_MODE.buf_format   = DISPBUFFORMAT_YUV422PACK;
		emu_disp_lyr.SEL.SET_MODE.buf_mode     = DISPBUFMODE_BUFFER_REPEAT;
		emu_disp_lyr.SEL.SET_MODE.buf_number   = DISPBUFNUM_3;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_MODE, &emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_MODE\r\n");

		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  buf_w;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  buf_h;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = buf_w;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_BUFSIZE %d %d\r\n",buf_w,buf_h);

		#if 0 //fill YUV color for test
		memset((void *)uiVDO_YAddr, 0x4C, VDO_BUF_SIZE/2);
		memset((void *)uiVDO_YAddr+VDO_BUF_SIZE/2, 0x67, VDO_BUF_SIZE/2);
		memset((void *)uiVDO_UVAddr, 0x4C, VDO_BUF_SIZE/2);
		memset((void *)uiVDO_UVAddr+VDO_BUF_SIZE/2, 0xAD, VDO_BUF_SIZE/2);
		#endif
		memset((void *)&emu_disp_lyr,0,sizeof(DISPLAYER_PARAM));
		emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = uiVDO_YAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = uiVDO_UVAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr0 = uiVDO_UVAddr;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1,DISPLAYER_OP_SET_VDOBUFADDR,&emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_VDOBUFADDR\r\n");

		emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
		emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO1;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &emu_disp_ctrl);
		p_emu_disp_obj->load(TRUE);
		p_emu_disp_obj->wait_frm_end(TRUE);

		_W_LOG("DISPCTRL_SET_ALL_LYR_EN\r\n");

		#if 0 //test for progresive bar
        mdelay(1000);
        for (i=0; i < 10; i++) {
            //printf("do_bootlogo: i = %d\r\n", i);
            for (j=0; j < 20; j++) {
                pixeloffset = 320*200 + 80 + i*16 + j*lineoffset;
                //pixeloffset = i*16 + j*lineoffset;
                printf("do_bootlogo: pixeloffset = %d\r\n", pixeloffset);
        		memset((void *)(uiVDO_YAddr+pixeloffset), 0x4C, 16);
        		memset((void *)(uiVDO_UVAddr+pixeloffset) , 0x4C, 16);
            }
            flush_dcache_range((unsigned long)logo_addr,(unsigned long)(logo_addr+logo_size));
            mdelay(500);
        }
		#endif

	}

	return 0;
}



U_BOOT_CMD(
	bootlogo,	2,	1,	do_bootlogo,
	"show lcd bootlogo",
	"no argument means LCD output only, \n"
);
