/*
    Public header file for IFE2 module.

    This file is the header file that define the API and data type for IFE2
    module.

    @file       ife2_lib.h
    @ingroup    mIIPPIFE2

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _IFE2_LIB_H_
#define _IFE2_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#ifdef __KERNEL__

#include "kwrap/type.h"

extern UINT32 _IFE2_REGIOBASE;

#else

#include "Type.h"

#endif
#endif

/**
    @addtogroup mIDrvIPP_IFE2
*/
//@{


/**
    Define IFE2 interrput enable.
*/
#define IFE2_INTE_LL_END   0x00000001  ///< linked-list end interrupt enable
#define IFE2_INTE_LL_ERR   0x00000002  ///< linked-list error interrupt enable
#define IFE2_INTE_LL_LATE  0x00000004  ///< linked-list read late interrupt enable
#define IFE2_INTE_LL_JEND  0x00000008  ///< linked-list jod end interrupt enable
#define IFE2_INTE_FRMERR   0x20000000  ///< frame error interrupt enable
#define IFE2_INTE_OVFL     0x40000000  ///< overflow interrupt enable
#define IFE2_INTE_FRMEND   0x80000000  ///< frame end interrupt enable
#define IFE2_INTE_ALL      (IFE2_INTE_FRMERR | IFE2_INTE_OVFL | IFE2_INTE_FRMEND | IFE2_INTE_LL_END | IFE2_INTE_LL_ERR | IFE2_INTE_LL_LATE | IFE2_INTE_LL_JEND)

/**
    Define IFE2 interrput status.
*/
#define IFE2_INTS_LL_END   0x00000001  ///< linked-list end interrupt status
#define IFE2_INTS_LL_ERR   0x00000002  ///< linked-list error interrupt status
#define IFE2_INTS_LL_LATE  0x00000004  ///< linked-list read late interrupt status
#define IFE2_INTS_LL_JEND  0x00000008  ///< linked-list jod end interrupt status
#define IFE2_INTS_FRMERR   0x20000000  ///< frame error interrupt status
#define IFE2_INTS_OVFL     0x40000000  ///< overflow interrupt status
#define IFE2_INTS_FRMEND   0x80000000  ///< frame end interrupt status
#define IFE2_INTS_ALL      (IFE2_INTS_FRMERR | IFE2_INTS_OVFL | IFE2_INTS_FRMEND | IFE2_INTS_LL_END | IFE2_INTS_LL_ERR | IFE2_INTS_LL_LATE | IFE2_INTS_LL_JEND)
//------------------------------------------------------------------


/**
    IFE2 enum - function enable/disable
*/
typedef enum {
	IFE2_FUNC_DISABLE   = 0,           ///< function disable
	IFE2_FUNC_ENABLE    = 1,           ///< function enable
	ENUM_DUMMY4WORD(IFE2_FUNC_EN)
} IFE2_FUNC_EN;
//------------------------------------------------------------------


/**
    IFE2 enum - operation mode selection
*/
typedef enum {
	IFE2_OPMODE_D2D     = 0,    ///< IFE2 only
	IFE2_OPMODE_IFE_IPP = 1,    ///< IFE->DCE->IPE->IME(IFE2)
	IFE2_OPMODE_SIE_IPP = 2,    ///< SIE + IFE->DCE->IPE->IME(IFE2) (all direct)
	IFE2_OPMODE_UNKNOWN = 3,    ///< Undefined mode
	ENUM_DUMMY4WORD(IFE2_OPMODE)
} IFE2_OPMODE;
//------------------------------------------------------------------


/**
    IFE2 enum - input image data format
*/
typedef enum {
	IFE2_FMT_YUVP   = 0,           ///< YUVYUV Packing
	IFE2_FMT_SIGL   = 1,           ///< Single Channel
	IFE2_FMT_Y_UVP  = 2,           ///< Y/UV-Packing
	ENUM_DUMMY4WORD(IFE2_FMT)
} IFE2_FMT;
//------------------------------------------------------------------


/**
    IFE2 enum - filter kernel size selection
*/
typedef enum {
	IFE2_FLTR_SIZE_3x3 = 0,           ///< 3x3 window
	IFE2_FLTR_SIZE_5x5 = 1,           ///< 5x5 window
	IFE2_FLTR_SIZE_7x7 = 2,           ///< 7x7 window
	IFE2_FLTR_SIZE_9x9 = 3,           ///< 9x9 window
	ENUM_DUMMY4WORD(IFE2_FILT_SIZE)
} IFE2_FILT_SIZE;
//------------------------------------------------------------------


