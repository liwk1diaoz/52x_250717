/*
    IFE2 module driver

    NT98520 IFE2 registers header file.

    @file       ife2_int.h
    @ingroup    mIIPPIFE2
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _IFE2_INT_H_
#define _IFE2_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#ifdef __KERNEL__

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "ife2_dbg.h"

#define pll_enableClock(x)
#define pll_disableClock(x)
#define pll_setClockRate(x,y)
#define pll_getPLLEn(x,y)
#define pll_setPLLEn(x)

#define dma_getNonCacheAddr(parm) 0
#define dma_getPhyAddr(parm) 0

#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)
#define dma_flushReadCacheWidthNEQLineOffset(parm,parm2)

#else

#include "Type.h"
#include "ErrorNo.h"
#include "Debug.h"
#include "dma.h"

#endif // end #ifdef __KERNEL__
#endif

#include "ife2_reg.h"
#include "ife2_platform.h"
#include "ife2_lib.h"

#define IFE2_IMAGE_SIZE_MAX  0x3FFF
#define IFE2_LOFS_SIZE_MAX   0xFFFFF
#define IFE2_DMA_SIZE_MAX    0x7FFFFFFF
//#define IFE2_IMAGE_SIZE_H_LIMT  672

//#define IFE2_IMAGE_SIZE_H_LIMT_D2D  672
//#define IFE2_IMAGE_SIZE_H_LIMT_IPP  660



#define IFE2_THRESHOLD_SIZE_MAX  0xFF

#define IFE2_RCWT_SIZE_MAX   0x8
#define IFE2_FWT_SIZE_MAX    0x10



/*
    IFE2 Engine Used Status
*/
typedef enum {
	IFE2_UNLOCKED = 0,
	IFE2_LOCKED = 1,
	ENUM_DUMMY4WORD(IFE2_LOCK_STATUS)
} IFE2_LOCK_STATUS;
//------------------------------------------------------------------

/**
    IFE2 enum - load type of parameters
*/
typedef enum {
	IFE2_START_LOAD   = 0,  ///< Frame start loaded
	IFE2_FRMEND_LOAD  = 1,  ///< Frame end loaded
	IFE2_DIRECT_START_LOAD  = 2,  ///< Frame start loaded when direct mode
	ENUM_DUMMY4WORD(IFE2_LOAD_TYPE)
} IFE2_LOAD_TYPE;
//------------------------------------------------------------------



/*
    IFE2 DMA Flush Type Selection
*/
typedef enum {
	IFE2_IFLUSH = 0,     ///< flush input cache buffer
	IFE2_OFLUSH_EQ = 1,  ///< flush output cache buffer when image width is equal to lineoffset
	IFE2_OFLUSH_NEQ = 2,  ///< flush output cache buffer when image width is not equal to lineoffset
	ENUM_DUMMY4WORD(IFE2_DMAFLUSH_TYPE)
} IFE2_DMAFLUSH_TYPE;
//------------------------------------------------------------------



/*
    IFE2 Operation
*/
typedef enum {
	IFE2_OP_OPEN            = 0,    ///< open
	IFE2_OP_CLOSE           = 1,    ///< close
	IFE2_OP_SETPARAM        = 2,    ///< Set parameter
	IFE2_OP_DYNAMICPARAM    = 3,    ///< Change parameter
	IFE2_OP_STARTRUN        = 4,    ///< start to run
	IFE2_OP_PAUSE           = 5,    ///< pause
	ENUM_DUMMY4WORD(IFE2_ENGINE_OPERATION)
} IFE2_ENGINE_OPERATION;
//------------------------------------------------------------------


/*
    IFE2 State machine
*/
typedef enum {
	IFE2_ENGINE_IDLE  = 0, ///< engine is idle
	IFE2_ENGINE_READY = 1, ///< engine is ready
	IFE2_ENGINE_PAUSE = 2, ///< engine is paused(ready to Run)
	IFE2_ENGINE_RUN   = 3, ///< engine is running
	ENUM_DUMMY4WORD(IFE2_ENGINE_STATUS)
} IFE2_ENGINE_STATUS;
//------------------------------------------------------------------


