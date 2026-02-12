/*;   Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
 *;
 *;   @file       BinInfo.c
 *;   @ingroup
 *;
 *;   @brief      Binary file information
 *;               Binary file information
 *;
 *;   @version    V1.20.000
 *;   @author     Novatek
 *;   @date       2012/07/19
 */

#include <hd_type.h>
#include <comm/bin_info.h>
extern char _section_01_addr[];
extern char _section_10_size[];

BININFO bin_info = {
	//HEADINFO
	{
		(UINT32)_section_01_addr, //<- fw CODE entry (4)
		{
			_BOARD_FDT_ADDR_,     //<- fdt entry
			_BOARD_SHMEM_ADDR_,   //<- shmem entry
#if defined(_FW_TYPE_PARTIAL_) || defined(_FW_TYPE_PARTIAL_COMPRESS_)
			BOOT_FLAG_PARTLOAD_EN, //<- partial flag
#else
			0,
#endif
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
		}, ///<- reserved (4*19)
		_BIN_NAME_, //<- CHIP-NAME (8)
		"10000000", //<- version (8)
		"20100701", //<- releasedate (8)
		(UINT32)_section_10_size, //<- Bin File Length (4)
		0xffffffff, //<- Check Sum or CRC (4)
		0,///<- Length check for CRC (4)
		0,///<- reserved (4)
		0,///<- Bin flag (4)
		0,///<- Binary Tag for CRC (4)
	},
	//RESERVED
	{0}
};


