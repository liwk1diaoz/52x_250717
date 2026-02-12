#include "kdrv_ai.h"
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
#include "nue2_platform.h"
#include "nue2_ll_cmd.h"
#include "nue2_lib.h"

#define NUE2_DEBUG_LL DISABLE

/*
    table index :   [0x0, 0xff]         bits : 7_0
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_null_cmd(UINT8 tbl_idx)
{
	return (UINT64)tbl_idx;
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_bit60_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)9 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_bit59_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)0x11 << 59) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)8 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx)
{
	return (UINT64)(((UINT64)2 << 60) + ((UINT64)addr << 8) + (UINT64)tbl_idx);
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nue2_ll_err_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)0xF << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/**
    NUE2 set register to ll buffer
*/
VOID nue2_setdata_err_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    *tmp_buf = nue2_ll_err_cmd(0xF, (UINT16) r_ofs, r_value);
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:A *llbuf=0x%x r_ofs=0x%x value=0x%llx \r\n", (UINT32)*llbuf, r_ofs, **llbuf);
#endif
    (*llbuf)++;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:Next *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    NUE2 set register to exit ll handle
*/
VOID nue2_setdata_exit_ll(UINT64 **llbuf, UINT32 ll_idx)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_exit:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    *tmp_buf = nue2_ll_null_cmd((UINT8) ll_idx);
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_exit:A *llbuf=0x%x ll_idx=0x%x value=0x%llx\r\n", (UINT32)*llbuf, ll_idx, **llbuf);
#endif
    (*llbuf)++;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_exit:NEXT *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    NUE2 set register to ll buffer
*/
VOID nue2_setdata_ll(UINT64 **llbuf, UINT32 r_ofs, UINT32 r_value, UINT8 mode) //mode:(1:bit60,2:bit59)
{
    UINT64 *tmp_buf;

    tmp_buf = *llbuf;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
	if (mode == 1) {
		*tmp_buf = nue2_ll_bit60_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	} else if (mode == 2) {
		*tmp_buf = nue2_ll_bit59_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	} else {
    	*tmp_buf = nue2_ll_upd_cmd(0xF, (UINT16) r_ofs, r_value);
	}
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:A *llbuf=0x%x r_ofs=0x%x value=0x%llx \r\n", (UINT32)*llbuf, r_ofs, **llbuf);
#endif
    (*llbuf)++;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll:Next *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
}

/**
    NUE2 set register to end ll handle
*/
VOID nue2_setdata_end_ll(UINT64 **llbuf, UINT32 ll_idx, UINT8 is_next_update)
{
    UINT64 *tmp_buf;
	static UINT32 count = 0;

    tmp_buf = *llbuf; 
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_end:B *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif
    (*llbuf)++;
#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_end:NEXT *llbuf=0x%x\r\n", (UINT32)*llbuf);
#endif

    if (is_next_update == 1) {
        *tmp_buf = nue2_ll_nextupd_cmd(nue2_pt_va2pa((UINT32) ((*llbuf)+4)));
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
		tmp_buf = *llbuf; (*llbuf)++;
        *tmp_buf = nue2_ll_nextll_cmd(nue2_pt_va2pa((UINT32) ((*llbuf)+4)), (UINT8) ll_idx);
		nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy 
	} else if (is_next_update == 2) {
		*tmp_buf = nue2_ll_nextupd_cmd(nue2_pt_va2pa((UINT32) ((*llbuf)+2)));
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
		tmp_buf = *llbuf; (*llbuf)++;
        *tmp_buf = nue2_ll_nextll_cmd(nue2_pt_va2pa((UINT32) ((*llbuf)+2)), (UINT8) ll_idx);
		nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
        nue2_setdata_ll(llbuf, 0xC, 0x5A5A5A5A, 0); //dummy
		
		if (count == 3) {
			**llbuf = 0xffff000000000000;
        	(*llbuf)++;
		} else {
			if (count < 3) {
				count++;
			}
		}
    } else {
        *tmp_buf = nue2_ll_nextll_cmd(nue2_pt_va2pa((UINT32) (*llbuf)), (UINT8) ll_idx);
    }

#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_end:A *llbuf=0x%x ll_idx=0x%x next_ll_addr=0x%x value=0x%llx\r\n", (UINT32) tmp_buf, ll_idx, (UINT32) *llbuf, *tmp_buf);
#endif
}

/**
    NUE2 set register to ll start addr
*/
VOID nue2_set_start_addr_ll(UINT32 start_addr)
{
    nue2_set_dmain_lladdr(nue2_pt_va2pa(start_addr));
#if (NUE2_DEBUG_LL==ENABLE)
	DBG_ERR("LL_START_ADDR: va=0x%x pa=0x%x\r\n", start_addr, nue2_pt_va2pa(start_addr));
#endif
}

/**
    NUE2 set register to trigger ll
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_flush_buf_ll(UINT64 *s_addr, UINT64 *e_addr)
{
    UINT32 flush_start_addr;
    UINT32 flush_size;

    flush_start_addr = (UINT32)s_addr;
    flush_size = (sizeof(UINT64)*(e_addr-s_addr));

#if (NUE2_DEBUG_LL==ENABLE)
    DBG_ERR("ll_mode: flush_start_addr=0x%x, flush_size=%d, is_trigger\r\n", flush_start_addr, flush_size);
#endif

    nue2_pt_dma_flush_mem2dev(flush_start_addr, flush_size);
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
