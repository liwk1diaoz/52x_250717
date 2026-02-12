#include "kwrap/stdio.h"
#include "PrjCfg.h"
#include "FileSysTsk.h"
#include "FileDB.h"
#include "PBXFileList/PBXFileList_FileDB.h"
#include "DCF.h"
#include "SysCommon.h"
#include "WifiAppCmd.h"
#include "UIAppWiFiCmdMovie.h"
#include "UIApp/Play/UIAppMoviePlay.h"
#include "UIApp/Photo/UIAppPhoto_Param.h"
#include "exif/Exif.h"
#include "exif/ExifDef.h"
#include "UIApp/AppDisp_PBView.h"
//#include "HwMem.h"
#include "GxVideoFile.h"
#include "WifiAppXML.h"
#include "ProjectInfo.h"
//#include "WiFiIpcAPI.h"
#include "UIAppNetwork.h"
#include "PlaybackTsk.h"
#include "UIWnd/UIFlow.h"
//#include "SysCfg.h"
#include "WifiAppCmdMapping.h"
#include "kflow_common/nvtmpp.h"
#if(WIFI_AP_FUNC==ENABLE)
//#include "nvt_auth.h"
#include "ImageApp/ImageApp_Photo.h"
//#include "ImageApp_Movie.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#if (FWS_FUNC == ENABLE)
#include "FwSrvApi.h"
#endif
#include "GxImageFile.h"
#include "avfile/media_def.h"
//#include "ImageUnit_VdoEnc.h"
#include "BinaryFormat.h"
#include <kwrap/debug.h>
#if defined(__FREERTOS)
#include "crypto.h"
#else
#include <string.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>
#endif
#include "vf_gfx.h"
#include "UIWnd/UIFlow.h"
#include "sys_mempool.h"

#include "avfile/AVFile_ParserTs.h"
#include "GxVideoFile.h"

#define TRANS_JPG_FROM_H264IDR  1
#define CREATE_PHOTO_FILEDB     0
#define THIS_DBGLVL             2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#if 0
#define __MODULE__          WifiAppXML
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
#endif
///////////////////////////////////////////////////////////////////////////////
#define QUERY_XML_PATH  "A:\\query.xml"
#define QUERY_CMD_CUR_STS_XML_PATH  "A:\\query_cmd_cur_sts.xml"
#define CMD_STR "custom=1&cmd="
#define PAR_STR "&par="
#define QUERY_FMT  "{\"mfr\":\"%s\",\"type\":\"%d\",\"model\":\"%s\",\"p2p_uuid\":\"\"}"
/*mfr:�t�ӦW��
type:1-�樮������ 2- IP camera
model:���~����
p2p_uuid:�Τ�p2p�лx,�S���i�H���� ""
*/

#define XML_PATH_LEN                 (128)
#define FAT_GET_DAY_FROM_DATE(x)     (x & 0x1F)              ///<  get day from date
#define FAT_GET_MONTH_FROM_DATE(x)   ((x >> 5) & 0x0F)       ///<  get month from date
#define FAT_GET_YEAR_FROM_DATE(x)    ((x >> 9) & 0x7F)+1980  ///<  get year from date
#define FAT_GET_SEC_FROM_TIME(x)     (x & 0x001F)<<1         ///<  seconds(2 seconds / unit)
#define FAT_GET_MIN_FROM_TIME(x)     (x & 0x07E0)>>5         ///<  Minutes
#define FAT_GET_HOUR_FROM_TIME(x)    (x & 0xF800)>>11        ///<  hours
#define XML_STRCPY(dst, src, dst_size)  do { strncpy(dst, src, dst_size-1); dst[dst_size-1] = '\0'; } while(0)

#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)
#define Perf_Mark()
#define Perf_GetDuration()

void XML_StringResult(UINT32 cmd, char *str, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType);
void XML_ValueResult(UINT32 cmd, UINT64 value, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType);


#define HW_TV_ENABLE                           0x00000001
#define HW_GSENDOR_ENABLE                      0x00000002
#define HW_WIFI_SOCKET_HANDSHAKE_ENABLE        0x00000004
#define HW_WIFI_CLIENT_SOCKET_NOTIFY_ENABLE    0x00000008
#define HW_SET_AUTO_RECORDING_DISABLE 	       0x00000010 //WIFIAPP_CMD_SET_AUTO_RECORDING (2012)


typedef enum {
	WIFI_AUTH_TYPE_WPA_TKIP = 0,
	WIFI_AUTH_TYPE_WPA_AES,
	WIFI_AUTH_TYPE_WPA_AES_TKIP,
	WIFI_AUTH_TYPE_WPA2_TKIP,
	WIFI_AUTH_TYPE_WPA2_AES,
	WIFI_AUTH_TYPE_WPA2_AES_TKIP,
	WIFI_AUTH_TYPE_NONE,
} WIFI_AUTH_TYPE;

typedef struct _XML_THUMB_INFO{
	UINT32          u32WorkBufPa;
	UINT32          u32WorkBufVa; //This buffer need to be released.
	HD_PATH_ID     *p_hd_vdec_path; //Take this form playback library. p_hd_vdec_path[0] is jpg ;  p_hd_vdec_path[1] is h.264 ; p_hd_vdec_path[2] is h.265
	HD_VIDEODEC_BS  hd_in_video_bs; //  I-frame bsstream. This buffer need to be released.
	HD_VIDEO_FRAME  hd_out_video_frame;  // YUV info for I-frmae.  This buffer need to be released.
	HD_VIDEO_FRAME  hd_scale_video_frame; // scale down to VGA size.  This buffer need to be released.
	HD_PATH_ID      hd_venc_path; // keep this encode path and release later.
	UINT32          hd_venc_bs_buf_va;
	UINT32          hd_venc_bs_buf_size;
	HD_BUFINFO      hd_venc_buf_info;
	HD_VIDEOENC_BS  hd_out_video_bs;  //re-encode scaled YUV to JPG
}XML_THUMB_INFO, *PXML_THUMB_INFO;

#if CREATE_PHOTO_FILEDB
static FILEDB_INIT_OBJ gWifiFDBInitObj = {
	"A:\\",  //rootPath
	NULL,  //defaultfolder
	NULL,  //filterfolder
	TRUE,  //bIsRecursive
	TRUE,  //bIsCyclic
	TRUE,  //bIsMoveToLastFile
	TRUE, //bIsSupportLongName
	FALSE, //bIsDCFFileOnly
	TRUE,  //bIsFilterOutSameDCFNumFolder
	{'N', 'V', 'T', 'I', 'M'}, //OurDCFDirName[5]
	{'I', 'M', 'A', 'G'}, //OurDCFFileName[4]
	FALSE,  //bIsFilterOutSameDCFNumFile
	FALSE, //bIsChkHasFile
	60,    //u32MaxFilePathLen
	10000,  //u32MaxFileNum
	(FILEDB_FMT_JPG | FILEDB_FMT_AVI | FILEDB_FMT_MOV | FILEDB_FMT_MP4 | FILEDB_FMT_TS),
	0,     //u32MemAddr
	0,     //u32MemSize
	NULL   //fpChkAbort

};
#endif
static MEM_RANGE XMLTmpPool;
//static NVTMPP_VB_BLK blk_xml =0;
static UINT32                          g_xml_pool_main_phy_adr = 0;
static UINT32                          g_xml_pool_main_va_adr  = 0;
static XML_THUMB_INFO g_xml_thumb_info = {0};
static PXML_THUMB_INFO g_pxml_thumb_info = &g_xml_thumb_info;
INT32 XML_RelPrivateMem(void)
{
#if 0
	NVTMPP_VB_POOL pool;

	if(!blk_xml)
	{
		return NVTMPP_ER_OK;
	}
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK) {
		DBG_WRN("not pb mode\r\n");
	}

    pool = nvtmpp_vb_addr2pool(nvtmpp_vb_block2addr(blk_xml));
	if (pool == NVTMPP_VB_INVALID_POOL){
		DBG_ERR("invalid block 0x%08X\r\n",blk_xml);
        return FALSE;
	} else {
		blk_xml =0;
		return nvtmpp_vb_destroy_pool(pool);
	}
#else
	ER                   ret;
	// free main memory
    if (g_xml_pool_main_va_adr != 0) {
        ret = hd_common_mem_free((UINT32)g_xml_pool_main_phy_adr, (void *)g_xml_pool_main_va_adr);
        if (ret != HD_OK) {
        	DBG_ERR("movply_main free mem failed! (%d)\r\n", ret);
        	return E_SYS;
        }

        g_xml_pool_main_phy_adr = 0;
        g_xml_pool_main_va_adr = 0;
	}

    if (g_pxml_thumb_info->u32WorkBufVa != 0){
        ret = hd_common_mem_free((UINT32)g_pxml_thumb_info->u32WorkBufPa, (void *)g_pxml_thumb_info->u32WorkBufVa);
        if (ret != HD_OK) {
        	DBG_ERR("Free g_pxml_thumb_info->u32WorkBufVa failed! (%d)\r\n", ret);
        	return E_SYS;
        }
    }

	if (g_pxml_thumb_info->hd_scale_video_frame.blk != 0){
		ret = hd_common_mem_release_block(g_pxml_thumb_info->hd_scale_video_frame.blk);
        if (ret != HD_OK) {
        	DBG_ERR("Free g_pxml_thumb_info->hd_in_video_bs failed! (%d)\r\n", ret);
        	return E_SYS;
        }
	}

    memset((void*)&g_xml_thumb_info, 0, sizeof(XML_THUMB_INFO));


	return E_OK;
#endif
}

UINT32 XML_AllocPrivateMem(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = NVTMPP_DDR_1;
#if 0
	NVTMPP_VB_POOL pool;
	//release previous,because always use the same pool
	XML_RelPrivateMem();

	//only in playback can get prvivate memory,and need release
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK) {
		DBG_ERR("not pb mode fail\r\n");
		return 0;
	}

	pool = nvtmpp_vb_create_pool("XML", blk_size, 1, NVTMPP_DDR_1);
	if (NVTMPP_VB_INVALID_POOL == pool)
	{
		DBG_ERR("create private pool err\r\n");
        return 0;
	}
	blk_xml = nvtmpp_vb_get_block(0, pool, blk_size, NVTMPP_DDR_1);
	if (NVTMPP_VB_INVALID_BLK == blk_xml)
	{
		DBG_ERR("get vb block err\r\n");
        return 0;

	}
	return nvtmpp_vb_block2addr(blk_xml);
#else
	// allocate mem
	ret = hd_common_mem_alloc("XML", &pa, (void **)&va, blk_size, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("alloc size(0x%x), ddr(%d)\r\n", (unsigned int)(blk_size), (int)ddr_id);
		return E_SYS;
	}
	DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));

	g_xml_pool_main_phy_adr = (UINT32)pa;
	g_xml_pool_main_va_adr = (UINT32)va;

	return (UINT32)va;
#endif
}

void XML_SetTempMem(UINT32 uiAddr, UINT32 uiSize)
{
	XMLTmpPool.addr = uiAddr;
	XMLTmpPool.size = uiSize;
}
UINT32 XML_GetTempMem(UINT32 uiSize)
{
	void *pBuf = 0;

	if (uiSize <= XMLTmpPool.size) {
		pBuf = (void *)XMLTmpPool.addr;
		//DBGH(pBuf);
	}else {
	    DBG_ERR("uiSize %x > Max %x \r\n",uiSize,XMLTmpPool.size);
    }
	if (pBuf == 0) {
		DBG_ERR("get buffer fail\r\n");
	}
	return (UINT32)pBuf;
}

UINT32 XML_snprintf(char **buf, UINT32 *size, const char *fmt, ...)
{
	UINT32 len = 0;
	va_list marker;

	va_start(marker, fmt);       // Initialize variable arguments.

	#if _TODO
	if (marker)
	#endif
	{
		len = vsnprintf(*buf, *size, fmt, marker);
	}

	va_end(marker);                // Reset variable arguments.

	*buf += len;
	*size = *size - len;
	return len;
}

static void XML_ecos2NvtPath(const char *inPath, char *outPath, UINT32 outPathSize)
{
	char *outOrgPath = outPath;
	outPath += snprintf(outPath, outPathSize, "A:");
	//inPath+=strlen(MOUNT_FS_ROOT);
	while (*inPath != 0) {
		if (*inPath == '/') {
			*outPath = '\\';
		} else {
			*outPath = *inPath;
		}
		inPath++;
		outPath++;
		if ((UINT32)outPath - (UINT32)outOrgPath == outPathSize - 1) {
			break;
		}
	}
	//*outPath++ = '\\'; //If adding this, it will be regarded as folder.
	*outPath = 0;
}
static void XML_Nvt2ecosPath(const char *inPath, char *outPath, UINT32 outPathSize)
{
	char *outOrgPath = outPath;

	//outPath+=sprintf(outPath,MOUNT_FS_ROOT);
	inPath += strlen("A:");
	while (*inPath != 0) {
		if (*inPath == '\\') {
			*outPath = '/';
		} else {
			*outPath = *inPath;
		}
		inPath++;
		outPath++;
		if ((UINT32)outPath - (UINT32)outOrgPath == outPathSize - 1) {
			break;
		}

	}
	*outPath = 0;
}
#if USE_FILEDB
int XML_PBGetData(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	static UINT32     gIndex;
	UINT32            fileCount, i, bufflen = *bufSize;
	char             *buf = (char *)bufAddr;
	PPBX_FLIST_OBJ    pFlist = PBXFList_FDB_getObject();
	UINT32            FileDBHandle = 0;
	UINT32            xmlBufSize = *bufSize;

	PB_WaitCommandFinish(PB_WAIT_INFINITE);

	if (segmentCount == 0) {
		XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");
	}

	pFlist->GetInfo(PBX_FLIST_GETINFO_DB_HANDLE, &FileDBHandle, 0);
	fileCount = FileDB_GetTotalFileNum(FileDBHandle);

	if (segmentCount == 0) {
		// reset some global variables
		gIndex = 0;
	}
	DBG_IND("gIndex = %d %d \r\n", gIndex, fileCount);
	for (i = gIndex; i < fileCount; i++) {
		PFILEDB_FILE_ATTR        pFileAttr;
		// check buffer length , reserved 512 bytes
		// should not write data over buffer length
		if ((HFS_U32)buf - bufAddr > bufflen - 512) {
			DBG_IND("totallen=%d >  bufflen(%d)-512\r\n", (HFS_U32)buf - bufAddr, bufflen);
			*bufSize = (HFS_U32)(buf) - bufAddr;
			gIndex = i;
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		}
		// get dcf file
		pFileAttr = FileDB_SearhFile(FileDBHandle, i);
		if (pFileAttr) {
			//DBG_IND("file %s %d\r\n",pFileAttr->filePath,pFileAttr->fileSize);
			XML_snprintf(&buf, &xmlBufSize, "<ALLFile><File>\n<NAME>%s</NAME>\n<FPATH>%s</FPATH>\n", pFileAttr->filename, pFileAttr->filePath);
			//#NT#2016/03/30#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//XML_snprintf(&buf,&xmlBufSize,"<SIZE>%d</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n<ATTR>%d</ATTR></File>\n</ALLFile>\n",pFileAttr->fileSize,(pFileAttr->lastWriteDate <<16) + pFileAttr->lastWriteTime,
			XML_snprintf(&buf, &xmlBufSize, "<SIZE>%lld</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n<ATTR>%d</ATTR></File>\n</ALLFile>\n", pFileAttr->fileSize64, (pFileAttr->lastWriteDate << 16) + pFileAttr->lastWriteTime,
						 FAT_GET_YEAR_FROM_DATE(pFileAttr->lastWriteDate), FAT_GET_MONTH_FROM_DATE(pFileAttr->lastWriteDate), FAT_GET_DAY_FROM_DATE(pFileAttr->lastWriteDate),
						 FAT_GET_HOUR_FROM_TIME(pFileAttr->lastWriteTime), FAT_GET_MIN_FROM_TIME(pFileAttr->lastWriteTime), FAT_GET_SEC_FROM_TIME(pFileAttr->lastWriteTime),
						 pFileAttr->attrib);
			//#NT#2016/03/30#Nestor Yang -end
		}
	}
	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}


