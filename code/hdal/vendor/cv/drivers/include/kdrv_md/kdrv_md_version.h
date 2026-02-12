/**
	@brief Vendor md implementation version.

	@file kdrv_md_version.h

	@ingroup kdrv_md

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KDRV_MD_VERSION_H_
#define _KDRV_MD_VERSION_H_

#include <kwrap/verinfo.h>

#define major     01
#define minor     00
#define bugfix    2012180
#define ext       0


#define _MDBC_STR(a,b,c)          #a"."#b"."#c
#define MDBC_STR(a,b,c)           _MDBC_STR(a,b,c)
#define KDRV_MD_IMPL_VERSION      MDBC_STR(major, minor, bugfix)//implementation version major.minor.yymmdds

#endif