/*
    IFE2 Engine Control
*/
typedef enum {
	IFE2_CTRL_RESET     = 0, ///< contrl engine reset
	IFE2_CTRL_START     = 1, ///< contrl engine start
	IFE2_CTRL_LOAD      = 2, ///< contrl engine load
	IFE2_CTRL_IFMT      = 3, ///< contrl engine input format
	IFE2_CTRL_OFMT      = 4, ///< contrl engine output format
	IFE2_CTRL_OUTDEST   = 5, ///< contrl engine output mode(destination)
	IFE2_CTRL_YFTREN    = 6, ///< contrl engine Y channel filter enable
	IFE2_CTRL_FTREN     = 7, ///< contrl engine filter enable
	IFE2_CTRL_INTPEN    = 8, ///< contrl engine interrupt enable
	IFE2_CTRL_INTP      = 9, ///< contrl engine interrupt status
	IFE2_CTRL_OUTDRAM   = 10, ///< contrl engine output dram when direct mode
	ENUM_DUMMY4WORD(IFE2_ENGINE_CTRL_SEL)
} IFE2_ENGINE_CTRL_SEL;
//------------------------------------------------------------------


/*
    IFE2 Engine Get Information Selection
*/
typedef enum {
	IFE2_GET_STARTSTATUS  = 0, ///< get start status
	IFE2_GET_INTSTATUS    = 1, ///< get interrupt status
	IFE2_GET_INTESTATUS   = 2, ///< get interrupt enable status
	IFE2_GET_IMG_HSIZE    = 3, ///< get image width
	IFE2_GET_IMG_VSIZE    = 4, ///< get image height
	IFE2_GET_IFMT         = 5, ///< get input image format
	IFE2_GET_OFMT         = 6, ///< get output image format
	IFE2_GET_AVG_U        = 7, ///< get U channel gray average
	IFE2_GET_AVG_V        = 8, ///< get V channel gray average
	//IFE2_GET_LOCKSTATUS   = 9, ///< get engine lock status
	IFE2_GET_IN_DMAY      = 10, ///< get input address of Y channel
	IFE2_GET_OUT_DMAY0    = 11, ///< get output address of Y0 channel
	IFE2_GET_IN_LOFSY     = 12, ///< get input Y channel lineoffset
	IFE2_GET_OUT_LOFSY    = 13, ///< get input Y channel lineoffset
	IFE2_GET_BASE_ADDR    = 14, ///< get register base address
	IFE2_GET_DEST         = 15, ///< get output destination
	ENUM_DUMMY4WORD(IFE2_ENGINE_GET_SEL)
} IFE2_ENGINE_GET_SEL;
//------------------------------------------------------------------


/*
    IFE2 Engine input/output type selection
*/
typedef enum {
	IFE2_IOTYPE_IN   = 0,
	IFE2_IOTYPE_OUT  = 1,
	ENUM_DUMMY4WORD(IFE2_IOTYPE_SEL)
} IFE2_IOTYPE_SEL;
//------------------------------------------------------------------

/*

    IFE2 Engine DMA Channel Selection
*/
typedef enum {
	IFE2_DMA_IN    = 0,
	IFE2_DMA_OUT0  = 1,
	ENUM_DUMMY4WORD(IFE2_DMA_SEL)
} IFE2_DMA_SEL;
//------------------------------------------------------------------


/*
    IFE2 Channel Selection
*/
typedef enum {
	IFE2_CHL_Y     = 0,
	IFE2_CHL_U     = 1,
	IFE2_CHL_V     = 2,
	IFE2_CHL_UV    = 3,
	IFE2_CHL_ALL   = 4,
	ENUM_DUMMY4WORD(IFE2_CHL_SEL)
} IFE2_CHL_SEL;
//------------------------------------------------------------------


