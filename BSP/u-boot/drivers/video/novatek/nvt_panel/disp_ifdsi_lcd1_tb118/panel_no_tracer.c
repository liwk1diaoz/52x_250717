/*
    Display object for driving DSI device

    @file       WTL096602G02_24M.c
    @ingroup
    @note       This panel MUST select ide clock to PLL1 ( 480 ). Once change to \n
				another frequence, the _IDE_FDCLK should be re calculated

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "./../include/dispdev_ifdsi.h"

#define PANEL_WIDTH     320
#define PANEL_HEIGHT    1488//1480

#define WTL096602G02_24M_IND_MSG(...)       debug_msg(__VA_ARGS__)

#define WTL096602G02_24M_ERR_MSG(...)       debug_msg(__VA_ARGS__)

#define WTL096602G02_24M_WRN_MSG(...)       debug_msg(__VA_ARGS__)
#define WTL096602G02_24M_TE_OFF              0
#define WTL096602G02_24M_TE_ON               1

#define WTL096602G02_24M_TE_PACKET           0
#define WTL096602G02_24M_TE_PIN              1

#define WTL096602G02_24M_NEWPANEL            1

/*
    RGB888 = 1 pixel = 3bytes packet
    If DSI src = 240MHz, internal clock = 30MHz, data rate = 30MHz x 1bytes = 30MB / sec per lane
    2 lane = 60MB = 20Mpixel ' ide need > 20MHz
*/


#define DSI_FORMAT_RGB565          0    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666P         1    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666L         2    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB888          3    //ide use 480 & DSI use 480

#define DSI_OP_MODE_CMD_MODE       1
#define DSI_OP_MODE_VDO_MODE       0
#define DSI_PACKET_FORMAT          DSI_FORMAT_RGB888

#define DSI_TARGET_CLK             480    //real chip use 480Mhz
//#define DSI_TARGET_CLK             240    //real chip use 240Mhz
//#define DSI_TARGET_CLK             160  //FPGA use 160MHz
//#define DSI_TARGET_CLK             120
//#define DSI_TARGET_CLK             54

#define DSI_OP_MODE                DSI_OP_MODE_VDO_MODE

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif

#if 1
#if(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
#if (DSI_TARGET_CLK == 480)
#define _IDE_FDCLK      120000000 //((((DSI_TARGET_CLK / 8) * 2)/3))
#else
#if (DSI_TARGET_CLK == 240)
#define _IDE_FDCLK      ((((DSI_TARGET_CLK / 8) * 4)/3)) // must > ((((DSI_TARGET_CLK / 8) * 2) / 3)) ->2: lane number
#else
#define _IDE_FDCLK      ((((DSI_TARGET_CLK / 8) * 4)/3))//13.33f
#endif
#endif
#else
#error "Format not RGB888"
#endif
#else
//#if (_FPGA_EMULATION_ == ENABLE)
#if defined (_NVT_FPGA_) 


#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#define _IDE_FDCLK      20000000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
#define _IDE_FDCLK      27000000 //((((((DSI_TARGET_CLK / 8) * 2))*4)/9)+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
#define _IDE_FDCLK      27000000 //(((((DSI_TARGET_CLK / 8) * 2)/ 2))+1)
#endif

#else

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#if (DSI_TARGET_CLK == 160)
//??6.66f //FPGA is 150MHz, 150/8/3 = 6.25, but PLL2 = 54, so 54/8 = 6.75
#define _IDE_FDCLK      6750000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#else
#define _IDE_FDCLK      15000000//(((DSI_TARGET_CLK / 8)/3))
#endif
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)

#if (DSI_TARGET_CLK == 160)
#define _IDE_FDCLK      18000000 //FPGA is 150MHz, (150/8*2*4)/9 = 16.66f
#elif(DSI_TARGET_CLK == 120)
#define _IDE_FDCLK      6660000
#else
#define _IDE_FDCLK      15000000//((((DSI_TARGET_CLK / 8) *2 * 4)/9))
#endif

#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
if(DSI_TARGET_CLK == 120)
#define _IDE_FDCLK      16000000
#else
#define _IDE_FDCLK      38000000//(((DSI_TARGET_CLK / 8) * 2 /2) + 1)
#endif
#endif
#endif

