/**
    Msdc-Nvt Vendor Callback for PC

    This system for PC control device(DSC) via MSDC. User can register callbacks
    to respond PC command.

    @file       MsdcNvtCallback.h
    @ingroup    mMsdcNvt

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _MSDCNVTCALLBACK_H
#define _MSDCNVTCALLBACK_H

#if defined(__UITRON)
#include "UMSDC.h"
#elif defined(_WIN32)
#include <wTypes.h>
typedef unsigned char UINT8, *PUINT8;
#else
#endif

/**
     Basic Data Structure

     Msdc-Nvt basic data structure for bi-direction transmission
*/
typedef struct _MSDCEXT_PARENT {
	UINT32 size;      ///< Structure Size
	UINT32 b_handled;    ///< Indicate Device Handle this function
	UINT32 b_ok;         ///< Indicate Function Action is OK or Not
	UINT8  reversed[4]; ///< reversed value for Structure DWORD Alignment
} MSDCEXT_PARENT;

/**
     BOOL

     Msdc-Nvt BOOL data for bi-direction transmission
*/
typedef struct _MSDCEXT_BOOL {
	MSDCEXT_PARENT parent;
	UINT32         value; ///< a BOOL value
} MSDCEXT_BOOL;

/**
     UINT32

     Msdc-Nvt BOOL data for bi-direction transmission
*/
typedef struct _MSDCEXT_UINT32 {
	MSDCEXT_PARENT parent;
	UINT32         value;///< a UINT32 value
} MSDCEXT_UINT32;

/**
     INT32

     Msdc-Nvt BOOL data for bi-direction transmission
*/
typedef struct _MSDCEXT_INT32 {
	MSDCEXT_PARENT parent;
	INT32          value; ///< a INT32 value
} MSDCEXT_INT32;
//------------------------------------------------------------------------------
// Photo Data Types for MsdcNvt_AddCallback_Bi
//------------------------------------------------------------------------------
/**
     Display Setting

     MsdcNvtCb_GetDisplaySettings used structure
*/
typedef struct _MSDCEXT_DISPLAY_SETTING { ///< PC <- DSC
	MSDCEXT_PARENT parent;
	UINT32 addr_y[3];    ///< IDE 3 Buffers of Y
	UINT32 addr_u[3];    ///< IDE 3 Buffers of U
	UINT32 addr_v[3];    ///< IDE 3 Buffers of V
	UINT32 pitch_y;      ///< IDE 3 Buffers of LineOffset Y
	UINT32 pitch_uv;     ///< IDE 3 Buffers of LineOffset UV
	UINT32 width;       ///< IDE 3 Buffers of width
	UINT32 height;     ///< IDE height of Y
	UINT32 height_uv;    ///< IDE height of UV
} MSDCEXT_DISPLAY_SETTING;

/**
     Display Setting 2

     MsdcNvtCb_GetDisplaySettings2 used structure
*/
typedef struct _MSDCEXT_DISPLAY_SETTING2 { ///< PC <- DSC
	MSDCEXT_PARENT parent;
	UINT32 num_valid_frm;///< Valid Frame Number
	UINT32 addr_y[10];   ///< Display Buffers of Y
	UINT32 addr_u[10];   ///< Display Buffers of U
	UINT32 addr_v[10];   ///< Display Buffers of V
	UINT32 pitch_y;      ///< Display Buffers of LineOffset Y
	UINT32 pitch_uv;     ///< Display Buffers of LineOffset UV
	UINT32 width;        ///< Display Buffers of width
	UINT32 height_y;     ///< IDE height of Y
	UINT32 height_uv;    ///< IDE height of UV
} MSDCEXT_DISPLAY_SETTING2;

/**
     Display Current

     MsdcNvtCb_GetDisplayCurrentBufSel used structure
*/
typedef struct _MSDCEXT_DISPLYA_CURRENT { ///< PC <- DSC
	MSDCEXT_PARENT parent;
	UINT32 opt_buf;      ///< Current IDE Buffer Index
	UINT32 buf_num;      ///< Total IDE Buffers
} MSDCEXT_DISPLYA_CURRENT;

