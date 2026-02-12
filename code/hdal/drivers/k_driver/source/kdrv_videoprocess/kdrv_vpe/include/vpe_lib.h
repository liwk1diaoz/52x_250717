#ifndef __VPE_HW_H__
#define __VPE_HW_H__

//#include "vpe_platform.h"


#define VPE_ENABLE_IRQ                   1

#define VPE_REG_GLOCTL               0x0
#define VPE_BIT_SW_RST               0
#define VPE_BIT_DMA0_DIS             1

#define VPE_BIT_FIRE                 30
#define VPE_BIT_LLFIRE               31

#define VPE_REG_LLADDR              0x300

#define VPE_REG_INTS_1               0x400
#define VPE_REG_INTE_1               0x404
#define VPE_BIT_FRAME_DONE           0
#define VPE_BIT_LL_DONE              2

#define VPE_REG_STATUS_3             0x410
#define VPE_BIT_DMA0_SD_DONE         1
#define VPE_BIT_HW_IDLE              3

#define VPE_REG_IP_VERSION           0x414
#define VPE_REG_IP_INFO              0x418


#define BIT0  (0x00000001)
#define BIT1  (0x00000002)
#define BIT2  (0x00000004)
#define BIT3  (0x00000008)
#define BIT4  (0x00000010)
#define BIT5  (0x00000020)
#define BIT6  (0x00000040)
#define BIT7  (0x00000080)
#define BIT8  (0x00000100)
#define BIT9  (0x00000200)
#define BIT10 (0x00000400)
#define BIT11 (0x00000800)
#define BIT12 (0x00001000)
#define BIT13 (0x00002000)
#define BIT14 (0x00004000)
#define BIT15 (0x00008000)
#define BIT16 (0x00010000)
#define BIT17 (0x00020000)
#define BIT18 (0x00040000)
#define BIT19 (0x00080000)
#define BIT20 (0x00100000)
#define BIT21 (0x00200000)
#define BIT22 (0x00400000)
#define BIT23 (0x00800000)
#define BIT24 (0x01000000)
#define BIT25 (0x02000000)
#define BIT26 (0x04000000)
#define BIT27 (0x08000000)
#define BIT28 (0x10000000)
#define BIT29 (0x20000000)
#define BIT30 (0x40000000)
#define BIT31 (0x80000000)

/**
    VPE enum - system flag clear selection
*/
typedef enum _VPE_FLAG_CLEAR_SEL {
	VPE_FLAG_NO_CLEAR = 0,    ///< No clear flag
	VPE_FLAG_CLEAR    = 1,    ///< Clear flag
	ENUM_DUMMY4WORD(VPE_FLAG_CLEAR_SEL)
} VPE_FLAG_CLEAR_SEL;

//typedef void (*VPE_INT_CB)(UINT32);






extern ER vpe_wait_flag_frame_end(VPE_FLAG_CLEAR_SEL is_clear_flag);
extern VOID vpe_clear_flag_frame_end(VOID);
extern ER vpe_wait_flag_linked_list_job_end(VPE_FLAG_CLEAR_SEL is_clear_flag);
extern VOID vpe_clear_flag_linked_list_job_end(VOID);





#endif

