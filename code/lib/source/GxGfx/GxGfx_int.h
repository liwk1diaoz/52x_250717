#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

//////////////////////////////////////////
//  config debug system
//////////////////////////////////////////
#define __NAME__    GXGFX
#define __CLASS__   DBG_CLASS_GFX
//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxGfx
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include "kflow_common/type_vdo.h"

#if (0)

#define GXGFX_ASSERT(x) {if((x)==FALSE){DBG_DUMP("^RASSERT: %d, %s\r\n",__LINE__,#x);}}

#else

#define GXGFX_ASSERT(x)

#endif




#define GX_HW_NT962XX   0
#define GX_HW_NT964XX   1
#define GX_HW_NT96630   2
#define GX_HW_NT96450   3
#define GX_HW_NT96220   4
#define GX_HW_NT96650   5
#define GX_HW_NT96660   5
#define GX_HW_NT96680   6
#define GX_HW_NT96510   7

#define GX_HW                       GX_HW_NT96510

#define FORCE_SW                    1   //force SW bitblt
#define FORCE_SW_96660              0   //force SW bitblt

#if (GX_HW >= GX_HW_NT96680)
#define GX_SUPPORT_DCFMT_RGB444     1   //support RGB444 format
#else
#define GX_SUPPORT_DCFMT_RGB444     0
#endif



#if ((GX_HW >= GX_HW_NT96450) && (GX_HW != GX_HW_NT96220))
#define GX_SUPPORT_DCFMT_RGB565     1   //support RGB565 format
#else
#define GX_SUPPORT_DCFMT_RGB565     0
#endif

#if ((GX_HW == GX_HW_NT96650))
#define GX_SUPPORT_DCFMT_UVPACK     1   //support UVPACK format
#else
#define GX_SUPPORT_DCFMT_UVPACK     0
#endif


#if ((GX_HW == GX_HW_NT96680))
#define GX_SUPPORT_ROTATE_ENG      1   //support rotate engine
#else
#define GX_SUPPORT_ROTATE_ENG      0
#endif

#define GX_SUPPPORT_YUV             0

#define GX_ICON_SWCOPY              1  //for hd_gfx copy sould be nvtmpp,but image tbl is code
//Text buffer config:
extern UINT32 _gGfxStringBuf_Size;
#define PRINTF_BUFFER_SIZE          32

extern UINT32 *_gDC_Text_instr;
extern UINT32 *_gDC_Text_outstr;
extern UINT32 *_gDC_Text_prnstr;
extern UINT32 *_gDC_Text_bufstr;
extern UINT32 *_gDC_Text_line;
extern UINT32 *tr_str;

//#include "grph.h"  ////janice vos todo
/*
    Destination output selection for _sw_setAOP() API

    Select which destination output for graphic engine operation result.

    @note for grph_setGOP()/grph_setGOP2()/grph_setAOP()
*/
typedef enum {
	GRPH_DST_1 = 0x00,                  //< Destination use image 1
	GRPH_DST_2 = 0x20,                  //< Destination use image 2

	ENUM_DUMMY4WORD(GRPH_DST_SEL)
} GRPH_DST_SEL;



#define GR_MASK                    0x40000000
#define GR_OP                      0x1F000000

#define GR_RCW_90                  0x00000000
#define GR_RCW_180                 0x01000000
#define GR_RCW_270                 0x02000000
#define GR_MRR_X                   0x04000000
#define GR_MRR_Y                   0x08000000
#define GR_RCW_360                 0x10000000



#if GX_SUPPORT_ROTATE_ENG
#include "rotation.h"

