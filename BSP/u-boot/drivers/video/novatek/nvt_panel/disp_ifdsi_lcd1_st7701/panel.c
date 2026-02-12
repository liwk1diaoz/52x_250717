/*
    Display object for driving DSI device

    @file       ST7701.c
    @ingroup
    @note       This panel MUST select ide clock to PLL1 ( 480 ). Once change to \n
				another frequence, the _IDE_FDCLK should be re calculated

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "./../include/dispdev_ifdsi.h"

#define PANEL_WIDTH     480
#define PANEL_HEIGHT    720

#define NT35510_IND_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35510_ERR_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35510_WRN_MSG(...)       debug_msg(__VA_ARGS__)
#define NT35510_TE_OFF              0
#define NT35510_TE_ON               1

#define NT35510_TE_PACKET           0
#define NT35510_TE_PIN              1

#define NT35510_NEWPANEL            1

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

//#define DSI_TARGET_CLK             480    //real chip use 480Mhz
//#define DSI_TARGET_CLK             240    //real chip use 240Mhz
#define DSI_TARGET_CLK             480  //FPGA use 160MHz
//#define DSI_TARGET_CLK             120
//#define DSI_TARGET_CLK             54
#define DSI_OP_MODE                DSI_OP_MODE_VDO_MODE

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif

//#if (_FPGA_EMULATION_ == ENABLE)


#define _IDE_FDCLK      80000000



/*
    panel Parameters for TCON NT35510
*/
//@{
/*Used in DSI*/

/*
{DSICMD_CMD,0x40},{DSICMD_DATA,0X01}, //BT  +2
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x41},{DSICMD_DATA,0X33},  //DVDDH DVDDL clamp 33=+-5.2v , 77=+-6V
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x42},{DSICMD_DATA,0X00}, //VGH
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x43},{DSICMD_DATA,0X05}, //VGH_CLAMP ON ; CLAMP 11V   05-AMBA   
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x44},{DSICMD_DATA,0X00}, //VGL_CLAMP ON ; CLAMP -10V   86
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x45},{DSICMD_DATA,0X26}, //vglreg=-10
{CMDDELAY_MS,1},
{DSICMD_CMD,0x46},{DSICMD_DATA,0X34}, //Power Control7

{CMDDELAY_MS,1},   

{DSICMD_CMD,0x47}, 
{DSICMD_DATA,0x44}, 

{CMDDELAY_MS,1},   
{DSICMD_CMD,0x50},{DSICMD_DATA,0X78}, //VREG1OUT +4.5V


{CMDDELAY_MS,1},   
{DSICMD_CMD,0x51},{DSICMD_DATA,0X78}, //VREG2OUT -4.5V
{CMDDELAY_MS,1},    
{DSICMD_CMD,0x52},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x53},{DSICMD_DATA,0X4B}, //Forward Flicker 0X58  0X4B
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x54},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x55},{DSICMD_DATA,0X46}, //Backward Flicker


*/

const T_PANEL_CMD t_cmd_mode_dsi[] = {

{DSICMD_CMD,0x01},                 // sw reset 
{CMDDELAY_MS,1},          
{DSICMD_CMD,0x11},                 // sw reset 
{CMDDELAY_MS,1},          

{DSICMD_CMD,0xFF},
{DSICMD_DATA,0xFF},
{DSICMD_DATA,0x98},
{DSICMD_DATA,0x06},
{DSICMD_DATA,0x04},
{DSICMD_DATA,0x01}, //Change to Page 1

{CMDDELAY_MS,1},   
{DSICMD_CMD,0x08},{DSICMD_DATA,0X10}, //output SDA
{DSICMD_CMD,0x20},{DSICMD_DATA,0X01}, //(0x01:VSYNC mode)  (0x00:DE mode)


{DSICMD_CMD,0x21},{DSICMD_DATA,0X01}, 
{DSICMD_CMD,0x23},{DSICMD_DATA,0X02},  //Normally Black
{DSICMD_CMD,0x30},{DSICMD_DATA,0X04},  //02=480 x 800 , 03=480 x 640 , 04=480 x 720
{DSICMD_CMD,0x31},{DSICMD_DATA,0X01}, //00=Column , 02= 2 Dot



{DSICMD_CMD,0x40},{DSICMD_DATA,0X01}, //BT  +2
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x41},{DSICMD_DATA,0X33},  //DVDDH DVDDL clamp 33=+-5.2v , 77=+-6V
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x42},{DSICMD_DATA,0X00}, //VGH
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x43},{DSICMD_DATA,0X85}, //VGH_CLAMP ON ; CLAMP 11V   81
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x44},{DSICMD_DATA,0X80}, //VGL_CLAMP ON ; CLAMP -10V   86
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x45},{DSICMD_DATA,0X26}, //vglreg=-10
{CMDDELAY_MS,1},
{DSICMD_CMD,0x46},{DSICMD_DATA,0X34}, //Power Control7
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x50},{DSICMD_DATA,0X78}, //VREG1OUT +4.5V
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x51},{DSICMD_DATA,0X78}, //VREG2OUT -4.5V
{CMDDELAY_MS,1},    
{DSICMD_CMD,0x52},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x53},{DSICMD_DATA,0X58}, //Forward Flicker 0X58  0X4B
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x54},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x55},{DSICMD_DATA,0X46}, //Backward Flicker
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x60},{DSICMD_DATA,0X07},  //SDTI
{DSICMD_CMD,0x61},{DSICMD_DATA,0X04},  //CRTI
{DSICMD_CMD,0x62},{DSICMD_DATA,0X08}, //EQTI
{DSICMD_CMD,0x63},{DSICMD_DATA,0X04}, //PCTI

