/**
	@brief dis driver implementation version.

	@file kdrv_dis_version.h

	@ingroup kdrv_dis

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KDRV_DIS_VERSION_H_
#define _KDRV_DIS_VERSION_H_

#include <kwrap/verinfo.h>


#define major     02
#define minor     00
#define bugfix    2101080
#define ext       0


#define _DIS_STR(a,b,c)          #a"."#b"."#c
#define DIS_STR(a,b,c)           _DIS_STR(a,b,c)
#define KDRV_DIS_VERSION         DIS_STR(major, minor, bugfix)//implementation version major.minor.yymmdds

#endif
