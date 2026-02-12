#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

void PB_SeekCurrent(void)
{
	// JIRA: IVOT_N12020_CO-382
	UINT32 NewIndex;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	NewIndex = gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1]; // get file index of current thumbnail
	//DBG_DUMP("[PB_SeekCurrent] CurFileIndex %d, NewIndex %d\r\n", gMenuPlayInfo.CurFileIndex, NewIndex);
	pFlist->SeekIndex(NewIndex, PBX_FLIST_SEEK_SET); // seek file by current file index
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.DispFileSeq[0], NULL); // set current file index to gMenuPlayInfo.DispFileSeq[0]
	gMenuPlayInfo.CurIndexOfTotal = NewIndex; // Restore correct gMenuPlayInfo.CurIndexOfTotal after leaving Browser mode.
}

//----- Local functions --------------------------------------------------------
/*
    Single view - Primary image handler
    This is internal API.

    @return void
*/
static void xPB_SinglePrimaryHandle(void)
{
	UINT64 tmpFileSize;
	INT32  Error_check;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;

	if (TRUE == gMenuPlayInfo.DecodePrimaryOK) {
		DBG_IND("Decode Pri-File Before !\r\n");
		PB_SetFinishFlag(DECODE_JPG_PRIMARY);
		return;
	}
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);

	Error_check = PB_DecodePrimaryHandler((UINT32)tmpFileSize, gMenuPlayInfo.CurFileFormat);
	if (Error_check > 0) {
		gMenuPlayInfo.DecodePrimaryOK = TRUE;
		DBG_IND("Decode Pri-File OK !\r\n");
		//#NT#2011/01/31#Ben Wang -begin
		//#NT#Add screen control to sync display with OSD
		if (PB_FLUSH_SCREEN == gScreenControlType) {
			//#NT#2012/05/17#Lincy Lin -begin
			//#NT#Porting disply Srv
#if 1
			PB_DispSrv_SetDrawAct(PB_DISPSRV_FLIP);
			PB_DispSrv_Trigger();
#else
			PB_FrameBufSwitch(PLAYSYS_COPY2TMPBUF);
			PB_CopyImage2Display(PLAYSYS_COPY2TMPBUF, PLAYSYS_COPY2FBBUF);
			PB_FrameBufSwitch(PLAYSYS_COPY2FBBUF);
#endif
			//#NT#2012/05/17#Lincy Lin -end
		} else {
			gScreenControlType = PB_FLUSH_SCREEN;
		}
		//#NT#2011/01/31#Ben Wang -end

		DBG_IND("Decode Pri-File OK !\r\n");
		PB_SetFinishFlag(DECODE_JPG_PRIMARY);
	}
	DBG_ERR("Decode Pri-File Error !\r\n");
	PB_SetFinishFlag(DECODE_JPG_DECODEERROR);
}
//------------------------------------------------------------------------------