/**
    IFE2 enum - edge kernel size selection
*/
typedef enum {
	IFE2_EKNL_SIZE_3x3 = 0,           ///< 3x3 window
	IFE2_EKNL_SIZE_5x5 = 1,           ///< 5x5 window
	ENUM_DUMMY4WORD(IFE2_EDGE_KERNEL_SIZE)
} IFE2_EDGE_KERNEL_SIZE;
//------------------------------------------------------------------


/**
    IFE2 enum - DMA burst length selection
*/
typedef enum {
	IFE2_BURST_32W = 0,           ///< burst 32 word
	IFE2_BURST_16W = 1,           ///< burst 16 word
	//IFE2_BURST_96W = 2,           ///< burst 96 word
	//IFE2_BURST_128W = 3,           ///< burst 128 word
	ENUM_DUMMY4WORD(IFE2_BURST_SEL)
} IFE2_BURST_SEL;
//------------------------------------------------------------------

/**

    IFE2 DMA Channel Handle Selection
*/
typedef enum {
	IFE2_DMAHDL_IN    = 0,  ///< selection input DMA channel
	IFE2_DMAHDL_OUT0  = 1,  ///< selection output DMA channel0
	ENUM_DUMMY4WORD(IFE2_DMAHDL_SEL)
} IFE2_DMAHDL_SEL;
//------------------------------------------------------------------

/**

    IFE2 Get Burst Length Selection
*/
typedef enum {
	IFE2_GET_BURST_IN  = 0,  ///< get input burst length
	IFE2_GET_BURST_OUT = 1,  ///< get output burst length
	ENUM_DUMMY4WORD(IFE2_GET_BURST_SEL)
} IFE2_GET_BURST_SEL;
//------------------------------------------------------------------


/**
    IFE2 structure - engine open object
*/
typedef struct _IFE2_OPENOBJ {
	void (*pfIfe2IsrCb)(UINT32 uiIntpStatus); ///< isr callback function
	UINT32 uiIfe2ClkSel;                    ///< Engine clock-rate in MHz; Selection: 48/60/80/120/160/192/220/240
} IFE2_OPENOBJ;
//------------------------------------------------------------------


/**
    IFE2 structure - image size parameters
*/
typedef struct _IFE2_IMG_SIZE {
	UINT32 img_size_h;   ///< input horizontal size, image width
	UINT32 img_size_v;  ///< input vertical size, image height
} IFE2_IMG_SIZE;
//------------------------------------------------------------------


/**
    IFE2 structure - image lineoffset parameters
*/
typedef struct _IFE2_LINEOFFSET {
	UINT32 lofs_y;   ///< lineoffset for Y channel
} IFE2_LINEOFFSET;
//------------------------------------------------------------------


/**
    IFE2 structure - input image DMA address
*/
typedef struct _IFE2_DMA_ADDR {
	UINT32 addr_y;   ///< lineoffset for Y channel
} IFE2_DMA_ADDR;
//------------------------------------------------------------------


/**
    IFE2 structure - reference center computation parameters
*/
typedef struct _IFE2_REFCENT_SET {
	UINT32 range_th[3];   ///< Range filter threshold for reference center
	UINT32 range_wet[4];   ///< Range filter weighting for reference center
	UINT32 cnt_wet;    ///< Reference center weighting
	UINT32 outlier_dth;  ///< Outlier difference threshold
	UINT32 outlier_th;   ///< Reference center outlier threshold
} IFE2_REFCENT_SET;
//------------------------------------------------------------------


/**
    IFE2 structure - edge direction threshold parameters
*/
typedef struct _IFE2_EDRCT_TH {
	UINT32  edth_hv;   ///< Edge direction threshold for two diagonal directions
	UINT32  edth_pn;   ///< Edge direction threshold for horizontal and vertical directions
} IFE2_EDRCT_TH;
//------------------------------------------------------------------


/**
    IFE2 structure - filter threshold and weighting LUT parameters
*/
typedef struct _IFE2_FILTER_SET {
	UINT32 filter_th[5];  ///< Filter threshold LUT
	UINT32 filter_wet[6];  ///< Filter weighting LUT
} IFE2_FILTER_SET;
//------------------------------------------------------------------


/**
    IFE2 structure - gray statistical parameters
*/
typedef struct _IFE2_GRAY_STATAL {
	UINT32 u_th0;  ///< Gray Statistical threshold for U channel
	UINT32 u_th1;  ///< Gray Statistical threshold for U channel
	UINT32 v_th0;  ///< Gray Statistical threshold for V channel
	UINT32 v_th1;  ///< Gray Statistical threshold for V channel
} IFE2_GRAY_STATAL;
//------------------------------------------------------------------


