#include "ive_ll_cmd.h"
#include "ive_platform.h"
extern VOID ive_setLLAddrReg(UINT32 uiInAddr);

#define IVE_DEBUG_LL DISABLE

/*
    table index :   [0x0, 0xff]         bits : 7_0
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_null_cmd(UINT8 tbl_idx)
{
	return (UINT64)tbl_idx;
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_bit60_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)9 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_bit59_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)0x11 << 59) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)8 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx)
{
	return (UINT64)(((UINT64)2 << 60) + ((UINT64)addr << 8) + (UINT64)tbl_idx);
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 ive_ll_err_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)0xF << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/**
    IVE set register to ll buffer
*/
VOID ive_setdata_err_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    *tmp_buf = ive_ll_err_cmd(0xF, (UINT16) r_ofs, r_value);
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:A *llbuf=0x%x r_ofs=0x%x value=0x%llx \r\n", (UINT32)*llbuf, r_ofs, **llbuf);
#endif
    (*llbuf)++;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:Next *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    IVE set register to exit ll handle
*/
VOID ive_setdata_exit_ll(UINT64 **llbuf, UINT32 ll_idx)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_exit:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    *tmp_buf = ive_ll_null_cmd((UINT8) ll_idx);
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_exit:A *llbuf=0x%x ll_idx=0x%x value=0x%llx\r\n", (UINT32)*llbuf, ll_idx, **llbuf);
#endif
    (*llbuf)++;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_exit:NEXT *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    IVE set register to ll buffer
*/
VOID ive_setdata_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value, UINT8 is_bit60)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
	if (is_bit60 == 1) {
		*tmp_buf = ive_ll_bit60_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	} else if (is_bit60 == 2) {
		*tmp_buf = ive_ll_bit59_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	} else {
    	*tmp_buf = ive_ll_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	}
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:A *llbuf=0x%x r_ofs=0x%x value=0x%llx \r\n", (UINT32)*llbuf, r_ofs, **llbuf);
#endif
    (*llbuf)++;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll:Next *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    IVE set register to end ll handle
*/
VOID ive_setdata_end_ll(UINT64 **llbuf, UINT32 ll_idx, UINT8 is_next_update)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf; 
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_end:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    (*llbuf)++;
#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_end:NEXT *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif

    if (is_next_update == 1) {
        *tmp_buf = ive_ll_nextupd_cmd(ive_platform_va2pa((UINT32) ((*llbuf)+4)));
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
		tmp_buf = *llbuf; (*llbuf)++;
        *tmp_buf = ive_ll_nextll_cmd(ive_platform_va2pa((UINT32) ((*llbuf)+4)), (UINT8) ll_idx);
		ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        ive_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy 
	} else if (is_next_update == 2) {
		*tmp_buf = ive_ll_nextupd_cmd(ive_platform_va2pa((UINT32) (*llbuf)));
        ive_setdata_err_ll(llbuf, 0xC, 0x5A5A5A5A); //dummy
		tmp_buf = *llbuf; (*llbuf)++;
        *tmp_buf = ive_ll_nextll_cmd(ive_platform_va2pa((UINT32) (*llbuf)), (UINT8) ll_idx);
		ive_setdata_err_ll(llbuf, 0xC, 0x5A5A5A5A); //dummy
    } else {
        *tmp_buf = ive_ll_nextll_cmd(ive_platform_va2pa((UINT32) (*llbuf)), (UINT8) ll_idx);
    }

#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_end:A *llbuf=0x%x ll_idx=0x%x next_ll_addr=0x%x value=0x%llx\r\n", (UINT32) tmp_buf, ll_idx, (UINT32) *llbuf, *tmp_buf);
#endif
}

/**
    IVE set register to ll start addr
*/
VOID ive_set_start_addr_ll(UINT32 start_addr)
{
	ive_setLLAddrReg(ive_platform_va2pa(start_addr));
#if (IVE_DEBUG_LL==ENABLE)
	nvt_dbg(ERR, "LL_START_ADDR: va=0x%x pa=0x%x\r\n", start_addr, ive_platform_va2pa(start_addr));
#endif
}

/**
    IVE set register to trigger ll
*/
VOID ive_flush_buf_ll(UINT64 *s_addr, UINT64 *e_addr)
{
    UINT32 flush_start_addr;
    UINT32 flush_size;
	UINT32 ret;

    flush_start_addr = (UINT32)s_addr;
    flush_size = (sizeof(UINT64)*(e_addr-s_addr));

#if (IVE_DEBUG_LL==ENABLE)
    nvt_dbg(ERR, "ll_mode: flush_start_addr=0x%x, flush_size=%d, is_trigger\r\n", flush_start_addr, flush_size);
#endif

    ret = ive_platform_dma_flush_mem2dev(flush_start_addr, flush_size);
	if (ret != 0) {
		nvt_dbg(ERR, "ll_mode: Error to do ive_platform_dma_flush_mem2dev().\r\n");
	}
	
}