#else

int XML_PBGetData(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
#if 0
	static UINT32     gIndex;
	UINT32            fileCount, i, bufflen = *bufSize, FileType;
	char             *buf = (char *)bufAddr;
	char              tempPath[XML_PATH_LEN];
	FST_FILE_STATUS   FileStat;
	FST_FILE          filehdl;
	char              dcfFilePath[XML_PATH_LEN];
	UINT32            xmlBufSize = *bufSize;

	// set the data mimetype
	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	XML_ecos2NvtPath(path, tempPath, sizeof(tempPath));

	if (segmentCount == 0) {
		XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");
	}
	fileCount = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
	if (segmentCount == 0) {
		// reset some global variables
		gIndex = 1;
	}
	DBG_IND("gIndex = %d %d \r\n", gIndex, fileCount);
	for (i = gIndex; i <= fileCount; i++) {
		// check buffer length , reserved 512 bytes
		// should not write data over buffer length
		if ((HFS_U32)buf - bufAddr > bufflen - 512) {
			DBG_IND("totallen=%d >  bufflen(%d)-512\r\n", (HFS_U32)buf - bufAddr, bufflen);
			*bufSize = (HFS_U32)(buf) - bufAddr;
			gIndex = i;
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		}
		// get dcf file
		DCF_SetCurIndex(i);
		FileType = DCF_GetDBInfo(DCF_INFO_CUR_FILE_TYPE);

		if (FileType & DCF_FILE_TYPE_JPG) {
			DCF_GetObjPath(i, DCF_FILE_TYPE_JPG, dcfFilePath);
		} else if (FileType & DCF_FILE_TYPE_MOV) {
			DCF_GetObjPath(i, DCF_FILE_TYPE_MOV, dcfFilePath);
		} else {
			continue;
		}
		// get file state
		filehdl = FileSys_OpenFile(dcfFilePath, FST_OPEN_READ);
		FileSys_StatFile(filehdl, &FileStat);
		FileSys_CloseFile(filehdl);
		XML_Nvt2ecosPath(dcfFilePath, tempPath, sizeof(tempPath));

		// this is a dir
		if (M_IsDirectory(FileStat.uiAttrib)) {
			XML_snprintf(&buf, &xmlBufSize, "<Dir>\n<NAME>\n<![CDATA[%s]]></NAME><FPATH>\n<![CDATA[%s]]></FPATH>", &tempPath[15], tempPath);
			XML_snprintf(&buf, &xmlBufSize, "<TIMECODE>%ld</TIMECODE><TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n</Dir>\n", (FileStat.uiModifiedDate << 16) + FileStat.uiModifiedTime,
						 FAT_GET_YEAR_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_MONTH_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_DAY_FROM_DATE(FileStat.uiModifiedDate),
						 FAT_GET_HOUR_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_MIN_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_SEC_FROM_TIME(FileStat.uiModifiedTime));
		}
		// this is a file
		else {
			XML_snprintf(&buf, &xmlBufSize, "<ALLFile><File>\n<NAME>\n<![CDATA[%s]]></NAME><FPATH>\n<![CDATA[%s]]></FPATH>", &tempPath[15], tempPath);
			XML_snprintf(&buf, &xmlBufSize, "<SIZE>%lld</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n</File>\n</ALLFile>\n", FileStat.uiFileSize, (FileStat.uiModifiedDate << 16) + FileStat.uiModifiedTime,
						 FAT_GET_YEAR_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_MONTH_FROM_DATE(FileStat.uiModifiedDate), FAT_GET_DAY_FROM_DATE(FileStat.uiModifiedDate),
						 FAT_GET_HOUR_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_MIN_FROM_TIME(FileStat.uiModifiedTime), FAT_GET_SEC_FROM_TIME(FileStat.uiModifiedTime));
		}
	}
	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;
#endif
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
#endif


int XML_GetModeData(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char             *buf = (char *)bufAddr;
	UINT32            par = 0;
	char              tmpString[32];
	UINT32            xmlBufSize = *bufSize;

	DBG_IND("path = %s, argument -> %s, mimeType= %s, bufsize= %d, segmentCount= %d\r\n", path, argument, mimeType, *bufSize, segmentCount);

	// set the data mimetype
	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	snprintf(tmpString, sizeof(tmpString), "custom=1&cmd=%d&par=", WIFIAPP_CMD_MODECHANGE);
	if (strncmp(argument, tmpString, strlen(tmpString)) == 0) {
		sscanf_s(argument + strlen(tmpString), "%d", &par);
	} else {
		*bufSize = 0;
		DBG_ERR("error par\r\n");
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	if (par == WIFI_APP_MODE_PLAYBACK) {
		return XML_PBGetData(path, argument, bufAddr, bufSize, mimeType, segmentCount);
	} else if (par == WIFI_APP_MODE_PHOTO) {
		XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
		XML_snprintf(&buf, &xmlBufSize, "<FREEPICNUM>%d</FREEPICNUM>\n", UI_GetData(FL_WIFI_PHOTO_FREEPICNUM));
		*bufSize = (HFS_U32)(buf) - bufAddr;

		return CYG_HFS_CB_GETDATA_RETURN_OK;
	} else if (par == WIFI_APP_MODE_MOVIE) {
		XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
		XML_snprintf(&buf, &xmlBufSize, "<MAXRECTIME>%d</MAXRECTIME>\n", UI_GetData(FL_WIFI_MOVIE_MAXRECTIME));
		*bufSize = (HFS_U32)(buf) - bufAddr;
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	*bufSize = 0;
	DBG_ERR("error mode\r\n");
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}




int XML_QueryCmd(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT32            i = 0;
	char             *buf = (char *)bufAddr;
	FST_FILE          filehdl;
	char              pFilePath[64];
	UINT32            fileLen = *bufSize - 512;
	UINT32            bufflen = *bufSize - 512;
	WIFI_CMD_ENTRY   *appCmd = 0;
	UINT32            xmlBufSize = *bufSize;
	INT32			  ifs_ret;

	// set the data mimetype
	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	snprintf(pFilePath, sizeof(pFilePath), "%s", QUERY_XML_PATH); //html of all command list
	filehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);
	if (filehdl) {
		ifs_ret = FileSys_ReadFile(filehdl, (UINT8 *)buf, &fileLen, 0, 0);
		if (ifs_ret != 0) {
			DBG_ERR("read file fail!\r\n");
		}

		FileSys_CloseFile(filehdl);
		*bufSize = fileLen;
		*(buf + fileLen) = '\0';
	} else {
		if (segmentCount == 0) {
			XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
		}

		XML_snprintf(&buf, &xmlBufSize, "<Function>\n");
		appCmd = WifiCmd_GetExecTable();
		while (appCmd[i].cmd != 0) {
			XML_snprintf(&buf, &xmlBufSize, "<Cmd>%d</Cmd>\n", appCmd[i].cmd);
			i++;
		}
		XML_snprintf(&buf, &xmlBufSize, "</Function>\n");

		*bufSize = (HFS_U32)(buf) - bufAddr;
	}

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}


	return CYG_HFS_CB_GETDATA_RETURN_OK;
}


FST_FILE          gMovFilehdl=NULL;
ER XML_VdoReadCB(UINT32 pos, UINT32 size, UINT32 addr)
{
	ER result = E_SYS;

	DBG_IND("XML_VdoReadCB  pos=0x%x, size=%d, addr=0x%x\r\n", pos, size, addr);
	if (gMovFilehdl) {
		result = FileSys_SeekFile(gMovFilehdl, pos, FST_SEEK_SET);
		if (result != FST_STA_OK) {
				DBG_ERR("seek file failed\r\n");
		} else {
			//not close file,close file in XML_GetThumbnail()
			result = FileSys_ReadFile(gMovFilehdl, (UINT8 *)addr, &size, 0, 0);
		}
	}
	return result;
}

#if TRANS_JPG_FROM_H264IDR
#include "SizeConvert.h"

#if (_BOARD_DRAM_SIZE_ > 0x04000000)
#define SCALE_BUFF_SIZE     ALIGN_CEIL_32(3840*2160*3/2)
#define JPEG_BS_SIZE        ALIGN_CEIL_32(SCALE_BUFF_SIZE/4)
#define WIFI_RESERVE_SIZE   ALIGN_CEIL_32(2*1024*1024)
#else
#define SCALE_BUFF_SIZE     ALIGN_CEIL_32(1920*1080*3/2)
#define JPEG_BS_SIZE        ALIGN_CEIL_32(SCALE_BUFF_SIZE/4)
#define WIFI_RESERVE_SIZE   ALIGN_CEIL_32(1*1024*1024)
#endif
#define XML_THUMB_W         320
#define XML_THUMB_H         180

typedef struct {
	UINT32  uiJpgWidth;     ///< JPEG width
	UINT32  uiJpgHeight;    ///< JPEG height
	UINT32  uiJpgBsAddr;    ///< JPEG bs address
	UINT32  uiJpgBsSize;    ///< JPEG bs size
	UINT32  uiDAR;          ///< original image DAR
	UINT32  uiFileSize;     ///< original file size
} IDR_TO_JPG, *PIDR_TO_JPG;

static HD_RESULT XML_start_dec_path(HD_PATH_ID video_dec_path)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videodec_start(video_dec_path);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_stop error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

	 return ret;
}

static HD_RESULT XML_stop_dec_path(HD_PATH_ID video_dec_path)
{
	HD_RESULT ret = HD_OK;

	ret = hd_videodec_stop(video_dec_path);
	if (ret != HD_OK) {
		DBG_ERR("hd_videodec_stop error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

    return ret;
}

static HD_RESULT XML_set_dec_param(HD_PATH_ID video_dec_path, HD_VIDEODEC_IN *p_video_in_param, GXVIDEO_INFO *pVideoInfo)
{
	HD_RESULT ret = HD_OK;
    HD_H26XDEC_DESC desc_info = {0};

	//--- HD_VIDEODEC_PARAM_IN ---
	ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, p_video_in_param);
	if (ret != HD_OK) {
		DBG_ERR("set HD_VIDEODEC_PARAM_IN error(%d) !!\r\n", ret);
		return HD_ERR_NG;
	}

   //--- HD_VIDEODEC_PARAM_IN_DESC ---
    if ((p_video_in_param->codec_type == HD_CODEC_TYPE_H264) || (p_video_in_param->codec_type == HD_CODEC_TYPE_H265)){
	    desc_info.addr = pVideoInfo->uiH264DescAddr;
		desc_info.len = pVideoInfo->uiH264DescSize;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
		if (ret != HD_OK) {
			DBG_ERR("set h.26x desc error(%d) !!\r\n", ret);
			return HD_ERR_NG;
		}
    }

	return ret;
}

// IVOT_N12020_CO-625
static void XML_set_h26x_start_code(UINT32 u32Addr, UINT32 bsSize)
{
#if 0
	UINT8 *ptr = (UINT8 *)u32Addr;

	ptr[0] = 0x00;
	ptr[1] = 0x00;
	ptr[2] = 0x00;
	ptr[3] = 0x01;
#else
	UINT8 *ptr = (UINT8 *)u32Addr;
	UINT32 tile_size = 0;
	UINT32 offset = 0;
	UINT8 tile_cnt = 0;
	UINT32 bs_size_count = 0;

	do {

		tile_size = (((*ptr) << 24) | ((*(ptr + 1))<<16) | ((*(ptr + 2))<< 8) | (*(ptr + 3)));

		offset = (4 + tile_size);

		*ptr = 0x00;
		*(ptr+1) = 0x00;
		*(ptr+2) = 0x00;
		*(ptr+3) = 0x01;

		ptr += offset;
		bs_size_count += offset;
		tile_cnt++;

	} while(bs_size_count < bsSize);

	DBG_IND("tile_cnt=%u\r\n", tile_cnt);
#endif
}


static INT32 XML_MakeJpgFromIFrame(char *pFilePath, IDR_TO_JPG *pIdrToJpg, GXVIDEO_INFO *pVideoInfo, BOOL bIsVideo, BOOL bIsTs, UINT32 file_buf)
{
    HD_RESULT hd_ret = HD_OK;
	PBVIEW_HD_COM_BUF hd_scale_buf = {0};
	HD_PATH_ID     *p_hd_vdec_path = NULL; //p_hd_vdec_path[0] is jpg ;  p_hd_vdec_path[1] is h.264 ; p_hd_vdec_path[2] is h.265
	HD_PATH_ID      video_enc_path;
	HD_VIDEODEC_IN video_in_param = {0};
	VF_GFX_SCALE   gfx_scale = {0};
	UINT32 loff[HD_VIDEO_MAX_PLANE] = {0};
	UINT32 addr[HD_VIDEO_MAX_PLANE] = {0};
	HD_VIDEOENC_PATH_CONFIG video_enc_path_config = {0};
	HD_VIDEOENC_IN  video_enc_in_param = {0};
	HD_VIDEOENC_OUT video_enc_out_param = {0};
	UINT8 *ptr;
	UINT32 uiFileSize = 0;
	FST_FILE  pFileHdl = NULL;
	PB_HD_COM_BUF hd_file_buf = {0};
	PB_HD_COM_BUF hd_raw_buf = {0};
#if _TODO
    UINT32 va_UVOffset = 0, u32Size = 0;
	FILE *f_out;
	char filepath[128];
#endif
	if (bIsVideo) { //make JPG from I frame
	    if (pVideoInfo == NULL) {
			return E_SYS;
		}
	    //allocate bitstream buffer for first I-frame
		PB_GetParam(PBPRMID_FILE_BUF_HD_INFO, (UINT32 *)&hd_file_buf);

		if (bIsTs == FALSE) {
			XML_VdoReadCB(pVideoInfo->ui1stFramePos, pVideoInfo->ui1stFrameSize, hd_file_buf.va); //read first I-frame
		}

		//decode I-frame
		PB_GetParam(PBPRMID_HD_VIDEODEC_PATH, (UINT32 *)&g_pxml_thumb_info->p_hd_vdec_path);
		g_pxml_thumb_info->hd_in_video_bs.sign = MAKEFOURCC('V','S','T','M');
		g_pxml_thumb_info->hd_in_video_bs.p_next = NULL;
		g_pxml_thumb_info->hd_in_video_bs.ddr_id = DDR_ID0;
		g_pxml_thumb_info->hd_in_video_bs.timestamp = hd_gettime_us();
		g_pxml_thumb_info->hd_in_video_bs.count	= 0;
		g_pxml_thumb_info->hd_in_video_bs.blk = hd_file_buf.blk;

		switch (pVideoInfo->uiVidType){
	    case MEDIAPLAY_VIDEO_H264:
			p_hd_vdec_path = &g_pxml_thumb_info->p_hd_vdec_path[1];
			XML_stop_dec_path(g_pxml_thumb_info->p_hd_vdec_path[1]);
			video_in_param.codec_type = HD_CODEC_TYPE_H264;
        	XML_set_dec_param(g_pxml_thumb_info->p_hd_vdec_path[1], &video_in_param, pVideoInfo);
			XML_start_dec_path(g_pxml_thumb_info->p_hd_vdec_path[1]);
			if (bIsTs) { // IVOT_N12044_CO-191
				UINT32 uiNewPlayFileBuf;
				uiNewPlayFileBuf = file_buf + GXVIDEO_H26X_WORK_BUFFER_SIZE + GXVIDEO_VID_ENTRY_BUFFER_SIZE + GXVIDEO_AUD_ENTRY_BUFFER_SIZE;
				uiNewPlayFileBuf += pVideoInfo->ui1stFramePos;
				g_pxml_thumb_info->hd_in_video_bs.phy_addr = PBView_get_hd_phy_addr((void *)uiNewPlayFileBuf);
				g_pxml_thumb_info->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_H264;
				g_pxml_thumb_info->hd_in_video_bs.size = pVideoInfo->ui1stFrameSize;
				#if 0
				{
					UINT32 i;
					ptr = (UINT8 *)g_pxml_thumb_info->hd_in_video_bs.phy_addr;
					DBG_DUMP("[XML_MakeJpgFromIFrame] hd_file_buf.va 0x%X\r\n", hd_file_buf.va);
					DBG_DUMP("[XML_MakeJpgFromIFrame] prt 0x%X, h26x 1st I-frame:\r\n", ptr);
					for (i = 0; i < 16; i++) {
						DBG_DUMP("0x%02X ", *(ptr + i));
					}
					DBG_DUMP("\r\n");
				}
				#endif
			} else {
				g_pxml_thumb_info->hd_in_video_bs.phy_addr = PBView_get_hd_phy_addr((void *)hd_file_buf.va);
				g_pxml_thumb_info->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_H264;
				g_pxml_thumb_info->hd_in_video_bs.size = pVideoInfo->ui1stFrameSize;
				// IVOT_N12020_CO-625
				XML_set_h26x_start_code(hd_file_buf.va, g_pxml_thumb_info->hd_in_video_bs.size);
			}
			break;

		case MEDIAPLAY_VIDEO_H265:
			p_hd_vdec_path = &g_pxml_thumb_info->p_hd_vdec_path[2];
			XML_stop_dec_path(g_pxml_thumb_info->p_hd_vdec_path[2]);
			video_in_param.codec_type = HD_CODEC_TYPE_H265;
        	XML_set_dec_param(g_pxml_thumb_info->p_hd_vdec_path[2], &video_in_param, pVideoInfo);
			XML_start_dec_path(g_pxml_thumb_info->p_hd_vdec_path[2]);
			ptr = (UINT8 *)hd_file_buf.va;
			ptr = ptr + pVideoInfo->uiH264DescSize;
			if (bIsTs) { // IVOT_N12044_CO-191
				UINT32 uiNewPlayFileBuf;
				uiNewPlayFileBuf = file_buf + GXVIDEO_H26X_WORK_BUFFER_SIZE + GXVIDEO_VID_ENTRY_BUFFER_SIZE + GXVIDEO_AUD_ENTRY_BUFFER_SIZE;
				uiNewPlayFileBuf += pVideoInfo->ui1stFramePos;
				g_pxml_thumb_info->hd_in_video_bs.phy_addr = PBView_get_hd_phy_addr((void *)uiNewPlayFileBuf);
				g_pxml_thumb_info->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_H265;
				g_pxml_thumb_info->hd_in_video_bs.size = pVideoInfo->ui1stFrameSize;
				#if 0
				{
					UINT32 i;
					ptr = (UINT8 *)g_pxml_thumb_info->hd_in_video_bs.phy_addr;
					DBG_DUMP("[XML_MakeJpgFromIFrame] hd_file_buf.va 0x%X\r\n", hd_file_buf.va);
					DBG_DUMP("[XML_MakeJpgFromIFrame] prt 0x%X, h26x 1st I-frame:\r\n", ptr);
					for (i = 0; i < 16; i++) {
						DBG_DUMP("0x%02X ", *(ptr + i));
					}
					DBG_DUMP("\r\n");
				}
				#endif
			} else {

				//XML_set_h26x_start_code((UINT32)ptr);
				g_pxml_thumb_info->hd_in_video_bs.phy_addr = PBView_get_hd_phy_addr((void *)ptr);
	            g_pxml_thumb_info->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_H265;
	            g_pxml_thumb_info->hd_in_video_bs.size    = pVideoInfo->ui1stFrameSize - pVideoInfo->uiH264DescSize;
				XML_set_h26x_start_code((UINT32)ptr, g_pxml_thumb_info->hd_in_video_bs.size);
			}
			break;
		}

        //To allocate deoced YUV buffer for first I-frame and get max decoded YUV buffer for playback library
		PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&g_pxml_thumb_info->hd_out_video_frame);
		g_pxml_thumb_info->hd_out_video_frame.dim.w = ALIGN_CEIL_64(PB_MAX_VIDEO_W);
		g_pxml_thumb_info->hd_out_video_frame.dim.h = ALIGN_CEIL_64(PB_MAX_VIDEO_H);
		PB_GetParam(PBPRMID_INFO_IMG_HDBUF, (UINT32 *)&hd_raw_buf); //hd_raw_buf.blk = g_pxml_thumb_info->hd_out_video_frame.blk

		//Deocde first I-frame
		hd_ret = hd_videodec_push_in_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_in_video_bs, &g_pxml_thumb_info->hd_out_video_frame, 0); // only support non-blocking mode now
		if ((hd_ret != HD_OK) || (p_hd_vdec_path == NULL)) {
			DBG_ERR("push_in error for videodec(%d) !!\r\n", hd_ret);
			return E_SYS;
		}

		hd_ret = hd_videodec_release_out_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_out_video_frame);//match with hd_videodec_push_in_buf
		if ((hd_ret != HD_OK) || (p_hd_vdec_path == NULL)) {
			DBG_ERR("release_ouf_buf fail, ret(%d)\r\n", hd_ret);
		}

		hd_ret = hd_videodec_pull_out_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_out_video_frame, -1);
		if ((hd_ret != HD_OK) || (p_hd_vdec_path == NULL)) {
			DBG_ERR("decode error for videodec(%d) !!\r\n", hd_ret);
			return E_SYS;
		}

