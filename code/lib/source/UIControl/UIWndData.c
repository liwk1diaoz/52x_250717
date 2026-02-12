/**
    @file       UIFrameworkAPI.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      NVT UI framework API
                Implement functions of UI framework API

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

//#include "kernel.h"
//#include "UIFramework.h"
#include "UIControl/UIControlWnd.h"
#include "UIWndInternal.h"
#include "UIControl/UIControl.h"
/**
@addtogroup mIAPPExtUIFrmwk
@{

@name Data Functions
@{
*/


void UxCtrl_SetDataTable(VControl *pCtrl, void *DataTable)
{
	pCtrl->CtrlData = DataTable ;
}

void *UxCtrl_GetDataTable(VControl *pCtrl)
{
	return pCtrl->CtrlData;
}
void UxCtrl_SetUsrData(VControl *pCtrl, void *usrData)
{
	pCtrl->usrData = usrData ;
}
void *UxCtrl_GetUsrData(VControl *pCtrl)
{
	return pCtrl->usrData;
}
//@}

