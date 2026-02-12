/**
	@brief kflow dis implementation version.

	@file kflow_dis_version.h

	@ingroup kflow_dis

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KFLOW_DIS_VERSION_H_
#define _KFLOW_DIS_VERSION_H_

#include <kwrap/verinfo.h>


#define major     00
#define minor     00
#define bugfix    2304190
#define ext       0

#define _DIS_STR(a,b,c)          #a"."#b"."#c
#define DIS_STR(a,b,c)           _DIS_STR(a,b,c)
#define KFLOW_DIS_VERSION         DIS_STR(major, minor, bugfix)//implementation version major.minor.yymmdds

#endif