#if _TODO
		sprintf(filepath, "/mnt/sd/%s", "vdo.y");
		if ((f_out = fopen(filepath, "w")) == NULL){
			DBG_ERR("open vdo.y failed");
		}
		u32Size = g_pxml_thumb_info->hd_out_video_frame.dim.w*g_pxml_thumb_info->hd_out_video_frame.dim.h;
		fwrite((UINT8*)hd_raw_buf.va, u32Size, 1, f_out);
		fclose(f_out);

		sprintf(filepath, "/mnt/sd/%s", "vdo.uv");
		if ((f_out = fopen(filepath, "w")) == NULL){
			DBG_ERR("open vdo.uv failed\r\n");
		}
		va_UVOffset = hd_raw_buf.va + (g_pxml_thumb_info->hd_out_video_frame.phy_addr[HD_VIDEO_PINDEX_UV] - g_pxml_thumb_info->hd_out_video_frame.phy_addr[HD_VIDEO_PINDEX_Y]);
		u32Size = g_pxml_thumb_info->hd_out_video_frame.dim.w*g_pxml_thumb_info->hd_out_video_frame.dim.h/2;
		fwrite((UINT8*)va_UVOffset, u32Size, 1, f_out);
		fclose(f_out);
#endif

	} else { //make JPG from decoded YUV data
	    DBG_ERR("open %s\r\n", pFilePath);
		PB_GetParam(PBPRMID_FILE_BUF_HD_INFO, (UINT32 *)&hd_file_buf);

		pFileHdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);
		if (pFileHdl) {
			uiFileSize = FileSys_GetFileLen(pFilePath);
            //coverity[check_return]
			FileSys_ReadFile(pFileHdl, (UINT8 *)hd_file_buf.va, &uiFileSize, 0, 0);
		} else {
		    DBG_ERR("open %s failed\r\n", pFilePath);
		 	return E_SYS;
		}

		FileSys_CloseFile(pFileHdl);

        PB_GetParam(PBPRMID_HD_VIDEODEC_PATH, (UINT32 *)&g_pxml_thumb_info->p_hd_vdec_path);
		p_hd_vdec_path = &g_pxml_thumb_info->p_hd_vdec_path[0];
		XML_stop_dec_path(g_pxml_thumb_info->p_hd_vdec_path[0]);
		video_in_param.codec_type = HD_CODEC_TYPE_JPEG;
		XML_set_dec_param(g_pxml_thumb_info->p_hd_vdec_path[0], &video_in_param, pVideoInfo);
		XML_start_dec_path(g_pxml_thumb_info->p_hd_vdec_path[0]);

		g_pxml_thumb_info->hd_in_video_bs.sign = MAKEFOURCC('V','S','T','M');
		g_pxml_thumb_info->hd_in_video_bs.p_next = NULL;
		g_pxml_thumb_info->hd_in_video_bs.ddr_id = DDR_ID0;
		g_pxml_thumb_info->hd_in_video_bs.timestamp = hd_gettime_us();
		g_pxml_thumb_info->hd_in_video_bs.count	= 0;
		g_pxml_thumb_info->hd_in_video_bs.blk = hd_file_buf.blk;
		g_pxml_thumb_info->hd_in_video_bs.phy_addr = PBView_get_hd_phy_addr((void *)hd_file_buf.va);
        g_pxml_thumb_info->hd_in_video_bs.vcodec_format = HD_CODEC_TYPE_JPEG;
        g_pxml_thumb_info->hd_in_video_bs.size = uiFileSize;

		//To allocate deoced YUV buffer for first I-frame and get max decoded YUV buffer for playback library
		PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&g_pxml_thumb_info->hd_out_video_frame);
		PB_GetParam(PBPRMID_INFO_IMG_HDBUF, (UINT32 *)&hd_raw_buf); //hd_raw_buf.blk = g_pxml_thumb_info->hd_out_video_frame.blk

		//Deocde primary image
		hd_ret = hd_videodec_push_in_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_in_video_bs, &g_pxml_thumb_info->hd_out_video_frame, 0); // only support non-blocking mode now
		if (hd_ret != HD_OK) {
			DBG_ERR("push_in error for videodec(%d) !!\r\n", hd_ret);
			return E_SYS;
		}

		hd_ret = hd_videodec_pull_out_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_out_video_frame, -1);
		if ((hd_ret != HD_OK) || (p_hd_vdec_path == NULL)) {
		    DBG_ERR("decode error for videodec(%d) !!\r\n", hd_ret);
		    return E_SYS;
		}
		else{
		    hd_ret = hd_videodec_release_out_buf(*p_hd_vdec_path, &g_pxml_thumb_info->hd_out_video_frame);//match with hd_videodec_push_in_buf
		    if ((hd_ret != HD_OK) || (p_hd_vdec_path == NULL)) {
		        DBG_ERR("release_ouf_buf fail, ret(%d)\r\n", hd_ret);
		    }
		}
#if _TODO
		sprintf(filepath, "/mnt/sd/%s", "img.y");
		if ((f_out = fopen(filepath, "w")) == NULL){
			DBG_ERR("open img.y failed");
		}
		u32Size = g_pxml_thumb_info->hd_out_video_frame.dim.w*g_pxml_thumb_info->hd_out_video_frame.dim.h;
		fwrite((UINT8*)hd_raw_buf.va, u32Size, 1, f_out);
		fclose(f_out);

		sprintf(filepath, "/mnt/sd/%s", "img.uv");
		if ((f_out = fopen(filepath, "w")) == NULL){
			DBG_ERR("open img.uv failed\r\n");
		}
		va_UVOffset = hd_raw_buf.va + (g_pxml_thumb_info->hd_out_video_frame.phy_addr[HD_VIDEO_PINDEX_UV] - g_pxml_thumb_info->hd_out_video_frame.phy_addr[HD_VIDEO_PINDEX_Y]);
		u32Size = g_pxml_thumb_info->hd_out_video_frame.dim.w*g_pxml_thumb_info->hd_out_video_frame.dim.h/2;
		fwrite((UINT8*)va_UVOffset, u32Size, 1, f_out);
		fclose(f_out);
#endif
	}

	// scaled to JPEG resolution //
	///////////////////////////////
	hd_scale_buf.blk_size = XML_THUMB_W*XML_THUMB_H*3/2; //page alignment
	PBView_get_hd_common_buf(&hd_scale_buf);
	loff[0] = XML_THUMB_W; //Besides rotate panel, the display device don't consider the line offset.
	addr[0] = hd_scale_buf.pa;
	loff[1] = XML_THUMB_W; //Besides rotate panel, the display device don't consider the line offset.
	addr[1] = addr[0] + (XML_THUMB_W*XML_THUMB_H);
	hd_ret = vf_init_ex(&gfx_scale.dst_img, XML_THUMB_W, XML_THUMB_H, HD_VIDEO_PXLFMT_YUV420, loff, addr);
	if (hd_ret != HD_OK) {
		DBG_ERR("vf_init_ex dst failed\r\n");
		return E_SYS;
	}

	gfx_scale.dst_img.blk = hd_scale_buf.blk;
	memcpy((void *)&gfx_scale.src_img, (void *)&g_pxml_thumb_info->hd_out_video_frame, sizeof(HD_VIDEO_FRAME));
	gfx_scale.engine = 0;
	gfx_scale.src_region.x = 0;
	gfx_scale.src_region.y = 0;
	gfx_scale.src_region.w = g_pxml_thumb_info->hd_out_video_frame.dim.w;
	gfx_scale.src_region.h = g_pxml_thumb_info->hd_out_video_frame.dim.h;
	gfx_scale.dst_region.x = 0; // dst frame buffer rather than IDE window.
	gfx_scale.dst_region.y = 0; // dst frame buffer rather than IDE window.
	gfx_scale.dst_region.w = XML_THUMB_W;
	gfx_scale.dst_region.h = XML_THUMB_H;
	gfx_scale.quality = HD_GFX_SCALE_QUALITY_BILINEAR;
	vf_gfx_scale(&gfx_scale, 1);
    memcpy((void *)&g_pxml_thumb_info->hd_scale_video_frame, (void *)&gfx_scale.dst_img, sizeof(HD_VIDEO_FRAME));

#if _TODO
			sprintf(filepath, "/mnt/sd/%s", "scale.y");
			if ((f_out = fopen(filepath, "w")) == NULL){
				DBG_ERR("open out.y failed");
			}
			u32Size = g_pxml_thumb_info->hd_scale_video_frame.dim.w*g_pxml_thumb_info->hd_scale_video_frame.dim.h;
			fwrite((UINT8*)hd_scale_buf.va, u32Size, 1, f_out);
			fclose(f_out);

			sprintf(filepath, "/mnt/sd/%s", "scale.uv");
			if ((f_out = fopen(filepath, "w")) == NULL){
				DBG_ERR("open out.uv failed");
			}
			va_UVOffset = hd_scale_buf.va + (g_pxml_thumb_info->hd_scale_video_frame.phy_addr[HD_VIDEO_PINDEX_UV] - g_pxml_thumb_info->hd_scale_video_frame.phy_addr[HD_VIDEO_PINDEX_Y]);
			u32Size = g_pxml_thumb_info->hd_scale_video_frame.loff[HD_VIDEO_PINDEX_UV]*g_pxml_thumb_info->hd_scale_video_frame.ph[HD_VIDEO_PINDEX_UV];
			fwrite((UINT8*)va_UVOffset, u32Size, 1, f_out);
			fclose(f_out);
