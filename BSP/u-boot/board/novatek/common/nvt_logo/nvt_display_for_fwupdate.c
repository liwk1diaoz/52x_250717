
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
#include "nvt_display_for_fwupdate.h"
#include <pwm.h>
#if CONFIG_TARGET_NA51055
#include <asm/arch-nvt-na51055_a32/top.h>
#elif CONFIG_TARGET_NA51089
#include <asm/arch-nvt-na51089_a32/top.h>
#endif

#define Y_COLOR  0x4C
#define UV_COLOR 0x4C  // UV is packed together.
#define Y_COLOR_1  0xFF
#define UV_COLOR_1 0x80  // UV is packed together.

#define UPFRAME_ROTATE0 3
#define DOWNFRAME_ROTATE0 3
#define LEFTFRAME_ROTATE0 3
#define RIGHTFRAME_ROTATE0 3
#define BAR_X_ROTATE0 60
#define BAR_Y_ROTATE0 200
#define BAR_W_ROTATE0 22  //Progressive bar and drawing by CPU. The x-offset is movable
#define BAR_H_ROTATE0 20
#define STRING1_X_ROTATE0 20   //"0 %" string
#define STRING1_Y_ROTATE0 194
#define STRING2_X_ROTATE0 252  //"100 %" string
#define STRING2_Y_ROTATE0 194
#define STRING3_X_ROTATE0 72   //"Updating FW..." string
#define STRING3_Y_ROTATE0 90
#define STRING4_X_ROTATE0 72   //"Updating FW OK!" string
#define STRING4_Y_ROTATE0 90
#define STRING5_X_ROTATE0 72   //"Updating FW failed!" string
#define STRING5_Y_ROTATE0 90
#define STRING6_X_ROTATE0 72   //"Updating FW failed and updating again." string
#define STRING6_Y_ROTATE0 90

#define UPFRAME_ROTATE90 3
#define DOWNFRAME_ROTATE90 3
#define LEFTFRAME_ROTATE90 3
#define RIGHTFRAME_ROTATE90 3
#define BAR_X_ROTATE90 200
#define BAR_Y_ROTATE90 260
#define BAR_W_ROTATE90 20  //Progressive bar and drawing by CPU. The x-offset is movable
#define BAR_H_ROTATE90 11
#define STRING1_X_ROTATE90 182
#define STRING1_Y_ROTATE90 252
#define STRING2_X_ROTATE90 182
#define STRING2_Y_ROTATE90 20
#define STRING3_X_ROTATE90 90
#define STRING3_Y_ROTATE90 72
#define STRING4_X_ROTATE90 90
#define STRING4_Y_ROTATE90 72
#define STRING5_X_ROTATE90 90
#define STRING5_Y_ROTATE90 72
#define STRING6_X_ROTATE90 90
#define STRING6_Y_ROTATE90 72

#define UPFRAME_ROTATE270 3
#define DOWNFRAME_ROTATE270 3
#define LEFTFRAME_ROTATE270 3
#define RIGHTFRAME_ROTATE270 3
#define BAR_X_ROTATE270 20
#define BAR_Y_ROTATE270 60
#define BAR_W_ROTATE270 20  //Progressive bar and drawing by CPU. The x-offset is movable
#define BAR_H_ROTATE270 11
#define STRING1_X_ROTATE270 10
#define STRING1_Y_ROTATE270 20
#define STRING2_X_ROTATE270 10
#define STRING2_Y_ROTATE270 252
#define STRING3_X_ROTATE270 102
#define STRING3_Y_ROTATE270 72
#define STRING4_X_ROTATE270 102
#define STRING4_Y_ROTATE270 72
#define STRING5_X_ROTATE270 102
#define STRING5_Y_ROTATE270 72
#define STRING6_X_ROTATE270 102
#define STRING6_Y_ROTATE270 72

static UINT32 g_uiUpFrame = UPFRAME_ROTATE0;
static UINT32 g_uiDownFrame = DOWNFRAME_ROTATE0;
static UINT32 g_uiLeftFrame = LEFTFRAME_ROTATE0;
static UINT32 g_uiRightFrame = RIGHTFRAME_ROTATE0;
static UINT32 g_uiBarX = BAR_X_ROTATE0;
static UINT32 g_uiBarY = BAR_Y_ROTATE0;
static UINT32 g_uiBarW = BAR_W_ROTATE0;
static UINT32 g_uiBarH = BAR_H_ROTATE0;
static UINT32 g_uiStr_1_X = STRING1_X_ROTATE0;
static UINT32 g_uiStr_1_Y = STRING1_Y_ROTATE0;
static UINT32 g_uiStr_2_X = STRING2_X_ROTATE0;
static UINT32 g_uiStr_2_Y = STRING2_Y_ROTATE0;
static UINT32 g_uiStr_3_X = STRING3_X_ROTATE0;
static UINT32 g_uiStr_3_Y = STRING3_Y_ROTATE0;
static UINT32 g_uiStr_4_X = STRING4_X_ROTATE0;
static UINT32 g_uiStr_4_Y = STRING4_Y_ROTATE0;
static UINT32 g_uiStr_5_X = STRING5_X_ROTATE0;
static UINT32 g_uiStr_5_Y = STRING5_Y_ROTATE0;
static UINT32 g_uiStr_6_X = STRING6_X_ROTATE0;
static UINT32 g_uiStr_6_Y = STRING6_Y_ROTATE0;
static UINT32 g_uiRotateDir = DISP_ROTATE_0;
static UINT32 g_bNvtDispInit = FALSE;
static UINT32 g_uiImg_w = 0;
static UINT32 g_uiImg_h = 0;
static UINT32 g_uiImgLineoffset = 0;
static UINT32 g_uiVDO_BUF_SIZE = 0;
static UINT32 g_uiVDO_YAddr = 0;
static UINT32 g_uiVDO_UVAddr = 0;
static UINT32 g_uiVDO2_YAddr = 0;  //ping-pong buffer
static UINT32 g_uiVDO2_UVAddr = 0; //ping-pong buffer
static UINT32 g_uiPinPongIdx = 0;
static UINT32 g_uiLogoAddr = 0;
static UINT32 g_uiLogoSize = 0;
static UINT32 g_uiTotalBarNum = 0; //(write partition + read back partition) for the bumber of progressive bar
static UINT32 g_uiXBarStep = 0;
static UINT32 g_uiPixelOffset = 0;
static UINT32 g_uiStrYBufSize = 64*1024;
static UINT32 g_uiStr1_w = 0;
static UINT32 g_uiStr1_h = 0;
static UINT32 g_uiStr2_w = 0;
static UINT32 g_uiStr2_h = 0;
static UINT32 g_uiStr3_w = 0;
static UINT32 g_uiStr3_h = 0;
static UINT32 g_uiStr4_w = 0;
static UINT32 g_uiStr4_h = 0;
static UINT32 g_uiStr5_w = 0;
static UINT32 g_uiStr5_h = 0;
static UINT32 g_uiStr6_w = 0;
static UINT32 g_uiStr6_h = 0;
static UINT32 g_uiStr1_YAddr = 0;  //"0 %" string
static UINT32 g_uiStr1_UVAddr = 0;
static UINT32 g_uiStr2_YAddr = 0;  //"100 %" string
static UINT32 g_uiStr2_UVAddr = 0;
static UINT32 g_uiStr3_YAddr = 0;  //"Updating FW..." string
static UINT32 g_uiStr3_UVAddr = 0;
static UINT32 g_uiStr4_YAddr = 0;  //"Updating FW OK!" string
static UINT32 g_uiStr4_UVAddr = 0;
static UINT32 g_uiStr5_YAddr = 0;  //"Updating FW failed" string
static UINT32 g_uiStr5_UVAddr = 0;
static UINT32 g_uiStr6_YAddr = 0;  //"Updating again" string
static UINT32 g_uiStr6_UVAddr = 0;
static UINT32 g_bUpdatFwAgain = FALSE;

