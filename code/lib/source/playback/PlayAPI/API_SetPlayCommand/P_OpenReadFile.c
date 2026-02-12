#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

void PB_OpenSpecFile(UINT32 PlayDirId, UINT32 PlayFileId)
{
	gPBCmdObjPlaySingle.DirId   = PlayDirId;
	gPBCmdObjPlaySingle.FileId  = PlayFileId;
	PB_PlaySetCmmd(PB_CMMD_OPEN_SPECFILE);
}

void PB_OpenSpecFileBySeq(UINT32 uiSeqID, BOOL bOnlyQuery)
{
	UINT32  DirID, FileID;
	ER      ret;

	PPBX_FLIST_OBJ        pFlist = g_PBSetting.pFileListObj;

	ret = pFlist->SeekIndex(uiSeqID, PBX_FLIST_SEEK_SET);
	if (ret == E_OK) {
		pFlist->GetInfo(PBX_FLIST_GETINFO_DIRID, &DirID, NULL);
		pFlist->GetInfo(PBX_FLIST_GETINFO_FILEID, &FileID, NULL);

		gPBCmdObjPlaySingle.DirId  = DirID;
		gPBCmdObjPlaySingle.FileId = FileID;
		if (bOnlyQuery) {
			gMenuPlayInfo.DispFileName[gMenuPlayInfo.CurFileIndex - 1] = FileID;
			gMenuPlayInfo.DispDirName[gMenuPlayInfo.CurFileIndex - 1]  = DirID;
			gMenuPlayInfo.CurIndexOfTotal = uiSeqID;
			gMenuPlayInfo.DispFileSeq[gMenuPlayInfo.CurFileIndex - 1]  = uiSeqID;
			PB_SetFinishFlag(DECODE_JPG_DONE);
		} else {
			PB_PlaySetCmmd(PB_CMMD_OPEN_SPECFILE);
		}
	} else {
		DBG_ERR("Open Specific file Error!\r\n");
		PB_SetFinishFlag(DECODE_JPG_READERROR);
	}
}

