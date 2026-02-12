#include <string.h>
#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "FileSysTsk.h"

static UINT32 guiPlayLastMoveOffset_X, guiPlayLastMoveOffset_Y;

void PB_PlayZoomMode(UINT32 PlayCommand)
{
	if (!(PlayCommand & (PLAYZOOM_IN | PLAYZOOM_OUT))) {
		return;
	}

	gucCurrPlayZoomCmmd = PlayCommand;
	PB_PlaySetCmmd(PB_CMMD_ZOOM);
}

INT32 PB_PlayZoomPanMode(UINT32 PlayCommand, UINT32 MoveOffsetX, UINT32 MoveOffsetY)
{
	UINT32 IfMoving2Bound = 0;

	if (!(PlayCommand & (PLAYZOOM_UP | PLAYZOOM_DOWN | PLAYZOOM_LEFT | PLAYZOOM_RIGHT))) {
		return PBERR_PAR;
	}

	////////////////////////////////////////////////////////////////////////////
	// Alignment for IME Scaling function
	////////////////////////////////////////////////////////////////////////////
	MoveOffsetX = ALIGN_CEIL_8(MoveOffsetX);
	MoveOffsetY = ALIGN_CEIL_8(MoveOffsetY);

	// Moving UP
	if (PlayCommand & PLAYZOOM_UP) {
		if (MoveOffsetY > gMenuPlayInfo.PanSrcHeight) {
			MoveOffsetY = gMenuPlayInfo.PanSrcHeight;
		}

		if (gMenuPlayInfo.PanSrcStartY == 0) {
			IfMoving2Bound = 1;
		} else if (gMenuPlayInfo.PanSrcStartY == gMenuPlayInfo.PanMaxY) {
			if ((guiPlayLastMoveOffset_Y > gMenuPlayInfo.PanSrcStartY) || (guiPlayLastMoveOffset_Y == 0)) {
				gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanSrcStartY - (gMenuPlayInfo.PanMaxY >> 1);
			} else {
				gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanSrcStartY - guiPlayLastMoveOffset_Y;
			}
		} else if ((gMenuPlayInfo.PanSrcStartY) >= MoveOffsetY) {
			gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanSrcStartY - MoveOffsetY;
			guiPlayLastMoveOffset_Y = gMenuPlayInfo.PanSrcStartY;
		} else {
			guiPlayLastMoveOffset_Y = gMenuPlayInfo.PanSrcStartY;
			gMenuPlayInfo.PanDstStartY = 0;
			IfMoving2Bound = 1;
		}
	}
	// Moving DOWN
	else if (PlayCommand & PLAYZOOM_DOWN) {
		if (MoveOffsetY > gMenuPlayInfo.PanSrcHeight) {
			MoveOffsetY = gMenuPlayInfo.PanSrcHeight;
		}

		if (gMenuPlayInfo.PanSrcStartY == gMenuPlayInfo.PanMaxY) {
			IfMoving2Bound = 1;
		} else if (gMenuPlayInfo.PanSrcStartY == 0) {
			if (guiPlayLastMoveOffset_Y > gMenuPlayInfo.PanMaxY) {
				gMenuPlayInfo.PanDstStartY = (gMenuPlayInfo.PanMaxY >> 1);
			} else {
				gMenuPlayInfo.PanDstStartY = guiPlayLastMoveOffset_Y;
			}
		} else if ((gMenuPlayInfo.PanSrcStartY + MoveOffsetY) <= (gMenuPlayInfo.PanMaxY)) {
			gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanSrcStartY + MoveOffsetY;
			guiPlayLastMoveOffset_Y = gMenuPlayInfo.PanMaxY - gMenuPlayInfo.PanSrcStartY;
		} else {
			guiPlayLastMoveOffset_Y = gMenuPlayInfo.PanMaxY - gMenuPlayInfo.PanSrcStartY;
			gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanMaxY;
			IfMoving2Bound = 1;
		}

	}
	// Moving LEFT
	//else if(PlayCommand & PLAYZOOM_LEFT)
	if (PlayCommand & PLAYZOOM_LEFT) {
		if (MoveOffsetX > gMenuPlayInfo.PanSrcWidth) {
			MoveOffsetX = gMenuPlayInfo.PanSrcWidth;
		}

		if (gMenuPlayInfo.PanSrcStartX == 0) {
			IfMoving2Bound = 1;
		} else if (gMenuPlayInfo.PanSrcStartX == gMenuPlayInfo.PanMaxX) {
			if ((guiPlayLastMoveOffset_X > gMenuPlayInfo.PanSrcStartX) || (guiPlayLastMoveOffset_X == 0)) {
				gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanSrcStartX - (gMenuPlayInfo.PanMaxX >> 1);
			} else {
				gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanSrcStartX - guiPlayLastMoveOffset_X;
			}
		} else if ((gMenuPlayInfo.PanSrcStartX) >= MoveOffsetX) {
			gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanSrcStartX - MoveOffsetX;
			guiPlayLastMoveOffset_X = gMenuPlayInfo.PanSrcStartX;
		} else {
			guiPlayLastMoveOffset_X = gMenuPlayInfo.PanSrcStartX;
			gMenuPlayInfo.PanDstStartX = 0;
			IfMoving2Bound = 1;
		}
	}
	// Moving RIGHT
	else if (PlayCommand & PLAYZOOM_RIGHT) {
		if (MoveOffsetX > gMenuPlayInfo.PanSrcWidth) {
			MoveOffsetX = gMenuPlayInfo.PanSrcWidth;
		}

		if (gMenuPlayInfo.PanSrcStartX == gMenuPlayInfo.PanMaxX) {
			IfMoving2Bound = 1;
		} else if (gMenuPlayInfo.PanSrcStartX == 0) {
			if (guiPlayLastMoveOffset_X > gMenuPlayInfo.PanMaxX) {
				gMenuPlayInfo.PanDstStartX = (gMenuPlayInfo.PanMaxX >> 1);
			} else {
				gMenuPlayInfo.PanDstStartX = guiPlayLastMoveOffset_X;
			}
		} else if ((gMenuPlayInfo.PanSrcStartX + MoveOffsetX) <= (gMenuPlayInfo.PanMaxX)) {
			gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanSrcStartX + MoveOffsetX;
			guiPlayLastMoveOffset_X = gMenuPlayInfo.PanMaxX - gMenuPlayInfo.PanSrcStartX;
		} else {
			guiPlayLastMoveOffset_X = gMenuPlayInfo.PanMaxX - gMenuPlayInfo.PanSrcStartX;
			gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanMaxX;
			IfMoving2Bound = 1;
		}
	}
	gucCurrPlayZoomCmmd = PlayCommand;
	if ((gMenuPlayInfo.PanSrcStartX != gMenuPlayInfo.PanDstStartX) ||
		(gMenuPlayInfo.PanSrcStartY != gMenuPlayInfo.PanDstStartY)) {
		PB_PlaySetCmmd(PB_CMMD_ZOOM);
	}

	if (IfMoving2Bound == 1) {
		return PB_STA_DONE;
	} else {
		return PBERR_OK;
	}
}

#if 0
#pragma mark -
#endif

INT32 PB_PlayZoomUserSetting(UINT32 LeftUp_X, UINT32 LeftUp_Y, UINT32 RightDown_X, UINT32 RightDown_Y)
{
	if ((RightDown_X <= LeftUp_X) || (RightDown_Y <= LeftUp_Y)) {
		return PBERR_PAR;
	}
	//if ((RightDown_X > gMenuPlayInfo.pJPGInfo->ori_imagewidth) || (RightDown_Y > gMenuPlayInfo.pJPGInfo->ori_imageheight)) {
	//	return PBERR_PAR;	
	//}

	gusPlayZoomUserLeftUp_X = LeftUp_X;
	gusPlayZoomUserLeftUp_Y = LeftUp_Y;
	gusPlayZoomUserRightDown_X = RightDown_X;
	gusPlayZoomUserRightDown_Y = RightDown_Y;

	gucCurrPlayZoomCmmd = PLAYZOOM_USER;
	PB_PlaySetCmmd(PB_CMMD_ZOOM);

	return PBERR_OK;
}

