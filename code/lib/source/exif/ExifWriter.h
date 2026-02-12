/*
    @file       ExifWriter.h
    @ingroup    mILIBEXIF

    @brief      exif header writer header file
    @version    V1.01.005
    @note       add ExifW_InsertDataTag/ExifW_InsertPtrTag
    @date       2011/06/30

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _EXIFWRITER_H
#define _EXIFWRITER_H

#include "kwrap/type.h"
//#if _Support_GPSInfo_Tag
#define DEFAULT_0THIFD_NUM      5//12
#define DEFAULT_0THIFD_GPS_NUM  1
//#else
//#define DEFAULT_0THIFD_NUM      11
//#endif

#define DEFAULT_EXIFIFD_NUM     6//31
#define BASIC_EXIFIFD_NUM       5

#define DEFAULT_0THINTOPE_NUM   2
#define DEFAULT_1STIFD_NUM      6

#define DEFAULT_GPS_NUM         9


//typedef UINT32  Exif_Offset;

typedef struct {
	UINT32 *ptagAddr;
	UINT32 tagLen;
	UINT16 tagtype;
	UINT16 tagId;//2010/12/22 Meg Lin
} EXIF_DATA;


/**
    Structure for ExifW_InsertTag()
*/
typedef struct {
	UINT32 tagPos;          ///< [in] tag position, EXIF_0thIFD or others
	UINT16 tagId;           ///< [in] tag id, 0x920A or others
	UINT16 tagType;         ///< [in] tag type, TypeBYTE or others
	UINT32 tagLen;          ///< [in] tag length (in bytes!!)
	UINT32 *ptagDataAddr;   ///< [in] tag data addr
	UINT32 offset;     ///< [out] don't care
} EXIF_INSERTTAG;



#endif //_EXIFWRITER_H
