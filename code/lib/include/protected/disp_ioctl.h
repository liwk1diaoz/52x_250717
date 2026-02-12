/**
    The header file of nvtdisp ioctl operations.

    @file       disp_ioctl.h
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __DISP_IOCTL_H
#define __DISP_IOCTL_H

#include <asm/ioctl.h>

/**
    Disp error code.
*/
#define E_DISP_OK              (0) ///< Operation correct
#define E_DISP_PAR            (-4) ///< Input parameter error (E_PAR)
#define E_DISP_SYS            (-5) ///< System error (E_SYS)
#define E_DISP_IPC            (-6) ///< Disp IPC error
#define E_DISP_BUF            (-7) ///< Disp buffer error

/**
    disp ioct get/set value structure.
*/
typedef struct _DISP_IOC_VALUE_S{
    UINT32  DevID;		///< Device ID
    UINT32  LayerID;	///< Layer ID
    UINT32  param;		///< Layer attribute
    UINT32  value;		///< Attribute value
    INT32  rtn;		///< return result
} DISP_IOC_VALUE_S;


typedef struct _DISP_IOC_MEM_S{
    UINT32  addr;		///< buffer address
    UINT32  size;		///< buffer size
    INT32  rtn;		///< return result
} DISP_IOC_MEM_S;

typedef struct _DISP_IOC_SIZE_S{
    UINT32  DevID;		///< Device ID
	ISIZE 	size;		///< Device size
    INT32  rtn;		///< return result
} DISP_IOC_SIZE_S;

typedef struct _DISP_IOC_ADDR_S{
    UINT32  DevID;		///< Device ID
    UINT32  LayerID;	///< Layer ID
    UINT32  addr1;		///< address1
    UINT32  addr2;		///< address2
    UINT32  addr3;		///< address3
    UINT32  wait;		///< wait VD
    INT32  rtn;		///< return result
} DISP_IOC_ADDR_S;

typedef struct _DISP_IOC_FOTMAT_S{
    UINT32	DevID;		///< Device ID
    UINT32  LayerID;	///< Layer ID
	UINT32 	format;		///< buffer format
	UINT32 	w;			///< buffer width (pixle unit)
	UINT32 	h;			///< buffer height (pixle unit)
	UINT32 	loff;		///< buffer line offset (byte unit)
    INT32  rtn;		///< return result
} DISP_IOC_FOTMAT_S;

typedef struct _DISP_IOC_WIN_S{
    UINT32  DevID;		///< Device ID
    UINT32  LayerID;	///< Layer ID
	UINT32 	x;			///< scale win start x
	UINT32 	y;			///< scale win start y
	UINT32 	w;			///< scale win width
	UINT32 	h;			///< scale win height
    INT32  rtn;		///< return result
} DISP_IOC_WIN_S;

#define DISP_IOC_MAGIC        	'D'
#define DISP_IOC_MEM 		_IOWR(DISP_IOC_MAGIC, 1, DISP_IOC_MEM_S)
#define DISP_IOC_GET_DSIZE 	_IOWR(DISP_IOC_MAGIC, 2, DISP_IOC_SIZE_S)
#define DISP_IOC_INIT   	_IOWR(DISP_IOC_MAGIC, 3, DISP_IOC_VALUE_S)
#define DISP_IOC_UNINIT  	_IOWR(DISP_IOC_MAGIC, 4, DISP_IOC_VALUE_S)
#define DISP_IOC_WIN 		_IOWR(DISP_IOC_MAGIC, 5, DISP_IOC_WIN_S)
#define DISP_IOC_FMT 		_IOWR(DISP_IOC_MAGIC, 6, DISP_IOC_FOTMAT_S)
#define DISP_IOC_ADDR 		_IOWR(DISP_IOC_MAGIC, 7, DISP_IOC_ADDR_S)
#define DISP_IOC_GET   		_IOWR(DISP_IOC_MAGIC, 8, DISP_IOC_VALUE_S)
#define DISP_IOC_SET  		_IOWR(DISP_IOC_MAGIC, 9, DISP_IOC_VALUE_S)

#endif /* __DISP_IOCTL_H */