#define HVALIDST    240//50//0x39  //uiHSyncBackPorch(HPB) -> 53
#define VVALIDSTODD    12//0x0A  //uiVSyncBackPorchOdd/Even
#define VVALIDSTEVEV   16//0x0A  //uiVSyncBackPorchOdd/Even
#define HSYNCT      23//0x18
#define VSYNCT      4//0x02

/*
    Panel Parameters for TCON HX8369B
*/
//@{
/*Used in DSI*/
const T_PANEL_CMD t_cmd_mode_dsi[] = {
	{DSICMD_CMD,  0x01},
	{CMDDELAY_MS,  120},

	{DSICMD_CMD,  0xF0},
	{DSICMD_DATA, 0x5A},
	{DSICMD_DATA, 0x59},

	{DSICMD_CMD,  0xF1},
	{DSICMD_DATA, 0xA5},
	{DSICMD_DATA, 0xA6},

	{DSICMD_CMD,  0xB4},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x04},
	{DSICMD_DATA, 0x05},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x1E},
	{DSICMD_DATA, 0x1F},
	{DSICMD_DATA, 0x01},
	{DSICMD_DATA, 0x10},
	{DSICMD_DATA, 0x11},
	{DSICMD_DATA, 0x12},
	{DSICMD_DATA, 0x13},
	{DSICMD_DATA, 0x0C},
	{DSICMD_DATA, 0x0D},
	{DSICMD_DATA, 0x0E},
	{DSICMD_DATA, 0x0F},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},
	{DSICMD_DATA, 0x03},

	{DSICMD_CMD,  0xB0},
	{DSICMD_DATA, 0xC6},
	{DSICMD_DATA, 0xBB},
	{DSICMD_DATA, 0xBB},
	{DSICMD_DATA, 0xBB},
	{DSICMD_DATA, 0x33},
	{DSICMD_DATA, 0x33},
	{DSICMD_DATA, 0x33},
	{DSICMD_DATA, 0x33},
	{DSICMD_DATA, 0x24},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x9C},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x80},

	{DSICMD_CMD,  0xB1},
	{DSICMD_DATA, 0x53},
	{DSICMD_DATA, 0xA0},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x85},
	{DSICMD_DATA, 0x04},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x7C},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x5F},
	  
	{DSICMD_CMD,  0xB2},
	{DSICMD_DATA, 0x07},
	{DSICMD_DATA, 0x09},
	{DSICMD_DATA, 0x08},
	{DSICMD_DATA, 0x8B},
	{DSICMD_DATA, 0x08},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x22},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x44},
	{DSICMD_DATA, 0xD9},

	{DSICMD_CMD,  0xB6},
	{DSICMD_DATA, 0x22},
	{DSICMD_DATA, 0x22},

	{DSICMD_CMD,  0xB7},
	{DSICMD_DATA, 0x01},
	{DSICMD_DATA, 0x01},
	{DSICMD_DATA, 0x09},
	{DSICMD_DATA, 0x0D},
	{DSICMD_DATA, 0x11},
	{DSICMD_DATA, 0x19},
	{DSICMD_DATA, 0x1D},
	{DSICMD_DATA, 0x15},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x25},
	{DSICMD_DATA, 0x21},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x02},
	{DSICMD_DATA, 0xF7},
	{DSICMD_DATA, 0x38},

	{DSICMD_CMD,  0xB8},
	{DSICMD_DATA, 0xB8},
	{DSICMD_DATA, 0x52},
	{DSICMD_DATA, 0x02},
	{DSICMD_DATA, 0xCC},

	{DSICMD_CMD,  0xBA},
	{DSICMD_DATA, 0x27},
	{DSICMD_DATA, 0xD3},

	{DSICMD_CMD,  0xBD},
	{DSICMD_DATA, 0x43},
	{DSICMD_DATA, 0x0E},
	{DSICMD_DATA, 0x0E},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x32},
	{DSICMD_DATA, 0x14},

	{DSICMD_CMD,  0xC1},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x0F},
	{DSICMD_DATA, 0x0E},
	{DSICMD_DATA, 0x01},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x36},
	{DSICMD_DATA, 0x3A},
	{DSICMD_DATA, 0x08},

	{DSICMD_CMD,  0xC2},
	{DSICMD_DATA, 0x22},
	{DSICMD_DATA, 0xF8},

	{DSICMD_CMD,  0xC3},
	{DSICMD_DATA, 0x22},
	{DSICMD_DATA, 0x31},

	{DSICMD_CMD,  0xC6},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0xFF},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0xFF},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},

	//G2.0
	{DSICMD_CMD,  0xC8},
	{DSICMD_DATA, 0x7C},
	{DSICMD_DATA, 0x66},
	{DSICMD_DATA, 0x57},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x49},
	{DSICMD_DATA, 0x3A},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x29},
	{DSICMD_DATA, 0x42},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x5D},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x54},
	{DSICMD_DATA, 0x48},
	{DSICMD_DATA, 0x44},
	{DSICMD_DATA, 0x39},
	{DSICMD_DATA, 0x26},
	{DSICMD_DATA, 0x06},
	{DSICMD_DATA, 0x7C},
	{DSICMD_DATA, 0x66},
	{DSICMD_DATA, 0x57},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x49},
	{DSICMD_DATA, 0x3A},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x29},
	{DSICMD_DATA, 0x42},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x3F},
	{DSICMD_DATA, 0x5D},
	{DSICMD_DATA, 0x4B},
	{DSICMD_DATA, 0x54},
	{DSICMD_DATA, 0x48},
	{DSICMD_DATA, 0x44},
	{DSICMD_DATA, 0x39},
	{DSICMD_DATA, 0x26},
	{DSICMD_DATA, 0x06},

	{DSICMD_CMD,  0xD0},
	{DSICMD_DATA, 0x07},
	{DSICMD_DATA, 0xFF},
	{DSICMD_DATA, 0xFF},

	{DSICMD_CMD,  0xD2},
	{DSICMD_DATA, 0x63},
	{DSICMD_DATA, 0x0B},
	{DSICMD_DATA, 0x08},
	{DSICMD_DATA, 0x88},

	{DSICMD_CMD,  0xD4},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x00},
	{DSICMD_DATA, 0x32},
	{DSICMD_DATA, 0x04},
	{DSICMD_DATA, 0x51},

	{DSICMD_CMD,  0xF1},
	{DSICMD_DATA, 0x5A},
	{DSICMD_DATA, 0x59},

	{DSICMD_CMD,  0xF0},
	{DSICMD_DATA, 0xA5},
	{DSICMD_DATA, 0xA6},

	{DSICMD_CMD,  0x11},
	{CMDDELAY_MS,  200},

	{DSICMD_CMD,  0x29},
	{CMDDELAY_MS,   50},

	//{DSICMD_CMD,  0x0A},
	//{DSICMD_DATA, 0X01},
};


