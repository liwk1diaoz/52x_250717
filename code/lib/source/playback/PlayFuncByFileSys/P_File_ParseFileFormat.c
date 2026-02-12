#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"

/*
    Do parsing current file format
    This is internal API.

    @return FileFormat (PBFMT_JPG/PBFMT_AVI)
*/
UINT16 PB_ParseFileFormat(void)
{
	UINT32 FileFormat;

	FileFormat = gusPlayThisFileFormat;
	if (FileFormat == PBX_FLIST_FILE_TYPE_WAV) {
		return PBFMT_WAV;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_JPG) {
		return PBFMT_JPG;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_AVI) {
		return PBFMT_AVI;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_MOV) {
		return PBFMT_MOVMJPG;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_RAW) {
		return PBFMT_RAW;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_MP4) {
		return PBFMT_MP4;
	} else if (FileFormat & PBX_FLIST_FILE_TYPE_TS) {
		return PBFMT_TS;
	}

	return PBFMT_UNKNOWN;
}

