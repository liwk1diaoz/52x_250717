/*
    Display object object panel configuration parameters for PW35P00

    @file       PW35P00.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#include "./../include/dispdev_if8bits.h"

#define IFCMD_CMD   0x01000000
#define IFCMD_DATA  0x02000000

/*
    panel Parameters for PW35P00
*/
//@{
const T_PANEL_CMD t_cmd_standby[] = {
  {IFCMD_CMD,0x28}, //Display off
  {CMDDELAY_US, 10},
  {IFCMD_CMD,0x10}, //enter sleep mode
  {IFCMD_CMD,0x2c}, //memory write
};

const T_PANEL_CMD t_cmd_mode_rgb320[] = {
    {CMDDELAY_MS, 10},                //Delay 120ms

    {IFCMD_CMD ,0x11},

    {CMDDELAY_MS, 10},                //Delay 120ms

    {IFCMD_CMD ,0x36},
    {IFCMD_DATA ,0x48},   //0x08

    {IFCMD_CMD ,0x3A},
    {IFCMD_DATA ,0x06},

    {IFCMD_CMD ,0xb0},
    {IFCMD_DATA ,0x11},
    {IFCMD_DATA ,0x04},

    {IFCMD_CMD ,0xB1},///1110 0000
    {IFCMD_DATA ,0xE0},
    {IFCMD_DATA ,0x03},
    {IFCMD_DATA ,0x14},

    {IFCMD_CMD ,0xB2},
    {IFCMD_DATA ,0x0C},
    {IFCMD_DATA ,0x0C},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x33},
    {IFCMD_DATA ,0x33},

    {IFCMD_CMD ,0xB7},
    {IFCMD_DATA ,0x72},

    {IFCMD_CMD ,0xBB},
    {IFCMD_DATA ,0x1E},

    {IFCMD_CMD ,0xC0},
    {IFCMD_DATA ,0x2C},

    {IFCMD_CMD ,0xC2},
    {IFCMD_DATA ,0x01},

    {IFCMD_CMD ,0xC3},
    {IFCMD_DATA ,0x10},

    {IFCMD_CMD ,0xC4},
    {IFCMD_DATA ,0x20},

    {IFCMD_CMD ,0xC6},
    {IFCMD_DATA ,0x0F},

    {IFCMD_CMD ,0xD0},
    {IFCMD_DATA ,0xA4},
    {IFCMD_DATA ,0xA1},

    {IFCMD_CMD ,0xD6},
    {IFCMD_DATA ,0xA1},

    {IFCMD_CMD ,0x21},

    {IFCMD_CMD ,0xE0},
    {IFCMD_DATA ,0xD0},
    {IFCMD_DATA ,0x09},
    {IFCMD_DATA ,0x12},
    {IFCMD_DATA ,0x07},
    {IFCMD_DATA ,0x08},
    {IFCMD_DATA ,0x05},
    {IFCMD_DATA ,0x3A},
    {IFCMD_DATA ,0x44},
    {IFCMD_DATA ,0x4F},
    {IFCMD_DATA ,0x25},
    {IFCMD_DATA ,0x10},
    {IFCMD_DATA ,0x10},
    {IFCMD_DATA ,0x2C},
    {IFCMD_DATA ,0x2E},

    {IFCMD_CMD ,0xE1},
    {IFCMD_DATA ,0xD0},
    {IFCMD_DATA ,0x15},
    {IFCMD_DATA ,0x18},
    {IFCMD_DATA ,0x0D},
    {IFCMD_DATA ,0x0B},
    {IFCMD_DATA ,0x27},
    {IFCMD_DATA ,0x39},
    {IFCMD_DATA ,0x43},
    {IFCMD_DATA ,0x4F},
    {IFCMD_DATA ,0x3D},
    {IFCMD_DATA ,0x19},
    {IFCMD_DATA ,0x19},
    {IFCMD_DATA ,0x2E},
    {IFCMD_DATA ,0x30},

    {IFCMD_CMD ,0x2A},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0xEF},

    {IFCMD_CMD ,0x2B},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0xEF},

    {IFCMD_CMD ,0xE4},
    {IFCMD_DATA ,0x1D},
    {IFCMD_DATA ,0x00},
    {IFCMD_DATA ,0x00},

    {IFCMD_CMD ,0x29},
    {CMDDELAY_MS, 120},    //ms
    {IFCMD_CMD ,0x2C},
};

