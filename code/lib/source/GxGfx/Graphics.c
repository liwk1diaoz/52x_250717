
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "Brush.h"
#include "GxGfx_int.h"
#include <kwrap/semaphore.h>

//--------------------------------------------------------------------------------------
//  gx
//--------------------------------------------------------------------------------------

UINT32 _gGfxInit = 0;
UINT32 _gGfxStringBuf_Size = 256; //defult 256 byte
UINT32 *_gGfxWorkBuf = 0;
UINT32 _gGfxWorkBuf_Size = 0;
UINT32 *_gGfxStackBuf = 0;
UINT32 _gGfxStackFree_Size = 0;

UINT32 *_gDC_Text_instr;
UINT32 *_gDC_Text_outstr;
UINT32 *_gDC_Text_prnstr;
UINT32 *_gDC_Text_bufstr;
UINT32 *_gDC_Text_line;
UINT32 *tr_str;
static ID GFX_SEM_ID = 0;

RESULT GxGfx_Config(UINT32 cfg, UINT32 value)
{
	DBG_FUNC_BEGIN("\r\n");
	if (cfg == CFG_STRING_BUF_SIZE) {
		if (value) {
			_gGfxStringBuf_Size = value;
		} else {
			return GX_ERROR_PARAM;
		}
	}
	return GX_OK;
}

RESULT GxGfx_Init(UINT32 *pWorkBuf, UINT32 nWorkBufSize)
{
	DBG_FUNC_BEGIN("\r\n");
	if (_gGfxInit != 0) {
		DBG_ERR("GxGfx init fail! already init, not exit yet.\r\n");
		return GX_STILLREADY;
	}
	if (pWorkBuf == 0) {
		DBG_ERR("GxGfx init fail! no working buffer.\r\n");
		return GX_NULL_BUF;
	}
	if (nWorkBufSize < 0x1000) {
		DBG_ERR("GxGfx init fail! working buffer is smaller than 0x1000.\r\n");
		return GX_OUTOF_MEMORY;
	}

	_VAR_Init();
	_BR_Init();

	//init string buffer
	_gDC_Text_instr = pWorkBuf;
	_gDC_Text_outstr = (UINT32 *)((UINT32)_gDC_Text_instr + _gGfxStringBuf_Size);
	_gDC_Text_prnstr = (UINT32 *)((UINT32)_gDC_Text_outstr + _gGfxStringBuf_Size);
	_gDC_Text_bufstr = (UINT32 *)((UINT32)_gDC_Text_prnstr + _gGfxStringBuf_Size);
	_gDC_Text_line = (UINT32 *)((UINT32)_gDC_Text_bufstr + _gGfxStringBuf_Size);
	tr_str = (UINT32 *)((UINT32)_gDC_Text_line + _gGfxStringBuf_Size);


	_gGfxWorkBuf = (UINT32 *)((UINT32)pWorkBuf + _gGfxStringBuf_Size * 6);
	_gGfxWorkBuf_Size = (nWorkBufSize - _gGfxStringBuf_Size * 6) / 4;

    if(_gGfxWorkBuf>(UINT32 *)((UINT32)pWorkBuf+nWorkBufSize)) {
        DBG_DUMP("_gGfxStringBuf_Size %x\r\n",_gGfxStringBuf_Size);
        DBG_DUMP("pWorkBuf %x \r\n",(UINT32)pWorkBuf);
        DBG_DUMP("_gDC_Text_instr %x \r\n",(UINT32)_gDC_Text_instr);
        DBG_DUMP("_gDC_Text_outstr %x \r\n",(UINT32)_gDC_Text_outstr);
        DBG_DUMP("_gDC_Text_prnstr %x \r\n",(UINT32)_gDC_Text_prnstr);
        DBG_DUMP("_gDC_Text_bufstr %x \r\n",(UINT32)_gDC_Text_bufstr);
        DBG_DUMP("tr_str %x \r\n",(UINT32)tr_str);
        DBG_DUMP("_gGfxWorkBuf %x _gGfxWorkBuf_Size %x\r\n",(UINT32)_gGfxWorkBuf,_gGfxWorkBuf_Size);

        DBG_ERR("but too samll\r\n");
        return GX_OUTOF_MEMORY;
    }
	OS_CONFIG_SEMPHORE(GFX_SEM_ID, 0, 1, 1);

	GxGfx_ResetStack();
	_gGfxInit = 1;
	return GX_OK;
}

RESULT GxGfx_Exit(void)
{
	DBG_FUNC_BEGIN("\r\n");
	if (_gGfxInit != 1) {
		DBG_ERR("GxGfx exit fail! not init yet, or already exit.\r\n");
		return GX_NOTREADY;
	}

	SEM_DESTROY(GFX_SEM_ID);

	_gGfxInit = 0;
	return GX_OK;
}

void GxGfx_ResetStack(void)
{
	DBG_FUNC_BEGIN("\r\n");

	_gGfxStackBuf = _gGfxWorkBuf;
	_gGfxStackFree_Size = _gGfxWorkBuf_Size;
}
UINT32 *GxGfx_PushStack(UINT32 uiSize)
{
	UINT32 *pBuf;
	UINT32 *pSizeInfo;
	UINT32 uiAllocSize;

	DBG_FUNC_BEGIN("\r\n");

	if (uiSize == 0) {
		return 0;
	}

	if ((!_gGfxStackBuf) || (_gGfxStackFree_Size == 0)) {
		DBG_ERR("GxGfx_PushStack - ERROR! Not assign work buffer for stack.\r\n");
		return 0;
	}

	uiAllocSize = (uiSize + 3) / 4;
	if ((uiAllocSize + 1) > _gGfxStackFree_Size) {
		DBG_ERR("GxGfx_PushStack - ERROR! Not enough space for push stack.\r\n");
		return 0;
	}
	pBuf = _gGfxStackBuf;
	_gGfxStackBuf += (uiAllocSize + 1);
	_gGfxStackFree_Size -= (uiAllocSize + 1);

	pSizeInfo = _gGfxStackBuf - 1;
	pSizeInfo[0] = uiAllocSize;

	return pBuf;
}
void GxGfx_PopStack(void)
{
	UINT32 *pSizeInfo;
	UINT32 uiAllocSize;

	DBG_FUNC_BEGIN("\r\n");

	if ((!_gGfxStackBuf) || (_gGfxStackFree_Size == 0)) {
		return;
	}

	if (_gGfxStackBuf == _gGfxWorkBuf) {
		return;
	}

	pSizeInfo = _gGfxStackBuf - 1;

	uiAllocSize = pSizeInfo[0];

	_gGfxStackBuf -= (uiAllocSize + 1);
	_gGfxStackFree_Size += (uiAllocSize + 1);
}

RESULT GxGfx_Lock(void)
{
    if (GFX_SEM_ID) {
    	return vos_sem_wait(GFX_SEM_ID);
    } else {
        DBG_ERR("not ini\r\n");
        return GX_NOTREADY;
    }
}
RESULT GxGfx_Unlock(void)
{
    if (GFX_SEM_ID) {
        vos_sem_sig(GFX_SEM_ID);
        return GX_OK;
    } else {
        DBG_ERR("not ini\r\n");
        return GX_NOTREADY;
    }
}