{DSICMD_CMD,0xA0},{DSICMD_DATA,0X00}, //GAMMA
{DSICMD_CMD,0xA1},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xA2},{DSICMD_DATA,0X14}, 
{DSICMD_CMD,0xA3},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xA4},{DSICMD_DATA,0X05}, 
{DSICMD_CMD,0xA5},{DSICMD_DATA,0X0c}, //0c
{DSICMD_CMD,0xA6},{DSICMD_DATA,0X06}, 
{DSICMD_CMD,0xA7},{DSICMD_DATA,0X04}, 
{DSICMD_CMD,0xA8},{DSICMD_DATA,0X08}, 
{DSICMD_CMD,0xA9},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xAA},{DSICMD_DATA,0X0F}, 
{DSICMD_CMD,0xAB},{DSICMD_DATA,0X07}, 
{DSICMD_CMD,0xAC},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xAD},{DSICMD_DATA,0X19}, 
{DSICMD_CMD,0xAE},{DSICMD_DATA,0X12}, 
{DSICMD_CMD,0xAF},{DSICMD_DATA,0X0A}, 
{CMDDELAY_MS,10},   
{DSICMD_CMD,0xC0},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0xC1},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xC2},{DSICMD_DATA,0X14}, 
{DSICMD_CMD,0xC3},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xC4},{DSICMD_DATA,0X05}, 
{DSICMD_CMD,0xC5},{DSICMD_DATA,0X0A}, 
{DSICMD_CMD,0xC6},{DSICMD_DATA,0X06}, 
{DSICMD_CMD,0xC7},{DSICMD_DATA,0X04}, 
{DSICMD_CMD,0xC8},{DSICMD_DATA,0X08}, 
{DSICMD_CMD,0xC9},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xCA},{DSICMD_DATA,0X0F}, 
{DSICMD_CMD,0xCB},{DSICMD_DATA,0X07}, 
{DSICMD_CMD,0xCC},{DSICMD_DATA,0X0C}, 
{DSICMD_CMD,0xCD},{DSICMD_DATA,0X19}, 
{DSICMD_CMD,0xCE},{DSICMD_DATA,0X12}, 
{DSICMD_CMD,0xCF},{DSICMD_DATA,0X0A}, 
{CMDDELAY_MS,10},   
{DSICMD_CMD,0xFF},
{DSICMD_DATA,0XFF},
{DSICMD_DATA,0X98},
{DSICMD_DATA,0X06},
{DSICMD_DATA,0X04},
{DSICMD_DATA,0X05}, //Change to Page 5
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x07},
{DSICMD_DATA,0XB1}, //PWM always high
{CMDDELAY_MS,1},   
{DSICMD_CMD,0xFF},
{DSICMD_DATA,0XFF},
{DSICMD_DATA,0X98},
{DSICMD_DATA,0X06},
{DSICMD_DATA,0X04},
{DSICMD_DATA,0X06},	//Change to Page 6 
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x00},{DSICMD_DATA,0X21}, 
{DSICMD_CMD,0x01},{DSICMD_DATA,0X0A}, 
{DSICMD_CMD,0x02},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x03},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x04},{DSICMD_DATA,0X32}, 
{DSICMD_CMD,0x05},{DSICMD_DATA,0X32}, 
{DSICMD_CMD,0x06},{DSICMD_DATA,0X98}, 
{DSICMD_CMD,0x07},{DSICMD_DATA,0X06}, 
{DSICMD_CMD,0x08},{DSICMD_DATA,0X05}, 
{DSICMD_CMD,0x09},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x0A},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x0B},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x0C},{DSICMD_DATA,0X32}, 
{DSICMD_CMD,0x0D},{DSICMD_DATA,0X32}, 
{DSICMD_CMD,0x0E},{DSICMD_DATA,0X01}, 
{DSICMD_CMD,0x0F},{DSICMD_DATA,0X01}, 
{DSICMD_CMD,0x10},{DSICMD_DATA,0XF0}, 
{DSICMD_CMD,0x11},{DSICMD_DATA,0XF0}, 
{DSICMD_CMD,0x12},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x13},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x14},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x15},{DSICMD_DATA,0X43}, 
{DSICMD_CMD,0x16},{DSICMD_DATA,0X0B}, 
{DSICMD_CMD,0x17},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x18},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x19},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x1A},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x1B},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x1C},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x1D},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x20},{DSICMD_DATA,0X01}, 
{DSICMD_CMD,0x21},{DSICMD_DATA,0X23}, 
{DSICMD_CMD,0x22},{DSICMD_DATA,0X45}, 
{DSICMD_CMD,0x23},{DSICMD_DATA,0X67}, 
{DSICMD_CMD,0x24},{DSICMD_DATA,0X01}, 