/*
    Single view handler
    This is internal API.

    @return void
*/
void PB_SingleHandle(void)
{
	UINT32 PlayCommand, CommandID;
	UINT64 tmpFileSize;
	INT32  Error_check, ReadFileSts;
	PPBX_FLIST_OBJ pFlist = g_PBSetting.pFileListObj;
	BOOL           isReadOnly, isWithMemo;

	PlayCommand = gPBCmdObjPlaySingle.PlayCommand;
	if (PlayCommand & PB_SINGLE_CURR) {
		CommandID = PB_FILE_READ_SPECIFIC;
	} else if (PlayCommand & PB_SINGLE_NEXT) {
		CommandID = PB_FILE_READ_NEXT;
	} else if (PlayCommand & PB_SINGLE_PREV) {
		CommandID = PB_FILE_READ_PREV;
	} else if (PlayCommand == PB_SINGLE_PRIMARY) {
		// just decode primary
		xPB_SinglePrimaryHandle();
		return;
	} else { // if(PlayCommand & PB_SINGLE_SLIDE_START)
		CommandID = PB_FILE_READ_NEXT;
	}

	if (PB_SearchValidFile(CommandID, &ReadFileSts) != E_OK) {
		DBG_WRN("Search no valid file\r\n");
		PB_SetFinishFlag(DECODE_JPG_NOIMAGE);
		return;
	}
	gMenuPlayInfo.DecodePrimaryOK = 0;
	gMenuPlayInfo.CurFileIndex    = 1;    // reset CurFileIndex must be after PB_SearchValidFile
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &gMenuPlayInfo.DispFileName[0], NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &gMenuPlayInfo.DispDirName[0], NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &gMenuPlayInfo.DispFileSeq[0], NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &tmpFileSize, NULL);

	// check file attribute
	pFlist->GetInfo(PBX_FLIST_GETINFO_READONLY, (UINT32 *)&isReadOnly, NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_ISWITHMEMO, (UINT32 *)&isWithMemo, NULL);
	if (isReadOnly == TRUE) {
		gMenuPlayInfo.CurFileFormat |= PBFMT_READONLY;
	}
	if (isWithMemo == TRUE) {
		gMenuPlayInfo.CurFileFormat |= PBFMT_JPGMEMO;
	}
	gMenuPlayInfo.AllThumbFormat[0] = gMenuPlayInfo.CurFileFormat;


	//if(PB_GetFileSysStatus() < 0)     // NOT OK
	if (ReadFileSts < 0) {
		DBG_ERR("Read File Error !!\r\n");
		PB_SetFinishFlag(DECODE_JPG_READERROR);
		//Show bad file warning image for read-error images
		gMenuPlayInfo.CurrentMode = PLAYMODE_PRIMARY;
		if (g_PBSetting.EnableFlags & PB_ENABLE_SHOW_BG_IN_BADFILE) {
			// Show bad file warning image
			//
			//[TBD] will modify the drawing backgroud image mechanism
			//
		} else {
			// Clear Frame buffer when DECODE_JPG_DECODEERROR
			xPB_DirectClearFrameBuffer(g_uiDefaultBGColor);
		}
		return;
	}
	DBG_IND("Read File dir=%3d name=%4d OK !!\r\n", gMenuPlayInfo.DispDirName[0], gMenuPlayInfo.DispFileName[0]);

	//--------------------------------------------------------
	// JPG
	//--------------------------------------------------------
	if (gMenuPlayInfo.CurFileFormat & PBFMT_JPG) {
		DBG_IND("Decode JPG-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_PRIMARY;

//#NT#2012/08/22#Scottie -begin
//#NT#Modify PB single view mechanism (Here should be parsing EXIF but not decoding thumbnail)
#if 0
		// decode thumbnail
		if (PlayCommand & PB_SINGLE_THUMB) {
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
		} else {
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMBNAIL, gMenuPlayInfo.CurFileFormat);
		}
#else
#if _TODO
		if (PlayCommand & PB_SINGLE_THUMB) {
			// decode thumbnail
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
		} else {
			// parse EXIF info only
			pJPGInfo->inbuf = (UINT8 *)guiPlayFileBuf;
			Error_check = Jpg_ParseHeader(pJPGInfo, DEC_THUMBNAIL);
			if (E_OK != Error_check) {
				DBG_ERR("ParseHeader NG!!\r\n");
			}
			xPB_UpdateEXIFOrientation(Error_check);
		}
#endif
#endif
//#NT#2012/08/22#Scottie -end
        #if _TODO
		if (Error_check != JPG_HEADER_ER_OK) {
			// Not Exif file
			g_bExifExist = FALSE;
			//#NT#2013/06/10#Ben Wang -begin
			//#NT#Add the mechanism that do NOT try to decode primary image if the JPEG-parsing result is "SOI marker not found".
			if (Error_check == JPG_HEADER_ER_SOI_NF) {
				goto ERROR_HANDLE;
			}
			//#NT#2013/06/10#Ben Wang -end
		} else {
			g_bExifExist = TRUE;
		}
		#endif

		// decode hidden-thumb or primary-image
		#if _TODO
		if ((PlayCommand & PB_SINGLE_PRIMARY) && (Error_check != DECODE_JPG_PRIMARY)) {
			MAKERNOTE_INFO MakernoteInfo;
			EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_MAKERNOTE, &MakernoteInfo, sizeof(MakernoteInfo));
			if (g_bExifExist && (MakernoteInfo.uiScreennailOffset != 0) && !(PlayCommand & PB_SINGLE_NO_HIDDEN)) {
				Error_check = PB_DecodeWhichFile(PB_DEC_HIDDEN, gMenuPlayInfo.CurFileFormat);
			} else {
				Error_check = PB_DecodePrimaryHandler((UINT32)tmpFileSize, gMenuPlayInfo.CurFileFormat);
			}
		}
		#else
		Error_check = PB_DecodePrimaryHandler((UINT32)tmpFileSize, gMenuPlayInfo.CurFileFormat);
		#endif

		if (Error_check == DECODE_JPG_PRIMARY) {
			gMenuPlayInfo.DecodePrimaryOK = TRUE;
			DBG_MSG("Decode Primary..\r\n");
		}

