/*
    H.26x wrapper operation.

    @file       h26x_wrap.c
    @ingroup    mIDrvCodec_H26x
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "h26xenc_int.h"

void h26XEnc_getRowRcState(SLICE_TYPE ePicType, H26XRegSet *pRegSet, H26XEncRowRc *pRowRc, UINT32 uiSVCId)
{
//	UINT32 is_p_frm = IS_PSLICE(ePicType);
	UINT32 uiFrmCmpxLsb;
	UINT32 uiFrmCmpxMsb;


	uiFrmCmpxMsb  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_MSB];
	uiFrmCmpxLsb  = pRegSet->CHK_REPORT[H26X_RRC_FRM_COMPLEXITY_LSB];

    pRowRc->i_all_ref_pred_tmpl = ((UINT64)uiFrmCmpxLsb) & 0xFFFFFFFF;
    pRowRc->i_all_ref_pred_tmpl |= ((UINT64)uiFrmCmpxMsb) << 32;

    //printf("%s %d, uiFrmCmpxMsb = 0x%08x  0x%08x, %lld  0x%08x, %d\n",__FUNCTION__,__LINE__,uiFrmCmpxLsb,uiFrmCmpxMsb,
        //pRowRc->i_all_ref_pred_tmpl, (int)pRowRc->i_all_ref_pred_tmpl, (int)pRowRc->i_all_ref_pred_tmpl);

}