{DSICMD_CMD,0x25},{DSICMD_DATA,0X23}, 
{DSICMD_CMD,0x26},{DSICMD_DATA,0X45}, 

{DSICMD_CMD,0x27},{DSICMD_DATA,0X67}, 
{DSICMD_CMD,0x30},{DSICMD_DATA,0X01}, 
{DSICMD_CMD,0x31},{DSICMD_DATA,0X11}, 
{DSICMD_CMD,0x32},{DSICMD_DATA,0X00}, 
{DSICMD_CMD,0x33},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x34},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x35},{DSICMD_DATA,0XCB}, 
{DSICMD_CMD,0x36},{DSICMD_DATA,0XDA}, 
{DSICMD_CMD,0x37},{DSICMD_DATA,0XAD}, 
{DSICMD_CMD,0x38},{DSICMD_DATA,0XBC}, 
{DSICMD_CMD,0x39},{DSICMD_DATA,0X66}, //START 4  
{DSICMD_CMD,0x3A},{DSICMD_DATA,0X77}, //START 2  
{DSICMD_CMD,0x3B},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x3C},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x3D},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x3E},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x3F},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x40},{DSICMD_DATA,0X22}, 
{DSICMD_CMD,0x52},{DSICMD_DATA,0X10},  //sleep out 
{DSICMD_CMD,0x53},{DSICMD_DATA,0X10},  //VGLO = VGL
{DSICMD_CMD,0x54},{DSICMD_DATA,0X13},  //

{CMDDELAY_MS,10},   
{DSICMD_CMD,0xFF},
{DSICMD_DATA,0XFF},
{DSICMD_DATA,0X98},
{DSICMD_DATA,0X06},
{DSICMD_DATA,0X04},
{DSICMD_DATA,0X07}, //Change to Page 7


{CMDDELAY_MS,1},   
{DSICMD_CMD,0x17},
{DSICMD_DATA,0X22},  //VGLO = VGL
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x18},
{DSICMD_DATA,0X1D},  //VREG1 VREG2 output
{CMDDELAY_MS,1},   
{DSICMD_CMD,0x02},
{DSICMD_DATA,0X77},
{CMDDELAY_MS,1},   
{DSICMD_CMD,0xB3},
{DSICMD_DATA,0X10},
 
{DSICMD_CMD,0xE1},
{DSICMD_DATA,0X79},  

{DSICMD_CMD,0xFF},
{DSICMD_DATA,0XFF},
{DSICMD_DATA,0X98},
{DSICMD_DATA,0X06},
{DSICMD_DATA,0X04},
{DSICMD_DATA,0X00},  	//Change to Page 0   

{DSICMD_CMD,0x36},
{DSICMD_DATA,0x02}, ///01   180du 
//00 01 10


{DSICMD_CMD,0x3a},
{DSICMD_DATA,0x55},


{CMDDELAY_MS,1},
{DSICMD_CMD,0x11},                 // Sleep-Out
{CMDDELAY_MS,10},
{DSICMD_CMD,0x29},                 // Display On
{CMDDELAY_MS,10},          



{DSICMD_CMD,0x35},                 // TE
{DSICMD_DATA,0x01},

{CMDDELAY_MS,5}, 

//֣���Ǳߴ���

};