const T_PANEL_CMD t_cmd_standby_dsi[] = {
    //{DSICMD_CMD,     0x28},     // Display OFF
    //{DSICMD_CMD,     0x10}      // Sleep in
};

const T_LCD_PARAM t_mode_dsi[] = {
	/***********       MI Serial Format 1      *************/
	{
		// T_PANEL_PARAM
		{
			/* Old prototype */
			//PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L,
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565,
#if (DSI_OP_MODE == DSI_OP_MODE_VDO_MODE)
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB565,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888,
			//PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888,
			PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888,
#endif

#else
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB565,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB666L,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB888,
#endif
#endif
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P,
			_IDE_FDCLK,                             //!< fd_clk
			(HVALIDST+HVALIDST+HSYNCT+PANEL_WIDTH), //!< uiHSyncTotalPeriod
			PANEL_WIDTH,                            //!< ui_hsync_active_period
			HVALIDST,                               //!< uiHSyncBackPorch
			(VVALIDSTODD+VVALIDSTEVEV + VSYNCT+ PANEL_HEIGHT),//!< ui_vsync_total_period
			PANEL_HEIGHT,                           //!< ui_vsync_active_period
			VVALIDSTODD,                            //!< ui_vsync_back_porch_odd
			VVALIDSTEVEV,                           //!< ui_vsync_back_porch_even
			PANEL_WIDTH,                            //!< ui_buffer_width
			PANEL_HEIGHT,                           //!< ui_buffer_height
			PANEL_WIDTH,                            //!< ui_window_width
			PANEL_HEIGHT,                           //!< ui_window_height
			FALSE,                                  //!< b_ycbcr_format

			/* New added parameters */
			HSYNCT,                                 //!< ui_hsync_sync_width
			VSYNCT                                  //!< ui_vsync_sync_width
		},

		// T_IDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_MIPI,            //!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                  //!< dithering[2];
			DISPLAY_DEVICE_MIPIDSI,         //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                           //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< yc_ex
			FALSE,                          //!< hlpf
			{FALSE, FALSE, FALSE},          //!< subpix_odd[3]
			{FALSE, FALSE, FALSE},          //!< subpix_even[3]
			{IDE_DITHER_5BITS, IDE_DITHER_6BITS, IDE_DITHER_5BITS}, //!< dither_bits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_dsi,                 //!< p_cmd_queue
		sizeof(t_cmd_mode_dsi) / sizeof(T_PANEL_CMD),  //!< n_cmd
	}
};

