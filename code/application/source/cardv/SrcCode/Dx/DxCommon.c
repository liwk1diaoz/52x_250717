#include "Dx.h"
#include "DxCommon.h"
#include "DxApi.h"
#include "DxStorage.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxComm
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define IS_SIGN(x) ((x&DXFLAG_SIGN_MASK)==DXFLAG_SIGN)

// Get Capability Flag
UINT32 Dx_Getcaps(DX_HANDLE DxObject, UINT32 CapID, UINT32 Param1)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	DBG_MSG("CapID %08x\r\n", CapID);

	if (!pDX || !IS_SIGN(pDX->sign)) {
		DBG_ERR("DX_NULL_POINTER\r\n");
		return 0;
	}
	if (!pDX->pfGetcaps) {
		DBG_ERR("DX_NOT_SUPPORT Getcaps\r\n");
		return 0;
	}


	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);

	return pDX->pfGetcaps(CapID, Param1);
}

// Set Config Setting
UINT32 Dx_Setconfig(DX_HANDLE DxObject, UINT32 CfgID, UINT32 Param1)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	DBG_MSG("CfgID %08x\r\n", CfgID);
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (!pDX->pfSetcfgs) {
		return DX_NOT_SUPPORT;
	}

	return pDX->pfSetcfgs(CfgID, Param1);
}

// Set Init Parameters
UINT32 Dx_Init(DX_HANDLE DxObject, void *pInitParam, DX_CB pfCallback, UINT32 CurrVer)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (pDX->sign & DXFLAG_OPEN) {
		return DX_ALREADY_OPEN;
	}
	if (pDX->ver != CurrVer) {
		return DX_VER_NOTMATCH;
	}
	if ((pDX->pfOpen == 0)
		|| (pDX->pfClose == 0)) {
		return DX_LACKOF_FUNC;
	}
	pDX->pfCallback = pfCallback;
	if (!pDX->pfInit) {
		return DX_NOT_SUPPORT;
	}

	//pDX->type = TYPE_NULL; //clear
	//pDX->sign = 0; //clear
	return pDX->pfInit(pInitParam);
}

// Common Constructor
UINT32 Dx_Open(DX_HANDLE DxObject)
{
	UINT32 r;
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (pDX->sign & DXFLAG_OPEN) {
		return DX_ALREADY_OPEN;
	}
	if (!pDX->pfOpen) {
		return DX_NOT_SUPPORT;
	}

	r = pDX->pfOpen();
	if (r == DX_OK) {
		pDX->sign |= DXFLAG_OPEN;
	}
	return r;
}

// Common Destructor
UINT32 Dx_Close(DX_HANDLE DxObject)
{
	UINT32 r;
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (!(pDX->sign & DXFLAG_OPEN)) {
		return DX_NOT_OPEN;
	}
	if (!pDX->pfClose) {
		return DX_NOT_SUPPORT;
	}

	r = pDX->pfClose();
	pDX->sign &= ~DXFLAG_OPEN;
	return r;
}

// General Properties
UINT32 Dx_GetState(DX_HANDLE DxObject, UINT32 StateID, UINT32 *pOut)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	//if(!(pDX->sign & DXFLAG_OPEN))
	//    return DX_NOT_OPEN;
	if (!pDX->pfState) {
		return DX_NOT_SUPPORT;
	}

	*pOut = pDX->pfState(DXGET | StateID, 0);

	return DX_OK;
}

// General Properties
UINT32 Dx_SetState(DX_HANDLE DxObject, UINT32 StateID, UINT32 Value)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (!(pDX->sign & DXFLAG_OPEN)) {
		return DX_NOT_OPEN;
	}
	if (!pDX->pfState) {
		return DX_NOT_SUPPORT;
	}

	return pDX->pfState(DXSET | StateID, Value);
}

// General Methods
UINT32 Dx_Control(DX_HANDLE DxObject, UINT32 CmdID, UINT32 Param1, UINT32 Param2)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	if (!(pDX->sign & DXFLAG_OPEN)) {
		return DX_NOT_OPEN;
	}
	if (!pDX->pfControl) {
		return DX_NOT_SUPPORT;
	}

	return pDX->pfControl(CmdID, Param1, Param2);
}

// General Command Console
BOOL Dx_Command(DX_HANDLE DxObject, CHAR *pcCmdStr)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}
	//DX_ASSERT((pDX->sign & DXFLAG_SIGN_MASK) == DXFLAG_SIGN);
	//if(!(pDX->sign & DXFLAG_OPEN))
	//    return DX_NOT_OPEN;
	if (!pDX->pfCommand) {
		return DX_NOT_SUPPORT;
	}

	return pDX->pfCommand(pcCmdStr);
}

UINT32 Dx_GetInfo(DX_HANDLE DxObject, DX_INFO InfoID, void *pOut)
{
	DX_OBJECT *pDX = (DX_OBJECT *)DxObject;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDX || !IS_SIGN(pDX->sign)) {
		return DX_NULL_POINTER;
	}

	switch (InfoID) {
	case DX_INFO_CLASS:
		*(UINT32 *)pOut = pDX->type;
		break;
	case DX_INFO_NAME:
		*(char **)pOut = pDX->name;
		break;
	default:
		return DX_NOT_SUPPORT;
	}

	return DX_OK;
}