/**
    IFE2 structure - input and output burst size parameters
*/
typedef struct _IFE2_BURST_LENGHT {
	IFE2_BURST_SEL  in_bst_len;      ///< engine input burst lenght
	IFE2_BURST_SEL  out_bst_len;      ///< engine output burst lenght, if direct to IME, fixed as 64 words
} IFE2_BURST_LENGHT;


/**
    IFE2 structure - DMA address changing parameters
*/
typedef struct _IFE2_ADDR_PARAM {
	IFE2_DMAHDL_SEL     chl_sel;  ///< channel selection
	IFE2_DMA_ADDR       addr;       ///< changed dma address
} IFE2_ADDR_PARAM;
//------------------------------------------------------------------

/**
    IFE2 structure -lineoffset changing parameters
*/
typedef struct _IFE2_LOFS_PARAM {
	IFE2_DMAHDL_SEL     chl_sel;  ///< channel selection
	IFE2_LINEOFFSET     lofs;        ///< channel lineoffset
} IFE2_LOFS_PARAM;
//------------------------------------------------------------------

/**
    IFE2 structure - reference center changing parameters
*/
typedef struct _IFE2_REFCENT_PARAM {
	IFE2_REFCENT_SET    ref_cent_y;   ///< reference center parameters for Y channel
	IFE2_REFCENT_SET    ref_cent_uv;   ///< reference center parameters for UV channel
} IFE2_REFCENT_PARAM;
//------------------------------------------------------------------

/**
    IFE2 structure - filter parameter changing
*/
typedef struct _IFE2_FILTER_PARAM {
	IFE2_FUNC_EN            filter_y_en;         ///< filter Y channel enable
	IFE2_FILT_SIZE          filter_size;        ///< filter size
	IFE2_EDRCT_TH           filter_edth;        ///< filter edge direction threshold
	IFE2_EDGE_KERNEL_SIZE   edge_ker_size;    ///< edge kernel size
	IFE2_FILTER_SET         filter_set_y;        ///< filter parameter for Y channel
	IFE2_FILTER_SET         filter_set_u;        ///< filter parameter for U channel
	IFE2_FILTER_SET         filter_set_v;        ///< filter parameter for V channel
} IFE2_FILTER_PARAM;
//------------------------------------------------------------------

/**
    IFE2 structure - full engine parameters

    @note integrated structure
*/
typedef struct _IFE2_PARAM {
	IFE2_OPMODE             op_mode;         ///< engine operation mode

	IFE2_IMG_SIZE           img_size;        ///< image size
	IFE2_FMT                src_fmt;         ///< input image format
	IFE2_FMT                dst_fmt;         ///< output image format

	IFE2_LOFS_PARAM         in_lofs;          ///< input lineoffset
	IFE2_ADDR_PARAM         in_addr;         ///< input dma address


	IFE2_LOFS_PARAM         out_lofs;         ///< output lineoffset for Y/UV
	IFE2_ADDR_PARAM         out_addr0;       ///< output dma address for set0

	IFE2_REFCENT_PARAM      ref_cent;        ///< reference center parameters for Y / UV channel

	IFE2_FILTER_PARAM       filter;            ///< filter parameters

	IFE2_GRAY_STATAL        gray_sta;     ///< gray statistical
	IFE2_BURST_LENGHT       bst_size;      ///< engine burst lenght

	UINT32                  interrupt_en;      ///< interrupt enable
	IFE2_FUNC_EN            out_dram_en;      ///< output to dram enable when direct mode
} IFE2_PARAM;
//------------------------------------------------------------------


/**
    check IFE2 is opened.

    check if IFE2 module is opened.

    @return
        -@b TRUE: IFE2 is open.
        -@b FALSE: IFE2 is closed.
*/
extern BOOL ife2_is_opened(VOID);
//------------------------------------------------------------------

/**
  Open IFE2 driver

  This function should be called before calling any other functions

  @param[in] p_obj_cb ISR callback function

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
extern ER ife2_open(IFE2_OPENOBJ *p_obj_cb);
//------------------------------------------------------------------

/**
  set IFE2 mode

  Set IFE2 mode to execute

  @param[in] p_set_param Parameter for IFE2 execution
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_set_mode(IFE2_PARAM *p_set_param);
//------------------------------------------------------------------


/**
  Pause IFE2

  Set IFE2 status to Pause.

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_pause(VOID);
//------------------------------------------------------------------

/**
  Start IFE2

  Set IFE2 start bit to 1.

  @param[in] VOID

  @return  VOID
*/
extern ER ife2_start(VOID);
//------------------------------------------------------------------