const T_LCD_PARAM t_mode[] = {
    /***********       RGB Dummy-320 MODE         *************/
    {
        // tPANEL_PARAM
        {
            /* Old prototype */
            PINMUX_LCDMODE_RGB_SERIAL,      //!< LCDMode
////300/(10*3) 6.25
		   15000000,							 //!< fDCLK
	 		940,                           //!< ui_hsync_total_period
	 		720,                            //!< ui_hsync_active_period
	 		57,                            //!< ui_hsync_back_porch


		   240+24,							 //!< uiVSyncTotalPeriod
		   240, 						   //!< uiVSyncActivePeriod
		   3,							  //!< uiVSyncBackPorchOdd
		   3,							  //!< uiVSyncBackPorchEven


	            720,                            //!< uiBufferWidth
	            240,                            //!< uiBufferHeight


	            720,                            //!< uiWindowWidth
	            240,                            //!< uiWindowHeight

	            FALSE,                          //!< bYCbCrFormat

	            /* New added parameters */
	            0x0a,                           //!< uiHSyncSYNCwidth
	            0x02                            //!< uiVSyncSYNCwidth
        },

        // tIDE_PARAM
        {
            /* Old prototype */
            PINMUX_LCD_SEL_SERIAL_RGB_6BITS,//!< pinmux_select_lcd;
            ICST_CCIR601,                   //!< icst;
            {1,1},                  //!< dithering[2];
            DISPLAY_DEVICE_TOPPOLY,         //!< **DONT-CARE**
            //
		    IDE_PDIR_RBG,IDE_LCD_G,IDE_LCD_G,
            TRUE,                           //!< hsinv;
            TRUE,                           //!< vsinv;
            FALSE,                          //!< hvldinv;
            FALSE,                          //!< vvldinv;
            FALSE,                           //!< clkinv;
            FALSE,                          //!< fieldinv;
            FALSE,                           //!< rgbdummy
            FALSE,                          //!< interlace;
            FALSE,                          //!< **DONT-CARE**
            0x40,                           //!< ctrst;
            0x00,                           //!< brt;
            0x40,                           //!< cmults;
            FALSE,                          //!< cex;
            FALSE,                          //!< **DONT-CARE**
            TRUE,                           //!< **DONT-CARE**
            TRUE,                           //!< tv_powerdown;
            {0x00,0x00},                    //!< **DONT-CARE**

            /* New added parameters */
            FALSE,                          //!< YCex
            FALSE,                          //!< HLPF
            {TRUE,TRUE,TRUE},               //!< subpixOdd[3]
            {FALSE,FALSE,FALSE},            //!< subpixEven[3]
            {IDE_DITHER_6BITS,IDE_DITHER_6BITS,IDE_DITHER_6BITS}//!< DitherBits[3]
        },

        (T_PANEL_CMD*)t_cmd_mode_rgb320,                  //!< pCmdQueue
        sizeof(t_cmd_mode_rgb320)/sizeof(T_PANEL_CMD),    //!< nCmd
    }
};
const T_PANEL_CMD t_cmd_rotate_none[] = {
	//{0x04,0x18}
};
const T_PANEL_CMD t_cmd_rotate_180[] = {
	//{0x04,0x1b}
};
const T_LCD_ROT t_rotate[] = {
	{DISPDEV_LCD_ROTATE_NONE, (T_PANEL_CMD *)t_cmd_rotate_none, 1},
	{DISPDEV_LCD_ROTATE_180, (T_PANEL_CMD *)t_cmd_rotate_180, 1}
};
/*SIF_bypass
const T_LCD_SIF_PARAM t_sif_param[] = {
	{
	// SIF mode 00
	0,
	//SIF_MODE_00,
	// Bus clock maximun = 20 MHz
	18000000,
	// SENS = 0
	0,
	// SENH = 30
	30,
	// 24 bits per transmission
	24,
	// MSb shift out first
	SIF_DIR_MSB,
	}
};*/
//@}

T_LCD_ROT *dispdev_get_lcd_rotate_cmd(UINT32 *mode_number)
{

	*mode_number = sizeof(t_rotate) / sizeof(T_LCD_ROT);

	return (T_LCD_ROT *)t_rotate;
}