#endif

    /*--- encode JPG ---*/
    //stop the decode jpg path
	XML_stop_dec_path(g_pxml_thumb_info->p_hd_vdec_path[0]);

	//init videoenc module
    if ((hd_ret = hd_videoenc_init()) != HD_OK){
		DBG_ERR("hd_videoenc_init failed (%d)\r\n", hd_ret);
		return E_SYS;
    }

	if ((hd_ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &video_enc_path)) != HD_OK){
		DBG_ERR("hd_videoenc_open failed (%d)\r\n", hd_ret);
		return E_SYS;
	}

    g_pxml_thumb_info->hd_venc_path = video_enc_path;

	//HD_VIDEOENC_PARAM_PATH_CONFIG
	video_enc_path_config.max_mem.codec_type = HD_CODEC_TYPE_JPEG;
	video_enc_path_config.max_mem.max_dim.w  = XML_THUMB_W;
	video_enc_path_config.max_mem.max_dim.h  = XML_THUMB_H;
	video_enc_path_config.max_mem.bitrate	 = XML_THUMB_W*XML_THUMB_H*3*8/(2*5);
	video_enc_path_config.max_mem.enc_buf_ms = 2000;
	video_enc_path_config.max_mem.svc_layer  = HD_SVC_DISABLE;
	video_enc_path_config.max_mem.ltr		 = FALSE;
	video_enc_path_config.max_mem.rotate	 = FALSE;
	video_enc_path_config.max_mem.source_output   = FALSE;
	video_enc_path_config.isp_id			 = 0;
	hd_ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_enc_path_config);
	if (hd_ret != HD_OK) {
		DBG_ERR("set_enc_path_config = %d\r\n", hd_ret);
		return E_SYS;
	}

	//HD_VIDEOENC_PARAM_IN
	video_enc_in_param.dir			 = HD_VIDEO_DIR_NONE;
	video_enc_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
	video_enc_in_param.dim.w   = XML_THUMB_W;
	video_enc_in_param.dim.h   = XML_THUMB_H;
	video_enc_in_param.frc	   = HD_VIDEO_FRC_RATIO(1,1);
	hd_ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_enc_in_param);
	if (hd_ret != HD_OK) {
		DBG_ERR("set_enc_param_in = %d\r\n", hd_ret);
		return E_SYS;
	}

	//HD_VIDEOENC_PARAM_OUT_ENC_PARAM
	video_enc_out_param.codec_type		   = HD_CODEC_TYPE_JPEG;
	video_enc_out_param.jpeg.retstart_interval = 0;
	video_enc_out_param.jpeg.image_quality = 50;
	hd_ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_enc_out_param);
	if (hd_ret != HD_OK) {
		DBG_ERR("set_enc_param_out = %d\r\n", hd_ret);
		return E_SYS;
	}

    //Only take virtual address
	if ((hd_ret = hd_videoenc_start(video_enc_path)) != HD_OK) {
		DBG_ERR("hd_videoenc_start fail(%d)\n", hd_ret);
		return E_SYS;
	}

	if ((hd_ret = hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_BUFINFO, &g_pxml_thumb_info->hd_venc_buf_info)) != HD_OK) {
		DBG_ERR("hd_videoenc_get fail(%d)\n", hd_ret);
		return E_SYS;
	}

	g_pxml_thumb_info->hd_venc_bs_buf_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, g_pxml_thumb_info->hd_venc_buf_info.phy_addr, g_pxml_thumb_info->hd_venc_buf_info.buf_size);


    //ENCODE START
	hd_ret = hd_videoenc_push_in_buf(video_enc_path, &gfx_scale.dst_img, NULL, 0); // only support non-blocking mode now
	if (hd_ret != HD_OK) {
		DBG_ERR("enc_push error=%d !!\r\n\r\n", hd_ret);
		return E_SYS;
	}

	hd_ret = hd_videoenc_pull_out_buf(video_enc_path, &g_pxml_thumb_info->hd_out_video_bs, -1); // -1 = blocking mode
	if (hd_ret != HD_OK) {
		DBG_ERR("enc_pull error=%d !!\r\n\r\n", hd_ret);
		return E_SYS;
	}

    pIdrToJpg->uiJpgBsAddr = g_pxml_thumb_info->hd_venc_bs_buf_va + (g_pxml_thumb_info->hd_out_video_bs.video_pack[0].phy_addr - g_pxml_thumb_info->hd_venc_buf_info.phy_addr);
	pIdrToJpg->uiJpgBsSize = g_pxml_thumb_info->hd_out_video_bs.video_pack[0].size;

#if _TODO
	sprintf(filepath, "/mnt/sd/%s", "test.jpg");
	if ((f_out = fopen(filepath, "w")) == NULL){
		DBG_ERR("open test.jpg failed");
	}
	fwrite((UINT8*)pIdrToJpg->uiJpgBsAddr, pIdrToJpg->uiJpgBsSize, 1, f_out);
	fclose(f_out);
	system("sync");
#endif

	#if 0 // save thumbnail JPEG TEST!!!
	FST_FILE file_hdl;
	file_hdl = FileSys_OpenFile("A:\\thumb.jpg", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	FileSys_WriteFile(file_hdl, (UINT8 *)pIdrToJpg->uiJpgBsAddr, &pIdrToJpg->uiJpgBsSize, 0, NULL);
	FileSys_CloseFile(file_hdl);
	#endif

	hd_ret = hd_videoenc_release_out_buf(video_enc_path, &g_pxml_thumb_info->hd_out_video_bs);
	if (hd_ret != HD_OK) {
		DBG_ERR("enc_release error=%d !!\r\n", hd_ret);
	}

	if ((hd_ret = hd_videoenc_stop(video_enc_path)) != HD_OK){
		DBG_ERR("hd_videoenc_stop (%d)\r\n\r\n", hd_ret);
	}

	if ((hd_ret = hd_videoenc_close(video_enc_path)) != HD_OK){
		DBG_ERR("hd_videoenc_close (%d)\r\n\r\n", hd_ret);
		}

	if ((hd_ret = hd_videoenc_uninit()) != HD_OK){
		DBG_ERR("hd_videoenc_uninit (%d)\r\n\r\n", hd_ret);
	}

    //restore the decode jpg path
	XML_start_dec_path(g_pxml_thumb_info->p_hd_vdec_path[0]);

	return E_OK;
}
#endif


typedef enum _XML_VID_TYPE {
	TYPE_MOV     =   0,
	TYPE_MP4     =   1,
	TYPE_MOV_MP4 =   2,
	TYPE_TS      =   3,
	ENUM_DUMMY4WORD(XML_VID_TYPE)
} XML_VID_TYPE;

BOOL XML_CheckSub(char *pch, XML_VID_TYPE  type)
{
	switch (type) {
	case TYPE_MOV:
		return ((strncmp(pch + 1, "mov", 3) == 0) || (strncmp(pch + 1, "MOV", 3) == 0));

	case TYPE_MP4:
		return ((strncmp(pch + 1, "mp4", 3) == 0) || (strncmp(pch + 1, "MP4", 3) == 0));

	case TYPE_MOV_MP4:
		return ((strncmp(pch + 1, "mov", 3) == 0) || (strncmp(pch + 1, "MOV", 3) == 0) ||
				(strncmp(pch + 1, "mp4", 3) == 0) || (strncmp(pch + 1, "MP4", 3) == 0));

	case TYPE_TS:
		return ((strncmp(pch + 1, "ts", 2) == 0) || (strncmp(pch + 1, "TS", 2) == 0));

	default:
		return FALSE;
	}
}

int XML_GetThumbnail(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char             *buf = (char *)bufAddr;
//	FST_FILE          filehdl;
	char              tempPath[XML_PATH_LEN];
	UINT32            TempLen, TempBuf = 0;
	UINT32            bufflen = *bufSize - 512;
	UINT32            ThumbOffset = 0;
	UINT32            ThumbSize = 0;
	static UINT32     remain = 0;
	static UINT32     ThumbBuf = 0;
	char             *pch;  //for test. *pch = ".jpg" or *pch = ".MP4"
	UINT32            result = 0;
	HD_RESULT         hd_ret;
	void              *pVa;
	UINT32            pa;
	IDR_TO_JPG        IdrToJpg = {0};
	INT32             iResult;
#if !TRANS_JPG_FROM_H264IDR
	UINT8            *ptr = 0;
#endif

	// set the data mimetype
	XML_STRCPY(mimeType, "image/jpeg", CYG_HFS_MIMETYPE_MAXLEN);

	XML_ecos2NvtPath(path, tempPath, sizeof(tempPath));

	pch = strchr(tempPath, '.');

	if (!pch) {
		XML_RelPrivateMem();
		XML_DefaultFormat(WIFIAPP_CMD_THUMB, WIFIAPP_RET_PAR_ERR, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	if (XML_CheckSub(pch, TYPE_MOV_MP4) || XML_CheckSub(pch, TYPE_TS)) {
		if (segmentCount == 0) {
			FST_FILE_STATUS FileStat = {0};
			GXVIDEO_INFO VideoInfo = {0};
			MEM_RANGE WorkBuf = {0};
			UINT32 uiBufferNeeded = 0;
			UINT32 thumbAddr = 0, thumbSize = 0;

			gMovFilehdl = FileSys_OpenFile(tempPath, FST_OPEN_READ);

			if (gMovFilehdl) {
				FileSys_StatFile(gMovFilehdl, &FileStat);
				GxVidFile_QueryParseVideoInfoBufSize(&uiBufferNeeded);
				// allocate working buffer memory

				hd_ret = hd_common_mem_alloc("XML", &pa, (void **)&pVa, uiBufferNeeded, NVTMPP_DDR_1);
				if (hd_ret != HD_OK) {
					DBG_ERR("alloc size(0x%x), ddr(%d)\r\n", (uiBufferNeeded), (int)NVTMPP_DDR_1);
					return E_SYS;
				}

				g_pxml_thumb_info->u32WorkBufPa = (UINT32)pa;
				g_pxml_thumb_info->u32WorkBufVa = (UINT32)pVa;
				TempBuf = g_pxml_thumb_info->u32WorkBufVa;

				if (!TempBuf) {
					FileSys_CloseFile(gMovFilehdl);
					gMovFilehdl=NULL;
					XML_DefaultFormat(WIFIAPP_CMD_THUMB, WIFIAPP_RET_NOBUF, bufAddr, bufSize, mimeType);
					return CYG_HFS_CB_GETDATA_RETURN_OK;
				}

				{
					//parse video info for single mode only
					WorkBuf.addr = TempBuf;
					WorkBuf.size = uiBufferNeeded;
					if (XML_CheckSub(pch, TYPE_TS)) {
						WorkBuf.size += 0x20000; // IVOT_N12044_CO-191
					}

					if (XML_CheckSub(pch, TYPE_MOV_MP4)) {
						result = GxVidFile_ParseVideoInfo(XML_VdoReadCB, &WorkBuf, (UINT32)FileStat.uiFileSize, &VideoInfo);
					} else if (XML_CheckSub(pch, TYPE_TS)) {
						result = GxVidFile_GetTsThumb(XML_VdoReadCB, &WorkBuf, &thumbAddr, &thumbSize);
						if (!thumbSize) {
							GxVidFile_ParseVideoInfo(XML_VdoReadCB, &WorkBuf, (UINT32)FileStat.uiFileSize, &VideoInfo);
						}
					}
                    //DBG_DUMP("VideoInfo.uiThumbSize = %d\r\n", VideoInfo.uiThumbSize);
					//DBG_DUMP("VideoInfo.ui1stFramePos = %d\r\n",VideoInfo.ui1stFramePos);
					//DBG_DUMP("VideoInfo.ui1stFrameSize = %d\r\n", VideoInfo.ui1stFrameSize);
					if ((result == GXVIDEO_PRSERR_OK) &&
						(XML_CheckSub(pch, TYPE_MOV_MP4)) && (VideoInfo.uiThumbSize)) {
						FileSys_SeekFile(gMovFilehdl, VideoInfo.uiThumbOffset, FST_SEEK_SET);
						FileSys_ReadFile(gMovFilehdl, (UINT8 *)TempBuf, &VideoInfo.uiThumbSize, 0, 0);
						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;

						//*bufSize = VideoInfo.uiThumbSize;
						remain = VideoInfo.uiThumbSize;
						ThumbBuf = TempBuf;
					} else if ((result == GXVIDEO_PRSERR_OK) &&
							   (XML_CheckSub(pch, TYPE_TS)) && (thumbSize)) {
						remain = thumbSize;
						ThumbBuf = thumbAddr;
						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;
					} else if (VideoInfo.ui1stFrameSize) { // Return 1st I frame back to APP.
#if TRANS_JPG_FROM_H264IDR
						// no DAR param now, suppose 2880x2160 is DAR 16:9
						//if ((VideoInfo.uiVidWidth == 2880) && (VideoInfo.uiVidHeight == 2160)) {
						//	IdrToJpg.uiDAR = MEDIAREC_DAR_16_9;
						//} else {
							IdrToJpg.uiDAR = MEDIAREC_DAR_DEFAULT;
						//}
						IdrToJpg.uiJpgWidth  = XML_THUMB_W;
						IdrToJpg.uiJpgHeight = XML_THUMB_H;
						IdrToJpg.uiFileSize  = FileStat.uiFileSize;
						//#NT#2016/02/15#Ben Wang -begin
						//#NEW,re-encode thumbnail for the photo that doesn't have EXIF.
						iResult = XML_MakeJpgFromIFrame(NULL, &IdrToJpg, &VideoInfo, TRUE, XML_CheckSub(pch, TYPE_TS), WorkBuf.addr); // IVOT_N12044_CO-191
						//#NT#2016/02/15#Ben Wang -end

						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;

						if (iResult == E_OK) {
							ThumbBuf = IdrToJpg.uiJpgBsAddr;
							remain = IdrToJpg.uiJpgBsSize;
						} else {
							ThumbBuf = 0;
							remain = 0;
						}
#else
						DBG_DUMP("I frame pos:%d, size:%d\r\n", VideoInfo.ui1stFramePos, VideoInfo.ui1stFrameSize);
						DBG_DUMP("SPS/PPS header size:%d\r\n", VideoInfo.uiH264DescSize);

						if (VideoInfo.uiH264DescSize > 0x40) {
							result = WIFIAPP_RET_FILE_ERROR;
							XML_RelPrivateMem();
							XML_DefaultFormat(WIFIAPP_CMD_THUMB, result, bufAddr, bufSize, mimeType);
							FileSys_CloseFile(gMovFilehdl);
							gMovFilehdl=NULL;
							return CYG_HFS_CB_GETDATA_RETURN_OK;
						}
						// H264 SPS/PPS header.
						#if 0
						hwmem_open();
						hwmem_memcpy((UINT32)TempBuf, VideoInfo.uiH264DescAddr, VideoInfo.uiH264DescSize);
						hwmem_close();
						#else
						memcpy((void *)TempBuf, (void *)VideoInfo.uiH264DescAddr, VideoInfo.uiH264DescSize);
						#endif

						// 1st I frame.
						FileSys_SeekFile(gMovFilehdl, VideoInfo.ui1stFramePos, FST_SEEK_SET);
						FileSys_ReadFile(gMovFilehdl, (UINT8 *)TempBuf + VideoInfo.uiH264DescSize, &VideoInfo.ui1stFrameSize, 0, 0);
						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;

						// Manipulate first 4 bytes in I frame as 0x01000000.
						ptr = (UINT8 *)(TempBuf + VideoInfo.uiH264DescSize);
						ptr[0] = 0x00;
						ptr[1] = 0x00;
						ptr[2] = 0x00;
						ptr[3] = 0x01;

						ThumbBuf = TempBuf;
						remain = VideoInfo.uiH264DescSize + VideoInfo.ui1stFrameSize;
#endif
					} else {
						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;
						result = WIFIAPP_RET_EXIF_ERR;
						DBG_ERR("No Thumbnail and I frame.\r\n");
					}
				}
			} else {
				result = WIFIAPP_RET_NOFILE;
				DBG_ERR("no %s\r\n", tempPath);
			}
		}
	} else {
		if (segmentCount == 0) {
			gMovFilehdl  = FileSys_OpenFile(tempPath, FST_OPEN_READ);
			if (gMovFilehdl ) {
				MEM_RANGE ExifData;
				EXIF_GETTAG exifTag;
				BOOL bIsLittleEndian = 0;
				UINT32 uiTiffOffsetBase = 0;

				TempBuf = XML_AllocPrivateMem(MAX_APP1_SIZE);
				if (!TempBuf) {
					FileSys_CloseFile(gMovFilehdl );
					gMovFilehdl=NULL;
					XML_DefaultFormat(WIFIAPP_CMD_THUMB, WIFIAPP_RET_NOBUF, bufAddr, bufSize, mimeType);
					return CYG_HFS_CB_GETDATA_RETURN_OK;
				}

				TempLen = MAX_APP1_SIZE;
                //coverity[check_return]
				FileSys_ReadFile(gMovFilehdl , (UINT8 *)TempBuf, &TempLen, 0, 0);
				FileSys_CloseFile(gMovFilehdl );
				gMovFilehdl=NULL;

				ExifData.size = MAX_APP1_SIZE;
				ExifData.addr = TempBuf;
				if (EXIF_ER_OK == EXIF_ParseExif(EXIF_HDL_ID_1, &ExifData)) {
					EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
					EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_TIFF_OFFSET_BASE, &uiTiffOffsetBase, sizeof(uiTiffOffsetBase));
					//find thumbnail
					exifTag.uiTagIfd = EXIF_IFD_1ST;
					exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT;
					if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
						ThumbOffset = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian) + uiTiffOffsetBase;
						exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT_LENGTH;
						if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
							ThumbSize = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);
						}
					}
					#if _TODO
					sprintf(filepath, "/mnt/sd/%s", "thumb.jpg");
					if ((f_out = fopen(filepath, "w")) == NULL){
						DBG_ERR("open thumb.jpg failed");
					}
					if (ThumbOffset){
						fwrite((UINT8*)(TempBuf + ThumbOffset), ThumbSize , 1, f_out);
					}else{
						fwrite((UINT8*)TempBuf, ThumbSize, 1, f_out);
					}
					fclose(f_out);
					#endif

				} else {
//#NT#2016/02/15#Ben Wang -begin
//#NEW,re-encode thumbnail for the photo that doesn't have EXIF.
					IdrToJpg.uiJpgWidth  = XML_THUMB_W;
					IdrToJpg.uiJpgHeight = XML_THUMB_H;
					IdrToJpg.uiDAR = MEDIAREC_DAR_DEFAULT;
					iResult = XML_MakeJpgFromIFrame(tempPath, &IdrToJpg, NULL, FALSE, FALSE, 0); // IVOT_N12044_CO-191
					if(gMovFilehdl ){
						FileSys_CloseFile(gMovFilehdl);
						gMovFilehdl=NULL;
					}
					if (iResult == E_OK) {
						ThumbOffset = 0;//means NOT from EXIF
						ThumbBuf = IdrToJpg.uiJpgBsAddr;
						ThumbSize = IdrToJpg.uiJpgBsSize;
					} else {
						ThumbBuf = 0;
						remain = 0;
					}
				}

				if (ThumbSize) {
					if (ThumbSize < bufflen) {
						//hwmem_open();
						if (ThumbOffset) {
							memcpy((void*)buf, (void *)(TempBuf + ThumbOffset), ThumbSize);
							//hwmem_memcpy((UINT32)buf, TempBuf + ThumbOffset, ThumbSize);
						} else {
						    memcpy((void*)buf, (void *)ThumbBuf, ThumbSize);
							//hwmem_memcpy((UINT32)buf, ThumbBuf, ThumbSize);
						}
						//hwmem_close();
						*bufSize = ThumbSize;
						//debug_msg("photo thumb trans %d\r\n",*bufSize);

					} else {
						result = WIFIAPP_RET_NOBUF;
						DBG_ERR("size too large\r\n");
					}
				}
			} else {
				result = WIFIAPP_RET_NOFILE;
				DBG_ERR("no %s\r\n", tempPath);
			}

			ThumbBuf = 0; // JPEG thumbnail is very samll. It just takes one time to transfer.
		}
	}
    //coverity[assigned_value]
    ThumbSize = ThumbSize * ThumbOffset * (*buf) * ThumbBuf * remain * bufflen; //invalid code and only for compiled error.
	if (ThumbBuf) {
		if (remain > bufflen) {
			#if 0
			hwmem_open();
			hwmem_memcpy((UINT32)buf, ThumbBuf + bufflen * segmentCount, bufflen);
			hwmem_close();
			#else
			memcpy((void *)buf, (void*)(ThumbBuf + bufflen * segmentCount), bufflen);
			#endif
			*bufSize = bufflen;
			remain -= bufflen;
			//XML_RelPrivateMem();
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		} else {
		    #if 0
			hwmem_open();
			hwmem_memcpy((UINT32)buf, ThumbBuf + bufflen * segmentCount, remain);
			hwmem_close();
			#else
			memcpy((void *)buf, (void *)(ThumbBuf + bufflen * segmentCount), remain);
			#endif
			*bufSize = remain;
			remain = 0;
			ThumbBuf = 0;
			//debug_msg("last trans ok\r\n");
		}
	}
