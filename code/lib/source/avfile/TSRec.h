/*
    Header file of mov recorder.

    Exported header file of mov fileformat writer.

    @file       movRec.h
    @ingroup    mIAVMOV
    @note       Nothing.
    @version    V1.00.000
    @date       2019/05/05
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _MP_TSREC_H
#define _MP_TSREC_H

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#if defined(_BSP_NA51000_)
#define TSWRITER_COUNT  	16
#elif defined(_BSP_NA51023_)
#define TSWRITER_COUNT      8
#elif defined(_BSP_NA51055_)
#define TSWRITER_COUNT      16
#else
#define TSWRITER_COUNT      16
#endif

#define TS_VIDEOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
#define TS_AUDIOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
#define TS_VIDEO_264_NAL_LENGTH    6
#define TS_VIDEO_265_NAL_LENGTH    7
#define TS_AUDIO_ADTS_LENGTH       0
#define TS_PTS_LENGTH              5
#define TS_PES_PACKET_LENGTH_END   6  //(start_code 3 +Stream_id 1 +PES_packet_length 2)

#define TS_IDTYPE_VIDPATHID      2//video pathid
#define TS_IDTYPE_AUDPATHID      3//audio pathid

#define TS_CODEC_MJPG            1       ///< motion jpeg
#define TS_CODEC_H264            2       ///< h.264
#define TS_CODEC_H265            3      ///< h.265

/*-----------------------------------------------------------------------------*/
/* Extern Functions                                                            */
/*-----------------------------------------------------------------------------*/
extern ER TSWriteLib_Initialize(UINT32 idnum);
extern ER TSWriteLib_SetMemBuf(UINT32 idnum, UINT32 startAddr, UINT32 size);
extern ER TSWriteLib_GetInfo(MEDIAWRITE_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
extern ER TSWriteLib_SetInfo(MEDIAWRITE_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);

extern PCONTAINERMAKER ts_getContainerMaker(void);
extern void TSWriteLib_MakePesHeader(UINT32 fstskid, UINT32 srcAddr, UINT32 payload_size, UINT32 idtype);

extern void TS_Write_PTS(UINT8 *q, UINT64 pts);
extern void TS_Write_VidNAL(UINT32 fstskid, UINT8 *q);
extern void TS_Write_AudADTS(UINT8 *q, UINT32 bsSize);
extern void TS_Write_Stuffing(UINT8 *q, UINT32 size);

#endif