T_LCD_PARAM *dispdev_get_config_mode(UINT32 *mode_number)
{
	*mode_number = sizeof(t_mode) / sizeof(T_LCD_PARAM);
	return (T_LCD_PARAM *)t_mode;
}

T_PANEL_CMD *dispdev_get_standby_cmd(UINT32 *cmd_number)
{
	*cmd_number = sizeof(t_cmd_standby) / sizeof(T_PANEL_CMD);
	return (T_PANEL_CMD *)t_cmd_standby;
}



#if 1
/*SIF_bypass
void dispdev_write_to_lcd_sif(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
{
	UINT32                  ui_sif_data;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	//ui_sif_data = ((((UINT32)addr << 8)|((UINT32)value)) << 16);
	//ui_sif_data = ((((UINT32)addr << 8)|((UINT32)value)) << 16);
	ui_sif_data = (UINT32)addr << 24 | (UINT32)value << 8;

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	sif_send(dev_io_ctrl.SEL.GET_REG_IF.ui_sif_ch, ui_sif_data, 0, 0);
}
*/

void dispdev_write_to_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value)
{
	UINT32                  j;
	UINT32                  sif_clk, sif_sen, sif_data;
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	p_disp_dev_control(DISPDEV_IOCTRL_GET_REG_IF, &dev_io_ctrl);
	sif_clk  = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_clk;
	sif_sen  = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_sen;
	sif_data = dev_io_ctrl.SEL.GET_REG_IF.ui_gpio_data;

	if (dispdev_platform_request_gpio(sif_clk, "clk")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_clk);
	}
	if (dispdev_platform_request_gpio(sif_sen, "sen")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_sen);
	}
	if (dispdev_platform_request_gpio(sif_data, "data")) {
		DBG_ERR("gpio%d requset err\r\n", (int)sif_data);
	}

	dispdev_platform_set_gpio_ouput(sif_sen, 1);
	dispdev_platform_set_gpio_ouput(sif_clk, 1);
	dispdev_platform_set_gpio_ouput(sif_data, 1);

	dispdev_platform_delay_us(500);

	dispdev_platform_set_gpio_ouput(sif_sen, 0);

    if (addr == IFCMD_CMD) {
        value = (value & 0x00ff);
    	for (j = 9; j > 0; j--) {
    		if (((value >> (j - 1)) & 0x01)) {
    			dispdev_platform_set_gpio_ouput(sif_data, 1);
    		} else {
    			dispdev_platform_set_gpio_ouput(sif_data, 0);
    		}

    		dispdev_platform_delay_us(100);
    		dispdev_platform_set_gpio_ouput(sif_clk, 0);
    		dispdev_platform_delay_us(200);
    		dispdev_platform_set_gpio_ouput(sif_clk, 1);
    		dispdev_platform_delay_us(100);
    	}
    } else if (addr == IFCMD_DATA) {
        value = ((value & 0x00ff) | 0x100);
        for (j = 9; j > 0; j--)
        {
            if (((value >> (j - 1)) & 0x01))
                dispdev_platform_set_gpio_ouput(sif_data, 1);
            else
                dispdev_platform_set_gpio_ouput(sif_data, 0);

            dispdev_platform_delay_us(100);
            dispdev_platform_set_gpio_ouput(sif_clk, 0);
            dispdev_platform_delay_us(200);
            dispdev_platform_set_gpio_ouput(sif_clk, 1);
            dispdev_platform_delay_us(100);
        }
    } else if (addr == CMDDELAY_MS) {
        dispdev_platform_delay_us(value * 1000);
    } else if (addr == CMDDELAY_US) {
        dispdev_platform_delay_us(value);
    }

	dispdev_platform_delay_us(500);
	dispdev_platform_set_gpio_ouput(sif_sen, 1);

	dispdev_platform_free_gpio(sif_clk);
	dispdev_platform_free_gpio(sif_sen);
	dispdev_platform_free_gpio(sif_data);

}

