/**
	@brief Vendor md implementation version.

	@file vendor_md_version.h

	@ingroup vendor_md

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_MD_VERSION_H_
#define _VENDOR_MD_VERSION_H_

#define md_major     01
#define md_minor     00
#define md_bugfix    2012180
#define md_ext       0

#define _MD_STR(a,b,c)          #a"."#b"."#c
#define MD_STR(a,b,c)           _MD_STR(a,b,c)
#define VENDOR_MD_VERSION    	MD_STR(md_major, md_minor, md_bugfix)//implementation version major.minor.yymmdds

#endif