int nvt_display_init(void)
{
	PDISP_OBJ       p_emu_disp_obj;
	DISPCTRL_PARAM  emu_disp_ctrl;
	DISPDEV_PARAM   emu_disp_dev;
	DISPLAYER_PARAM emu_disp_lyr;
	ulong logo_addr, logo_size;
	int ret=0;
	int nodeoffset; /* next node offset from libfdt */
	const u32 *nodep; /* property node pointer */
	int len;
	int clock_source = 1;
	int clock_freq = 300000000;
	UINT32 ui_reg_offset;
    int logo_fdt_offset = 0;
    int display_fdt_offset = 0;
    int memcfg_fdt_offset = 0;
    uint8_t *nvt_orig_fdt_buffer = NULL;
    UINT32 u32LcdRotateDir = DISP_ROTATE_0;
    UINT32 u32LcdType = 0;
    UINT32 u32LcdResetPin = 0;  //lcd reset
	UINT32 u32LcdEnablePin = 0;  //lcd enable, optional for the circuit design.
	UINT32 u32LcdEnableOutIoSts = 0; //lcd enable output high or low, optional for the circuit design.
	UINT32 u32LcdPowerPin = 0;  //lcd power, optional for the circuit design. 
	UINT32 u32LcdPowerOutIoSts = 0;  //lcd power output high or low, optional for the circuit design.
	UINT32 u32LcdStandbyPin = 0;  //lcdsStandby, optional for the circuit design.    
	UINT32 u32LcdStandbyOutIoSts = 0; //lcd standby output high or low, optional for the circuit design.
    UINT32 u32LcdBlCtrlPin = 0; //lcd backlight for gpio
    UINT32 u32LcdBlOutIoSts = 0; //lcd backlight output high or low
    UINT32 u32LcdBl2CtrlPin = 0; //lcd backlight for gpio, optional for the circuit design.
    UINT32 u32LcdBl2OutIoSts = 0; //lcd backlight output high or low, optional for the circuit design.
    int i32PwmId = 0xFFFFFFFF;
    int i32PwmInv = 0;
    int i32PwmPeriod = 0;
    int i32PwmDuty = 0;
    UINT32 ui_sif_ch = SIF_CH2; //sif channel
    UINT32 ui_sif_cs = L_GPIO(22); //sif chip select
    UINT32 ui_sif_clk = L_GPIO(23);
    UINT32 ui_sif_data = L_GPIO(24);


    if (g_bNvtDispInit) {
        return 0;
    }

	_W_LOG("nvt_display_init\r\n");

	nvt_pinmux_probe();

    memcfg_fdt_offset = fdt_path_offset(nvt_fdt_buffer, "/nvt_memory_cfg/fdt");
    if (memcfg_fdt_offset < 0) {
        printf("nvt_memory_cfg node isn't existed\r\n");
    } else {
        nodep = (u32 *)fdt_getprop(nvt_fdt_buffer, memcfg_fdt_offset, "reg", NULL);
        if (nodep > 0) {
            nvt_orig_fdt_buffer = ( uint8_t *)be32_to_cpu(nodep[0]);
            if (nvt_orig_fdt_buffer > 0) {
                printf("nvt_display_init: nvt_orig_fdt_buffer = 0x%08x\r\n", (uint32_t)nvt_orig_fdt_buffer);
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

  	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_type", &len);
        if (nodep > 0) {
            u32LcdType = be32_to_cpu(nodep[0]);

            switch (u32LcdType) {
            case PINMUX_LCD_SEL_MIPI:
               	clock_source = 0; //480MHz
                clock_freq = 480000000;
                break;

            default:
               	clock_source = 1; //PLL6
                clock_freq = 300000000;
                break;
            }
        }

  	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_rotate", &len);
        if (nodep > 0) {
            u32LcdRotateDir = be32_to_cpu(nodep[0]);

            printf("u32LcdRotateDir = %d\r\n", u32LcdRotateDir);
            switch (u32LcdRotateDir) {
            case 0:
                g_uiRotateDir = DISP_ROTATE_0;
                break;

            case 90:
                g_uiRotateDir = DISP_ROTATE_90;
                break;

            case 270:
                g_uiRotateDir = DISP_ROTATE_270;
                break;

            default:
                g_uiRotateDir = DISP_ROTATE_0;
                break;
            }
        }

   	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_enable", &len); //optional for the circuit design
        if (nodep > 0) {
            u32LcdEnablePin  = be32_to_cpu(nodep[0]);
            u32LcdEnableOutIoSts = be32_to_cpu(nodep[1]);
        }

   	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_power", &len); //optional for the circuit design
        if (nodep > 0) {
            u32LcdPowerPin  = be32_to_cpu(nodep[0]);
            u32LcdPowerOutIoSts = be32_to_cpu(nodep[1]);
        }

		nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_standby", &len); //optional for the circuit design
        if (nodep > 0) {
            u32LcdStandbyPin  = be32_to_cpu(nodep[0]); 
            u32LcdStandbyOutIoSts = be32_to_cpu(nodep[1]);
        }

  	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_reset", &len);
        if (nodep > 0){
            u32LcdResetPin = be32_to_cpu(nodep[0]);
        }

   	    nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_bl_gpio", &len);
        if (nodep > 0) {
            u32LcdBlCtrlPin  = be32_to_cpu(nodep[0]);
            u32LcdBlOutIoSts = be32_to_cpu(nodep[1]);
        }

    	nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_bl2_gpio", &len); //optional for the circuit design
        if (nodep > 0) {
            u32LcdBl2CtrlPin  = be32_to_cpu(nodep[0]);
            u32LcdBl2OutIoSts = be32_to_cpu(nodep[1]);
        }

     	nodep = fdt_getprop((const void *)nvt_fdt_buffer, logo_fdt_offset, "lcd_bl_pwm", &len);
         if (nodep > 0) {
            i32PwmId      =  be32_to_cpu(nodep[0]) - 0x20; //Refer to "#define P_GPIO(pin) (pin + 0x20)"
            i32PwmInv     =  be32_to_cpu(nodep[1]);
            i32PwmPeriod  =  be32_to_cpu(nodep[2]);
            i32PwmDuty    =  be32_to_cpu(nodep[3]);
        }
    }

    display_fdt_offset = fdt_path_offset(nvt_fdt_buffer, "/display");
    if (display_fdt_offset < 0) {
        printf("display node isn't existed\r\n");
    } else {
      	nodep = fdt_getprop((const void *)nvt_fdt_buffer, display_fdt_offset, "sif_channel", &len);
        if (nodep > 0){
            ui_sif_ch = be32_to_cpu(nodep[0]);
        }

      	nodep = fdt_getprop((const void *)nvt_fdt_buffer, display_fdt_offset, "gpio_cs", &len);
        if (nodep > 0){
            ui_sif_cs = be32_to_cpu(nodep[0]);
        }

      	nodep = fdt_getprop((const void *)nvt_fdt_buffer, display_fdt_offset, "gpio_clk", &len);
        if (nodep > 0){
            ui_sif_clk = be32_to_cpu(nodep[0]);
        }

      	nodep = fdt_getprop((const void *)nvt_fdt_buffer, display_fdt_offset, "gpio_data", &len);
        if (nodep > 0){
            ui_sif_data = be32_to_cpu(nodep[0]);
        }
    }

    if (u32LcdEnablePin != 0) {
        nvt_gpio_setDir(u32LcdEnablePin, GPIO_DIR_OUTPUT);
        if (u32LcdEnableOutIoSts) {
            nvt_gpio_setPin(u32LcdEnablePin);
        } else {
            nvt_gpio_clearPin(u32LcdEnablePin);
        }
    }

    if (u32LcdPowerPin != 0) {
        nvt_gpio_setDir(u32LcdPowerPin, GPIO_DIR_OUTPUT);
        if (u32LcdPowerOutIoSts) {
            nvt_gpio_setPin(u32LcdPowerPin);
        } else {
            nvt_gpio_clearPin(u32LcdPowerPin);
        }
    }	
	
    if (u32LcdStandbyPin != 0) {
        nvt_gpio_setDir(u32LcdStandbyPin, GPIO_DIR_OUTPUT);
        if (u32LcdStandbyOutIoSts) {
            nvt_gpio_setPin(u32LcdStandbyPin);
        } else {
            nvt_gpio_clearPin(u32LcdStandbyPin);
        }
    }		

    if (u32LcdResetPin !=0) {
        nvt_gpio_setDir(u32LcdResetPin, GPIO_DIR_OUTPUT);
        nvt_gpio_setPin(u32LcdResetPin);
        mdelay(2);
        nvt_gpio_clearPin(u32LcdResetPin);
        mdelay(5);
        nvt_gpio_setPin(u32LcdResetPin);
    }

    if (u32LcdBlCtrlPin != 0) {
        nvt_gpio_setDir(u32LcdBlCtrlPin, GPIO_DIR_OUTPUT);
        if (u32LcdBlOutIoSts) {
            nvt_gpio_setPin(u32LcdBlCtrlPin);
        } else {
            nvt_gpio_clearPin(u32LcdBlCtrlPin);
        }
    }

    if (u32LcdBl2CtrlPin != 0) {
        nvt_gpio_setDir(u32LcdBl2CtrlPin, GPIO_DIR_OUTPUT);
        if (u32LcdBl2OutIoSts) {
            nvt_gpio_setPin(u32LcdBl2CtrlPin);
        } else {
            nvt_gpio_clearPin(u32LcdBl2CtrlPin);
        }
    }

    if (i32PwmId != 0xFFFFFFFF) {
        pwm_init(i32PwmId, 0, i32PwmInv);
        pwm_config(i32PwmId, i32PwmDuty, i32PwmPeriod);
        pwm_enable(i32PwmId);
    }

    nvt_display_dsi_reset(); //only for DSI LCD

	p_emu_disp_obj = disp_get_display_object(DISP_1);
	p_emu_disp_obj->open();

	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_PANEL;
	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = dispdev_get_lcd1_dev_obj();
	p_emu_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &emu_disp_dev);

	pll_set_ideclock_rate(clock_source, clock_freq);

	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = (DISPCTRL_SRCCLK)clock_source;//DISPCTRL_SRCCLK_PLL6;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_SRCCLK, &emu_disp_ctrl);

	////////////////////////////DISP-1//////////////////////////////////////////////////
	emu_disp_dev.SEL.SET_REG_IF.lcd_ctrl     = DISPDEV_LCDCTRL_GPIO;
	emu_disp_dev.SEL.SET_REG_IF.ui_sif_ch     = ui_sif_ch;
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_sen   = ui_sif_cs;
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_clk   = ui_sif_clk;
	emu_disp_dev.SEL.SET_REG_IF.ui_gpio_data  = ui_sif_data;
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

	ret = nvt_getfdt_logo_addr_size((ulong)nvt_fdt_buffer, &logo_addr, &logo_size);
	if(ret!=0) {
		printf("err:%d\r\n",ret);
		return -1;
	}
	_Y_LOG("logo_addr %x logo_size %x\r\n",logo_addr,logo_size); //logo_size is logo-fb in nvt-mem-tbl.dtsi
    g_uiLogoAddr = logo_addr;
    if ((logo_size >= 0x1C0000)) {
        g_uiLogoSize = 0x100000; //logo_size is 0x180000 and the 512KB is for string display
        g_uiStrYBufSize = 64*1024;
        g_uiStr1_YAddr = g_uiLogoAddr + g_uiLogoSize; //shit 1MB from g_uiLogoAddr
        g_uiStr2_YAddr = g_uiStr1_YAddr + g_uiStrYBufSize*2; //128KB, address and size must be 64 byte alignment
        g_uiStr3_YAddr = g_uiStr2_YAddr + g_uiStrYBufSize*2;
        g_uiStr4_YAddr = g_uiStr3_YAddr + g_uiStrYBufSize*2;
        g_uiStr5_YAddr = g_uiStr4_YAddr + g_uiStrYBufSize*2;
        g_uiStr6_YAddr = g_uiStr5_YAddr + g_uiStrYBufSize*2;
    } else {
        printf("\033[0;31m logo_size is smaller than 0x100000\033[0m\r\n");
        return -1;
    }

	ret = nvt_display_decode_string();
    if (ret < 0) {
        printf("\033[0;31m Decode string errr. \033[0m\r\n");
        return -1;
    }

    {
        switch (g_uiRotateDir) {
        case DISP_ROTATE_90:
        case DISP_ROTATE_270:
            g_uiImg_w = 240;
            g_uiImg_h = 320;
            g_uiImgLineoffset = 240;
            break;

        case DISP_ROTATE_0:
        default:
            g_uiImg_w = 320;
            g_uiImg_h = 240;
            g_uiImgLineoffset = 320;
            break;
        }

        switch (g_uiRotateDir) {
        case DISP_ROTATE_90:
            g_uiUpFrame = UPFRAME_ROTATE90;
            g_uiDownFrame = DOWNFRAME_ROTATE90;
            g_uiLeftFrame = LEFTFRAME_ROTATE90;
            g_uiRightFrame = RIGHTFRAME_ROTATE90;
            g_uiBarX = BAR_X_ROTATE90;
            g_uiBarY = BAR_Y_ROTATE90;
            g_uiBarW = BAR_W_ROTATE90;
            g_uiBarH = BAR_H_ROTATE90;
            g_uiStr_1_X = STRING1_X_ROTATE90;
            g_uiStr_1_Y = STRING1_Y_ROTATE90;
            g_uiStr_2_X = STRING2_X_ROTATE90;
            g_uiStr_2_Y = STRING2_Y_ROTATE90;
            g_uiStr_3_X = STRING3_X_ROTATE90;
            g_uiStr_3_Y = STRING3_Y_ROTATE90;
            g_uiStr_4_X = STRING4_X_ROTATE90;
            g_uiStr_4_Y = STRING4_Y_ROTATE90;
            g_uiStr_5_X = STRING5_X_ROTATE90;
            g_uiStr_5_Y = STRING5_Y_ROTATE90;
            g_uiStr_6_X = STRING6_X_ROTATE90;
            g_uiStr_6_Y = STRING6_Y_ROTATE90;
            break;
        case DISP_ROTATE_270:
            g_uiUpFrame = UPFRAME_ROTATE270;
            g_uiDownFrame = DOWNFRAME_ROTATE270;
            g_uiLeftFrame = LEFTFRAME_ROTATE270;
            g_uiRightFrame = RIGHTFRAME_ROTATE270;
            g_uiBarX = BAR_X_ROTATE270;
            g_uiBarY = BAR_Y_ROTATE270;
            g_uiBarW = BAR_W_ROTATE270;
            g_uiBarH = BAR_H_ROTATE270;
            g_uiStr_1_X = STRING1_X_ROTATE270;
            g_uiStr_1_Y = STRING1_Y_ROTATE270;
            g_uiStr_2_X = STRING2_X_ROTATE270;
            g_uiStr_2_Y = STRING2_Y_ROTATE270;
            g_uiStr_3_X = STRING3_X_ROTATE270;
            g_uiStr_3_Y = STRING3_Y_ROTATE270;
            g_uiStr_4_X = STRING4_X_ROTATE270;
            g_uiStr_4_Y = STRING4_Y_ROTATE270;
            g_uiStr_5_X = STRING5_X_ROTATE270;
            g_uiStr_5_Y = STRING5_Y_ROTATE270;
            g_uiStr_6_X = STRING6_X_ROTATE270;
            g_uiStr_6_Y = STRING6_Y_ROTATE270;
            break;

        case DISP_ROTATE_0:
        default:
            g_uiUpFrame = UPFRAME_ROTATE0;
            g_uiDownFrame = DOWNFRAME_ROTATE0;
            g_uiLeftFrame = LEFTFRAME_ROTATE0;
            g_uiRightFrame = RIGHTFRAME_ROTATE0;
            g_uiBarX = BAR_X_ROTATE0;
            g_uiBarY = BAR_Y_ROTATE0;
            g_uiBarW = BAR_W_ROTATE0;
            g_uiBarH = BAR_H_ROTATE0;
            g_uiStr_1_X = STRING1_X_ROTATE0;
            g_uiStr_1_Y = STRING1_Y_ROTATE0;
            g_uiStr_2_X = STRING2_X_ROTATE0;
            g_uiStr_2_Y = STRING2_Y_ROTATE0;
            g_uiStr_3_X = STRING3_X_ROTATE0;
            g_uiStr_3_Y = STRING3_Y_ROTATE0;
            g_uiStr_4_X = STRING4_X_ROTATE0;
            g_uiStr_4_Y = STRING4_Y_ROTATE0;
            g_uiStr_5_X = STRING5_X_ROTATE0;
            g_uiStr_5_Y = STRING5_Y_ROTATE0;
            g_uiStr_6_X = STRING6_X_ROTATE0;
            g_uiStr_6_Y = STRING6_Y_ROTATE0;
            break;
        }


        g_uiVDO_BUF_SIZE = g_uiImgLineoffset*g_uiImg_h;
        g_uiVDO_YAddr = g_uiLogoAddr;
        g_uiVDO_UVAddr = g_uiVDO_YAddr + g_uiVDO_BUF_SIZE;
        g_uiVDO2_YAddr = g_uiLogoAddr + g_uiLogoSize/2;
        g_uiVDO2_UVAddr = g_uiVDO2_YAddr + g_uiVDO_BUF_SIZE;

        nvt_display_set_background();

        nvt_display_draw_init_string();

        nvt_display_copy_pingpong_buffer();

        flush_dcache_range((unsigned long)g_uiLogoAddr,(unsigned long)(g_uiLogoAddr+g_uiLogoSize));

		_W_LOG("show logo %x %x\r\n",g_uiVDO_YAddr,g_uiVDO_UVAddr);

		emu_disp_lyr.SEL.SET_MODE.buf_format   = DISPBUFFORMAT_YUV422PACK;
		emu_disp_lyr.SEL.SET_MODE.buf_mode     = DISPBUFMODE_BUFFER_REPEAT;
		emu_disp_lyr.SEL.SET_MODE.buf_number   = DISPBUFNUM_3;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_MODE, &emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_MODE\r\n");

		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  g_uiImg_w;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  g_uiImg_h;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = g_uiImg_w;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_BUFSIZE %d %d\r\n",g_uiImg_w,g_uiImg_h);

		memset((void *)&emu_disp_lyr,0,sizeof(DISPLAYER_PARAM));
		emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = g_uiVDO_YAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = g_uiVDO_UVAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr0 = g_uiVDO_UVAddr;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1,DISPLAYER_OP_SET_VDOBUFADDR,&emu_disp_lyr);
		_W_LOG("DISPLAYER_OP_SET_VDOBUFADDR\r\n");

		emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
		emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO1;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &emu_disp_ctrl);
		p_emu_disp_obj->load(TRUE);
		p_emu_disp_obj->wait_frm_end(TRUE);

		_W_LOG("DISPCTRL_SET_ALL_LYR_EN\r\n");
    }

    g_bNvtDispInit = TRUE;
	return 0;
}