const T_LCD_ROT *t_rot_dsi = NULL;

//@}

T_LCD_ROT *dispdev_get_lcd_rotate_dsi_cmd(UINT32 *mode_number)
{
#if 0
	if (t_rot_dsi != NULL) {
		*mode_number = sizeof(t_rot_dsi) / sizeof(T_LCD_ROT);
	} else
#endif
	{
		*mode_number = 0;
	}
	return (T_LCD_ROT *)t_rot_dsi;
}

T_LCD_PARAM *dispdev_get_config_mode_dsi(UINT32 *mode_number)
{
	*mode_number = sizeof(t_mode_dsi) / sizeof(T_LCD_PARAM);
	return (T_LCD_PARAM *)t_mode_dsi;
}

T_PANEL_CMD *dispdev_get_standby_cmd_dsi(UINT32 *cmd_number)
{
	*cmd_number = sizeof(t_cmd_standby_dsi) / sizeof(T_PANEL_CMD);
	return (T_PANEL_CMD *)t_cmd_standby_dsi;
}

void dispdev_set_dsi_drv_config(DSI_CONFIG_ID id, UINT32 value)
{
	if (dsi_set_config(id, value) != E_OK)	{
		DBG_DUMP("dsi_set_config not support, id = 0x%x, value = 0x%x\r\n", (unsigned int)id, (unsigned int)value);
	}
}

void dispdev_set_dsi_config(DSI_CONFIG *p_dsi_config)
{
#if 0
	// DSI input source clock = 480
	// Target can be 480 / 240 / 160 / 120
	FLOAT   dsi_target_clk = DSI_TARGET_CLK;
	UINT32  div;


	div = (UINT32)(p_dsi_config->f_dsi_src_clk / dsi_target_clk);

	if (div == 0) {
		WTL096602G02_24M_WRN_MSG("div = 0 force ++\r\n");
		div++;
	}
	pll_setClockRate(PLL_CLKSEL_DSI_CLKDIV, PLL_DSI_CLKDIV(div - 1));
#else
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_FREQ, DSI_TARGET_CLK * 1000000);
#endif
#if (DSI_TARGET_CLK == 160) //real is 150MHz
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 1);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 7);

	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 1);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 4);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 2);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 3);

	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 7);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 8);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 1);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);

#elif(DSI_TARGET_CLK == 240)
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 3);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 4);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 6);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 7);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 6);

	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 16);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);
#elif (DSI_TARGET_CLK == 480)
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 3);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 4);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 6);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 7);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 6);

    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 16);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 2);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);
	
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_HANDSK_TMOUT_VAL, 0x40);
    //dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_PHASE_OFS, 0x4);
    //dispdev_set_dsi_drv_config(DSI_CONFIG_ID_PHASE_DELAY_ENABLE_OFS, 0x1);
//#elif(DSI_TARGET_CLK == 120)
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 3);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 4);

//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 4);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 8);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 7);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 6);

//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 48);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 48);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 7);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);
//#elif(DSI_TARGET_CLK == 54)
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 1);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 4);

//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 2);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 0);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 7);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 4);

//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 2);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 0);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 1);
//    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#endif
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_3);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TE_BTA_INTERVAL, 0x1F);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_PHASE_OFS, 0x03);
	//dispdev_set_dsi_drv_config(DSI_CONFIG_ID_PHASE_DELAY_ENABLE_OFS, 0x1); //Shaun&KT: must disable clock phase delay
	
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x0);//0x0 check
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_SYNC_DLY_CNT, 0xF);
	
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_EOT_PKT_EN, TRUE);//mask check
}

#if defined __FREERTOS
int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    DBG_DUMP("Hello, panel: WTL096602G02_24M\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("WTL096602G02_24M, Goodbye\r\n");
}

#elif defined __KERNEL__
static int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    pr_info("Hello, panel: tb118\n");
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

MODULE_DESCRIPTION("WTL096602G02_24M Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
*/
#endif


