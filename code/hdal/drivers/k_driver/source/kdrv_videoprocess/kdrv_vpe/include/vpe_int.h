#ifndef __VPE_INT_H__
#define __VPE_INT_H__

#include "vpe_dbg.h"
#include "vpe_reg.h"

/*
    @name VPE enum - Engine lock status selection
*/
typedef enum _VPE_USED_STATUS {
	VPE_ENGINE_UNLOCKED = 0,  ///< input DMA direction
	VPE_ENGINE_LOCKED = 1,    ///< output DMA direction
	ENUM_DUMMY4WORD(VPE_USED_STATUS)
} VPE_USED_STATUS;


extern volatile NT98528_VPE_REGISTER_STRUCT *vpeg;
extern BOOL fw_vpe_power_saving_en;


extern UINT32 vpe_hw_read(UINT32 reg);
extern INT32 vpe_hw_write(UINT32 reg, UINT32 val);
extern int vpe_hw_init(void);
extern int vpe_hw_exit(void);


//extern VOID vpe_load(VOID);
extern ER vpe_lock(VOID);
extern ER vpe_unlock(VOID);
extern ER vpe_attach(VOID);
extern ER vpe_detach(VOID);

extern ER vpe_reset(VOID);


#endif