/**
  Close IFE2 driver

  Release IFE2 driver

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
extern ER ife2_close(VOID);
//------------------------------------------------------------------

/**
    IFE2 engine hw reset

    @param[in] VOID

    @return ER  error code\n
    -@b E_OK:   setting done\n
    -@b Others: setting error\n

    @note after hw reset, user must reopen engine
*/
extern ER ife2_reset(VOID);
//------------------------------------------------------------------


/**
  Wait IFE2 Processing done

  Wait IFE2 Processing done

  @param[in] is_clear_flag   Decide to clear flag before waiting frame-end

  @return void
*/
extern VOID ife2_wait_flag_frame_end(BOOL is_clear_flag);
//------------------------------------------------------------------

/**
  Wait IFE2 Linked-list Processing done

  Wait IFE2 Linked-list Processing done

  @param[in] is_clear_flag   Decide to clear flag before waiting frame-end

  @return void
*/
extern VOID ife2_wait_flag_linked_list_end(BOOL is_clear_flag);
//------------------------------------------------------------------

/**
  Wait IFE2 Linked-list Job Processing done

  Wait IFE2 Linked-list Job Processing done

  @param[in] is_clear_flag   Decide to clear flag before waiting frame-end

  @return void
*/
extern VOID ife2_wait_flag_linked_list_job_end(BOOL is_clear_flag);
//------------------------------------------------------------------



/**
  Clear IFE2 Processing done Flag

  Clear IFE2 Processing done Flag

  @return void
*/
extern VOID ife2_clear_flag_frame_end(VOID);
//------------------------------------------------------------------

/**
  Clear IFE2 Linked-list done Flag

  Clear IFE2 Linked-list done Flag

  @return void
*/
extern VOID ife2_clear_flag_linked_list_end(VOID);
//------------------------------------------------------------------

/**
  Clear IFE2 Linked-list job done Flag

  Clear IFE2 Linked-list job done Flag

  @return void
*/
extern VOID ife2_clear_flag_linked_list_job_end(VOID);
//------------------------------------------------------------------


/**
  Check IFE2 Processing done Flag

  Check IFE2 Processing done Flag

  @return void
*/
//extern BOOL ife2_checkFlagFrameEnd(VOID);
//------------------------------------------------------------------

/**
  IFE2 interrupt status.

  Check IFE2 interrupt status.

  @return IFE2 interrupt status readout.
*/
extern UINT32 ife2_get_int_status(VOID);
//------------------------------------------------------------------

/**
  IFE2 clear interrupt status.

  IFE2 clear interrupt status.

  @param[in] uiIntrp 1's in this word will clear corresponding interrupt

  @return  VOID
*/
extern VOID ife2_clear_interrupt(UINT32 intp_val);
//------------------------------------------------------------------

/**
  Set input size

  Set input size

  @param[in] p_set_size    input image size

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_in_size(IFE2_IMG_SIZE *p_set_size);
//------------------------------------------------------------------

/**
  Get input size

  Get input size

  @param[in] puiHSize  input horizontal size
  @param[in] puiVSize  input vertical size

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern VOID ife2_get_in_size(UINT32 *p_size_h, UINT32 *p_size_v);
//------------------------------------------------------------------

/**
  Set input source format

  Set input source format

  @param[in] psrc_fmt input image format

  @return  VOID
*/
extern ER ife2_chg_src_fmt(IFE2_FMT src_fmt);
//------------------------------------------------------------------

/**
  Get input source format

  Get input source format

  @param[in] psrc_fmt source format 0: normal; 1:RAW

  @return  engine source format
*/
extern UINT32 ife2_get_src_fmt(VOID);
//------------------------------------------------------------------

/**
  Set output destination format

  Set output destination format

  @param[in] dst_fmt output image format

  @return  VOID
*/
extern ER ife2_chg_dst_fmt(IFE2_FMT dst_fmt);
//------------------------------------------------------------------

/**
  Get output source format

  Get output source format

  @return UINT32  output image format
*/
extern UINT32 ife2_get_dst_fmt(VOID);
//------------------------------------------------------------------

