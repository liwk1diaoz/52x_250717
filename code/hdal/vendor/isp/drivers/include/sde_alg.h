#ifndef _SDE_ALG_H_
#define _SDE_ALG_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "kwrap/type.h"
#endif

//=============================================================================
// struct & definition
//=============================================================================

/**
	SDE process id
*/
typedef enum _SDE_ID {
	SDE_ID_1 = 0,                      ///< sde id 1
	SDE_ID_2,                          ///< sde id 2
	SDE_ID_3,                          ///< sde id 3
	SDE_ID_4,                          ///< sde id 4
	SDE_ID_5,                          ///< sde id 5
	SDE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(SDE_ID)
} SDE_ID;

typedef struct _SDE_IF_IO_INFO {
	UINT32 width;                      ///< image width
	UINT32 height;                     ///< image height
	UINT32 in0_addr;                   ///< image input physical address of input 0 (left)
	UINT32 in1_addr;                   ///< image input physical address of input 1 (right)
	UINT32 cost_addr;                  ///< image output physical address of cost volume
	UINT32 out0_addr;                  ///< image output physical address of disparity
	UINT32 out1_addr;                  ///< image output physical address of confidence
	UINT32 in0_lofs;                   ///< image input lineoffset of input 0 (left)
	UINT32 in1_lofs;                   ///< image input lineoffset of input 1 (right)
	UINT32 cost_lofs;                  ///< image output lineoffset of cost volume
	UINT32 out0_lofs;                  ///< image output lineoffset of disparity
	UINT32 out1_lofs;                  ///< image output lineoffset of confidence
} SDE_IF_IO_INFO;

typedef enum _SDE_IF_DISP_OP_MODE {
	SDE_IF_MAX_DISP_31 = 0,            ///< max disparity 31
	SDE_IF_MAX_DISP_63,                ///< max disparity 63
	SDE_IF_OPMODE_UNKNOWN,             ///< undefined mode
	ENUM_DUMMY4WORD(SDE_IF_DISP_OP_MODE)
} SDE_IF_DISP_OP_MODE;

typedef enum _SDE_IF_DISP_INV_SEL {
	SDE_IF_INV_0 = 0,                  ///< fill in 0 to invalid point
	SDE_IF_INV_63,                     ///< fill in 63 to invalid point
	SDE_IF_INV_255,                    ///< fill in 255 to invalid point
	ENUM_DUMMY4WORD(SDE_IF_DISP_INV_SEL)
} SDE_IF_DISP_INV_SEL;

typedef struct _SDE_IF_CTRL_PARAM {
	// out0 disparity control
	BOOL disp_val_mode;                ///< SDE disparity value mode. 0: out_disp = disp << 2 (6b in 8b MSB ) ; 1: out_disp = disp (6b in 8b LSB )
	SDE_IF_DISP_OP_MODE disp_op_mode;  ///< SDE opertaion mode, disaprity range selection.
	SDE_IF_DISP_INV_SEL disp_inv_sel;  ///< invalid value selection

	// out1 confidence control
	BOOL conf_en;                      ///< SDE confidence output enable
} SDE_IF_CTRL_PARAM;

typedef struct _SDE_IF_PARAM_PTR {
	SDE_IF_IO_INFO           *io_info;
	SDE_IF_CTRL_PARAM        *ctrl;
} SDE_IF_PARAM_PTR;

#endif
