#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"
#include "kdrv_vdocdc_dbg.h"

extern const UINT32 prefix_mask[33];

UINT32 log2bin(UINT32 uiVal)
{
	UINT32 uiRet = 0;

	while (uiVal) {
		uiVal >>= 1;
		uiRet++;
	}
	return uiRet;
}

UINT32 get_from_reg(UINT32 reg, UINT32 b_addr, UINT32 bits)
{
    UINT32 val;

    val = (reg >> b_addr) & prefix_mask[bits];
    return val;
}

void __inline save_to_reg(UINT32 *reg, INT32 val, UINT32 b_addr, UINT32 bits)
{
	*reg &= (~(prefix_mask[bits]<<b_addr));
	*reg |= ((val & prefix_mask[bits])<<b_addr);
}

UINT8 bit_reverse(UINT8 pix)
{
    int i;
    char temp_pix = 0;
    for(i = 7; i >= 0; i--)
    {
        temp_pix |= ((pix>>i)&prefix_mask[1])<<(7-i);
    }

    if(pix&0xffffff00)
    {
        DBG_ERR("warning: pix value is over 8 bit\n");
        while(1);
    }

    return temp_pix;
}



UINT32 set_ll_cmd(H26X_LL_CMD cmd, int job_id, int offset, int burst)
{
	UINT32 val = 0;

	save_to_reg(&val, burst,   0,  8);
	save_to_reg(&val, offset,  8, 12);
	save_to_reg(&val, job_id, 20,  8);
	save_to_reg(&val, cmd,    28,  4);

	return val;
}

void SetMemoryAddr(UINT32 *puiVirAddr, UINT32 *puiVirMemAddr, UINT32 uiSize)
{
    *puiVirAddr = *puiVirMemAddr; *puiVirMemAddr = *puiVirMemAddr + SIZE_32X(uiSize);
    h26x_flushCache((UINT32)puiVirAddr, SIZE_32X(uiSize));
}

UINT32 rbspToebsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiSize)
{
	UINT32 uiCount = 0, i, j;

	for (i = 0, j = 0; j < uiSize; i++, j++) {
		if (uiCount == 2 && ((pucBuf[j] & 0xFC) == 0)) {
			pucAddr[i++] = 0x03;
			uiCount = 0;
		}
		pucAddr[i] = pucBuf[j];

		uiCount = (pucAddr[i]) ? 0 : uiCount + 1;
	}

	return i;
}

UINT32 ebspTorbsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiTotalBytes)
{
    UINT32 uiCount, i, j;

    for(i = 0, j = 0, uiCount = 0; i < uiTotalBytes; i++)
    {
        // starting from begin_bytepos to avoid header information
        if(uiCount == 2 && pucAddr[i] == 0x03)
        {
            i++;
            uiCount = 0;
        }
        pucBuf[j] = pucAddr[i];

        uiCount = (pucAddr[i]) ? 0 : uiCount + 1;
        j++;
    }

    return j;
}

static void h26x_wrapLLCmd(UINT32 id, UINT32 uiVaApbAddr, UINT32 uiVaCurLLAddr, UINT32 uiPaNxtLLAddr)
{
	UINT32 uiPaApbAddr = h26x_getPhyAddr(uiVaApbAddr);

	UINT32 *pAddr = (UINT32 *)uiVaCurLLAddr;
	UINT32 i = 0;

	pAddr[i++] = set_ll_cmd(H26X_LL_START,  id, 0, 0);
	pAddr[i++] = set_ll_cmd(H26X_LL_WR,     id, H26X_REG_BASIC_START_OFFSET, H26X_REG_BASIC_SIZE);
	memcpy(pAddr + i, (void *)uiVaApbAddr, sizeof(UINT32)*H26X_REG_BASIC_SIZE);
	i += H26X_REG_BASIC_SIZE;
	pAddr[i++] = set_ll_cmd(H26X_LL_WR,     id, H26X_REG_PLUG_IN_START_OFFSET, H26X_REG_PLUG_IN_SIZE);
	memcpy(pAddr+ i, (void *)(uiVaApbAddr + (H26X_REG_PLUG_IN_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_SIZE);
	i += H26X_REG_PLUG_IN_SIZE;
	pAddr[i++] = set_ll_cmd(H26X_LL_WR,     id, H26X_REG_PLUG_IN_2_START_OFFSET, H26X_REG_PLUG_IN_2_SIZE);
	memcpy(pAddr+ i, (void *)(uiVaApbAddr + (H26X_REG_PLUG_IN_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_2_SIZE);
	i += H26X_REG_PLUG_IN_2_SIZE;
	pAddr[i++] = set_ll_cmd(H26X_LL_WR,     id, H26X_REG_PLUG_IN_3_START_OFFSET, H26X_REG_PLUG_IN_3_SIZE);
	memcpy(pAddr+ i, (void *)(uiVaApbAddr + (H26X_REG_PLUG_IN_3_START_OFFSET - H26X_REG_BASIC_START_OFFSET)), sizeof(UINT32)*H26X_REG_PLUG_IN_3_SIZE);
	i += H26X_REG_PLUG_IN_3_SIZE;
	pAddr[i++] = set_ll_cmd(H26X_LL_RD,     id, H26X_REG_REPORT_START_OFFSET, H26X_REPORT_NUMBER);
	pAddr[i++] = (uiPaApbAddr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	pAddr[i++] = set_ll_cmd(H26X_LL_FINISH, id, 0, 0);
	pAddr[i++] = uiPaNxtLLAddr;
}

INT32 h26x_setLLCmd(UINT32 id, UINT32 uiVaApbAddr, UINT32 uiVaCurLLAddr, UINT32 uiPaNxtLLAddr, UINT32 uiLLBufSize)
{
	if (uiLLBufSize < (h26x_getHwRegSize() + 64)) {
		//DBG_ERR("uiLLBufSize(%d) small than (%d), not enough\r\n", uiLLBufSize, h26x_getHwRegSize() + 64);
		return -1;
	}

	h26x_wrapLLCmd(id, uiVaApbAddr, uiVaCurLLAddr, uiPaNxtLLAddr);
	h26x_flushCache(uiVaCurLLAddr, uiLLBufSize);

	return 0;
}

UINT32 buf_chk_sum(UINT8 *buf, UINT32 size, UINT32 format)
{
	UINT32 i;
	UINT32 chk_sum = 0;

	for (i = 0; i < size; i++)
		chk_sum += (format) ? bit_reverse(*(buf + i)) : *(buf + i);

	return chk_sum;
}



