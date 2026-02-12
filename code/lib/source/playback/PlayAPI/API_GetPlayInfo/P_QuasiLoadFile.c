#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

//#NT#2012/10/30#Scottie -begin
//#NT#Implement PB_QuasiLoadFile() to check if a drawable file
PB_ERR PB_QuasiLoadFile(PB_FILE_TYPE FileType)
{
	INT32  iErrChk;
	UINT64 uiFileSize;
	UINT32 uiOriSeqID;
	UINT32 FolderID, FileID, FileFmt;
	#if _TODO
	JPG_DEC_INFO DecOneJPGInfo = {0};
	JPGHEAD_DEC_CFG JPGCfg = {0};
	#endif
	PPBX_FLIST_OBJ        pFlist = g_PBSetting.pFileListObj;

	// Save the last Sequence ID
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESEQ, &uiOriSeqID, NULL);

	// Read the current file..
	PB_GetDefaultFileBufAddr();
	PB_ReadFileByFileSys(PB_FILE_READ_SPECIFIC, (INT8 *)guiPlayFileBuf, FST_READ_THUMB_BUF_SIZE, 0, gMenuPlayInfo.JumpOffset);

	pFlist->GetInfo(PBX_FLIST_GETINFO_FILESIZE64, &uiFileSize, NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &FileID, NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &FolderID, NULL);
	pFlist->GetInfo(PBX_FLIST_GETINFO_FILETYPE, &FileFmt, NULL);

	if ((PB_FILE_JPEG == FileType) && (PBX_FLIST_FILE_TYPE_JPG == FileFmt)) {
		if ((UINT32)uiFileSize > FST_READ_THUMB_BUF_SIZE) {
			// read total file & decode primary image
			INT32 ReadSts;

			PB_GetNewFileBufAddr();
			ReadSts = PB_ReadFileByFileSys(PB_FILE_READ_CONTINUE, (INT8 *)guiPlayFileBuf, (UINT32)uiFileSize, 0, gMenuPlayInfo.JumpOffset);
			if (ReadSts < 0) {
				return PBERR_FAIL;
			}
		}
        #if _TODO
		DecOneJPGInfo.p_src_addr = (UINT8 *)guiPlayFileBuf;
		DecOneJPGInfo.jpg_file_size = (UINT32)uiFileSize;
		DecOneJPGInfo.p_dst_addr = (UINT8 *)guiPlayRawBuf;
		DecOneJPGInfo.p_dec_cfg = &JPGCfg;
		DecOneJPGInfo.decode_type = DEC_PRIMARY;
		#endif
		//iErrChk = PB_DecodeOneImg(&DecOneJPGInfo);.
		iErrChk = PB_DecodeOneImg(NULL);
		if (iErrChk != E_OK) {
			return PBERR_FAIL;
		}
	}
	//
	// ToDO: implement other PB_FILE_TYPE..
	//
	else {
		return PBERR_FAIL;
	}

	// Restore to the original file
	PB_OpenSpecFileBySeq(uiOriSeqID, TRUE);
	PB_WaitCommandFinish(PB_WAIT_INFINITE);

	// Update the current image info..
	gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1] = FileID;
	gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1]  = FolderID;

	return PBERR_OK;
}
//#NT#2012/10/30#Scottie -end