const T_PANEL_CMD t_cmd_standby_dsi[] = {
	//{DSICMD_CMD,     0x28},         // Display OFF
	//{CMDDELAY_MS,    10},
//    {DSICMD_CMD,     0x10},      // Sleep in
//    {CMDDELAY_MS,    10},
};

const T_LCD_PARAM t_mode_dsi[] = {
	/***********       MI Serial Format 1      *************/
	{
		// T_PANEL_PARAM
		{
			/* Old prototype */
			//PINMUX_DSI_2_LANE_CMD_MODE_RGB565,    //!< lcd_mode
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565,//!< lcd_mode
			//PINMUX_DSI_2_LANE_CMD_MODE_RGB565,
			//PINMUX_DSI_2_LANE_CMD_MODE_RGB666L,
			PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888,

			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P,
			_IDE_FDCLK,                             //!< fd_clk
			(0x75 + PANEL_WIDTH),                   //!< ui_hsync_total_period
			PANEL_WIDTH,                            //!< ui_hsync_active_period
			0x40,                                   //!< ui_hsync_back_porch
			0x40 + PANEL_HEIGHT,                    //!< ui_vsync_total_period
			PANEL_HEIGHT,                           //!< ui_vsync_active_period
			17,                                   //!< ui_vsync_back_porch_odd
			17,                                   //!< ui_vsync_back_porch_even
			PANEL_WIDTH,                            //!< ui_buffer_width
			PANEL_HEIGHT,                           //!< ui_buffer_height
			PANEL_WIDTH,                            //!< ui_window_width
			PANEL_HEIGHT,                           //!< ui_window_height
			FALSE,                                  //!< b_ycbcr_format

			/* New added parameters */
			0x04,                                   //!< ui_hsync_sync_width
			0x02                                    //!< ui_vsync_sync_width
		},

		// T_IDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_GPIO,            //!< pinmux_select_lcd;
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


void dispdev_set_dsi_config(DSI_CONFIG *p_dsi_config)
{
#if 0
	// DSI input source clock = 480
	// Target can be 480 / 240 / 160 / 120
	FLOAT   dsi_target_clk = DSI_TARGET_CLK;
	UINT32  div;


	div = (UINT32)(p_dsi_config->f_dsi_src_clk / dsi_target_clk);

	if (div == 0) {
		NT35510_WRN_MSG("div = 0 force ++\r\n");
		div++;
	}
	pll_setClockRate(PLL_CLKSEL_DSI_CLKDIV, PLL_DSI_CLKDIV(div - 1));
#else
	dsi_set_config(DSI_CONFIG_ID_FREQ, DSI_TARGET_CLK * 1000000);
#endif
#if (DSI_TARGET_CLK == 160) //real is 150MHz
	/*dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 7);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 2);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 3);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 7);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 8);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
	*/
	dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
	//??dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 7);
	//??dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 7);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 2);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 1);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GET, 2);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 0);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 2);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 9);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 4);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 3);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 9);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 2);

#elif(DSI_TARGET_CLK == 480)
	    dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
	    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 15);

	    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 4);
	    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 5);
	    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 4);
	    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 7);

	    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
	    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 14);
	    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 10);
	    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
	    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);

#elif(DSI_TARGET_CLK == 240)
	dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 6);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 7);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 6);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);

	dsi_set_config(DSI_CONFIG_ID_BTA_HANDSK_TMOUT_VAL, 0x40);
//#elif(DSI_TARGET_CLK == 120)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 0);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 3);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 2);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 1);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 5);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 8);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
//#elif(DSI_TARGET_CLK == 54)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 2);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 7);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 7);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 9);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 4);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 2);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 15);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 7);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#endif
	dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_1);
	dsi_set_config(DSI_CONFIG_ID_TE_BTA_INTERVAL, 0x1F);
	//dsi_set_config(DSI_CONFIG_ID_CLK_PHASE_OFS, 0x3);
	//dsi_set_config(DSI_CONFIG_ID_PHASE_DELAY_ENABLE_OFS, 0x1);

	dsi_set_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x1);
	//dsi_set_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x0);
	dsi_set_config(DSI_CONFIG_ID_SYNC_DLY_CNT, 0xF);

	dsi_set_config(DSI_CONFIG_ID_EOT_PKT_EN, 1);

}

#if defined __FREERTOS
int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    DBG_DUMP("Hello, panel: ST7701\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("ST7701, Goodbye\r\n");
}

#elif defined __KERNEL__
static int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    pr_info("Hello, panel: st7701\n");
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

MODULE_DESCRIPTION("ST7701 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
*/
#endif