/**
     Capture Jpeg Information

     MsdcNvtCb_CaptureJpgwidthGetInfo used structure
*/
typedef struct _MSDCEXT_INFO_CAPTURE_JPG { ///< PC <- DSC
	MSDCEXT_PARENT parent;
	UINT32  jpg_addr;    ///< Jpeg Data Address
	UINT32  jpg_filesize;///< Jpg File Size
} MSDCEXT_INFO_CAPTURE_JPG;

/**
     Capture Raw Information

     MsdcNvtCb_CaptureRawwidthGetInfo used structure
*/
typedef struct _MSDCEXT_INFO_CAPTURE_RAW { ///< PC <- DSC
	MSDCEXT_PARENT parent;
	UINT32  raw_addr;    ///< Memory Address of Raw Data
	UINT32  raw_filesize;///< Raw Data Size
	UINT32  raw_width;   ///< Raw width
	UINT32  raw_height;  ///< Raw height
	UINT32  raw_n_bit;   ///< Raw 8bits or 12bits
} MSDCEXT_INFO_CAPTURE_RAW;

//------------------------------------------------------------------------------
// Display Utility Implementation Data Types for MsdcNvt_AddCallback_Bi
//------------------------------------------------------------------------------
/**
     Display format
*/
typedef enum _MSDCEXT_DISP_FORMAT {
	MSDCEXT_DISP_FORMAT_4_BIT = 0,  ///< Color format 4 bit
	MSDCEXT_DISP_FORMAT_2_BIT,      ///< Color format 2 bit
	MSDCEXT_DISP_FORMAT_1_BIT,      ///< Color format 1 bit
	MSDCEXT_DISP_FORMAT_8_BIT,      ///< Color format 8 bit
	MSDCEXT_DISP_FORMAT_YCBCR444,   ///< Color format YCBCR444
	MSDCEXT_DISP_FORMAT_YCBCR422,   ///< Color format YCBCR422
	MSDCEXT_DISP_FORMAT_YCBCR420,   ///< Color format YCBCR420
	MSDCEXT_DISP_FORMAT_ARGB4565,   ///< Color format ARGB4565
	MSDCEXT_DISP_FORMAT_ARGB8565,   ///< Color format ARGB8565
	MSDCEXT_DISP_FORMAT_YCBCR422_UVPACK, ///< Color format YCBCR422 with UV packing
	MSDCEXT_DISP_FORMAT_YCBCR420_UVPACK, ///< Color format YCBCR420 with UV packing
	MSDCEXT_DISP_FORMAT_YCBCR444_UVPACK, ///< Color format YCBCR444 with UV packing
	MSDCEXT_DISP_FORMAT_UNKNOWN = -1 ///< Unknown format
} MSDCEXT_DISP_FORMAT;

/**
     Display Engine
*/
typedef enum _MSDCEXT_DISP_ENG {
	MSDCEXT_DISP_ENG_VDO_0 = 0, ///< Video 0 Engine
	MSDCEXT_DISP_ENG_VDO_1, ///< Video 1 Engine
	MSDCEXT_DISP_ENG_OSD_0, ///< OSD 0 Engine
	MSDCEXT_DISP_ENG_OSD_1  ///< OSD 1 Engine
} MSDCEXT_DISP_ENG;

/**
     Video Engine Information
*/
typedef struct _MSDCEXT_DISP_VDO_INFO {
	BOOL                    b_enable;
	UINT32                  format;     ///< cast to eMSDCEXT_DISP_FORMAT
	UINT32                  addr_y[3];   ///< addr_y[0] is RGB565 when format is ARGB565
	UINT32                  addr_u[3];   ///< addr_u[0] is Alpha Plane when format is ARGB565
	UINT32                  addr_v[3];   ///< V address
	UINT32                  width;      ///< width
	UINT32                  height;     ///< height
	UINT32                  pitch_y;     ///< Y Lineoffset
	UINT32                  pitch_uv;    ///< UV Lineoffset
} MSDCEXT_DISP_VDO_INFO;

