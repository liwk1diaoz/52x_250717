/*
    @file       ExifRW.h
    @ingroup    mILIBEXIF

    @brief      exif writer/reader header file
    @version    V1.01.007
    @date       2011/08/30


    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _EXIFRW_H
#define _EXIFRW_H

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/type.h"
#include "exif/Exif.h"
#include "BinaryFormat.h"
#include "ExifReader.h"
#include "ExifWriter.h"

typedef UINT32  Exif_Offset;

#define THIS_DBGLVL         1 //0=OFF, 1=ERROR, 2=TRACE
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EXIF
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include <kwrap/debug.h>

#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig

/**
    @name valid tag position

    Valid tag position.
*/
//@{
#define EXIF_0thIFD     EXIF_IFD_0TH                //1       ///< tag position: zeroth IFD
#define EXIF_1stIFD     EXIF_IFD_1ST                //2       ///< tag position: 1st IFD
#define EXIF_ExifIFD    EXIF_IFD_EXIF               //3       ///< tag position: exif IFD
#define EXIF_0thIntIFD  EXIF_IFD_INTEROPERABILITY   //4       ///< tag position: zeroth interoperability IFD
#define EXIF_GPSIFD     EXIF_IFD_GPS                //5       ///< tag position: GPS IFD
//@}

#define ERROR_NO_EXIFIFD 0x8769

/**
    @name valid parameter type

    Valid parameter type.
*/
//@{
#define EXIFW_SETPARAM_BASIC    1       ///< set basic type: enable/disable
#define EXIFW_SETPARAM_GPS      2       ///< set GPS info: on/off
#define EXIFW_SETPARAM_MAX_LEN  3       ///< set max exif header size//2011/08/30 Meg Lin
//@}

#define CFG_EXIF_INIT_KEY    MAKEFOURCC('E','X','I','F') ///< a key value 'E','X','I','F' for indicating system initial.

#define MAX_EXIFREADTAG  65
#define EXIF_INSERT_MAX 80
#define EXIF_PADDATA_MAX 30  //max pading data in one IFD

/**
    Status type for ExifW_WriteExif() and ExifW_CompleteHeader()
*/
typedef struct {
	UINT32 *ExifAddr; ///< [in] output Exif header address
	UINT32 *ExifSize; ///< [in] avaliable buffer size
} EXIFPARAM;

#if 0
/**
    EXIF information structure for ExifR_GetTagData()
*/
typedef struct {
	UINT16 tagId;       ///< [in] wanted exif id
	UINT16 tagType;     ///< [out] tag data type
	UINT32 tagLen;      ///< [out] tag data length
	UINT32 tagDataAddr; ///< [out] if length <=4, value. if length > 4, tag data offset (to beginning of file)
} EXIF_GETTAG;
#endif
//#NT#2011/06/30#Meg Lin -begin
/**
    Structure for ExifW_ModifyDataTag() and ExifW_ModifyInsertDataTag()
*/
typedef struct {
	UINT32 tagLen;          ///< [in] new tag length (in bytes!!)
	UINT32 tagDataValue;    ///< [in] new tag data value
	UINT16 tagId;           ///< [in] tag id
	UINT16 rev;             ///< reserved
} EXIF_MODIFYDATATAG;
/**
    Structure for ExifW_ModifyPtrTag() and ExifW_ModifyInsertPtrTag()
*/
typedef struct {
	UINT32 tagLen;          ///< [in] new tag length (in bytes!!)
	UINT32 *ptagDataAddr;   ///< [in] new tag data address
	UINT16 tagId;           ///< [in] tag id
	UINT16 rev;             ///< reserved
} EXIF_MODIFYPTRTAG;

/**
    Structure for ExifW_InsertDataTag()
*/
typedef struct {
	UINT32 tagPos;          ///< [in] tag position, EXIF_0thIFD or others
	UINT16 tagId;           ///< [in] tag id, 0x920A or others
	UINT16 tagType;         ///< [in] tag type, TypeBYTE or others
	UINT32 tagLen;          ///< [in] tag length (in bytes!!)
	UINT32 tagDataValue;    ///< [in] tag data value
	Exif_Offset offset;     ///< [out] don't care
} EXIF_INSERTDATATAG;
/**
    Structure for ExifW_InsertPtrTag()
*/
typedef struct {
	UINT32 tagPos;          ///< [in] tag position, EXIF_0thIFD or others
	UINT16 tagId;           ///< [in] tag id, 0x920A or others
	UINT16 tagType;         ///< [in] tag type, TypeBYTE or others
	UINT32 tagLen;          ///< [in] tag length (in bytes!!)
	UINT32 *ptagDataAddr;   ///< [in] tag data addr
	Exif_Offset offset;     ///< [out] don't care
} EXIF_INSERTPTRTAG;
//#NT#2011/06/30#Meg Lin -end



typedef struct _EXIF_CTRL {
	UINT32      uiInitKey;
	UINT32      WorkBufAddr;
	UINT32      WorkBufSize;
	FPEXIFCB    fpExifCB;
	UINT32      AvailableAddr;
	MAKERNOTE_INFO MakernoteInfo;
	UINT32      ReadBufBase;
	UINT32      ReadOffset;
	UINT32      ReadTagIndex;
	EXIF_FILE_INFO FileInfo;
	UINT16      ByteOrder;
	EXIF_PARSETAG ReadTag[MAX_EXIFREADTAG];
	UINT32      MpoOffsetBase;
	UINT32      HeaderSize;
	UINT32      TiffOffsetBase;
	UINT32      App1SizeOffset;
	UINT32      WriteBufBase;
	UINT32      WriteOffset;
	UINT16      Add0thTagNum;
	UINT16      AddExifTagNum;
	UINT16      Add1stTagNum;
	UINT16      AddTagTotal;
	UINT16      AddGpsTagNum;
	UINT16      Add0thIntTagNum;
	EXIF_INSERTTAG  InsertTag[EXIF_INSERT_MAX];
	EXIF_DATA       IFDPaddingData[EXIF_PADDATA_MAX];
	UINT32          ThumbAddr;
	UINT32          ThumbSize;
	UINT32          TempValue;
	UINT32          ExifDataAddr; //exif data address of EXIF_CreateExif or EXIF_ParseExif
} EXIF_CTRL, *PEXIF_CTRL;


extern ER xExif_lock(EXIF_HANDLE_ID HandleID);
extern void xExif_unlock(EXIF_HANDLE_ID HandleID);
extern BOOL xExif_CheckInited(PEXIF_CTRL pCtrl);
extern BOOL xExif_CheckHandleID(EXIF_HANDLE_ID HandleID);


extern UINT32   ExifR_ReadExif(EXIF_HANDLE_ID HandleID, UINT32 addr, UINT32 size);
extern void     ExifR_GetTagData(PEXIF_CTRL pCtrl, EXIF_GETTAG *pTag);

extern UINT32 xExif_GetWorkingBuf(PEXIF_CTRL pCtrl, UINT32 size);
extern UINT32 xExif_GetRestWorkingBufSize(PEXIF_CTRL pCtrl);
extern void xExif_ResetWorkingBuf(PEXIF_CTRL pCtrl);

extern PEXIF_CTRL xExif_GetCtrl(EXIF_HANDLE_ID HandleID);

extern UINT32 _SECTION(".kercfg_data") SEMID_EXIF[EXIF_HDL_MAX_NUM];

#endif //_EXIFRW_H