int nvt_display_decode_string(void)
{
	unsigned int img_width, img_height;

    jpeg_setfmt(1);

    //String 1
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(percent0_str, (unsigned char*)g_uiStr1_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(percent0_rotate90_str, (unsigned char*)g_uiStr1_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(percent0_rotate270_str, (unsigned char*)g_uiStr1_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str1: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr1_YAddr,(unsigned long)(g_uiStr1_YAddr + g_uiStrYBufSize*2));
    g_uiStr1_w = img_width;
    g_uiStr1_h = img_height;
    g_uiStr1_UVAddr = g_uiStr1_YAddr + img_width*img_height;

    //String 2
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(percent100_str, (unsigned char*)g_uiStr2_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(percent100_rotate90_str, (unsigned char*)g_uiStr2_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(percent100_rotate270_str, (unsigned char*)g_uiStr2_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str2: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr2_YAddr,(unsigned long)(g_uiStr2_YAddr + g_uiStrYBufSize*2));
    g_uiStr2_w = img_width;
    g_uiStr2_h = img_height;
    g_uiStr2_UVAddr = g_uiStr2_YAddr + img_width*img_height;

    //String 3
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(updatefw_str, (unsigned char*)g_uiStr3_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(updatefw_rotate90_str, (unsigned char*)g_uiStr3_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(updatefw_rotate270_str, (unsigned char*)g_uiStr3_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str3: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr3_YAddr,(unsigned long)(g_uiStr3_YAddr + g_uiStrYBufSize*2));
    g_uiStr3_w = img_width;
    g_uiStr3_h = img_height;
    g_uiStr3_UVAddr = g_uiStr3_YAddr + img_width*img_height;

    //String 4
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(updatefwok_str, (unsigned char*)g_uiStr4_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(updatefwok_rotate90_str, (unsigned char*)g_uiStr4_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(updatefwok_rotate270_str, (unsigned char*)g_uiStr4_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str4: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr4_YAddr,(unsigned long)(g_uiStr4_YAddr + g_uiStrYBufSize*2));
    g_uiStr4_w = img_width;
    g_uiStr4_h = img_height;
    g_uiStr4_UVAddr = g_uiStr4_YAddr + img_width*img_height;

    //String 5
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(updatefwfailed_str, (unsigned char*)g_uiStr5_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(updatefwfailed_rotate90_str, (unsigned char*)g_uiStr5_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(updatefwfailed_rotate270_str, (unsigned char*)g_uiStr5_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str5: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr5_YAddr,(unsigned long)(g_uiStr5_YAddr + g_uiStrYBufSize*2));
    g_uiStr5_w = img_width;
    g_uiStr5_h = img_height;
    g_uiStr5_UVAddr = g_uiStr5_YAddr + img_width*img_height;

    //String 6
    if (g_uiRotateDir == DISP_ROTATE_0) {
        jpeg_decode(updatefwfailed_updatefwagain_str, (unsigned char*)g_uiStr6_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_90) {
	    jpeg_decode(updatefwfailed_updatefwagain_rotate90_str, (unsigned char*)g_uiStr6_YAddr);
    } else if (g_uiRotateDir == DISP_ROTATE_270) {
        jpeg_decode(updatefwfailed_updatefwagain_rotate270_str, (unsigned char*)g_uiStr6_YAddr);
    }
	jpeg_getdim(&img_width, &img_height);
	_Y_LOG("str6: image size: %d x %d\n", img_width, img_height);
	if(g_uiStrYBufSize*2 < img_width*img_height*2) {
		printf("(%d,%d) size small 0x%x\r\n",img_width,img_height,g_uiStrYBufSize*2);
		return -1;
	}
	flush_dcache_range((unsigned long)g_uiStr6_YAddr,(unsigned long)(g_uiStr6_YAddr + g_uiStrYBufSize*2));
    g_uiStr6_w = img_width;
    g_uiStr6_h = img_height;
    g_uiStr6_UVAddr = g_uiStr6_YAddr + img_width*img_height;

    return 0;
}

void nvt_display_set_background(void)
{
    switch (g_uiPinPongIdx){
    case 0:
		memset((void *)g_uiVDO_YAddr, Y_COLOR_1, g_uiVDO_BUF_SIZE);
		memset((void *)g_uiVDO_UVAddr,UV_COLOR_1, g_uiVDO_BUF_SIZE);
        break;

    case 1:
		memset((void *)g_uiVDO2_YAddr, Y_COLOR_1, g_uiVDO_BUF_SIZE);
		memset((void *)g_uiVDO2_UVAddr,UV_COLOR_1, g_uiVDO_BUF_SIZE);
        break;
    }
}

void nvt_display_copy_pingpong_buffer(void)
{
    switch (g_uiPinPongIdx){
    case 0:
		memcpy((void *)g_uiVDO2_YAddr, (void *)g_uiVDO_YAddr, g_uiVDO_BUF_SIZE);
		memcpy((void *)g_uiVDO2_UVAddr,(void *)g_uiVDO_UVAddr, g_uiVDO_BUF_SIZE);
        break;

    case 1:
		memcpy((void *)g_uiVDO_YAddr, (void *)g_uiVDO2_YAddr, g_uiVDO_BUF_SIZE);
		memcpy((void *)g_uiVDO_UVAddr,(void *)g_uiVDO2_UVAddr, g_uiVDO_BUF_SIZE);
        break;
    }
}

void nvt_display_draw_init_string(void)
{
    UINT32 i,j;

    switch (g_uiPinPongIdx){
    case 0:
        if (g_uiRotateDir == DISP_ROTATE_0) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        } else if (g_uiRotateDir == DISP_ROTATE_90) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);
            }

            for (i = 0; i<g_uiStr3_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        } else if (g_uiRotateDir == DISP_ROTATE_270) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);
            }

            for (i = 0; i<g_uiStr3_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        }
        break;

    case 1:
        if (g_uiRotateDir == DISP_ROTATE_0) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        } else if (g_uiRotateDir == DISP_ROTATE_90) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);
            }

            for (i = 0; i<g_uiStr3_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        } else if (g_uiRotateDir == DISP_ROTATE_270) {
            for (i = 0; i<g_uiStr1_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_1_Y + g_uiStr_1_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr1_YAddr + i*g_uiStr1_w), g_uiStr1_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr1_UVAddr + i*g_uiStr1_w), g_uiStr1_w);

                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_2_Y + g_uiStr_2_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr2_YAddr + i*g_uiStr2_w), g_uiStr2_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr2_UVAddr + i*g_uiStr2_w), g_uiStr2_w);
            }

            for (i = 0; i<g_uiStr3_h; i++) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_3_Y + g_uiStr_3_X + i*g_uiImgLineoffset;
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr3_YAddr + i*g_uiStr3_w), g_uiStr3_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr3_UVAddr + i*g_uiStr3_w), g_uiStr3_w);
            }
        }
        break;
    }
}

