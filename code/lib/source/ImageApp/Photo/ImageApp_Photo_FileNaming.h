/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ImageApp_Photo_FileNaming.h
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2017/06/14
*/
#ifndef IA_PHOTO_FILE_NAMING_H
#define IA_PHOTO_FILE_NAMING_H
#include "ImageApp/ImageApp_Photo.h"

extern INT32 ImageApp_Photo_FileNaming_SetFileDBPool(MEM_RANGE *pPool);
extern INT32 ImageApp_Photo_FileNaming_SetRootPath(CHAR *prootpath);
extern INT32 ImageApp_Photo_FileNaming_SetFolder(PHOTO_CAP_FOLDER_NAMING *pfolder_naming);
extern void ImageApp_Photo_FileNaming_SetFileNameCB(PHOTO_FILENAME_CB *pfFileNameCb);
extern INT32 ImageApp_Photo_FileNaming_Open(void);
extern INT32 ImageApp_Photo_FileNaming_Close(void);
extern INT32 ImageApp_Photo_WriteCB(UINT32 Addr, UINT32 Size, UINT32 Fmt, UINT32 uiPathId);
extern void ImageApp_Photo_FileNaming_SetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn);

#endif