//GET_THUMBNAIL_RET:
//#NT#2016/02/15#Ben Wang -end
	if (result != 0) {
		// result OK return data,fail return err status
		XML_DefaultFormat(WIFIAPP_CMD_THUMB, result, bufAddr, bufSize, mimeType);
	}
	XML_RelPrivateMem();
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
int XML_GetMovieFileInfo(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT32            TempBuf;
	UINT32            result = 0;
	GXVIDEO_INFO      VideoInfo;
	char              szFileInfo[XML_PATH_LEN];
	char              tempPath[XML_PATH_LEN];
	char             *pch;

	TempBuf = XML_GetTempMem(POOL_SIZE_XML_TEMP_BUF);
	if (!TempBuf) {
		XML_DefaultFormat(WIFIAPP_CMD_MOVIE_FILE_INFO, WIFIAPP_RET_NOBUF, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	// set the data mimetype
	XML_STRCPY(mimeType, "image/jpeg", CYG_HFS_MIMETYPE_MAXLEN);
	XML_ecos2NvtPath(path, tempPath, sizeof(tempPath));

	pch = strchr(tempPath, '.');

	if (!pch) {
		XML_DefaultFormat(WIFIAPP_CMD_MOVIE_FILE_INFO, WIFIAPP_RET_PAR_ERR, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	if (XML_CheckSub(pch, TYPE_MOV_MP4) || XML_CheckSub(pch, TYPE_TS)) {
		FST_FILE_STATUS FileStat = {0};
		MEM_RANGE WorkBuf = {0};
		UINT32 uiBufferNeeded = 0;


		gMovFilehdl = FileSys_OpenFile(tempPath, FST_OPEN_READ);
		if (gMovFilehdl) {
			FileSys_StatFile(gMovFilehdl, &FileStat);
			GxVidFile_Query1stFrameWkBufSize(&uiBufferNeeded, FileStat.uiFileSize);  // 2016.09.22 unmark by Boyan : we have to call this function to update GxVideo's "guiGxVidTotalFileSize" so the TS format could calculate correct movie second.
			uiBufferNeeded = POOL_SIZE_XML_TEMP_BUF;
			{
				//parse video info for single mode only
				WorkBuf.addr = TempBuf;
				WorkBuf.size = uiBufferNeeded;
				result = GxVidFile_ParseVideoInfo(XML_VdoReadCB, &WorkBuf, (UINT32)FileStat.uiFileSize, &VideoInfo);
				if (result == GXVIDEO_PRSERR_OK) {
					FileSys_CloseFile(gMovFilehdl);
					gMovFilehdl=NULL;
				} else {
					FileSys_CloseFile(gMovFilehdl);
					gMovFilehdl=NULL;
					result = WIFIAPP_RET_EXIF_ERR;
					DBG_ERR("exif error\r\n");
				}
			}
		} else {
			result = WIFIAPP_RET_NOFILE;
			DBG_ERR("no %s\r\n", tempPath);
		}

	} else {
		result = WIFIAPP_RET_FILE_ERROR;
		DBG_ERR("no %s\r\n", tempPath);
	}


	if (result != 0) {
		XML_DefaultFormat(WIFIAPP_CMD_MOVIE_FILE_INFO, result, bufAddr, bufSize, mimeType);
	} else {
		snprintf(szFileInfo, sizeof(szFileInfo), "Width:%d, Height:%d, Length:%d sec", VideoInfo.uiVidWidth, VideoInfo.uiVidHeight, VideoInfo.uiToltalSecs);
		XML_StringResult(WIFIAPP_CMD_MOVIE_FILE_INFO, szFileInfo, bufAddr, bufSize, mimeType);
	}

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetRawEncJpg(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char             *buf = (char *)bufAddr;
	UINT32            bufflen = *bufSize - 512;
	static UINT32     remain = 0;
	static UINT32     uiJpgAddr = 0;
	UINT32            uiJpgSize = 0;
	UINT32            result = 0;


	// set the data mimetype
	XML_STRCPY(mimeType, "image/jpeg", CYG_HFS_MIMETYPE_MAXLEN);

	if (segmentCount == 0) {
		//MovRec_RawEncGetJpgData(&uiJpgAddr, &uiJpgSize, 0);
		#if _TODO
        ImageUnit_Begin(&ISF_VdoEnc, 0);
        uiJpgAddr = ImageUnit_GetParam(&ISF_VdoEnc, ISF_OUT1, VDOENC_PARAM_IMGCAP_GET_JPG_ADDR);
        uiJpgSize = ImageUnit_GetParam(&ISF_VdoEnc, ISF_OUT1, VDOENC_PARAM_IMGCAP_GET_JPG_SIZE);
        DBG_DUMP("ImageApp A=0x%x, S=%d\r\n", uiJpgAddr, uiJpgSize);
        ImageUnit_End();
		#endif

		if (uiJpgAddr) {
			remain = uiJpgSize;
		} else {
			result = WIFIAPP_RET_NOFILE;
			DBG_ERR("no JPG file\r\n");
		}
	}

	if (uiJpgAddr) {
		if (remain > bufflen) {
			//hwmem_open();
			memcpy((void *)buf, (void *)(uiJpgAddr + bufflen * segmentCount), bufflen);
			//hwmem_close();
			*bufSize = bufflen;
			remain -= bufflen;
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		} else { // last data.
			//hwmem_open();
			memcpy((void *)buf, (void *)(uiJpgAddr + bufflen * segmentCount), remain);
			//hwmem_close();
			*bufSize = remain;
			remain = 0;
			uiJpgAddr = 0;
			//debug_msg("last trans ok\r\n");
		}
	}

	if (result != 0) {
		XML_DefaultFormat(WIFIAPP_CMD_MOVIE_GET_RAWENC_JPG, result, bufAddr, bufSize, mimeType);
	}

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetVersion(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	XML_StringResult(WIFIAPP_CMD_VERSION, Prj_GetVersionString(), bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetLiveViewFmt(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;
	UINT32 strm_codec_type = _CFG_CODEC_H264;

	XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");


	ImageApp_MovieMulti_GetParam(_CFG_STRM_ID_1, MOVIEMULTI_PARAM_CODEC, &strm_codec_type);
	// Check movie live view streaming format and assign corresponding streaming link.
	if (strm_codec_type == _CFG_CODEC_MJPG) {
		XML_snprintf(&buf, &xmlBufSize, "<MovieLiveViewLink>http://%s:8192</MovieLiveViewLink>\n", UINet_GetIP());
	} else {
		XML_snprintf(&buf, &xmlBufSize, "<MovieLiveViewLink>rtsp://%s/xxx.mov</MovieLiveViewLink>\n", UINet_GetIP());
	}

	//not photo mode, ImageApp_Photo not setup
	//ImageApp_Photo_Config(PHOTO_CFG_STRM_INFO, (UINT32)(UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1)));
	//if (ImageApp_Photo_GetConfig(PHOTO_CFG_STRM_TYPE) == PHOTO_STRM_TYPE_HTTP) {
	if (UIAppPhoto_get_StreamConfig(UIAPP_PHOTO_STRM_ID_1)->strm_type== PHOTO_STRM_TYPE_HTTP) {
		XML_snprintf(&buf, &xmlBufSize, "<PhotoLiveViewLink>http://%s:8192</PhotoLiveViewLink>\n", UINet_GetIP());
	} else {
		XML_snprintf(&buf, &xmlBufSize, "<PhotoLiveViewLink>rtsp://%s/xxx.mov</PhotoLiveViewLink>\n", UINet_GetIP());
	}

	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_FileList(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
#if CREATE_PHOTO_FILEDB
	PFILEDB_INIT_OBJ        pFDBInitObj = &gWifiFDBInitObj;
#endif
	static FILEDB_HANDLE    FileDBHandle = 0;
	static UINT32           gIndex;
	UINT32                  fileCount, i, bufflen = *bufSize;
	char                   *buf = (char *)bufAddr;
	UINT32                  xmlBufSize = *bufSize;

	Perf_Mark();
#if CREATE_PHOTO_FILEDB
	if ((segmentCount == 0) && (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO)) {
		pFDBInitObj->u32MemAddr = XML_GetTempMem(POOL_SIZE_FILEDB);

		if (!pFDBInitObj->u32MemAddr) {
			XML_DefaultFormat(WIFIAPP_CMD_FILELIST, WIFIAPP_RET_NOBUF, bufAddr, bufSize, mimeType);
			return CYG_HFS_CB_GETDATA_RETURN_OK;
		}

		pFDBInitObj->u32MemSize = POOL_SIZE_FILEDB;
		FileDBHandle = FileDB_Create(pFDBInitObj);
		DBG_IND("createTime = %04d ms \r\n", Perf_GetDuration() / 1000);
		Perf_Mark();
		FileDB_SortBy(FileDBHandle, FILEDB_SORT_BY_MODDATE, FALSE);
		DBG_IND("sortTime = %04d ms \r\n", Perf_GetDuration() / 1000);
	} else //PRIMARY_MODE_PLAYBACK,PRIMARY_MODE_MOVIE use default FileDB 0
#endif
	{
		FileDBHandle = 0;
		FileDB_Refresh(0);
	}
	//FileDB_DumpInfo(FileDBHandle);

	if (segmentCount == 0) {
		XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");
	}

	fileCount = FileDB_GetTotalFileNum(FileDBHandle);
	DBG_DUMP("fileCount  %d\r\n", fileCount);

	if (segmentCount == 0) {
		// reset some global variables
		gIndex = 0;
	}
	DBG_IND("gIndex = %d %d \r\n", gIndex, fileCount);
	for (i = gIndex; i < fileCount; i++) {
		PFILEDB_FILE_ATTR        pFileAttr;
		// check buffer length , reserved 512 bytes
		// should not write data over buffer length
		if ((HFS_U32)buf - bufAddr > bufflen - 512) {
			DBG_IND("totallen=%d >  bufflen(%d)-512\r\n", (HFS_U32)buf - bufAddr, bufflen);
			*bufSize = (HFS_U32)(buf) - bufAddr;
			gIndex = i;
			return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
		}
		// get dcf file
		pFileAttr = FileDB_SearhFile(FileDBHandle, i);
		if (pFileAttr) {
			//#NT#2016/03/30#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//debug_msg("file %d %s %d\r\n",i,pFileAttr->filePath,pFileAttr->fileSize);
			DBG_IND("file %d %s %lld\r\n", i, pFileAttr->filePath, pFileAttr->fileSize64);
			//#NT#2016/03/30#Nestor Yang -end

			XML_snprintf(&buf, &xmlBufSize, "<ALLFile><File>\n<NAME>%s</NAME>\n<FPATH>%s</FPATH>\n", pFileAttr->filename, pFileAttr->filePath);
			//#NT#2016/03/30#Nestor Yang -begin
			//#NT# Support fileSize larger than 4GB
			//XML_snprintf(&buf,&xmlBufSize,"<SIZE>%d</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n<ATTR>%d</ATTR></File>\n</ALLFile>\n",pFileAttr->fileSize,(pFileAttr->lastWriteDate <<16) + pFileAttr->lastWriteTime,
			XML_snprintf(&buf, &xmlBufSize, "<SIZE>%lld</SIZE>\n<TIMECODE>%ld</TIMECODE>\n<TIME>%04d/%02d/%02d %02d:%02d:%02d</TIME>\n<ATTR>%d</ATTR></File>\n</ALLFile>\n", pFileAttr->fileSize64, (pFileAttr->lastWriteDate << 16) + pFileAttr->lastWriteTime,
						 FAT_GET_YEAR_FROM_DATE(pFileAttr->lastWriteDate), FAT_GET_MONTH_FROM_DATE(pFileAttr->lastWriteDate), FAT_GET_DAY_FROM_DATE(pFileAttr->lastWriteDate),
						 FAT_GET_HOUR_FROM_TIME(pFileAttr->lastWriteTime), FAT_GET_MIN_FROM_TIME(pFileAttr->lastWriteTime), FAT_GET_SEC_FROM_TIME(pFileAttr->lastWriteTime),
						 pFileAttr->attrib);
			//#NT#2016/03/30#Nestor Yang -end
		}
	}
	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;
#if CREATE_PHOTO_FILEDB
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO) {
		FileDB_Release(FileDBHandle);
	}
#endif
	return CYG_HFS_CB_GETDATA_RETURN_OK;

}


void XML_DefaultFormat(UINT32 cmd, UINT32 ret, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType)
{
	UINT32 bufflen = *bufSize - 512;
	char  *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_RET, cmd, ret);
	*bufSize = (HFS_U32)(buf) - bufAddr;

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}
}

void XML_StringResult(UINT32 cmd, char *str, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType)
{
	UINT32 bufflen = *bufSize - 512;
	char  *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_STR, cmd, 0, str); //status OK
	*bufSize = (HFS_U32)(buf) - bufAddr;

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}
}

void XML_ValueResult(UINT32 cmd, UINT64 value, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType)
{
	UINT32 bufflen = *bufSize - 512;
	char  *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_VALUE, cmd, 0, value); //status OK
	*bufSize = (HFS_U32)(buf) - bufAddr;

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}
}

int XML_QueryCmd_CurSts(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	static UINT32     uiCmdIndex;
	char             *buf = (char *)bufAddr;
	FST_FILE          filehdl;
	char              pFilePath[64];
	UINT32            fileLen = *bufSize - 512;
	UINT32            bufflen = *bufSize - 512;
	WIFI_CMD_ENTRY   *appCmd = 0;
	UINT32            xmlBufSize = *bufSize;
	INT32             iret;

	// set the data mimetype
	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	snprintf(pFilePath, sizeof(pFilePath), "%s", QUERY_CMD_CUR_STS_XML_PATH); //html of all command status list
	filehdl = FileSys_OpenFile(pFilePath, FST_OPEN_READ);
	if (filehdl) {
		iret = FileSys_ReadFile(filehdl, (UINT8 *)buf, &fileLen, 0, 0);
		if (iret != 0) {
			DBG_ERR("read file fail!\r\n");
		}
		FileSys_CloseFile(filehdl);
		*bufSize = fileLen;
		*(buf + fileLen) = '\0';

		uiCmdIndex = 0;
	} else {
		if (segmentCount == 0) {
			XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
			uiCmdIndex = 0;
		}

		XML_snprintf(&buf, &xmlBufSize, "<Function>\n");
		appCmd = WifiCmd_GetExecTable();
		while (appCmd[uiCmdIndex].cmd != 0) {
			if (appCmd[uiCmdIndex].UIflag != FL_NULL) {
#if (WIFI_FINALCAM_APP_STYLE == DISABLE)
				XML_snprintf(&buf, &xmlBufSize, DEF_XML_CMD_CUR_STS, appCmd[uiCmdIndex].cmd, UI_GetData(appCmd[uiCmdIndex].UIflag));
#else
				UINT32 uiRecSizeIdx = 0;

				if (appCmd[uiCmdIndex].UIflag != FL_MOVIE_SIZE) {
					XML_snprintf(&buf, &xmlBufSize, DEF_XML_CMD_CUR_STS, appCmd[uiCmdIndex].cmd, UI_GetData(appCmd[uiCmdIndex].UIflag));
				} else {
					// Transfer movie size index for FinalCam APP.
					switch (UI_GetData(appCmd[uiCmdIndex].UIflag)) {
					default:
					case MOVIE_SIZE_FRONT_1920x1080P30:   // 1920 x 1080
						uiRecSizeIdx = FINALCAM_MOVIE_REC_SIZE_1080P;
						break;

					case MOVIE_SIZE_FRONT_1280x720P30:    // 1280 x 720
						uiRecSizeIdx = FINALCAM_MOVIE_REC_SIZE_720P;
						break;

					case MOVIE_SIZE_FRONT_848x480P30:    // 848 x 480
						uiRecSizeIdx = FINALCAM_MOVIE_REC_SIZE_WVGA;
						break;

					case MOVIE_SIZE_FRONT_640x480P30:     // 640 x 480
						uiRecSizeIdx = FINALCAM_MOVIE_REC_SIZE_VGA;
						break;
					}

					XML_snprintf(&buf, &xmlBufSize, DEF_XML_CMD_CUR_STS, appCmd[uiCmdIndex].cmd, uiRecSizeIdx);
				}
#endif

				//debug_msg(DEF_XML_CMD_CUR_STS, appCmd[uiCmdIndex].cmd, UI_GetData(appCmd[uiCmdIndex].UIflag));
			}
			uiCmdIndex++;

			// check buffer length , reserved 512 bytes
			// should not write data over buffer length
			if ((HFS_U32)buf - bufAddr > bufflen - 512) {
				DBG_IND("totallen=%d >  bufflen(%d)-512\r\n", (HFS_U32)buf - bufAddr, bufflen);
				*bufSize = (HFS_U32)(buf) - bufAddr;
				return CYG_HFS_CB_GETDATA_RETURN_CONTINUE;
			}
		}

		XML_snprintf(&buf, &xmlBufSize, "</Function>\n");
		*bufSize = (HFS_U32)(buf) - bufAddr;
	}

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}


	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetHeartBeat(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	XML_DefaultFormat(WIFIAPP_CMD_HEARTBEAT, 0, bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetFreePictureNum(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
#if (PHOTO_MODE==ENABLE)
	XML_ValueResult(WIFIAPP_CMD_FREE_PIC_NUM, PhotoExe_GetFreePicNum(), bufAddr, bufSize, mimeType);
#else
	XML_ValueResult(WIFIAPP_CMD_FREE_PIC_NUM, 0, bufAddr, bufSize, mimeType);
#endif
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
int XML_GetMaxRecordTime(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT32 sec = Movie_GetFreeSec();
	XML_ValueResult(WIFIAPP_CMD_MAX_RECORD_TIME, sec, bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetDiskFreeSpace(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	DBG_IND("FileSys_GetDiskInfo(FST_INFO_DISK_SIZE) %lld\r\n", FileSys_GetDiskInfo(FST_INFO_DISK_SIZE));
	XML_ValueResult(WIFIAPP_CMD_DISK_FREE_SPACE, FileSys_GetDiskInfo(FST_INFO_FREE_SPACE), bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_DeleteOnePicture(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char    fullPath[XML_PATH_LEN] = {0};
	int     ret;
	char   *pch = 0;
	UINT8   attrib = 0;

	pch = strrchr(argument, '&');
	if (pch) {
		if (strncmp(pch, PARS_STR, strlen(PARS_STR)) == 0) {
			sscanf_s(pch + strlen(PARS_STR), "%s", &fullPath, sizeof(fullPath));
		}
		DBG_IND("fullPath path=%s\r\n", fullPath);
	} else {
		XML_DefaultFormat(WIFIAPP_CMD_DELETE_ONE, WIFIAPP_RET_FILE_LOCKED, bufAddr, bufSize, mimeType);
	}

	ret = FileSys_GetAttrib(fullPath, &attrib);
	if ((ret == E_OK) && (M_IsReadOnly(attrib) == TRUE)) {
		DBG_IND("File Locked\r\n");
		XML_DefaultFormat(WIFIAPP_CMD_DELETE_ONE, WIFIAPP_RET_FILE_LOCKED, bufAddr, bufSize, mimeType);
	} else if (ret == FST_STA_FILE_NOT_EXIST) {
		DBG_IND("File not existed\r\n");
		XML_DefaultFormat(WIFIAPP_CMD_DELETE_ONE, WIFIAPP_RET_NOFILE, bufAddr, bufSize, mimeType);
	} else {
		ret = FileSys_DeleteFile(fullPath);
		DBG_IND("ret = %d\r\n", ret);
		if (ret != FST_STA_OK) {
			XML_DefaultFormat(WIFIAPP_CMD_DELETE_ONE, WIFIAPP_RET_FILE_ERROR, bufAddr, bufSize, mimeType);
		} else {
#if 0
			Index = FileDB_GetIndexByPath(FileDBHdl, fullPath);
			DBG_IND("Index = %04d\r\n", Index);
			if (Index != FILEDB_SEARCH_ERR) {
				FileDB_DeleteFile(FileDBHdl, Index);
			}
#endif

			XML_DefaultFormat(WIFIAPP_CMD_DELETE_ONE, WIFIAPP_RET_OK, bufAddr, bufSize, mimeType);
		}
	}

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_DeleteAllPicture(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char  tempPath[XML_PATH_LEN];
	BOOL  isCurrFileReadOnly = FALSE;
	UINT32 Index;
	PFILEDB_FILE_ATTR FileAttr = NULL;
	INT32             ret;
	INT32             FileNum, i;
#if CREATE_PHOTO_FILEDB
	PFILEDB_INIT_OBJ        pFDBInitObj = &gWifiFDBInitObj;
#endif
	FILEDB_HANDLE      FileDBHandle = 0;

	Perf_Mark();

#if CREATE_PHOTO_FILEDB
	if ((segmentCount == 0) && (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO)) {
		pFDBInitObj->u32MemAddr = XML_GetTempMem(POOL_SIZE_FILEDB);

		if (!pFDBInitObj->u32MemAddr) {
			XML_DefaultFormat(WIFIAPP_CMD_DELETE_ALL, WIFIAPP_RET_NOBUF, bufAddr, bufSize, mimeType);
			return CYG_HFS_CB_GETDATA_RETURN_OK;
		}

		pFDBInitObj->u32MemSize = POOL_SIZE_FILEDB;
		FileDBHandle = FileDB_Create(pFDBInitObj);
		DBG_IND("createTime = %04d ms \r\n", Perf_GetDuration() / 1000);
		Perf_Mark();
		FileDB_SortBy(FileDBHandle, FILEDB_SORT_BY_MODDATE, FALSE);
		DBG_IND("sortTime = %04d ms \r\n", Perf_GetDuration() / 1000);
	} else //PRIMARY_MODE_PLAYBACK,PRIMARY_MODE_MOVIE use default FileDB 0
#endif
	{
		FileDBHandle = 0;
		FileDB_Refresh(0);
	}
	//FileDB_DumpInfo(FileDBHandle);

	FileAttr = FileDB_CurrFile(FileDBHandle);
	if (FileAttr && M_IsReadOnly(FileAttr->attrib)) {
		isCurrFileReadOnly = TRUE;
		XML_STRCPY(tempPath, FileAttr->filePath, sizeof(tempPath));
	}

	FileNum = FileDB_GetTotalFileNum(FileDBHandle);
	for (i = FileNum - 1; i >= 0; i--) {
		FileAttr = FileDB_SearhFile(FileDBHandle, i);
		if (FileAttr) {
			if (M_IsReadOnly(FileAttr->attrib)) {
				//DBG_IND("File Locked\r\n");
				DBG_IND("File %s is locked\r\n", FileAttr->filePath);
				continue;
			}
			ret = FileSys_DeleteFile(FileAttr->filePath);
			//DBG_IND("i = %04d path=%s\r\n",i,FileAttr->filePath);
			if (ret != FST_STA_OK) {
				DBG_IND("Delete %s failed\r\n", FileAttr->filePath);
			} else {
				FileDB_DeleteFile(FileDBHandle, i);
				DBG_IND("Delete %s OK\r\n", FileAttr->filePath);
			}
		} else {
			DBG_IND("FileAttr not existed\r\n");
		}
	}

	if (isCurrFileReadOnly) {
		Index = FileDB_GetIndexByPath(FileDBHandle, tempPath);
		FileDB_SearhFile(FileDBHandle, Index);
	}

#if CREATE_PHOTO_FILEDB
	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_PHOTO) {
		FileDB_Release(FileDBHandle);
	}
#endif

	XML_DefaultFormat(WIFIAPP_CMD_DELETE_ALL, WIFIAPP_RET_OK, bufAddr, bufSize, mimeType);

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

//#NT#2016/03/23#Isiah Chang -begin
//#NT#add new Wi-Fi UI flow.
extern UINT32 WifiCmd_WaitFinish(FLGPTN waiptn);

int XML_APP_STARTUP(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
#if 0 // Return fail if it's not allowed to run Wi-Fi APP command.
	XML_DefaultFormat(WIFIAPP_CMD_APP_STARTUP, WIFIAPP_RET_FAIL, bufAddr, bufSize, mimeType);
#else
	XML_DefaultFormat(WIFIAPP_CMD_APP_STARTUP, WIFIAPP_RET_OK, bufAddr, bufSize, mimeType);

//#NT#2016/05/11#Isiah Chang -begin
//#NT#clear wifi flag before waiting for it.
	WifiCmd_ClrFlg(WIFIFLAG_MODE_DONE);
//#NT#2016/05/11#Isiah Chang -end
	Ux_PostEvent(NVTEVT_SYSTEM_MODE, 2, PRIMARY_MODE_MOVIE, SYS_SUBMODE_WIFI);

	WifiCmd_WaitFinish(WIFIFLAG_MODE_DONE); // Moved to WifiAppCmd.c. Fill waiting flag in WIFI_CMD_ITEM table.

#endif

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
//#NT#2016/03/23#Isiah Chang -end

int XML_GetPictureEnd(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char             *buf = (char *)bufAddr;
	UINT32            result = 0;
	UINT32            FreePicNum = 0;
	char             *pFilePath = 0;
	char             *pFileName = 0;
	UINT32            xmlBufSize = *bufSize;

	// set the data mimetype
	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	DBG_IND("path = %s, argument -> %s, mimeType= %s, bufsize= %d, segmentCount= %d\r\n", path, argument, mimeType, *bufSize, segmentCount);

	#if _TODO
	if (UIStorageCheck(STORAGE_CHECK_FOLDER_FULL, NULL)) {
		result = WIFIAPP_RET_FOLDER_FULL;
		DBG_ERR("folder full\r\n");
	} else if (UIStorageCheck(STORAGE_CHECK_FULL, &FreePicNum)) {
		result = WIFIAPP_RET_STORAGE_FULL;
		DBG_ERR("storage full\r\n");
	} else
	#endif
	if (System_GetState(SYS_STATE_FS) == FS_DISK_ERROR) {
		result = WIFIAPP_RET_FILE_ERROR;
		DBG_ERR("write file fail\r\n");
	} else {
	    #if 0
        //not support get file after capture,get from file list
        result = 0;
        #else
		pFilePath = ImageApp_Photo_GetLastWriteFilePath();
		DBG_IND("path %s %d",pFilePath,strlen(pFilePath));
		if (pFilePath && strlen(pFilePath)) {
			pFileName = strrchr(pFilePath, '\\') + 1;
		} else {
			result = WIFIAPP_RET_FILE_ERROR;
			DBG_ERR("write file fail, path %s %d\r\n",pFilePath,strlen(pFilePath));
		}
        #endif
	}
	if (result == 0) {
		XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
		XML_snprintf(&buf, &xmlBufSize, "<Function>\n");
		XML_snprintf(&buf, &xmlBufSize, DEF_XML_CMD_CUR_STS, WIFIAPP_CMD_CAPTURE, result);
		XML_snprintf(&buf, &xmlBufSize, "<File>\n<NAME>%s</NAME>\n<FPATH>%s</FPATH></File>\n",pFileName,pFilePath);
		XML_snprintf(&buf, &xmlBufSize, "<FREEPICNUM>%d</FREEPICNUM>\n", FreePicNum);
		XML_snprintf(&buf, &xmlBufSize, "</Function>\n");
		*bufSize = (HFS_U32)(buf) - bufAddr;
	} else {
		XML_DefaultFormat(WIFIAPP_CMD_CAPTURE, result, bufAddr, bufSize, mimeType);
	}
	DBG_DUMP("0x%x", bufAddr);
	return CYG_HFS_CB_GETDATA_RETURN_OK;

}

int XML_GetBattery(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	XML_ValueResult(WIFIAPP_CMD_GET_BATTERY, GetBatteryLevel(), bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
int XML_HWCapability(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT64 value = 0;
#if (TV_FUNC == ENABLE)
	value |= HW_TV_ENABLE;
#endif
#if (GSENSOR_FUNCTION == ENABLE)
	value |= HW_GSENDOR_ENABLE;
#endif
#if(WIFI_UI_FLOW_VER == WIFI_UI_VER_2_0)
	value |= HW_WIFI_SOCKET_HANDSHAKE_ENABLE;
#endif
#if 0//(USOCKET_CLIENT == ENABLE) // Enable client socket notification for Android APP to get DHCP client's IP.
	value |= HW_WIFI_CLIENT_SOCKET_NOTIFY_ENABLE;
#endif
	//auto recording is not support after 96680 platform
	value |= HW_SET_AUTO_RECORDING_DISABLE;

	//DBG_ERR("XML_HWCapability value %x\r\n",value);
	XML_ValueResult(WIFIAPP_CMD_GET_HW_CAP, value, bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}


int XML_GetMovieRecStatus(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	INT32 iStatus = WiFiCmd_GetStatus();

	if (iStatus >= WIFI_MOV_ST_MAX) {
		DBG_ERR("Rec Sts err\r\n");
		return CYG_HFS_CB_GETDATA_RETURN_ERROR;
	}

	if (iStatus == WIFI_MOV_ST_RECORD) {
		XML_ValueResult(WIFIAPP_CMD_MOVIE_RECORDING_TIME, FlowMovie_GetRecCurrTime(), bufAddr, bufSize, mimeType);
	} else {
		XML_ValueResult(WIFIAPP_CMD_MOVIE_RECORDING_TIME, 0, bufAddr, bufSize, mimeType);
	}

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetCardStatus(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	// check card inserted
#if 0
	XML_ValueResult(WIFIAPP_CMD_GET_CARD_STATUS, System_GetState(SYS_STATE_CARD), bufAddr, bufSize, mimeType);
#else
	//Fix CID 32318
    UINT32 cardStatus = System_GetState(SYS_STATE_CARD);

	if (cardStatus  != CARD_REMOVED) {
		#if 1//_TODO
		if (UIStorageCheck(STORAGE_CHECK_ERROR, NULL) == FALSE) {
			XML_ValueResult(WIFIAPP_CMD_GET_CARD_STATUS, cardStatus, bufAddr, bufSize, mimeType);
		} else
		#endif
		{ // Filesystem error happened.
			// Return WIFIAPP_CMD_GET_CARD_STATUS + UI_FSStatus.
			/*
			typedef enum
			{
			    FS_DISK_ERROR = 0,
			    FS_UNKNOWN_FORMAT,
			    FS_UNFORMATTED,
			    FS_NOT_INIT,
			    FS_INIT_OK,
			    FS_NUM_FULL
			} FS_STATUS; */
			// coverity[check_return]:this function is for get card and fs status
			XML_ValueResult(WIFIAPP_CMD_GET_CARD_STATUS, WIFIAPP_CMD_GET_CARD_STATUS + System_GetState(SYS_STATE_FS), bufAddr, bufSize, mimeType);
		}
	} else {
		XML_ValueResult(WIFIAPP_CMD_GET_CARD_STATUS, cardStatus, bufAddr, bufSize, mimeType);
	}
#endif
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

//#NT#2016/06/01#Isiah Chang -begin
//#NT#add a Wi-Fi command to get current mode status.
int XML_GetCntModeStatus(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	INT32 iCurMode = System_GetState(SYS_STATE_CURRMODE);

	if (iCurMode == SYS_MODE_UNKNOWN) {
		DBG_ERR("Mode err or Sts err!\r\n");
		return CYG_HFS_CB_GETDATA_RETURN_ERROR;
	}

	if (iCurMode == PRIMARY_MODE_MOVIE) {
	// coverity[check_return]:this function is getting record status
#if(SENSOR_CAPS_COUNT == 1)
	    UINT32 isRec = ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1) ;
#else
	    UINT32 isRec = ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1) | ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_2) ;
#endif
		XML_ValueResult(WIFIAPP_CMD_GET_MODE_STAUTS, isRec, bufAddr, bufSize, mimeType);
	} else if (iCurMode == PRIMARY_MODE_PLAYBACK) {
		XML_ValueResult(WIFIAPP_CMD_GET_MODE_STAUTS, 3, bufAddr, bufSize, mimeType);
	} else if (iCurMode == PRIMARY_MODE_PHOTO) {
		XML_ValueResult(WIFIAPP_CMD_GET_MODE_STAUTS, 4, bufAddr, bufSize, mimeType);
	} else {
		INT32 iCurStatus = WiFiCmd_GetStatus();

        if ((iCurStatus != WIFI_MOV_ST_IDLE) && (iCurStatus != WIFI_MOV_ST_LVIEW) && (iCurStatus != WIFI_MOV_ST_RECORD)) {
            DBG_ERR("iCurStatus = %d\r\n", iCurStatus);
        }
		XML_ValueResult(WIFIAPP_CMD_GET_MODE_STAUTS, iCurStatus, bufAddr, bufSize, mimeType);
	}

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
//#NT#2016/06/01#Isiah Chang -end
int XML_GetDownloadURL(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	XML_StringResult(WIFIAPP_CMD_GET_DOWNLOAD_URL, WIFI_APP_DOWNLOAD_URL, bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

int XML_GetUpdateFWPath(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char url[XML_PATH_LEN];
	char FWPath[XML_PATH_LEN];

	XML_Nvt2ecosPath(FW_UPDATE_NAME, FWPath, sizeof(FWPath));
	snprintf(url, sizeof(url), "http://%s%s", UINet_GetIP(), (char *)FWPath);

	XML_StringResult(WIFIAPP_CMD_GET_UPDATEFW_PATH, url, bufAddr, bufSize, mimeType);
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
//#NT#2016/06/06#Charlie Chang -begin
//#NT# support wifi AP search
static NVT_SS_STATUS_T UINet_ss_status;

int XML_GetWifiAP(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{

	char *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;
	WIFI_AUTH_TYPE auth_type = WIFI_AUTH_TYPE_NONE;
	char ssidbuf[40], tmp2Buf[20] = {0}, tmp3Buf[20] = {0};
	int i = 0;
	struct nvt_bss_desc *pBss;

	UINet_SiteSurvey("wlan0", &UINet_ss_status);

	XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");

	XML_snprintf(&buf, &xmlBufSize, "<List>\n");
	for (i = 0; i < UINet_ss_status.number && UINet_ss_status.number != 0xff; i++) {
		pBss = &UINet_ss_status.bssdb[i];
		DBG_DUMP("search AP===============\r\n");
		XML_snprintf(&buf, &xmlBufSize, "<AP_index>\n");
		memset(ssidbuf, '\0', sizeof(ssidbuf));
		memcpy(ssidbuf, pBss->ssid, pBss->ssidlen);
		ssidbuf[pBss->ssidlen] = '\0';
		DBG_DUMP("%s\r\n", ssidbuf);
		XML_snprintf(&buf, &xmlBufSize, "<SSID>%s</SSID>\n", ssidbuf);


		DBG_DUMP("MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",
				 pBss->bssid[0], pBss->bssid[1], pBss->bssid[2],
				 pBss->bssid[3], pBss->bssid[4], pBss->bssid[5]);


		// refer to get_security_info() for details
		if ((pBss->capability & nvt_cPrivacy) == 0) {
			/////NONE
			auth_type = WIFI_AUTH_TYPE_NONE;
		} else {
			if (pBss->t_stamp[0] == 0) {
				////WEP
				DBG_DUMP("\r\n");
			} else {
				int wpa_exist = 0, idx = 0;
				if (pBss->t_stamp[0] & 0x0000ffff) {

					////WPA
					idx = sprintf(tmp2Buf, "%s", "WPA");
					if (((pBss->t_stamp[0] & 0x0000f000) >> 12) == 0x4) {
						////PSK
						idx += sprintf(tmp2Buf + idx, "%s", "-PSK");
					} else if (((pBss->t_stamp[0] & 0x0000f000) >> 12) == 0x2) {
						/////-1X
						idx += sprintf(tmp2Buf + idx, "%s", "-1X");
					}

					wpa_exist = 1;

					if (((pBss->t_stamp[0] & 0x00000f00) >> 8) == 0x5) {
						//////AES/TKIP
						sprintf(tmp3Buf, "%s", "AES/TKIP");
						auth_type = WIFI_AUTH_TYPE_WPA_AES_TKIP;
					} else if (((pBss->t_stamp[0] & 0x00000f00) >> 8) == 0x4) {
						/////AES
						sprintf(tmp3Buf, "%s", "AES");
						auth_type = WIFI_AUTH_TYPE_WPA_AES;
					} else if (((pBss->t_stamp[0] & 0x00000f00) >> 8) == 0x1) {
						////TKIP
						sprintf(tmp3Buf, "%s", "TKIP");
						auth_type = WIFI_AUTH_TYPE_WPA_TKIP;
					}
				}
				if (pBss->t_stamp[0] & 0xffff0000) {

					if (wpa_exist) {
						idx += sprintf(tmp2Buf + idx, "%s", "/");
					}
					////WPA2
					idx += sprintf(tmp2Buf + idx, "%s", "WPA2");
					if (((pBss->t_stamp[0] & 0xf0000000) >> 28) == 0x4) {
						////-PSK
						idx += sprintf(tmp2Buf + idx, "%s", "-PSK");
					} else if (((pBss->t_stamp[0] & 0xf0000000) >> 28) == 0x2) {
						////-1X;
						idx += sprintf(tmp2Buf + idx, "%s", "-1X");
					}

					if (((pBss->t_stamp[0] & 0x0f000000) >> 24) == 0x5) {
						////AES/TKIP
						sprintf(tmp3Buf, "%s", "AES/TKIP");
						auth_type = WIFI_AUTH_TYPE_WPA2_AES_TKIP;
					} else if (((pBss->t_stamp[0] & 0x0f000000) >> 24) == 0x4) {
						////AES
						sprintf(tmp3Buf, "%s", "AES");
						auth_type = WIFI_AUTH_TYPE_WPA2_AES;
					} else if (((pBss->t_stamp[0] & 0x0f000000) >> 24) == 0x1) {
						////TKIP
						sprintf(tmp3Buf, "%s", "TKIP");
						auth_type = WIFI_AUTH_TYPE_WPA2_TKIP;
					}
				}
			}
		}
		DBG_DUMP("%-10s ", tmp3Buf);
		DBG_DUMP("%-16s\r\n", tmp2Buf);
		XML_snprintf(&buf, &xmlBufSize, "<Auth_type>%d</Auth_type>\n", auth_type);

		DBG_DUMP("rssi %d\r\n", pBss->rssi);
		XML_snprintf(&buf, &xmlBufSize, "<RSSI>%d dBm</RSSI>\n", pBss->rssi);

		XML_snprintf(&buf, &xmlBufSize, "</AP_index>\n");
	}
	XML_snprintf(&buf, &xmlBufSize, "</List>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;
	return CYG_HFS_CB_GETDATA_RETURN_OK;

}
//#NT#2016/06/06#Charlie Chang -end
int XML_GetSSID_passphrase(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");

	XML_snprintf(&buf, &xmlBufSize, "<SSID>%s</SSID>\n", UINet_GetSSID());

	XML_snprintf(&buf, &xmlBufSize, "<PASSPHRASE>%s</PASSPHRASE>\n", UINet_GetPASSPHRASE());

	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}


int XML_GetMovieSizeCapability(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char   *buf = (char *)bufAddr;
	TM_ITEM    *pItem = 0;
	TM_OPTION  *pOption;
	UINT16      i = 0;
	UINT32      xmlBufSize = *bufSize;

	pItem = TM_GetItem(&gMovieMenu, IDM_MOVIE_SIZE);
	if (pItem) {
		XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

		XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");
		for (i = 0; i < pItem->Count; i++) {
			pOption = &pItem->pOptions[i];
			if ((pOption->Status & TM_OPTION_STATUS_MASK) == TM_OPTION_ENABLE) {
				XML_snprintf(&buf, &xmlBufSize, "<Item>\n");


				#if defined(_UI_STYLE_LVGL_)
				XML_snprintf(&buf, &xmlBufSize, "<Name>%s</Name>\n", lv_plugin_get_string(pOption->TextId)->ptr);
				#else
				XML_snprintf(&buf, &xmlBufSize, "<Name>%s</Name>\n", UIRes_GetUserString(pOption->TextId));
				#endif


				XML_snprintf(&buf, &xmlBufSize, "<Index>%d</Index>\n", i);

				XML_snprintf(&buf, &xmlBufSize, "<Size>%d*%d</Size>\n", GetMovieSizeWidth_2p(0, i), GetMovieSizeHeight_2p(0, i));

				XML_snprintf(&buf, &xmlBufSize, "<FrameRate>%d</FrameRate>\n", GetMovieFrameRate_2p(0, i));

				XML_snprintf(&buf, &xmlBufSize, "<Type>%d</Type>\n", GetMovieRecType_2p(i));

				XML_snprintf(&buf, &xmlBufSize, "</Item>\n");
			}
		}

		XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
		*bufSize = (HFS_U32)(buf) - bufAddr;
	} else {
		XML_ValueResult(WIFIAPP_CMD_QUERY_MOVIE_SIZE, WIFIAPP_RET_FAIL, bufAddr, bufSize, mimeType);
	}
	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

#define QUERYMENU_CMD_STR "custom=1&cmd=3031&str="
#define SWAP(a,b)           { unsigned long t = a; a = b; b = t; t = 0; }
#define HW2FW_64BIT(X)      (SWAP(*((UINT32 *)(X)), *((UINT32 *)((X)+4))))
#define HW2FW_128BIT(X)     {(SWAP(*((UINT32 *)(X)), *((UINT32 *)((X)+12))));(SWAP(*((UINT32 *)(X+4)),*((UINT32 *)((X)+8))));}

#define DATA_SIZE           16//(8*1024)

#define AES_BLOCK_SIZE      16
#define AES_ECB_KEY_SIZE    16//32
#define AES_CBC_KEY_SIZE    16
#define AES_CTR_KEY_SIZE    16
#define AES_GCM_KEY_SIZE    16
#define AES_GCM_AUTH_SIZE   31

static BOOL AES_CRYP(UINT8 *PlanText, UINT8 *uiKey, UINT8 *CypherText)
{
    #if defined(__FREERTOS)

	crypto_open();
    crypto_setConfig(CRYPTO_CONFIG_ID_MODE, CRYPTO_MODE_AES128);
    crypto_setConfig(CRYPTO_CONFIG_ID_TYPE, CRYPTO_TYPE_ENCRYPT);
    crypto_setKey(uiKey);
    crypto_setInput(PlanText);
    crypto_pio_enable();
    crypto_getOutput(CypherText);

	crypto_close();
	#else
	{
	    int               cfd = -1;
	    struct session_op sess;
	    struct crypt_op   cryp;

	    DBG_DUMP("AES_ECB thread start!\n");

	    /* Open the crypto device */
	    cfd = open("/dev/crypto", O_RDWR, 0);
	    if (cfd < 0) {
	        perror("open(/dev/crypto)");
	        goto exit;
	    }

	    memset(&sess,        0, sizeof(sess));
	    memset(&cryp,        0, sizeof(cryp));

	    /* get crypto session */
	    sess.cipher = CRYPTO_AES_CBC;
	    sess.keylen = AES_ECB_KEY_SIZE;
	    sess.key    = uiKey;
	    if (ioctl(cfd, CIOCGSESSION, &sess)) {
	        perror("ioctl(CIOCGSESSION)");
	        goto free_mem;
	    }

	    //while (aes_ecb_stop == 0)
		{
	        /* Encryption */
	        cryp.ses = sess.ses;
	        cryp.len = DATA_SIZE;
	        cryp.src = PlanText;
	        cryp.dst = CypherText;
	        cryp.iv  = NULL;
	        cryp.op  = COP_ENCRYPT;

	        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
	            perror("ioctl(CIOCCRYPT)");
	            goto free_session;
	        }
	    }

	free_session:
	    /* finish crypto session */
	    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
	        perror("ioctl(CIOCFSESSION)");
	    }

	free_mem:
	    /* Close file descriptor */
	    if (close(cfd))
	        perror("close(cfd)");

	exit:
	    DBG_DUMP("AES_ECB thread exit!\n");
	}
    #endif

	return TRUE;
}

UINT32 Nvt_AuthGen(UINT8 *key, UINT8 *profile, UINT32 profileLen, UINT8 *auth)
{
	UINT32 i = 0, j = 0;
	UINT8 data[16];
	UINT8 genKey[32]; // Enlarge it if using CRYPTO_MODE_AES256.
	UINT8 cypher[16] = {0};

	for (i = 0; i < 4; i++) {
		memcpy(&genKey[i * 4], key, 4);
	}
	// wrap profile into 16-byte value
	memset(data, 0, 16);

	for (i = 0; i < profileLen; i++) {
		data[j++] += profile[i];
		if (j == 16) {
			j = 0;
		}
	}

	AES_CRYP(data, genKey, cypher);

	memset(auth, 0, 2);
	for (i = 0; i < 16; i += 2) {
		auth[0] += cypher[i];
		auth[1] += cypher[i + 1];
	}

#if 1
	for (i = 0; i < 16; i++) {
		DBG_DUMP("%02x ", *(UINT8 *)(genKey + i));
	}
	DBG_DUMP("\r\n");

	for (i = 0; i < 16; i++) {
		DBG_DUMP("%02x ", *(UINT8 *)(data + i));
	}
	DBG_DUMP("\r\n");

	for (i = 0; i < 16; i++) {
		DBG_DUMP("%02x ", *(UINT8 *)(cypher + i));
	}
	DBG_DUMP("\r\n");

	for (i = 0; i < 2; i++) {
		DBG_DUMP("%02x ", *(UINT8 *)(auth + i));
	}
	DBG_DUMP("\r\n");

#endif


	return TRUE;
}

int XML_GetMenuItem(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	char  *buf = (char *)bufAddr;
	WIFI_CMD_ENTRY  *appCmd = 0;
	UINT32          uiCmdIndex = 0;
	WIFIAPPINDEXMAP  *mapTbl = 0;
	WIFIAPPMAP       *wifiAppFlagMap;
	UINT32 i = 0, j = 0;
	UINT8 auth[2] = {0};
	UINT8 key[4] = {0};
	UINT32 xmlBufSize = *bufSize;

	appCmd = WifiCmd_GetExecTable();
	wifiAppFlagMap = WifiCmd_GetMapTbl();

	if ((!appCmd) || (!wifiAppFlagMap)) {
		XML_DefaultFormat(WIFIAPP_CMD_QUERY_MENUITEM, WIFIAPP_RET_FAIL, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);
	XML_snprintf(&buf, &xmlBufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<LIST>\n");

	while (appCmd[uiCmdIndex].cmd != 0) {
		if (appCmd[uiCmdIndex].UIflag != FL_NULL) {
			i = 0;
			while (wifiAppFlagMap[i].uiFlag != FL_NULL) {
				if (wifiAppFlagMap[i].uiFlag == appCmd[uiCmdIndex].UIflag) {
					XML_snprintf(&buf, &xmlBufSize, "<Item>\n");
					XML_snprintf(&buf, &xmlBufSize, "<Cmd>%d</Cmd>\n", appCmd[uiCmdIndex].cmd);
					XML_snprintf(&buf, &xmlBufSize, "<Name>%s</Name>\n", wifiAppFlagMap[i].name);
					XML_snprintf(&buf, &xmlBufSize, "<MenuList>\n");

					mapTbl = wifiAppFlagMap[i].maptbl;
					j = 0;
					while (mapTbl[j].uiIndex != INDEX_END) {
						XML_snprintf(&buf, &xmlBufSize, "<Option>\n");
						XML_snprintf(&buf, &xmlBufSize, "<Index>%d</Index>\n", mapTbl[j].uiIndex);
						XML_snprintf(&buf, &xmlBufSize, "<Id>%s</Id>\n", mapTbl[j].string);
						XML_snprintf(&buf, &xmlBufSize, "</Option>\n");
						j++;
					}
					XML_snprintf(&buf, &xmlBufSize, "</MenuList>\n");
					XML_snprintf(&buf, &xmlBufSize, "</Item>\n");
				}
				i++;
			}
		}
		uiCmdIndex++;
	}

	//DBG_DUMP("Data2 path = %s, argument -> %s, mimeType= %s, bufsize= %d, segmentCount= %d\r\n",path,argument, mimeType, *bufSize, segmentCount);

	if (strncmp(argument, QUERYMENU_CMD_STR, strlen(QUERYMENU_CMD_STR)) == 0) {
		UINT32 keyValue = 0;
		sscanf_s(argument + strlen(QUERYMENU_CMD_STR), "%x", &keyValue);

		key[0] = keyValue >> 24 & 0xFF;
		key[1] = keyValue >> 16 & 0xFF;
		key[2] = keyValue >> 8 & 0xFF;
		key[3] = keyValue & 0xFF;
	} else {
		DBG_ERR("no key\r\n");
		XML_DefaultFormat(WIFIAPP_CMD_QUERY_CUR_STATUS, WIFIAPP_RET_PAR_ERR, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	#if 1//_TODO
	Nvt_AuthGen(key, (UINT8 *)bufAddr, (UINT32)((buf) - bufAddr), auth);
	#else
	DBG_WRN("Nvt_AuthGen to do:%x %x %x %x\r\n", key[0], key[1], key[2], key[3]);
	#endif

	XML_snprintf(&buf, &xmlBufSize, "<CHK>%02X%02X</CHK>\n", auth[0], auth[1]);

	XML_snprintf(&buf, &xmlBufSize, "</LIST>\n");
	*bufSize = (HFS_U32)(buf) - bufAddr;

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}
INT32 WifiCmd_getQueryData(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT32 bufflen = *bufSize - 512;
	char  *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	DBG_DUMP("Data2 path = %s, argument -> %s, mimeType= %s, bufsize= %d, segmentCount= %d\r\n", path, argument, mimeType, *bufSize, segmentCount);

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	XML_snprintf(&buf, &xmlBufSize, QUERY_FMT, WIFI_APP_MANUFACTURER, 1, WIFI_APP_MODLE);
	DBG_DUMP(buf);

	*bufSize = (HFS_U32)(buf) - bufAddr;

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}
	return CYG_HFS_CB_GETDATA_RETURN_OK;

}

int XML_AutoTestCmdDone(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType, HFS_U32 segmentCount)
{
	UINT32            par = 0;
	char              tmpString[32];

	snprintf(tmpString, sizeof(tmpString), "custom=1&cmd=%d&par=", WIFIAPP_CMD_AUTO_TEST_CMD_DONE);
	if (strncmp(argument, tmpString, strlen(tmpString)) == 0) {
		sscanf_s(argument + strlen(tmpString), "%d", &par);
		DBG_DUMP("(`';)\r\n");
	} else {
		*bufSize = 0;
		DBG_ERR("error par\r\n");

		XML_ValueResult(WIFIAPP_CMD_AUTO_TEST_CMD_DONE, WIFIAPP_RET_FAIL, bufAddr, bufSize, mimeType);
		return CYG_HFS_CB_GETDATA_RETURN_OK;
	}

	XML_ValueResult(WIFIAPP_CMD_AUTO_TEST_CMD_DONE, WIFIAPP_RET_OK, bufAddr, bufSize, mimeType);

	return CYG_HFS_CB_GETDATA_RETURN_OK;
}

#define MAX_UPLOAD_FILE_SIZE                 0x00A00000
#define FW_UPLOAD_USAGE_SAVE_AS_FILE         0x00
#define FW_UPLOAD_USAGE_UPDATE_FW_IN_DRAM    0x01
#define FW_UPLOAD_USAGE_SAVE_IN_DRAM         0x02
#if _TODO
static FWSRV_CMD Cmd = {0};
#endif
int XML_UploadFile(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 bufSize, HFS_U32 segmentCount, HFS_PUT_STATUS putStatus)
{
	static      FST_FILE fhdl = 0;
	static      UINT32   uiTempBuf = 0, uiCopiedBytes = 0;
	static      UINT32   uiUsage = FW_UPLOAD_USAGE_SAVE_AS_FILE;
	UINT32      fileSize = 0;
	INT32		ret_fs;
#if _TODO
	FWSRV_ER    fws_er;
#endif
	char        tempPath[XML_PATH_LEN];
	char        tmpString[32];

	DBG_IND("path =%s, argument = %s, bufAddr = 0x%x, bufSize =0x%x , segmentCount  =%d , putStatus = %d\r\n", path, argument, bufAddr, bufSize, segmentCount, putStatus);

	if (putStatus == HFS_PUT_STATUS_ERR) {
		DBG_ERR("receive data error\r\n");
		return CYG_HFS_UPLOAD_FAIL_RECEIVE_ERROR;
	} else {
		if (segmentCount == 0) {
			// Check if upload to a file or DRAM buffer.
			snprintf(tmpString, sizeof(tmpString), "custom=1&cmd=%d&par=", WIFIAPP_CMD_UPLOAD);
			if (strncmp(argument, tmpString, strlen(tmpString)) == 0) {
				sscanf_s(argument + strlen(tmpString), "%d", &uiUsage);
			} else {
				uiUsage = FW_UPLOAD_USAGE_SAVE_AS_FILE;
			}

			if (uiUsage == FW_UPLOAD_USAGE_SAVE_AS_FILE) {
				XML_ecos2NvtPath(path, tempPath, sizeof(tempPath));
				fhdl = FileSys_OpenFile(tempPath, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
				ret_fs = FileSys_SeekFile(fhdl, 0, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("seek file failed\r\n");
				}
			} else {
				uiCopiedBytes = 0;
				uiTempBuf = XML_GetTempMem(MAX_UPLOAD_FILE_SIZE);
                if(!uiTempBuf){
					DBG_ERR("buf not enough\r\n");
					return CYG_HFS_UPLOAD_FAIL_WRITE_ERROR;
                }

			}
		}

		if (uiUsage == FW_UPLOAD_USAGE_SAVE_AS_FILE) {
			fileSize = bufSize;
			ret_fs = FileSys_SeekFile(fhdl, 0, FST_SEEK_END);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("[WifiXML] seek file failed\r\n");
			} else {
				if (FileSys_WriteFile(fhdl, (UINT8 *)bufAddr, &fileSize, 0, NULL) != FST_STA_OK) {
					DBG_ERR("[WifiXML] write file fail\r\n");
				} else {
					DBG_DUMP("[WifiXML] write file ok\r\n");
				}
			}
		} else { // Copy file to a buffer.
			if ((uiCopiedBytes + bufSize) <= MAX_UPLOAD_FILE_SIZE) {
				#if _TODO
				hwmem_open();
				hwmem_memcpy((UINT32)(uiTempBuf + uiCopiedBytes), bufAddr, bufSize);
				hwmem_close();
				#else
				memcpy((void *)(uiTempBuf + uiCopiedBytes), (void *)bufAddr, bufSize);
				#endif

				uiCopiedBytes += bufSize;
				//debug_msg("^BCopying...%d\r\n", uiCopiedBytes);
			} else {
				DBG_ERR("Out of bound of buffer!\r\n");
				return CYG_HFS_UPLOAD_FAIL_RECEIVE_ERROR;
			}
		}

		if (putStatus == HFS_PUT_STATUS_FINISH) {
			if (uiUsage == FW_UPLOAD_USAGE_SAVE_AS_FILE) {
				FileSys_CloseFile(fhdl);
				fhdl = 0;
			} else if (uiUsage == FW_UPLOAD_USAGE_UPDATE_FW_IN_DRAM) { // Update FW into Flash. Might send POWER_OFF command then.
				DBG_DUMP("^GCopied bytes:%d\r\n", uiCopiedBytes);

#if 1
#if (FWS_FUNC == ENABLE)
#if _TODO
				DBG_DUMP("^BUpdate FW - begin\r\n");
				//#NT#2016/12/28#Niven Cho -begin
				//#NT#FIXED, update from dram should still be all-in-one
				FWSRV_BIN_UPDATE_ALL_IN_ONE FwInfo = {0};
				FwInfo.uiSrcBufAddr = uiTempBuf;
				FwInfo.uiSrcBufSize = uiCopiedBytes;
				FwInfo.uiWorkBufAddr = uiTempBuf + ALIGN_CEIL_16(uiCopiedBytes);
				FwInfo.uiWorkBufSize = MAX_UPLOAD_FILE_SIZE;
				memset(&Cmd, 0, sizeof(Cmd));
				Cmd.Idx = FWSRV_CMD_IDX_BIN_UPDATE_ALL_IN_ONE;
				Cmd.In.pData = &FwInfo;
				Cmd.In.uiNumByte = sizeof(FwInfo);
				Cmd.Prop.bExitCmdFinish = TRUE;
				fws_er = FwSrv_Cmd(&Cmd);
				//#NT#2016/12/28#Niven Cho -end
				if (fws_er != FWSRV_ER_OK) {
					DBG_DUMP("FW bin write failed\r\n");
					return UPDNAND_STS_FW_WRITE_CHK_ERR;
				}

				DBG_DUMP("^BUpdate FW - end\r\n");
#endif
#endif
#else // Save as file to do verification.
				{
					fhdl = FileSys_OpenFile("A:\\test.bin", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
					FileSys_WriteFile(fhdl, (UINT8 *)uiTempBuf, &uiCopiedBytes, 0, NULL);
					FileSys_CloseFile(fhdl);
					fhdl = 0;
				}
#endif
			} else {
				//debug_msg("File saved in DRAM.\r\n");
			}
		}
	}

	return CYG_HFS_UPLOAD_OK;
}

#if defined(_CPU2_LINUX_)
int XML_UploadAudio(char *path, char *argument, HFS_U32 bufAddr, HFS_U32 bufSize, HFS_U32 segmentCount, HFS_PUT_STATUS putStatus)
{
	//static      UINT32   uiUsage = 0;
	//char        tmpString[32];

	//DBG_MSG("path =%s, argument = %s, bufAddr = 0x%x, bufSize =0x%x , segmentCount  =%d , putStatus = %d\r\n",path,argument ,bufAddr, bufSize, segmentCount, putStatus);
	/*
	[path]

	[argument]
	http://192.168.0.3/?custom=1&cmd=5002&par=0  //
	http://192.168.0.3/?custom=1&cmd=5002&par=1
	http://192.168.0.3/?custom=1&cmd=5002&par=2

	[bufAddr]

	[bufSize]

	[segmentCount]
	0, 1, 2, ....

	[putStatus]
	HFS_PUT_STATUS_CONTINUE
	HFS_PUT_STATUS_FINISH
	HFS_PUT_STATUS_ERR
	*/
	if (putStatus == HFS_PUT_STATUS_ERR) {
		DBG_ERR("receive data error\r\n");
		return CYG_HFS_UPLOAD_FAIL_RECEIVE_ERROR;
	} else {
		if (segmentCount == 0) { //first-segment
			//send data to system audio
			#if (AUDIO_FUNC == ENABLE)
			System_PutAudioData(0, (UINT32)bufAddr, (UINT32 *)&bufSize);
			#endif
		}
		if (putStatus == HFS_PUT_STATUS_FINISH) { //last-segment
			//DBG_MSG("HFS: AUDIO UPLOAD - end\r\n");
		}
	}

	return CYG_HFS_UPLOAD_OK;
}
#endif
#endif