/**
  change input and output lineoffset

  change input and output DMA address

  @param[in] p_set_lofs   lineoffset parameters

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_lineoffset(IFE2_LOFS_PARAM *p_set_lofs);
//------------------------------------------------------------------

/**
  Get input and output lineoffset

  @param[in] chl_sel     channel selection

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern VOID ife2_get_lineoffset(IFE2_DMAHDL_SEL chl_sel, IFE2_LINEOFFSET *p_set_lofs);
//------------------------------------------------------------------

/**
  Change input and output DMA address

  @param[in] pChgAddr   change channel DMA address parameters

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_dma_addr(IFE2_ADDR_PARAM *p_set_addr);
//------------------------------------------------------------------

/*
  Get input and output DMA address

  Get input and output DMA address

  @param[in] chl_sel     input or output channel selection
  @param[in] pGetAddr   return channel address

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern VOID ife2_get_dma_addr(IFE2_DMAHDL_SEL chl_sel, IFE2_DMA_ADDR *p_get_addr);
//------------------------------------------------------------------

/**
  Change Reference Center Parameters

  @param[in] pChgref_cent    reference Center Parameters

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_ref_center_param(IFE2_REFCENT_PARAM *p_set_ref_cent);
//------------------------------------------------------------------

/**
  Change Filter Parameters

  @param[in] p_set_filter    Filter Parameters

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_filter_param(IFE2_FILTER_PARAM *p_set_filter);
//------------------------------------------------------------------

/**
  Change IFE2 Gray Statistical

  @param[in] p_set_gray_sta   output edge information

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
extern ER ife2_chg_gray_statal_param(IFE2_GRAY_STATAL *p_set_gray_sta);
//------------------------------------------------------------------



/**
  Get IFE2 clock-rate

  @param[in] VOID

  @return IFE2 clock-rate in MHz
*/
extern UINT32 ife2_get_clock_rate(VOID);
//------------------------------------------------------------------

/**
  Get IFE2 Base Address of Register

  @param[in] VOID

  @return UINT32 Address
*/
extern UINT32 ife2_get_reg_base_addr(VOID);
//------------------------------------------------------------------

/**
  Get IFE2 Get gray averagel for U and V channels

  @param[in] puiGetAvgU Get gray average of U channel
  @param[in] puiGetAvgV Get gray average of V channel

  @return VOID
*/
extern VOID ife2_get_gray_average(UINT32 *p_get_avg_u, UINT32 *p_get_avg_v);
//------------------------------------------------------------------

/**
  Get IFE2 HW Debug Function

  @param[in] uiEdgeEn Output edge information
  @param[in] uiref_centEn Output reference center information

  @return VOID
*/
extern VOID ife2_set_debug_function_enable(IFE2_FUNC_EN edge_en, IFE2_FUNC_EN ref_cent_en, IFE2_FUNC_EN eng_en);
//------------------------------------------------------------------

/**
  Change all parameters

  @param[in] p_set_param Parameter for IFE2 execution
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
//extern ER ife2_chg_all_param(IFE2_PARAM *p_set_param);
//------------------------------------------------------------------

/**
    Get burst length information

    @param[in] p_set_param Parameter for IFE2 execution
    @return INT32
*/
extern INT32 ife2_get_burst_length(IFE2_GET_BURST_SEL get_bst_sel);
//------------------------------------------------------------------


extern ER ife2_chg_direct_in_param(IFE2_PARAM *p_set_param);
extern ER ife2_chg_direct_out_param(IFE2_PARAM *p_set_param);
extern ER ife2_chg_direct_interrupt_enable_param(IFE2_PARAM *p_set_param);
extern ER ife2_chg_direct_gray_stl_param(IFE2_PARAM *p_set_param);
extern ER ife2_chg_direct_reference_center_param(IFE2_PARAM *p_set_param);
extern ER ife2_chg_direct_filter_param(IFE2_PARAM *p_set_param);

extern VOID ife2_chg_linked_list_addr(UINT32 addr);
extern VOID ife2_set_linked_list_fire(VOID);
extern VOID ife2_set_linked_list_terminate(VOID);

extern VOID ife2_set_builtin_flow_disable(VOID);

#if defined (_NVT_EMULATION_)
extern BOOL ife2_end_time_out_status;
//For FPGA verify only
extern ER ife2_start_without_load(void);
#endif

#if 0
#ifdef __KERNEL__

extern VOID ife2_set_base_addr(UINT32 uiAddr);

extern VOID ife2_create_resource(VOID);

extern VOID ife2_release_resource(VOID);

#endif
#endif


#ifdef __cplusplus
}  // extern "C"
#endif


#endif

//@}