void nvt_display_draw(UINT32 u32Id)
{
	PDISP_OBJ       p_emu_disp_obj = NULL;
	DISPCTRL_PARAM  emu_disp_ctrl = {0};
	DISPLAYER_PARAM emu_disp_lyr = {0};
    UINT32 i,j;
    UINT32 uiYAddr = 0, uiUVAddr = 0;

    if (!g_bNvtDispInit) {
        return;
    }

    p_emu_disp_obj = disp_get_display_object(DISP_1);

    if (g_uiPinPongIdx == 0) {
        g_uiPinPongIdx = 1;
    } else {
        g_uiPinPongIdx = 0;
    }

    if (!g_uiVDO2_YAddr) {
        g_uiPinPongIdx = 0;
    }

    switch (u32Id) {
    case DISP_DRAW_INIT:
        break;

    case DISP_DRAW_EXTERNAL_FRAME:
        if (g_uiRotateDir == DISP_ROTATE_0) {;
            for (j=0; j < (g_uiUpFrame + g_uiBarH + g_uiDownFrame); j++) {
                g_uiPixelOffset = g_uiImgLineoffset*(g_uiBarY - g_uiUpFrame) + (g_uiBarX - g_uiLeftFrame) + j*g_uiImgLineoffset;
                if (j < g_uiUpFrame) {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                } else if (j >= (g_uiUpFrame + g_uiBarH)) {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW + g_uiRightFrame));
                } else {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW), 0x80, g_uiRightFrame);

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarW), 0x80, g_uiRightFrame);
                }
            }
        } else if (g_uiRotateDir == DISP_ROTATE_90) {
            for (j=0; j < (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum + g_uiDownFrame); j++) {
                g_uiPixelOffset = (g_uiBarY + g_uiDownFrame)*g_uiImgLineoffset + (g_uiBarX - g_uiLeftFrame) - j*g_uiImgLineoffset;
                if ((j < g_uiDownFrame) || (j >= (g_uiDownFrame + g_uiBarH*g_uiTotalBarNum))) {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                } else {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x80, g_uiRightFrame);

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x80, g_uiRightFrame);

                }
            }
        } else if (g_uiRotateDir == DISP_ROTATE_270) {
            for (j=0; j < (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum + g_uiDownFrame); j++) {
                g_uiPixelOffset = g_uiImg_w*(g_uiBarY - g_uiUpFrame) + (g_uiBarX - g_uiLeftFrame) + j*g_uiImgLineoffset;
                if ((j < g_uiUpFrame) || (j >= (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum))) {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarW + g_uiRightFrame));
                } else {
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x80, g_uiRightFrame);

                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                    memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x00, g_uiRightFrame);
                    memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarW), 0x80, g_uiRightFrame);

                }
            }
        }
        break;

    case DISP_DRAW_UPDATING_BAR:
        g_uiXBarStep++;
                if (g_uiRotateDir == DISP_ROTATE_0) {
			for (i=1; i <= g_uiXBarStep; i++) 
			{
		            	for (j = 0; j < g_uiBarH; j++) 
				{
                    g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + (i-1)*g_uiBarW /*offset for next bar*/ + j*g_uiImgLineoffset;
		            	

						
                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
			}
			}
		} else if ((g_uiRotateDir == DISP_ROTATE_90) || (g_uiRotateDir == DISP_ROTATE_270)) {
	        for (j = 0; j < g_uiBarH*g_uiXBarStep; j++) {
				if (g_uiRotateDir == DISP_ROTATE_90) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX - j*g_uiImgLineoffset;
                } else if (g_uiRotateDir == DISP_ROTATE_270) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + j*g_uiImgLineoffset;
                }

                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
            }
        }
        break;

    case DISP_DRAW_UPDATEFW_OK:
		if (g_uiRotateDir == DISP_ROTATE_0) {
        for (i=1; i <= g_uiXBarStep; i++) {
            for (j = 0; j < g_uiBarH; j++) {
					g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + (i-1)*g_uiBarW /*offset for next bar*/ + j*g_uiImgLineoffset;
            	}
                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
			}
		} else if ((g_uiRotateDir == DISP_ROTATE_90) || (g_uiRotateDir == DISP_ROTATE_270)) {
	        for (j = 0; j < g_uiBarH*g_uiXBarStep; j++) {
				if (g_uiRotateDir == DISP_ROTATE_90) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX - j*g_uiImgLineoffset;
                } else if (g_uiRotateDir == DISP_ROTATE_270) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + j*g_uiImgLineoffset;
                }

                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
            }
        }

        for (i=0 ;i < g_uiStr4_h; i++) {
            if (g_uiRotateDir == DISP_ROTATE_0) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_4_Y + g_uiStr_4_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_90) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_4_Y + g_uiStr_4_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_270) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_4_Y + g_uiStr_4_X + i*g_uiImgLineoffset;
            }

            switch (g_uiPinPongIdx){
            case 0:
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr4_YAddr + i*g_uiStr4_w), g_uiStr4_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr4_UVAddr + i*g_uiStr4_w),g_uiStr4_w);
                break;

            case 1:
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr4_YAddr + i*g_uiStr4_w), g_uiStr4_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr4_UVAddr + i*g_uiStr4_w),g_uiStr4_w);
                break;
            }
        }
        break;

    case DISP_DRAW_UPDATEFW_FAILED:
		if (g_uiRotateDir == DISP_ROTATE_0) {
        for (i=1; i <= g_uiXBarStep; i++) {
            for (j = 0; j < g_uiBarH; j++) {
					g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + (i-1)*g_uiBarW /*offset for next bar*/ + j*g_uiImgLineoffset;
            	}
                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
			}
		} else if ((g_uiRotateDir == DISP_ROTATE_90) || (g_uiRotateDir == DISP_ROTATE_270)) {
	        for (j = 0; j < g_uiBarH*g_uiXBarStep; j++) {
				if (g_uiRotateDir == DISP_ROTATE_90) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX - j*g_uiImgLineoffset;
                } else if (g_uiRotateDir == DISP_ROTATE_270) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + j*g_uiImgLineoffset;
                }

                switch (g_uiPinPongIdx){
                case 0:
	        		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
	        		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
	        		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
	        		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
            }
        }

        for (i=0 ;i < g_uiStr5_h; i++) {
            if (g_uiRotateDir == DISP_ROTATE_0) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_90) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_270) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            }

            switch (g_uiPinPongIdx){
            case 0:
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr5_YAddr + i*g_uiStr5_w), g_uiStr5_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr5_UVAddr + i*g_uiStr5_w),g_uiStr5_w);
                break;

            case 1:
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr5_YAddr + i*g_uiStr5_w), g_uiStr5_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr5_UVAddr + i*g_uiStr5_w),g_uiStr5_w);
                break;
            }
        }
        break;

    case DISP_DRAW_UPDATEFW_FAILED_AND_RETRY:
		if (g_uiRotateDir == DISP_ROTATE_0) {
        for (i=1; i <= g_uiXBarStep; i++) {
            for (j = 0; j < g_uiBarH; j++) {
					g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + (i-1)*g_uiBarW /*offset for next bar*/ + j*g_uiImgLineoffset;
            	}
                switch (g_uiPinPongIdx){
                case 0:
            		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
            		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
            		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
			}
		} else if ((g_uiRotateDir == DISP_ROTATE_90) || (g_uiRotateDir == DISP_ROTATE_270)) {
	        for (j = 0; j < g_uiBarH*g_uiXBarStep; j++) {
				if (g_uiRotateDir == DISP_ROTATE_90) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX - j*g_uiImgLineoffset;
                } else if (g_uiRotateDir == DISP_ROTATE_270) {
	                g_uiPixelOffset = g_uiBarY*g_uiImgLineoffset + g_uiBarX + j*g_uiImgLineoffset;
                }

                switch (g_uiPinPongIdx){
                case 0:
	        		memset((void *)(g_uiVDO_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
	        		memset((void *)(g_uiVDO_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                case 1:
	        		memset((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x4C, g_uiBarW);
	        		memset((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset) , 0x4C, g_uiBarW);
                    break;
                }
            }
        }

        for (i=0 ;i < g_uiStr6_h; i++) {
            if (g_uiRotateDir == DISP_ROTATE_0) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_90) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            } else if (g_uiRotateDir == DISP_ROTATE_270) {
                g_uiPixelOffset = g_uiImgLineoffset*g_uiStr_5_Y + g_uiStr_5_X + i*g_uiImgLineoffset;
            }

            switch (g_uiPinPongIdx){
            case 0:
                memcpy((void *)(g_uiVDO_YAddr + g_uiPixelOffset), (void *)(g_uiStr6_YAddr + i*g_uiStr6_w), g_uiStr6_w);
                memcpy((void *)(g_uiVDO_UVAddr + g_uiPixelOffset), (void *)(g_uiStr6_UVAddr + i*g_uiStr6_w),g_uiStr6_w);
                break;

            case 1:
                memcpy((void *)(g_uiVDO2_YAddr + g_uiPixelOffset), (void *)(g_uiStr6_YAddr + i*g_uiStr6_w), g_uiStr6_w);
                memcpy((void *)(g_uiVDO2_UVAddr + g_uiPixelOffset), (void *)(g_uiStr6_UVAddr + i*g_uiStr6_w),g_uiStr6_w);
                break;
            }
        }
        break;

    case DISP_DRAW_UPDATEFW_AGAIN:
        g_bUpdatFwAgain = TRUE;
        g_uiXBarStep = 0;

        nvt_display_set_background();

        nvt_display_draw_init_string();

        //Drawing the external frame of the bar.
        if (g_uiRotateDir == DISP_ROTATE_0) {
            for (i = 0; i < g_uiTotalBarNum; i++) {
                for (j=0; j < (g_uiUpFrame + g_uiBarH + g_uiDownFrame); j++) {
                    g_uiPixelOffset = g_uiImg_w*(g_uiBarY - g_uiUpFrame) + (g_uiBarX - g_uiLeftFrame) + j*g_uiImgLineoffset;
                    if (j < g_uiUpFrame) {
                        switch (g_uiPinPongIdx) {
                        case 0:
                            memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            break;

                        case 1:
                            memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            break;
                        }
                    } else if (j >= (g_uiUpFrame + g_uiBarH)) {
                        switch (g_uiPinPongIdx) {
                        case 0:
                            memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            break;

                        case 1:
                            memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiUpFrame + g_uiTotalBarNum*g_uiBarH + g_uiDownFrame));
                            break;
                        }
                    } else {
                        switch (g_uiPinPongIdx) {
                        case 0:
                            memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                            memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                            memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarH), 0x00, g_uiRightFrame);
                            memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarH), 0x80, g_uiRightFrame);
                            break;

                        case 1:
                            memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                            memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                            memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarH), 0x00, g_uiRightFrame);
                            memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiTotalBarNum*g_uiBarH), 0x80, g_uiRightFrame);
                            break;
                        }
                    }
                }
            }
        } else if (g_uiRotateDir == DISP_ROTATE_90) {
            for (j=0; j < (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum + g_uiDownFrame); j++) {
                g_uiPixelOffset = (g_uiBarY + g_uiDownFrame)*g_uiImgLineoffset + (g_uiBarX - g_uiLeftFrame) - j*g_uiImgLineoffset;
                if ((j < g_uiDownFrame) || (j >= (g_uiDownFrame + g_uiBarH*g_uiTotalBarNum))) {
                    switch (g_uiPinPongIdx) {
                    case 0:
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        break;

                    case 1:
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        break;
                    }
                } else {
                    switch (g_uiPinPongIdx) {
                    case 0:
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x00, g_uiRightFrame);
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x80, g_uiRightFrame);
                        break;

                    case 1:
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x00, g_uiRightFrame);
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x80, g_uiRightFrame);
                        break;
                    }
                }
            }
        } else if (g_uiRotateDir == DISP_ROTATE_270) {
            for (j=0; j < (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum + g_uiDownFrame); j++) {
                g_uiPixelOffset = g_uiImg_w*(g_uiBarY - g_uiUpFrame) + (g_uiBarX - g_uiLeftFrame) + j*g_uiImgLineoffset;
                if ((j < g_uiUpFrame) || (j >= (g_uiUpFrame + g_uiBarH*g_uiTotalBarNum))) {
                    switch (g_uiPinPongIdx) {
                    case 0:
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        break;

                    case 1:
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, (g_uiLeftFrame + g_uiBarH + g_uiRightFrame));
                        break;
                    }
                } else {
                    switch (g_uiPinPongIdx) {
                    case 0:
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                        memset((void*)(g_uiVDO_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x00, g_uiRightFrame);
                        memset((void*)(g_uiVDO_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x80, g_uiRightFrame);
                        break;

                    case 1:
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset), 0x00, g_uiLeftFrame);
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset), 0x80, g_uiLeftFrame);
                        memset((void*)(g_uiVDO2_YAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x00, g_uiRightFrame);
                        memset((void*)(g_uiVDO2_UVAddr + g_uiPixelOffset + g_uiLeftFrame + g_uiBarH), 0x80, g_uiRightFrame);
                        break;
                    }
                }
            }
        }
        break;
    }

    flush_dcache_range((unsigned long)g_uiVDO_YAddr,(unsigned long)(g_uiVDO_YAddr+g_uiLogoSize));

    p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_GET_VDOBUFADDR, &emu_disp_lyr);
    switch (g_uiPinPongIdx){
    case 0:
  		emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = g_uiVDO_YAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = g_uiVDO_UVAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr0 = g_uiVDO_UVAddr;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1,DISPLAYER_OP_SET_VDOBUFADDR,&emu_disp_lyr);
        break;

    case 1:
 		emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel = DISPBUFADR_0;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0 = g_uiVDO2_YAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = g_uiVDO2_UVAddr;
		emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cr0 = g_uiVDO2_UVAddr;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1,DISPLAYER_OP_SET_VDOBUFADDR,&emu_disp_lyr);
        break;
    }

	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO1;
	p_emu_disp_obj->load(TRUE);
	p_emu_disp_obj->wait_frm_end(TRUE);

    if (g_bUpdatFwAgain == TRUE) {
        g_bUpdatFwAgain = FALSE;

        nvt_display_copy_pingpong_buffer();

        flush_dcache_range((unsigned long)g_uiVDO_YAddr,(unsigned long)(g_uiVDO_YAddr+g_uiLogoSize));
    }
}

void nvt_display_set_status(INT32 vlaue)
{
    if (!g_bNvtDispInit) {
        return;
    }

    if (vlaue == DISP_UPDATEFW_OK) {
        nvt_display_draw(DISP_DRAW_UPDATEFW_OK);
    } else if (vlaue == DISP_UPDATEFW_FAILED) {
        nvt_display_draw(DISP_DRAW_UPDATEFW_FAILED);
    } else if (vlaue == DISP_DRAW_UPDATEFW_FAILED_AND_RETRY) {
        nvt_display_draw(DISP_DRAW_UPDATEFW_FAILED_AND_RETRY);
    } else {
        printf("%s No display status is selected %s\r\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
    }
}

void nvt_display_config(UINT32 config_id, UINT32 vlaue)
{
    if (!g_bNvtDispInit) {
        return;
    }

    switch (config_id) {
    case DISP_CFG_TOTAL_DRAWING_BAR:
        g_uiTotalBarNum = vlaue*2; //wrtie partition + read back partition
        break;
    }
}
