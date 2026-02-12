/*
    Header file of mov fileformat writer.

    Exported header file of mov fileformat writer.

    @file       MovWriteLib.h
    @ingroup    mIAVMOV
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _MP_MOVWRITELIB_H
#define _MP_MOVWRITELIB_H


/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
/*
    checkID of mov writer
*/
//@{
#define MOVWRITELIB_CHECKID  0xBC191380 ///< mdat base time as checkID
//@}

#define MOV_AUDFMT_AAC      0x6D706461 // "mp4a"
#define MOV_AUDFMT_PCM      0x736f7774 // "sowt"
#define MOV_AUDFMT_ULAW     0x756c6177 // "ulaw"
#define MOV_AUDFMT_ALAW     0x616c6177 // "alaw"

#define MOVRVCY_MAX_CNT  2000//max 20G for one file

/*-----------------------------------------------------------------------------*/
/* Extern Functions                                                            */
/*-----------------------------------------------------------------------------*/
/*
    Public mov writer functions
*/
extern ER MovWriteLib_Initialize(UINT32 id);
extern ER MovWriteLib_SetMemBuf(UINT32 id, UINT32 startAddr, UINT32 size);
extern ER MovWriteLib_MakeHeader(UINT32 hdrAddr, UINT32 *pSize, MEDIAREC_HDR_OBJ *pobj);
extern ER MovWriteLib_GetInfo(MEDIAWRITE_GETINFO_TYPE type, UINT32 *pparam1, UINT32 *pparam2, UINT32 *pparam3);
extern ER MovWriteLib_SetInfo(MEDIAWRITE_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MovWriteLib_UpdateHeader(UINT32 type, void *pobj);
extern ER MovWriteLib_CustomizeFunc(UINT32 type, void *pobj);
extern void MOVRcvy_OpenCutMsg(UINT8 on);
extern void MOV_SetGPS_Entry(UINT32 id, UINT32 index, MOV_Ientry *ptr);
extern UINT32 MOVWriteCountTotalSeconds(void);
extern void MOV_SeekOffset(char *pb, MOV_Offset pos, UINT16 type);
extern void MOV_WriteB32Tag(char *pb, char *string);
extern void MOV_WriteB32Bits(char *pb, UINT32 value);
extern void MOV_WriteB64Bits(char *pb, UINT64 value);
extern UINT32 MovWriteLib_MakefrontFtyp(UINT32 outAddr, UINT32 uifType);
#if 1	//add for meta -begin
extern void MOV_SetMeta_Entry(UINT32 id, UINT32 index, MOV_Ientry *ptr);
#endif	//add for meta -end
#endif//_MOVWRITELIB_H