//#NT#2012/08/06#Scottie -begin
//#NT#Refine code for auto-rotated image for DisplaySrv
		if (Error_check > 0) {
			if ((g_bExifExist && (g_uiExifOrientation != JPEG_EXIF_ORI_DEFAULT)) ||
				(g_PBSetting.DispDirection == PB_DISPDIR_VERTICAL)) {
				if (PB_EXIFOrientationHandle()) {
					gMenuPlayInfo.AllThumbFormat[0] = gMenuPlayInfo.CurFileFormat;
					if (Error_check == DECODE_JPG_THUMBOK) {   // only decode thumb
						EXIF_GETTAG exifTag;
						BOOL bIsLittleEndian;

						EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
						exifTag.uiTagIfd = EXIF_IFD_EXIF;
						exifTag.uiTagId = TAG_ID_PIXEL_X_DIMENSION;
						if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
							//pJPGInfo->ori_imagewidth  = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
						}
						exifTag.uiTagId = TAG_ID_PIXEL_Y_DIMENSION;
						if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
							//pJPGInfo->ori_imageheight  = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
						}
					} else { //if(Error_check == DECODE_JPG_PRIMARY)
						gMenuPlayInfo.DecodePrimaryOK = TRUE;
					}

					DBG_IND("Decode File OK !\r\n");
					PB_MoveImageTo(PB_JPG_FMT_YUV211);
					PB_SetFinishFlag(DECODE_JPG_DONE);
					return;
				}
			} else {
				gMenuPlayInfo.RotateDir = PLAY_ROTATE_DIR_UNKNOWN;
			}
		}
