/*
    @file       ExifReader.h
    @ingroup    mILIBEXIF

    @brief      exif header writer header file
    @version    V1.01.003
    @date       2011/03/09

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _EXIFREADER_H
#define _EXIFREADER_H

#include "kwrap/type.h"
typedef struct {
	//UINT32 app1LenPos;
	UINT32 app1Len;
	UINT32 zeroThIFDnum;
	UINT32 exifIFDOffset;
	UINT32 firstIFDPos;
	UINT32 exifIFDnum;
	UINT32 interOprPos;
	UINT32 firstIFDnum;
	UINT32 totalTagNum;
	UINT32 gpsIFDOffset;
	UINT32 gpsIFDnum;
	UINT16 rev;
	//UINT32 app2Len;
} EXIF_FILE_INFO;


typedef struct {
	UINT32 tagPos;      ///< out, EXIF_0thIFD
	UINT16 tagId;       ///< out
	UINT16 tagType;     ///< out
	UINT32 tagLen;      ///< out
	UINT32 tagDataOffset; ///< out
	UINT32 tagOffset;
} EXIF_PARSETAG;

#endif //_EXIFREADER_H