/**
     OSD Engine Information
*/
typedef struct _MSDCEXT_DISP_OSD_INFO {
	BOOL                    b_enable;   ///< enable / disable
	UINT32                  format;     ///< cast to eMSDCEXT_DISP_FORMAT
	UINT32                  addr_buf;   ///< main buffer
	UINT32                  addr_alpha; ///< ARGB565's Alpha
	UINT32                  width;      ///< width
	UINT32                  height;     ///< height
	UINT32                  pitch_buf;  ///< main buffer lineoffset
	UINT32                  pitch_alpha; ///< alpah buffer lineoffset
} MSDCEXT_DISP_OSD_INFO;

/**
     IDE Engine configure
*/
typedef struct _MSDCEXT_DISP_CFG {
	MSDCEXT_PARENT         parent; ///< basic data
	//[Device Set]
	MSDCEXT_DISP_VDO_INFO  vdo_info[2];    ///< 2 videoes information
	MSDCEXT_DISP_OSD_INFO  osd_info[2];    ///< 2 OSDs information
} MSDCEXT_DISP_CFG;

/**
     Video Current Buffer Index
*/
typedef struct _MSDCEXT_DISP_VDO_CURR_BUFF_IDX {
	MSDCEXT_PARENT         parent; ///< basic data
	//[Device Set]
	UINT32                 sel_idx[2]; ///< 0:Video1, 1:Video2
} MSDCEXT_DISP_VDO_CURR_BUFF_IDX;

/**
     Enable / Disable a engine
*/
typedef struct _MSDCEXT_DISP_ENG_ONOFF {
	MSDCEXT_PARENT         parent; ///< basic data
	//[Host Set]
	UINT32                 engine; ///< cast to MSDCEXT_DISP_ENG
	BOOL                   b_enable;
} MSDCEXT_DISP_ENG_ONOFF;

/**
     OSD Palette
*/
typedef struct _MSDCEXT_DISP_OSD_PALETTE {
	MSDCEXT_PARENT         parent;        ///< basic data
	//[Host Set or Device Set]
	UINT32                 sel_idx;       ///< 0 or 1 for Palette Select
	UINT32                 entry_cnt;     ///< Total effective enterys
	UINT32                 palette[256];  ///< if is 16 color only use first 16 entrys
} MSDCEXT_DISP_OSD_PALETTE;

/**
     Gamma Table
*/
typedef struct _MSDCEXT_DISP_GAMMA_TBL {
	MSDCEXT_PARENT         parent;        ///< basic data
	//[Host Set or Device Set]
	UINT8                  gamma_tbl[256];  ///< Gamma Table, Max size is 256 Level for any chip
} MSDCEXT_DISP_GAMMA_TBL;

/**
     Color Matrix Coefficients
*/
typedef struct _MSDCEXT_DISP_ICST_COEF_INFO {
	MSDCEXT_PARENT         parent;        ///< basic data
	//[Device Set]
	UINT32                 num_bits_integer; ///< Numbers of Maxrix Coefficient Integer Bits
	UINT32                 num_bits_fraction;///< Numbers of Maxrix Coefficient Fraction Bits
	INT32                  pre_matrix[9];    ///< CoefTrans = PreMatrix*GainMatrix*PostMatrix
	INT32                  post_matrix[9];   ///< CoefTrans = PreMatrix*GainMatrix*PostMatrix
} MSDCEXT_DISP_ICST_COEF_INFO;

/**
     Color Matrix Enable/Disable
*/
typedef struct _MSDCEXT_DISP_ICST_COEF {
	MSDCEXT_PARENT         parent;        ///< basic data
	//[Host Set]
	UINT32                 coef_trans[9];   ///< 3x3 Transform Matrix
} MSDCEXT_DISP_ICST_COEF;

/**
     Saturation Information
*/
typedef struct _MSDCEXT_DISP_SATURATION_INFO {
	MSDCEXT_PARENT         parent;    ///< basic data
	//[Device Set]
	INT32                  val_max;     ///< maximum value
	INT32                  val_min;     ///< minimum value
	INT32                  val_default; ///< default value
} MSDCEXT_DISP_SATURATION_INFO;

/**
     Contrast Information
*/
typedef struct _MSDCEXT_DISP_CONTRAST_INFO {
	MSDCEXT_PARENT         parent;    ///< basic data
	//[Device Set]
	INT32                  val_max;     ///< maximum value
	INT32                  val_min;     ///< minimum value
	INT32                  val_default; ///< default value
} MSDCEXT_DISP_CONTRAST_INFO;

