#include "cnn_ll_cmd.h"


/*
    table index :   [0x0, 0xff]         bits : 7_0
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_null_cmd(UINT8 tbl_idx)
{
	return (UINT64)tbl_idx;
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)8 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_supd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)9 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}
/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_oupd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)17 << 59) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}
/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx)
{
	return (UINT64)(((UINT64)2 << 60) + ((UINT64)addr << 8) + (UINT64)tbl_idx);
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 cnn_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}