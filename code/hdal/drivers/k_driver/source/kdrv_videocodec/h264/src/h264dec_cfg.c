/*
    H264DEC module driver for NT96660

    use INIT and INFO to setup SeqCfg and PicCfg

    @file       h264dec_cfg.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "h264dec_cfg.h"

//! Initialize decoder configurate
/*!
    Convert pH264DecInit to SeqCfg

    @return
        - @b H264DEC_SUCCESS: init success
        - @b H264DEC_FAIL   : init fail
*/
void h264Dec_initCfg(H264DEC_INIT *pH264DecInit,H264DecSeqCfg *pH264DecSeqCfg,H264DecPicCfg *pH264DecPicCfg,H264DecHdrObj *pH264DecHdrObj){
    H264DecSpsObj *pH264DecSpsObj = &pH264DecHdrObj->sH264DecSpsObj;

    // Sequence Cfg //
    pH264DecSeqCfg->uiSeqCfg  = (pH264DecSpsObj->uiLog2MaxFrmMinus4);
    pH264DecSeqCfg->uiSeqCfg |= (pH264DecSpsObj->uiLog2MaxPocLsbMinus4<<4);

    // Picture Size //
    pH264DecSeqCfg->uiPicSize  = ((pH264DecSpsObj->uiMbWidth - 1) << 4);
    pH264DecSeqCfg->uiPicSize |= ((pH264DecSpsObj->uiMbHeight - 1) << 20);

    // Source Picture height //
    pH264DecSeqCfg->uiPicWidth = pH264DecInit->uiWidth;
    pH264DecSeqCfg->uiPicHeight = pH264DecInit->uiHeight;

    // Rec LineOffset //
    pH264DecSeqCfg->uiRecLineOffset  = (pH264DecInit->uiYLineOffset);
    pH264DecSeqCfg->uiRecLineOffset |= (pH264DecInit->uiUVLineOffset << 16);
}
//! Update decoder configurate