/**
     Brightness Information
*/
typedef struct _MSDCEXT_DISP_BRIGHTNESS_INFO {
	MSDCEXT_PARENT         parent;    ///< basic data
	//[Device Set]
	INT32                  val_max;     ///< maximum value
	INT32                  val_min;     ///< minimum value
	INT32                  val_default; ///< default value
} MSDCEXT_DISP_BRIGHTNESS_INFO;

//------------------------------------------------------------------------------
// Filesys Utility Implementation Data Types for MsdcNvt_AddCallback_Bi
//------------------------------------------------------------------------------
/**
     Working Buffer
*/
typedef struct _MSDCEXT_FILE_WORKING_MEM {
	MSDCEXT_PARENT         parent; ///< basic data
	//[Device Set]
	UINT32                 address; ///< address of working buffer begin
	UINT32                 size;    ///< size of working buffer
} MSDCEXT_FILE_WORKING_MEM;

/**
     File Information
*/
typedef struct _MSDCEXT_FILE_INFO {
	MSDCEXT_PARENT         parent;
	//[Host Set(Copy PC To Device) or Device Set(Copy Device To PC)]
	char                   file_path[256]; ///< File Name / Path
	UINT32                 file_size;      ///< File Total Size
	UINT32                 transmit_offset;///< File Transmit Offset
	UINT32                 transmit_size;  ///< File Transmit Size
	UINT32                 working_mem;    ///< Device Wroking Buffer
} MSDCEXT_FILE_INFO;

//------------------------------------------------------------------------------
// Universal Adjustment Implementation Data Types for MsdcNvt_AddCallback_Bi
//------------------------------------------------------------------------------
/**
     Adjustment Description
*/
typedef struct _MSDCEXT_ADJ_ITEM_DESC {
	UINT32 is_sign;     ///< signed or unsigned type
	UINT32 max_value;   ///< maximum value
	UINT32 min_value;   ///< minimum value
	CHAR   tag[64];     ///< a tag of string
} MSDCEXT_ADJ_ITEM_DESC;

/**
     Adjustment Basic

     MSDCEXT_ADJ_ITEM_DESC* with following data
*/
typedef struct _MSDCEXT_ADJ_ALL_DESC {
	MSDCEXT_PARENT         parent; ///< basic data
} MSDCEXT_ADJ_ALL_DESC;

/**
     Adjustment Data

     void*  pVars; with following data
*/
typedef struct _MSDCEXT_ADJ_DATA {
	MSDCEXT_PARENT         parent;  ///< basic data
} MSDCEXT_ADJ_DATA;

//------------------------------------------------------------------------------
// Standard Update Firmware
//------------------------------------------------------------------------------
/**
     PC Get Nand Block Information
*/
typedef struct _MSDCEXT_UPDFW_GET_BLOCK_INFO {
	MSDCEXT_PARENT parent;    ///< basic data
	UINT32 bytes_per_block;   ///< Each Block Size
} MSDCEXT_UPDFW_GET_BLOCK_INFO;

typedef struct _MSDCEXT_UPDFW_GET_WORK_MEM {
	MSDCEXT_PARENT parent;  ///< basic data
	UINT32 addr; ///< working memory address
	UINT32 size; ///< working memory size
} MSDCEXT_UPDFW_GET_WORK_MEM;

typedef MSDCEXT_UPDFW_GET_WORK_MEM MSDCEXT_UPDFWALLINONE_STEP1;

/**
     PC Send Nand Block Data
*/
typedef struct _MSDCEXT_UPDFW_SET_BLOCK_INFO {
	MSDCEXT_PARENT parent;    ///< basic data
	UINT32 key;               ///< a key value to indicate valid command ('N','O','V','A')
	UINT32 blk_idx;           ///< Block Index of this Packet
	UINT32 b_last_block;      ///< Indicate Last Block for Reset System
	UINT32 effect_data_size;  ///< Current Effective Data Size
} MSDCEXT_UPDFW_SET_BLOCK_INFO;

typedef MSDCEXT_UPDFW_SET_BLOCK_INFO MSDCEXT_UPDFWALLINONE_STEP2;
#endif
