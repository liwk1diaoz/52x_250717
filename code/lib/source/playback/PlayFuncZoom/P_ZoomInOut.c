#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

#define RATIO_16VS9_THRESHOLD   1.45
#define PB_ZOOMSCALE_UINT       200

/*
    Digital zoom
    This is internal API.

    @param ZoomIN_OUT (PLAYZOOM_IN/PLAYZOOM_OUT)
    @return void
*/
void PB_DigitalZoom(UINT32 ZoomIN_OUT)
{
#if _TODO
	UINT32 SrcWidth, SrcHeight, SrcStart_X = 0, SrcStart_Y = 0;
	UINT32 DstZoomLevel, StartZoomLevel, ZoomIndex;
	INT32  ZoomStep;
	UINT32 DstWidth = 0, DstHeight = 0, OriSrcWidth, OriSrcHeight, OriDstWidth;
	UINT16 usPBDisplayWidth, usPBDisplayHeight;
	UINT32 LastSrcWidth, LastSrcHeight; //LastZoomLevel

	usPBDisplayWidth  = g_PBDispInfo.uiDisplayW;
	usPBDisplayHeight = g_PBDispInfo.uiDisplayH;
	ZoomIndex = gMenuPlayInfo.ZoomIndex;

	if (gMenuPlayInfo.uipZoomLevelTbl == NULL) {
		if (ZoomIN_OUT == PLAYZOOM_IN) {
			StartZoomLevel = (ZoomIndex * PB_ZOOMSCALE_UINT);
			ZoomStep = PB_ZOOMSCALE_UINT;
		} else { //if(ZoomIN_OUT == PLAYZOOM_OUT)
			StartZoomLevel = ((ZoomIndex + 2) * PB_ZOOMSCALE_UINT);
			ZoomStep = -PB_ZOOMSCALE_UINT;
		}
	} else {
		if (ZoomIN_OUT == PLAYZOOM_IN) {
			StartZoomLevel = gMenuPlayInfo.uipZoomLevelTbl[ZoomIndex - 1];
		} else { //if(ZoomIN_OUT == PLAYZOOM_OUT)
			StartZoomLevel = gMenuPlayInfo.uipZoomLevelTbl[ZoomIndex + 1];
		}

		StartZoomLevel = StartZoomLevel * 2; //200;
		DstZoomLevel = gMenuPlayInfo.uipZoomLevelTbl[ZoomIndex];
		DstZoomLevel = DstZoomLevel * 2;    //200;
		ZoomStep = (INT32)(DstZoomLevel - StartZoomLevel);
	}

	if (StartZoomLevel == 0) {
		StartZoomLevel = PB_ZOOMSCALE_UINT;
	}

	//LastZoomLevel = StartZoomLevel;
	StartZoomLevel += ZoomStep;

	if ((gMenuPlayInfo.RotateDir == PLAY_ROTATE_DIR_90) || (gMenuPlayInfo.RotateDir == PLAY_ROTATE_DIR_270)) {
		gMenuPlayInfo.pJPGInfo->lineoffset_y  = gMenuPlayInfo.pJPGInfo->imageheight;
		gMenuPlayInfo.pJPGInfo->lineoffset_uv = gMenuPlayInfo.pJPGInfo->imageheight >> 1;
		OriSrcWidth  = gMenuPlayInfo.pJPGInfo->imageheight;
		OriSrcHeight = gMenuPlayInfo.pJPGInfo->imagewidth;
	} else {
		OriSrcWidth  = gMenuPlayInfo.pJPGInfo->imagewidth;
		OriSrcHeight = gMenuPlayInfo.pJPGInfo->imageheight;
	}

	// Digital zoom setting
	SrcWidth  = ((OriSrcWidth * PB_ZOOMSCALE_UINT) / StartZoomLevel);
	SrcHeight = ((OriSrcHeight * PB_ZOOMSCALE_UINT) / StartZoomLevel);
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61071 & 61073 (Unused value)
//    LastSrcWidth  = ((OriSrcWidth *PB_ZOOMSCALE_UINT)/LastZoomLevel);
//    LastSrcHeight = ((OriSrcHeight*PB_ZOOMSCALE_UINT)/LastZoomLevel);
//#NT#2016/05/30#Scottie -end

	// normal
	if (PB_KEEPAR_PILLARBOXING != PB_ChkImgAspectRatio(SrcWidth, SrcHeight, g_PBDispInfo.uiARatioW, g_PBDispInfo.uiARatioH)) {
		float fImageRatio;

		fImageRatio = (float)OriSrcWidth / (float)OriSrcHeight;
		if (fImageRatio > RATIO_16VS9_THRESHOLD) {
			UINT32 uiOriDstHeight;

			DstWidth  = usPBDisplayWidth;
			DstHeight = OriSrcHeight * DstWidth / OriSrcWidth * usPBDisplayHeight / usPBDisplayWidth * g_PBDispInfo.uiARatioW / g_PBDispInfo.uiARatioH;

			if (gMenuPlayInfo.uipZoomLevelTbl == NULL) {
				uiOriDstHeight = DstHeight * StartZoomLevel / PB_ZOOMSCALE_UINT;
			} else {
				uiOriDstHeight = DstHeight * gMenuPlayInfo.uipZoomLevelTbl[ZoomIndex] / gMenuPlayInfo.uipZoomLevelTbl[0];
			}

			if (uiOriDstHeight > usPBDisplayHeight) {
				DstHeight = usPBDisplayHeight;
				SrcHeight = OriSrcHeight * DstHeight / uiOriDstHeight;
				SrcWidth = SrcHeight * g_PBDispInfo.uiARatioW / g_PBDispInfo.uiARatioH;
			} else {
				SrcHeight = OriSrcHeight;
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61072 (Unused value)
#if 0
				if (ZoomIN_OUT == PLAYZOOM_IN) {
					LastSrcHeight = OriSrcHeight;
				}
#endif
//#NT#2016/05/30#Scottie -end
			}
		}

		gMenuPlayInfo.PanMaxX = (OriSrcWidth - SrcWidth) & 0xFFFFFFF8;
		// Use FW-Scale for images that width < 80, so we don't do alignment for it
		if (SrcWidth > 80) {
			float fTmpRatio;

			fTmpRatio = (float)SrcHeight / (float) SrcWidth;

			SrcWidth = (SrcWidth + 7) & 0xFFFFFFF0;// IME Input Stripe Width must be 16B-align
			if (SrcHeight != OriSrcHeight) {
				// Keep the image ratio after alignment
				SrcHeight = (UINT32)((FLOAT)SrcWidth * fTmpRatio);
				SrcHeight = (SrcHeight + 3) & 0xFFFFFFF8;
			}
		}

		gMenuPlayInfo.PanMaxY = (OriSrcHeight - SrcHeight);

		if (((ZoomIndex != 1) && (ZoomIN_OUT == PLAYZOOM_IN)) || (ZoomIN_OUT == PLAYZOOM_OUT)) {
			LastSrcWidth  = gMenuPlayInfo.PanSrcWidth;
			LastSrcHeight = gMenuPlayInfo.PanSrcHeight;
			if (ZoomIN_OUT == PLAYZOOM_IN) {
				SrcStart_X = gMenuPlayInfo.PanSrcStartX + ((LastSrcWidth - SrcWidth) >> 1);
				SrcStart_Y = gMenuPlayInfo.PanSrcStartY + ((LastSrcHeight - SrcHeight) >> 1);
			} else {
				UINT32 tmpStartX, tmpStartY;

				tmpStartX = ((SrcWidth - LastSrcWidth) >> 1);
				tmpStartY = ((SrcHeight - LastSrcHeight) >> 1);
				if (tmpStartX <= 4) {
					SrcStart_X = gMenuPlayInfo.PanSrcStartX;
				} else if (gMenuPlayInfo.PanSrcStartX > tmpStartX) {
					SrcStart_X = gMenuPlayInfo.PanSrcStartX - tmpStartX;
					if ((SrcStart_X + SrcWidth) > OriSrcWidth) {
						SrcStart_X = gMenuPlayInfo.PanMaxX;
					}
				} else {
					SrcStart_X = 0;
				}

				if (gMenuPlayInfo.PanSrcStartY > tmpStartY) {
					SrcStart_Y = gMenuPlayInfo.PanSrcStartY - tmpStartY;
					if ((SrcStart_Y + SrcHeight) > OriSrcHeight) {
						SrcStart_Y = OriSrcHeight - SrcHeight;
					}
				} else {
					SrcStart_Y = 0;
				}
			}

			if (SrcStart_X + SrcWidth > OriSrcWidth) {
				SrcStart_X = OriSrcWidth - SrcWidth;
			}

			if (SrcStart_Y + SrcHeight > OriSrcHeight) {
				SrcStart_Y = OriSrcHeight - SrcHeight;
			}
		} else {
			SrcStart_X = (OriSrcWidth - SrcWidth)  >> 1;
			SrcStart_Y = (OriSrcHeight - SrcHeight) >> 1;
		}

		// Use FW-Scale for images that width < 80, so we don't do alignment for it
		if (SrcWidth > 80) {
			SrcStart_X = (SrcStart_X + 3) & 0xFFFFFFF8;// IME->input\output addr must be word-align
		}
		if (gMenuPlayInfo.pJPGInfo->fileformat == PB_JPG_FMT_YUV420) {
			SrcStart_Y &= 0xFFFFFFFE;
		}
		PB_Scale2Display(SrcWidth, SrcHeight, SrcStart_X, SrcStart_Y,
						 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
	} else { // if(SrcHeight > SrcWidth)
		if (StartZoomLevel == PB_ZOOMSCALE_UINT) { // 1X
			SrcStart_X = 0;
			SrcStart_Y = 0;
			PB_Scale2Display(SrcWidth, SrcHeight, SrcStart_X, SrcStart_Y,
							 gMenuPlayInfo.pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
			// record PanMaxX & PanMaxY
			gMenuPlayInfo.PanMaxX = 0;
			gMenuPlayInfo.PanMaxY = 0;
		} else {
			// another scale
#if PB_ZOOMROTATED_WIDTH_LIMIT
			UINT32 uiWidthLimit;

			uiWidthLimit = usPBDisplayHeight * 3 / 4 * usPBDisplayWidth / usPBDisplayHeight * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;
#endif
			//OriDstWidth = usPBDisplayHeight * usPBDisplayWidth/usPBDisplayHeight * g_PBDispInfo.uiARatioH/g_PBDispInfo.uiARatioW;
			OriDstWidth = usPBDisplayWidth * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;
			DstHeight = usPBDisplayHeight;
			DstWidth  = OriSrcWidth * DstHeight / OriSrcHeight * usPBDisplayWidth / usPBDisplayHeight * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;

			if (gMenuPlayInfo.uipZoomLevelTbl == NULL) {
				DstWidth = DstWidth * StartZoomLevel / PB_ZOOMSCALE_UINT;
			} else {
				DstWidth = DstWidth * gMenuPlayInfo.uipZoomLevelTbl[ZoomIndex] / gMenuPlayInfo.uipZoomLevelTbl[0];
			}

#if (PB_ZOOMROTATED_WIDTH_LIMIT == DISABLE)
			if (DstWidth > usPBDisplayWidth) {
				DstWidth = usPBDisplayWidth;
				SrcWidth = SrcWidth * DstWidth / OriDstWidth;
				SrcHeight = SrcWidth * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;
				SrcHeight &= 0xfffffff8;
			} else
#endif
			{
				SrcWidth = OriSrcWidth;
				DstWidth = usPBDisplayWidth;
				SrcHeight = SrcWidth * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;
				SrcHeight &= 0xfffffff8;
//#NT#2016/05/30#Scottie -begin
//#NT#Remove for Coverity: 61074 (Unused value)
#if 0
				if (ZoomIN_OUT == PLAYZOOM_IN) {
					LastSrcWidth = OriSrcWidth;
				}
#endif
//#NT#2016/05/30#Scottie -end
			}

			if (SrcWidth > OriSrcWidth) {
				SrcHeight = (SrcHeight * OriSrcWidth) / SrcWidth;
				DstWidth = (DstWidth * OriSrcWidth) / SrcWidth * DstWidth / DstWidth;
				SrcWidth  = OriSrcWidth;
			}

#if (PB_ZOOMROTATED_WIDTH_LIMIT == DISABLE)
			if (DstWidth >= usPBDisplayWidth) {
				if (SrcHeight > SrcWidth * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW) {
					// SrcHeight = SrcWidth * usPBDisplayHeight/usPBDisplayWidth * usPBDisplayWidth/usPBDisplayHeight * g_PBDispInfo.uiARatioH/g_PBDispInfo.uiARatioW;
					SrcHeight = SrcWidth * g_PBDispInfo.uiARatioH / g_PBDispInfo.uiARatioW;
				}
			}
#endif

			// record PanMaxX & PanMaxY
			gMenuPlayInfo.PanMaxX = (OriSrcWidth - SrcWidth) & 0xFFFFFFF8;
			gMenuPlayInfo.PanMaxY = OriSrcHeight - SrcHeight;

			if (((ZoomIndex != 1) && (ZoomIN_OUT == PLAYZOOM_IN)) || (ZoomIN_OUT == PLAYZOOM_OUT)) {
#if PB_ZOOMROTATED_WIDTH_LIMIT
				if (uiWidthLimit < DstWidth) {
					SrcWidth = SrcWidth * uiWidthLimit / DstWidth;
				}
#endif

				LastSrcWidth  = gMenuPlayInfo.PanSrcWidth;
				LastSrcHeight = gMenuPlayInfo.PanSrcHeight;
				if (ZoomIN_OUT == PLAYZOOM_IN) {
					SrcStart_X = gMenuPlayInfo.PanSrcStartX + ((LastSrcWidth - SrcWidth) >> 1);
					SrcStart_Y = gMenuPlayInfo.PanSrcStartY + ((LastSrcHeight - SrcHeight) >> 1);
				} else {
					UINT32 tmpStartX, tmpStartY;

					tmpStartX = ((SrcWidth - LastSrcWidth) >> 1);
					tmpStartY = ((SrcHeight - LastSrcHeight) >> 1);
					if (tmpStartX <= 4) {
						SrcStart_X = gMenuPlayInfo.PanSrcStartX;
					} else if (gMenuPlayInfo.PanSrcStartX > tmpStartX) {
						SrcStart_X = gMenuPlayInfo.PanSrcStartX - tmpStartX;
						if ((SrcStart_X + SrcWidth) > OriSrcWidth) {
							SrcStart_X = gMenuPlayInfo.PanMaxX;
						}
					} else {
						SrcStart_X = 0;
					}

					if (gMenuPlayInfo.PanSrcStartY > tmpStartY) {
						SrcStart_Y = gMenuPlayInfo.PanSrcStartY - tmpStartY;
						if ((SrcStart_Y + SrcHeight) > OriSrcHeight) {
							SrcStart_Y = OriSrcHeight - SrcHeight;
						}
					} else {
						SrcStart_Y = 0;
					}
				}

				if (SrcStart_X + SrcWidth > OriSrcWidth) {
					SrcStart_X = OriSrcWidth - SrcWidth;
				}

				if (SrcHeight > SrcWidth * usPBDisplayHeight / usPBDisplayWidth) {
					if (SrcStart_Y + SrcWidth * usPBDisplayHeight / usPBDisplayWidth > OriSrcHeight) {
						SrcStart_Y = OriSrcHeight - SrcWidth * usPBDisplayHeight / usPBDisplayWidth;
					}
				} else {
					if (SrcStart_Y + SrcHeight > OriSrcHeight) {
						SrcStart_Y = OriSrcHeight - SrcHeight;
					}
				}
			} else {
#if PB_ZOOMROTATED_WIDTH_LIMIT
				if (uiWidthLimit < DstWidth) {
					SrcWidth = SrcWidth * uiWidthLimit / DstWidth;
				}
#endif
				SrcStart_X = (OriSrcWidth - SrcWidth)  >> 1;
				SrcStart_Y = (OriSrcHeight - SrcHeight)  >> 1;
			}

			// Use FW-Scale for images that width < 80, so we don't do alignment for it
			if (SrcWidth > 80) {
				SrcWidth = (SrcWidth + 7) & 0xFFFFFFF0;     // IME Input Stripe Width must be 16B-align
				SrcStart_X = (SrcStart_X + 3) & 0xFFFFFFF8; // IME->input\output addr must be word-align
			}
			DstWidth &= 0xFFFFFFF8;

			if (gMenuPlayInfo.pJPGInfo->fileformat == PB_JPG_FMT_YUV420) {
				SrcStart_Y &= 0xFFFFFFFE;
			}
			gSrcRegion.w = SrcWidth;
			gSrcRegion.h = SrcHeight;
			gSrcRegion.x = SrcStart_X;
			gSrcRegion.y = SrcStart_Y;
			gDstRegion.w = DstWidth;
			gDstRegion.h = DstHeight;
			gPbPushSrcIdx = PB_DISP_SRC_DEC;
			PB_DispSrv_SetDrawCb(PB_VIEW_STATE_SPEZOOM);
			PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW);
			PB_DispSrv_Trigger();
		}
	}// end of else if(SrcHeight > SrcWidth)

	// record PanSrcStartX & PanSrcStartY
	gMenuPlayInfo.PanSrcStartX = SrcStart_X;
	gMenuPlayInfo.PanSrcStartY = SrcStart_Y;
	gMenuPlayInfo.PanSrcWidth = SrcWidth;
	gMenuPlayInfo.PanSrcHeight = SrcHeight;
	gMenuPlayInfo.PanDstWidth  = DstWidth;
	gMenuPlayInfo.PanDstHeight = DstHeight;


	if (PB_FLUSH_SCREEN == gScreenControlType) {
		PB_DispSrv_SetDrawAct(PB_DISPSRV_FLIP);
		PB_DispSrv_Trigger();
	} else {
		gScreenControlType = PB_FLUSH_SCREEN;
	}
	gMenuPlayInfo.PanDstStartX = gMenuPlayInfo.PanSrcStartX;
	gMenuPlayInfo.PanDstStartY = gMenuPlayInfo.PanSrcStartY;

	gucZoomUpdateNewFB = TRUE;  // for rotate
#endif //_TODO	
}