#define 	HW_GOP_ROT_90                 GOP_ROT_90                    ///< Rotate 90 degree clockwise
#define 	HW_GOP_ROT_270                GOP_ROT_270                   ///< Rotate 270 degree clockwise
#define 	HW_GOP_ROT_180                GOP_ROT_180                   ///< Rotate 180 degree
#define 	HW_GOP_HRZ_FLIP               GOP_HRZ_FLIP                  ///< Horizontal flip
#define 	HW_GOP_VTC_FLIP               GOP_VTC_FLIP                  ///< Vertical flip
#define 	HW_GOP_HRZ_FLIP_ROT_90        GOP_HRZ_FLIP_ROT_90           ///< Horizontal flip and Rotate 90 degree clockwise
#define 	HW_GOP_HRZ_FLIP_ROT_270       GOP_HRZ_FLIP_ROT_270          ///< Horizontal flip and Rotate 270 degree clockwise
#define     HW_FORMAT_32BITS              FORMAT_32BITS
#define     HW_FORMAT_16BITS              FORMAT_16BITS
#define     HW_FORMAT_8BITS               FORMAT_8BITS
#else
#define 	HW_GOP_ROT_90                 GRPH_CMD_ROT_90                    ///< Rotate 90 degree clockwise
#define 	HW_GOP_ROT_270                GRPH_CMD_ROT_270                   ///< Rotate 270 degree clockwise
#define 	HW_GOP_ROT_180                GRPH_CMD_ROT_180                   ///< Rotate 180 degree
#define 	HW_GOP_HRZ_FLIP               GRPH_CMD_HRZ_FLIP                  ///< Horizontal flip
#define 	HW_GOP_VTC_FLIP               GRPH_CMD_VTC_FLIP                  ///< Vertical flip
#define 	HW_GOP_HRZ_FLIP_ROT_90        GRPH_CMD_ROT_90                    ///< Horizontal flip and Rotate 90 degree clockwise
#define 	HW_GOP_HRZ_FLIP_ROT_270       GRPH_CMD_ROT_270                   ///< Horizontal flip and Rotate 270 degree clockwise
#define     HW_FORMAT_32BITS               GRPH_FORMAT_32BITS
#define     HW_FORMAT_16BITS              GRPH_FORMAT_16BITS
#define     HW_FORMAT_8BITS               GRPH_FORMAT_8BITS

#endif


/*
    Arithmetric operation control

    Select which Arithmetric operation control mode for graphic engine operation.

*/
typedef enum {
	GRPH_AOP_A_COPY,            //0x00        //< A -> destination
	GRPH_AOP_PLUS_SHF,          //0x01        //< (A+(B>>SHF[1:0])) -> destination
	GRPH_AOP_MINUS_SHF,         //0x02        //< (A-(B>>SHF[1:0])) -> destination
	GRPH_AOP_COLOR_EQ,          //0x03        //< A -> destination with color key (=)
	GRPH_AOP_COLOR_LE,          //0x04        //< A -> destination with color key (<)
	GRPH_AOP_A_AND_B,           //0x05        //< A & B -> destination
	GRPH_AOP_A_OR_B,            //0x06        //< A | B -> destination
	GRPH_AOP_A_XOR_B,           //0x07        //< A ^ B -> destination
	GRPH_AOP_TEXT_COPY,         //0x08        //< text -> destination
	GRPH_AOP_TEXT_AND_A,        //0x09        //< text & A -> destination
	GRPH_AOP_TEXT_OR_A,         //0x0A        //< text | A -> destination
	GRPH_AOP_TEXT_XOR_A,        //0x0B        //< text ^ A -> destination
	GRPH_AOP_TEXT_AND_AB,       //0x0C        //< (text & A) | (~text & B) -> destination
	GRPH_AOP_BLENDING,          //0x0D        //< ((A * WA) + (B * WB) + 128  >> 8 )-> destination
	GRPH_AOP_ACC,               //0x0E        //< Acc(A): pixel accumulation
	GRPH_AOP_MULTIPLY_DIV,      //0x0F        //< (A * B + (1<<DIV[2:0]-1))>>DIV[2:0] -> destination
	GRPH_AOP_PACKING,           //0x10        //< Pack(A,B)->C
	GRPH_AOP_DEPACKING,         //0x11        //< Unpack(A) ->(B,C)
	GRPH_AOP_TEXT_MUL,          //0x12        //< (A*text+(1<<SHF-1)) >> SHF -> destination
	GRPH_AOP_PLANE_BLENDING,    //0x13        //< (A*B + C*(256-B) + 128) >> 8 -> destination
	GRPH_AOP_1D_LUT = 0x16,     //0x16        //< 1D Look up table OP -> destinaion
	GRPH_AOP_2D_LUT,            //0x17        //< 2D Look up table OP -> destinaion
	GRPH_AOP_RGBYUV_BLEND,      //0x18        //< RGB blending on YUV
	GRPH_AOP_RGBYUV_COLORKEY,   //0x19        //< RGB color key on YUV


	GRPH_AOP_MINUS_SHF_ABS = 0x80, //0x80        //< abs(A-(B>>SHF[1:0])) -> destination
	GRPH_AOP_COLOR_MR,          //0x81        //< A -> destination with color key (>)

	ENUM_DUMMY4WORD(GRPH_AOP_MODE)
} GRPH_AOP_MODE;




#endif //_GRAPHICS_H_