/*
    IFE2 Output Destination Selection
*/
typedef enum {
	IFE2_OUTDEST_DRAM  = 0,
	IFE2_OUTDEST_IME   = 1,
	IFE2_OUTDEST_ADRT  = 2,
	ENUM_DUMMY4WORD(IFE2_OUTDEST_SEL)
} IFE2_OUTDEST_SEL;
//------------------------------------------------------------------

/*
    IFE2 Input direct mode selection
*/
typedef enum {
	IFE2_DRCTMODE_IPP = 0,
	IFE2_DRCTMODE_SIE_IPP  = 1,
	ENUM_DUMMY4WORD(IFE2_INDRCTMODE_SEL)
} IFE2_INDRCTMODE_SEL;
//------------------------------------------------------------------




/*
    IFE2 Filter Setting
*/
typedef struct {
	IFE2_FILTER_SET         filter_set_y;        ///< filter parameter for Y channel
	IFE2_FILTER_SET         filter_set_u;        ///< filter parameter for U channel
	IFE2_FILTER_SET         filter_set_v;        ///< filter parameter for V channel
} IFE2_FILTER_LUT_PARAM;
//------------------------------------------------------------------

extern volatile NT98520_IFE2_REGISTER_STRUCT *ifeg2;


extern VOID ife2_set_engine_control(IFE2_ENGINE_CTRL_SEL ctrl_sel, UINT32 op_val);
extern UINT32 ife2_get_engine_info(IFE2_ENGINE_GET_SEL get_sel);

//extern VOID ife2_set_image_size(UINT32 img_size_h, UINT32 img_size_v);
//extern VOID ife2_chg_burst_length(IFE2_BURST_LENGHT *p_set_bst);
//extern VOID ife2_set_lineoffset(IFE2_IOTYPE_SEL io_sel, UINT32 lofs_y, UINT32 lofs_c);
//extern VOID ife2_set_address(IFE2_DMA_SEL io_sel, UINT32 addr_y, UINT32 addr_c);
//extern VOID ife2_set_reference_center_info(IFE2_CHL_SEL chl_sel, IFE2_REFCENT_PARAM *p_ref_cent_info);
//extern VOID ife2_set_reference_center_info(IFE2_CHL_SEL chl_sel, IFE2_REFCENT_SET *p_set_ref_cent_y, IFE2_REFCENT_SET *p_set_ref_cent_uv);
//extern VOID ife2_set_filter_win_size(IFE2_FILT_SIZE filter_size, IFE2_EDGE_KERNEL_SIZE edge_ker_size);
//extern VOID ife2_ife2_set_filter_info(IFE2_CHL_SEL chl_sel, IFE2_FILTER_LUT_PARAM *p_filter_info);
//extern VOID ife2_ife2_set_filter_info(IFE2_CHL_SEL chl_sel, IFE2_FILTER_SET *p_filter_y, IFE2_FILTER_SET *p_filter_u, IFE2_FILTER_SET *p_filter_v);
//extern VOID ife2_set_edge_direction_threshold(IFE2_EDRCT_TH *p_dth);
//extern VOID ife2_set_statistical_info(IFE2_GRAY_STATAL *p_sta_info);



extern ER ife2_check_state_machine(IFE2_ENGINE_STATUS current_status, IFE2_ENGINE_OPERATION action_op);


extern VOID ife2_flush_dma_buf(IFE2_DMAFLUSH_TYPE flash_type, UINT32 addr, UINT32 buf_size, IFE2_OPMODE op_mode);
extern ER ife2_check_dma_addr(UINT32 addr);

//extern VOID ife2_set_debug_function(UINT32 edge_en, UINT32 ref_cent_en, UINT32 eng_en);

extern INT32 ife2_get_burst_length_info(UINT32 get_sel);




#ifdef __cplusplus
} // extern "C"
#endif

#endif  // #ifndef _IFE2_INT_H_