void dispdev_read_from_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 *p_value)
{
	DBG_ERR("Has not implement dispdev_read_from_lcd_gpio \r\n");
}
/* SIF_bypass
T_LCD_SIF_PARAM *dispdev_get_lcd_sif_param(void)
{
	return (T_LCD_SIF_PARAM *)t_sif_param;
}
*/
#if defined __FREERTOS
int panel_init(void)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	int nodeoffset;
	uint32_t *cell = NULL;
	DISPDEV_PARAM   disp_dev;
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->open();

    if(fdt_addr == NULL)
		return -1;


	nodeoffset = fdt_path_offset((const void*)fdt_addr, "/display");
	if (nodeoffset >= 0) {

		cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "lcd_ctrl", NULL);
		if (cell > 0) {
			if (be32_to_cpu(cell[0]) == 0) {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_SIF;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = 0;

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "sif_channel", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_sif_ch = (SIF_CH)(be32_to_cpu(cell[0]));
					p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);
				} else {
					DBG_WRN("panel dtsi sif_channel is not set!\r\n");
				}
			} else {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_GPIO;
				disp_dev.SEL.SET_REG_IF.ui_sif_ch = 0;

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_cs", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_sen = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_cs is not set!\r\n");
				}

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_clk", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_clk = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_clk is not set!\r\n");
				}

				cell = (uint32_t*)fdt_getprop((const void*)fdt_addr, nodeoffset, "gpio_data", NULL);
				if (cell > 0) {
					disp_dev.SEL.SET_REG_IF.ui_gpio_data = be32_to_cpu(cell[0]);
				} else {
					DBG_WRN("panel dtsi gpio_data is not set!\r\n");
				}

				p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);

			}
		} else {
			DBG_WRN("panel dtsi lcd_ctrl is not set!\r\n");
		}
	} else {
		DBG_WRN("can not find display node!\r\n");
	}

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
	p_disp_obj->close();
    DBG_DUMP("Hello, panel: PW35P00\r\n");

    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("PW35P00, Goodbye\r\n");
}
#elif defined __KERNEL__

static int panel_init(void)
{
	DISPDEV_PARAM   disp_dev;
	u32 gpio_cs;
	u32 gpio_ck;
	u32 gpio_dat;
	//struct device_node *panel;
	int panel;
	u32 value;
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->open();

	//panel = of_find_node_by_name(NULL, "display");
	panel=1;
	if (panel) {
        value =1;
		//if (!of_property_read_u32(panel, "lcd_ctrl", &value)) {
			if (value == 0) {/*
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_SIF; SIF_bypass
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = 0;
				if (!of_property_read_u32(panel, "sif_channel", &value)) {
					disp_dev.SEL.SET_REG_IF.ui_sif_ch = (SIF_CH)value;
					p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);
				} else {
					DBG_WRN("panel dtsi sif_channel is not set!\r\n");
				}*/
			} else {
				disp_dev.SEL.SET_REG_IF.lcd_ctrl = DISPDEV_LCDCTRL_GPIO;
				if (!of_property_read_u32(panel, "gpio_cs", &gpio_cs)) {
				} else {
					DBG_WRN("panel dtsi gpio_cs is not set!\r\n");
				}
				if (!of_property_read_u32(panel, "gpio_clk", &gpio_ck)) {
				} else {
					DBG_WRN("panel dtsi gpio_clk is not set!\r\n");
				}
				if (!of_property_read_u32(panel, "gpio_data", &gpio_dat)) {
				} else {
					DBG_WRN("panel dtsi gpio_data is not set!\r\n");
				}

				disp_dev.SEL.SET_REG_IF.ui_sif_ch = 0;
				disp_dev.SEL.SET_REG_IF.ui_gpio_sen = gpio_cs;
				disp_dev.SEL.SET_REG_IF.ui_gpio_clk = gpio_ck;
				disp_dev.SEL.SET_REG_IF.ui_gpio_data = gpio_dat;
				p_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &disp_dev);

			}
		} else {
			DBG_WRN("panel dtsi lcd_ctrl is not set!\r\n");
		}
	//} else {
	//	DBG_WRN("can not find display node!\r\n");
	//}
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
	p_disp_obj->close();
    pr_info("Hello, panel: PW35P00\n");

    return 0;
}

static void panel_exit(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = NULL;
    printk(KERN_INFO "Goodbye\n");
}

/*
module_init(panel_init);
module_exit(panel_exit);

MODULE_DESCRIPTION("PW35P00 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
*/
#endif
#endif
