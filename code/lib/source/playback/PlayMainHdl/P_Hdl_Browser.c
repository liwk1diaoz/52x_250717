#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

//----- Local functions define --------------------------------------------
static void PB_PlayBrowserView(void);
//-------------------------------------------------------------------------
/*
    Browser view handler.
    This is internal API.

    @return void
*/
void PB_BrowserHandle(void)
{
	UINT32 CommandID, BrowserCommand;

	BrowserCommand = gPBCmdObjPlayBrowser.BrowserCommand;
	gMenuPlayInfo.JumpOffset = gPBCmdObjPlayBrowser.JumpOffset;
	gMenuPlayInfo.CurrentMode = PLAYMODE_THUMB;

	// Need to set the follwing flag to "0" when entering thumbnail mode.
	gMenuPlayInfo.DecodePrimaryOK = 0;

	// Read next file
	if (BrowserCommand & PB_BROWSER_CURR) {
		CommandID = PB_FILE_READ_SPECIFIC;
	} else if (BrowserCommand & PB_BROWSER_NEXT) {
		CommandID = PB_FILE_READ_NEXT;
	} else { // if(BrowserCommand & PB_BROWSER_PREV)
		CommandID = PB_FILE_READ_PREV;
	}

	PB_GetDefaultFileBufAddr();
	PB_ReadFileByFileSys(CommandID, (INT8 *)guiPlayFileBuf, FST_READ_THUMB_BUF_SIZE, 0, gMenuPlayInfo.JumpOffset);
	PB_PlayBrowserView();
	PB_SetFinishFlag(DECODE_JPG_DONE);
}

#if 0
#pragma mark -
#endif

/*
    Do browser view.
    This is internal API.

    @return   void
*/
void PB_PlayBrowserView(void)
{
	UINT32  i, CurIndex, Thumb1stDirFileSequ, jump_offset, read_loop;
	UINT32  FileSequInFolder, FileNumsInFolder, ThumbFileNums;
	INT32   Error_check, ReadSts = 0;
	FLGPTN  Flag;
	//#NT#2012/06/22#Lincy Lin -begin
	//#NT#Add TV plug in flow
	UINT32  OriCurIndexOfTotal, OriFileSequInFolder, OriCurFileIndex;
	//#NT#2012/06/22#Lincy Lin -end
	BOOL    isReadOnly, isWithMemo;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;
	UINT32 tmpFileSize;

	if (g_PBSetting.EnableFlags & PB_ENABLE_THUMB_WITH_DIFF_FOLDER) {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS_INDIR, &FileNumsInFolder, NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ_INDIR, &FileSequInFolder, NULL);
	} else {
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILENUMS, &FileNumsInFolder, NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &FileSequInFolder, NULL);
	}

	if (g_PBSetting.EnableFlags & PB_ENABLE_SHOW_BACKGROUND_IN_THUMB) {
		#if 0
		//#NT#2011/01/31#Ben Wang -begin
		//#NT#Add screen control to sync display with OSD
		if (PB_FLUSH_SCREEN == gScreenControlType) {
			PB_FrameBufSwitch(PLAYSYS_COPY2TMPBUF);
			PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
			PB_FrameBufSwitch(PLAYSYS_COPY2FBBUF);
		} else {
			gScreenControlType = PB_FLUSH_SCREEN;
		}
		//#NT#2011/01/31#Ben Wang -end
		#else
		gScreenControlType = PB_FLUSH_SCREEN;
		#endif

	}

	PB_InitThumbOffset();

//#NT#2009/11/30#Ben Wang -begin
//#Re-open this for UI-tools List control application
	for (i = 0; i < PLAY_MAX_THUMB_NUM; i++) {
		gMenuPlayInfo.AllThumbFormat[i] = PBFMT_UNKNOWN;
	}