//#NT#2012/08/06#Scottie -end
	}
	//--------------------------------------------------------
	// AVI
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & PBFMT_AVI) {
		// decode AVI
		DBG_IND("Decode AVI-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_AVI;
		//#NT#2012/08/28#Ben Wang -begin
		//#NT#Use DEC_PRIMARY to decode the first frame of video and DEC_THUMB_ONLY for thumbnail
		if (PlayCommand & PB_SINGLE_THUMB) {
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
		} else {
			Error_check = PB_DecodeWhichFile(PB_DEC_PRIMARY, gMenuPlayInfo.CurFileFormat);
		}
		//#NT#2012/08/28#Ben Wang -end
	}
	//--------------------------------------------------------
	// MPG1
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & PBFMT_MPG) { // Brad /04/11
		// decode MPG1
		DBG_IND("Decode MPG-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_MPG;
		Error_check = PB_DecodeWhichFile(PB_DEC_THUMBNAIL, gMenuPlayInfo.CurFileFormat);

	}
	//--------------------------------------------------------
	// MovMjpg
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & (PBFMT_MOVMJPG | PBFMT_MP4)) {
		// decode AVI
		DBG_IND("Decode MovMjpg-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_MOVMJPG;
		//#NT#2012/08/28#Ben Wang -begin
		//#NT#Use DEC_PRIMARY to decode the first frame of video and DEC_THUMB_ONLY for thumbnail
		if (PlayCommand & PB_SINGLE_THUMB) {
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
		} else {
			Error_check = PB_DecodeWhichFile(PB_DEC_PRIMARY, gMenuPlayInfo.CurFileFormat);
		}
		//#NT#2012/08/28#Ben Wang -end
	}
	//--------------------------------------------------------
	// TS
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & PBFMT_TS) {
		// decode TS
		DBG_IND("Decode TS-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_MOVMJPG;
		if (PlayCommand & PB_SINGLE_THUMB) {
			Error_check = PB_DecodeWhichFile(PB_DEC_THUMB_ONLY, gMenuPlayInfo.CurFileFormat);
		} else {
			Error_check = PB_DecodeWhichFile(PB_DEC_PRIMARY, gMenuPlayInfo.CurFileFormat);
		}
	}
	//--------------------------------------------------------
	// ASF
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & PBFMT_ASF) {
		// decode ASF
		DBG_IND("Decode ASF-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_ASF;
		Error_check = PB_DecodeWhichFile(PB_DEC_THUMBNAIL, gMenuPlayInfo.CurFileFormat);
		if (Error_check > 0) {
			Error_check = DECODE_JPG_PRIMARY;
			g_bExifExist = FALSE;
			//pJPGInfo->ori_imagewidth  = pJPGInfo->imagewidth;
			//pJPGInfo->ori_imageheight = pJPGInfo->imageheight;
		}
	}
	//--------------------------------------------------------
	// WAV
	//--------------------------------------------------------
	else if (gMenuPlayInfo.CurFileFormat & PBFMT_WAV) {
		//#NT#2009/12/03#Photon Lin  -begin
		//#Modify to correspond AVI play task and playback task usage
		//AVIUti_ParseHeader(guiPlayFileBuf, &g_PBAVIHeaderInfo, FST_READ_THUMB_BUF_SIZE+FST_READ_SOMEAVI_SIZE);
		//#NT#2009/12/03#Photon Lin  -end
		DBG_IND("Decode WAV-File...\r\n");
		gMenuPlayInfo.CurrentMode = PLAYMODE_WAV;
		PB_SetFinishFlag(DECODE_JPG_DONE);
		return;
	} else {
		DBG_ERR("Read Unknown-File Error !!\r\n");
		PB_SetFinishFlag(DECODE_JPG_READERROR);
		return;
	}

	if (Error_check > 0) {
		//PB_SetFinishFlag(DECODE_JPG_DONE);
		//PB_Scale2Display(pJPGInfo->imagewidth, pJPGInfo->imageheight, 0, 0,
		//				 pJPGInfo->fileformat, PLAYSYS_COPY2TMPBUF);
		DBG_IND("Decode File OK !\r\n");
		PB_SetFinishFlag(DECODE_JPG_DONE);
		PB_MoveImageTo(PB_JPG_FMT_YUV211);
		gMenuPlayInfo.AllThumbFormat[0] = gMenuPlayInfo.CurFileFormat;
		if (Error_check == DECODE_JPG_THUMBOK) {   // only decode thumb
			EXIF_GETTAG exifTag;
			BOOL bIsLittleEndian;
			EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
			exifTag.uiTagIfd = EXIF_IFD_EXIF;
			exifTag.uiTagId = TAG_ID_PIXEL_X_DIMENSION;
			if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
				//pJPGInfo->ori_imagewidth  = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
			}
			exifTag.uiTagId = TAG_ID_PIXEL_Y_DIMENSION;
			if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
				//pJPGInfo->ori_imageheight  = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
			}
		} else { //if(Error_check == DECODE_JPG_PRIMARY)
			gMenuPlayInfo.DecodePrimaryOK = TRUE;
		}
	} else { //if( (Error_check != DECODE_JPG_READERROR) && (Error_check != DECODE_JPG_DECODEERROR) )

		// Show bad file warning image
#if _TODO
ERROR_HANDLE:
#endif

#if _TODO
		if (g_PBSetting.EnableFlags & PB_ENABLE_SHOW_BG_IN_BADFILE) {
			//[ToDo] Show bad file warning image..
		} else {
			// Clear Frame buffer when DECODE_JPG_DECODEERROR
//#NT#2012/08/22#Scottie -begin
//#NT#Modify for clearing frame buffer by DisplaySrv
//            PB_FillInData2Display(0x00008080);
			xPB_DirectClearFrameBuffer(g_uiDefaultBGColor);
//#NT#2012/08/22#Scottie -end
		}
#endif
		if (isReadOnly == TRUE) {
			gMenuPlayInfo.AllThumbFormat[0] |= PBFMT_READONLY;
		} else {
			gMenuPlayInfo.AllThumbFormat[0] &= (~PBFMT_READONLY);
		}

		DBG_ERR("Decode File Error !\r\n");
		PB_SetFinishFlag(DECODE_JPG_DECODEERROR);
		PB_DispSrv_Trigger();
		return;
	};
}

#if 0
#pragma mark -
#endif

/*
    Decode primary-image handler
    This is internal API.

    @param[in] tmpFileSize: current file size
    @param[in] CurFormat: current file format
    @return DECODE_JPG_DONE: done; DECODE_JPG_READERROR: fail
*/
INT32 PB_DecodePrimaryHandler(UINT32 tmpFileSize, UINT32 CurFormat)
{
	if (tmpFileSize > guiPlayMaxFileBufSize) {
		DBG_ERR("File is too big to decode!\r\n");
		return DECODE_JPG_BIGFILE;
	}

	// read total file & decode primary image
	if (tmpFileSize > FST_READ_THUMB_BUF_SIZE) {
		INT32 ReadSts;
		// Read continue file
		PB_GetNewFileBufAddr();
		ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_CONTINUE, (INT8 *)guiPlayFileBuf, tmpFileSize, 0, 1);
		//if(PB_GetFileSysStatus() < 0)
		if (ReadSts < 0) {
			DBG_ERR("Read file error!\r\n");
			return DECODE_JPG_READERROR;
		}
	}

	return PB_DecodeWhichFile(PB_DEC_PRIMARY, CurFormat);
}