//#NT#2009/11/30#Ben Wang -end

	ThumbFileNums = gPBCmdObjPlayBrowser.BrowserNums;

	CurIndex = (FileSequInFolder % ThumbFileNums);
	if (CurIndex == 0) {
		Thumb1stDirFileSequ = (FileSequInFolder - ThumbFileNums);
		CurIndex = ThumbFileNums;
	} else {
		Thumb1stDirFileSequ = (FileSequInFolder / ThumbFileNums) * ThumbFileNums;
	}

	Thumb1stDirFileSequ += 1;
	jump_offset = FileSequInFolder - Thumb1stDirFileSequ;

	if ((Thumb1stDirFileSequ + (ThumbFileNums - 1)) > FileNumsInFolder) {
		read_loop = (FileNumsInFolder - Thumb1stDirFileSequ) + 1;
	} else {
		read_loop = (ThumbFileNums);
	}

	gMenuPlayInfo.DispThumbNums = read_loop;    // decode how many thumbs

	//#NT#2012/06/22#Lincy Lin -begin
	//#NT#Add TV plug in flow
	OriCurIndexOfTotal = gMenuPlayInfo.CurIndexOfTotal;
	OriCurFileIndex = gMenuPlayInfo.CurFileIndex;
	OriFileSequInFolder = FileSequInFolder;
	//#NT#2012/06/22#Lincy Lin -end

	//#NT#2012/05/17#Lincy Lin -begin
	//#NT#Porting disply Srv
	PB_DispSrv_SetDrawAct(PB_DISPSRV_DRAW);
	PB_DispSrv_Trigger();
	//#NT#2012/05/17#Lincy Lin -end
	set_flg(FLG_ID_PB_DISP, FLGPB_IMG_0);

	for (i = 0; i < read_loop; i++) {
		//#NT#2012/06/22#Lincy Lin -begin
		//#NT#Add TV plug in flow
		if (guiIsSuspendPB) {
			gMenuPlayInfo.CurIndexOfTotal = OriCurIndexOfTotal;
			gMenuPlayInfo.CurFileIndex = OriCurFileIndex;
			pFlist->SeekIndex(OriFileSequInFolder, PBX_FLIST_SEEK_SET);
			return;
		}
		//#NT#2012/06/22#Lincy Lin -end
		//DBG_DUMP("Thumbi=%d\r\n",i);

		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
		UINT32 uiLockIdx = (i & 0x1);
		FLGPTN setptn = (FLGPB_IMG_0 << uiLockIdx);

		//prepare your source into m_ImgSrc
		wai_flg(&Flag, FLG_ID_PB_DISP, setptn, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_PB_DISP, FLGPB_IMG_0|FLGPB_IMG_1);
		//#NT#2012/05/17#Lincy Lin -end
		PB_GetDefaultFileBufAddr();

		//PB_GetParam(PBPRMID_MAX_FILE_SIZE,  &tmpFileSize);
		tmpFileSize=FST_READ_THUMB_BUF_SIZE;

		if (i == 0) {
			// Read previous N file
			ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_PREV, (INT8 *)guiPlayFileBuf, tmpFileSize, 0, jump_offset);
		} else {
			// Read next file
			ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_NEXT, (INT8 *)guiPlayFileBuf, tmpFileSize, 0, 1);
		}

		pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &gMenuPlayInfo.DispFileName[i], NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &gMenuPlayInfo.DispDirName[i], NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.DispFileSeq[i], NULL);

		Error_check = E_SYS;

		// Update File info
		gMenuPlayInfo.CurFileIndex = i + 1;
		gMenuPlayInfo.CurFileFormat = PB_ParseFileFormat();

		pFlist->GetInfo(PBX_FLIST_GETINFO_READONLY, (UINT32 *)&isReadOnly, NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_ISWITHMEMO, (UINT32 *)&isWithMemo, NULL);
		if (isReadOnly == TRUE) {
			gMenuPlayInfo.CurFileFormat |= PBFMT_READONLY;
		}
		if (isWithMemo == TRUE) {
			gMenuPlayInfo.CurFileFormat |= PBFMT_JPGMEMO;
		}
		gMenuPlayInfo.AllThumbFormat[i] = gMenuPlayInfo.CurFileFormat;

		//if(PB_GetFileSysStatus() >= 0)
		if (ReadSts >= 0) {
			// decode & scale
			if (gPBCmdObjPlayBrowser.BrowserCommand & PB_BROWSER_THUMB) {
				Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
				//Error_check = PB_DecodeWhichFile(PB_DEC_PRIMARY, gMenuPlayInfo.CurFileFormat);
			} else {
				Error_check = PB_DecodeWhichFile(PB_DEC_THUMBNAIL, gMenuPlayInfo.CurFileFormat);
				//Error_check = PB_DecodeWhichFile(PB_DEC_PRIMARY, gMenuPlayInfo.CurFileFormat);
			}

			ThumbSrcW[i] = g_pPbHdInfo->hd_out_video_frame.dim.w;
			ThumbSrcH[i] = g_pPbHdInfo->hd_out_video_frame.dim.h;

			#if _TODO
			if ((Error_check > 0) && (Error_check != DECODE_JPG_WAVEOK)) {
				PB_Thumb2Display(gMenuPlayInfo.pJPGInfo->imagewidth, gMenuPlayInfo.pJPGInfo->imageheight, i);
			}
			#endif
		}
		if (gMenuPlayInfo.CurFileFormat & (PBFMT_AVI | PBFMT_MOVMJPG | PBFMT_MP4 | PBFMT_TS)) {
			gMenuPlayInfo.MovieLength[i] = g_PBVideoInfo.uiToltalSecs;
		} else {
			gMenuPlayInfo.MovieLength[i] = 0;
		}
		if (Error_check < 0) {
			if (g_PBSetting.EnableFlags & PB_ENABLE_SHOW_BG_IN_BADFILE) {
				//
				//[TBD] will modify the drawing backgroud image mechanism
				//
			}
			gMenuPlayInfo.bThumbDecErr[i] = TRUE;

		} else {
			gMenuPlayInfo.bThumbDecErr[i] = FALSE;
		}
		//#NT#2012/05/17#Lincy Lin -begin
		//#NT#Porting disply Srv
		set_flg(FLG_ID_PLAYBACK, setptn);
		PB_DispSrv_Trigger();
		//#NT#2012/05/17#Lincy Lin -end
        //clr_flg(FLG_ID_PB_DISP, 0xFFFFFFFF);
	}
	//#NT#2012/06/06#Lincy Lin -begin
	//#NT#Wait draw end
	wai_flg(&Flag, FLG_PB, FLGPB_PULL_DRAWEND, TWF_ORW | TWF_CLR);
	//#NT#2012/06/06#Lincy Lin -end

	gMenuPlayInfo.CurFileIndex = CurIndex;// current index

	if (g_PBSetting.ThumbShowMethod == PB_SHOW_THUMBNAIL_IN_THE_SAME_TIME) {
		//#NT#2010/11/12#Ben Wang -begin
		//#NT#prevent screen from flash
		//#NT#2011/01/31#Ben Wang -begin
		//#NT#Add screen control to sync display with OSD
		if (PB_FLUSH_SCREEN == gScreenControlType) {
			//#NT#2012/05/17#Lincy Lin -begin
			//#NT#Porting disply Srv
			//PB_DispSrv_SetDrawAct(PB_DISPSRV_FLIP);
			//PB_DispSrv_Trigger();
			//#NT#2012/05/17#Lincy Lin -end
		} else {
			gScreenControlType = PB_FLUSH_SCREEN;
		}
		//#NT#2011/01/31#Ben Wang -end
		//#NT#2010/11/12#Ben Wang -end
	} else {
		// Move image from temp buffer to display buffer with effect
		PB_MoveImageTo(PB_JPG_FMT_YUV211);
	}
}

