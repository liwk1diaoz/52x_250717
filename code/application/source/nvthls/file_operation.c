#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "file_operation.h"
#include "FileSysTsk.h"
#include "vendor_common.h"
#include "sdio.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>









UINT32 			g_filesys_va =0;
HD_COMMON_MEM_VB_BLK 	g_filesys_blk;
UINT32               	g_filesys_pa = 0;

static UINT32 g_media_seq=0;
static CHAR g_root_folder[256]={0};


//get ip

static HD_RESULT get_current_ip(CHAR * host)
{
	struct ifaddrs *ifaddr, *ifa;
	int  s;
	HD_RESULT ret = HD_ERR_NG;
	if(host == NULL)
	{
		printf("get_current_ip host == NULL\n");
		return HD_ERR_NG;
	}
	if (getifaddrs(&ifaddr) == -1)
	{
		printf("get_current_ip getifaddrs fail\n");
		return HD_ERR_NG;
	}
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
		continue;
		s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if((strcmp(ifa->ifa_name,HLS_NET_INTERFACE)==0)&&(ifa->ifa_addr->sa_family==AF_INET))
		{
			if (s != 0)
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				return HD_ERR_NG;
			}
			printf("\tInterface :%s\n",ifa->ifa_name );
			printf("\t  Address :%s\n", host);
			ret = HD_OK;
		}
	}
	return ret;
}




static HD_RESULT get_current_file_name_seq(CHAR *fpath, UINT32 *output_value)
{

	CHAR tmp_path[256]={0};
	if(fpath == NULL)
	{
		printf("delete_old_ts_file fpath == NULL\n");
		return HD_ERR_NULL_PTR;
	}
	
	sprintf(tmp_path,"%s%s%%u%s",HLS_DRIVE,HLS_PREFIX_NAME,HLS_EXTENSION_NAME);
	sscanf(fpath,tmp_path,output_value);
	return HD_OK;
}


HD_RESULT delete_old_ts_file(CHAR *fpath, VIDEO_RECORD *stream)
{

	UINT32 current_file_name_seq=0;
	int ret = 0;
	CHAR remove_file[256]={0};
	//remove the number of stream->max_num_of_file_in_m3u8 file
	UINT32 i=0;
	if(fpath == NULL)
	{
		printf("delete_old_ts_file fpath == NULL\n");
		return HD_ERR_NULL_PTR;
	}
	ret = get_current_file_name_seq(fpath, &current_file_name_seq);
	if(ret != HD_OK)
	{
		printf("get_current_file_name_seq fail fpath:%s\n",fpath);
		return HD_ERR_INV;
	}
	for(i=0; i< stream->max_num_of_file_in_m3u8;i++)
	{
		sprintf(remove_file,"%s/%s%u%s",g_root_folder,
					HLS_PREFIX_NAME,
					(current_file_name_seq)-2*stream->max_num_of_file_in_m3u8+i,
					HLS_EXTENSION_NAME);
		printf("remove old file:%s\n",remove_file);
		ret = remove(remove_file);
		if(ret != 0)
		{
			printf("remove %s fail\n",remove_file);
		}
	}

	return HD_OK;

}

INT32 need_delete_old_ts_file(CHAR *fpath, VIDEO_RECORD *stream)
{

	HD_RESULT ret = HD_OK;
	UINT32 current_file_name_seq = 0;
	if(fpath == NULL)
	{
		printf("need_delete_old_ts_file fpath == NULL\n");
		return HD_ERR_NULL_PTR;
	}
	ret = get_current_file_name_seq(fpath, &current_file_name_seq);
	if(ret != HD_OK)
	{
		printf("need_delete_old_ts_file fail fpath:%s\n",fpath);
		return HD_ERR_INV;
	}
	if((current_file_name_seq) < 2*stream->max_num_of_file_in_m3u8 )
	{
		//first 2*max_num_of_file_in_m3u8 no need to remove
		return 0;
	}
	if(((current_file_name_seq)%stream->max_num_of_file_in_m3u8) == 0)
	{
		return 1;
	}
	return 0;
}
#if 0
INT32 need_create_new_m3u8(CHAR *fpath)
{

	HD_RESULT ret = HD_OK;
	UINT32 current_file_name_seq = 0;
	if(fpath == NULL)
	{
		printf("need_delete_old_ts_file fpath == NULL\n");
		return HD_ERR_NULL_PTR;
	}
	ret = get_current_file_name_seq(fpath, &current_file_name_seq);
	if(ret != HD_OK)
	{
		printf("need_delete_old_ts_file fail fpath:%s\n",fpath);
		return HD_ERR_INV;
	}
	if(((current_file_name_seq)%HLS_M3U8_INCLUDE_MAX_FILE) == 0)
	{
		return 1;
	}
	return 0;
}
#endif
HD_RESULT update_m3u8_file(CHAR *fpath, VIDEO_RECORD *stream)
{

#if 0

	FILE *fp = NULL;
	CHAR *fpath_name= NULL;
	CHAR m3u8_file[256]={0};
	sprintf(m3u8_file,"%s/%s",g_root_folder,M3U8_FILE_NAME);
	fpath_name = (CHAR *)(fpath + strlen(HLS_DRIVE));
	//sprintf(m3u8_file,"%s/%s",g_root_folder,M3U8_FILE_NAME);
	fp =fopen(m3u8_file,"aw");
	if(fp == NULL)
	{
		printf("update %s fail\n",M3U8_FILE_NAME);
		return HD_ERR_NULL_PTR;
	}

	fprintf(fp,"#EXTINF:%d,\n",HLS_RECORD_TIME);
	fprintf(fp,"%s\n",fpath_name);
	fclose(fp);
#else
	FILE *fp = NULL;
	CHAR fpath_name[256]= {0};
	CHAR m3u8_file[256]={0};
	UINT32 current_file_name_seq = 0;
	HD_RESULT ret =0;
	UINT32 ts_file_index=0;
	UINT32 i=0;
	UINT32 count =0;
	ret = get_current_file_name_seq(fpath, &current_file_name_seq);
	if(ret != HD_OK)
	{
		printf("get_current_file_name_seq fail fpath:%s\n",fpath);
		return HD_ERR_INV;
	}
	sprintf(m3u8_file,"%s/%s",g_root_folder,M3U8_FILE_NAME);
	fp =fopen(m3u8_file,"w");
	if(fp == NULL)
	{
		printf("update %s fail\n",M3U8_FILE_NAME);
		return HD_ERR_NULL_PTR;

	}
	fprintf(fp,"#EXTM3U\n");
	fprintf(fp,"#EXT-X-VERSION:3\n");
	fprintf(fp,"#EXT-X-TARGETDURATION:%d\n",stream->record_time); // the same with hdal_flow.c
	fprintf(fp,"#EXT-X-MEDIA-SEQUENCE:%u\n",g_media_seq); // the same with HLS_RECORD_TIME
	g_media_seq++;
	if(current_file_name_seq <  stream->max_num_of_file_in_m3u8)
	{
		ts_file_index = 0;
		count = current_file_name_seq + 1;
	}
	else
	{
		ts_file_index = current_file_name_seq - stream->max_num_of_file_in_m3u8;
		count = stream->max_num_of_file_in_m3u8;
	}


	for(i=0 ; i< count ; i++ )
	{
		fprintf(fp,"#EXTINF:%d,\n",HLS_RECORD_TIME);
		memset(fpath_name,0,sizeof(fpath_name));
		sprintf(fpath_name,"%s%d%s\n", HLS_PREFIX_NAME, ts_file_index + i, HLS_EXTENSION_NAME);
		fprintf(fp,"%s\n",fpath_name);

	}
	fflush(fp);
	fclose(fp);


#endif
	return HD_OK;
}
#if 1
HD_RESULT create_m3u8_file(VIDEO_RECORD *stream)
{
	FILE *fp=NULL;
	CHAR m3u8_file[256]={0};
	CHAR m3u_file[256]={0};
	CHAR ip_addr[256]={0};
	HD_RESULT ret=0;
	sprintf(m3u8_file,"%s/%s",g_root_folder,M3U8_FILE_NAME);
	printf("m3u8:%s\n",m3u8_file);
	sprintf(m3u_file,"%s/%s",g_root_folder,M3U_FILE_NAME);
	printf("m3u:%s\n",m3u_file);
	if(access(m3u8_file,0) ==0)
	{
		remove(m3u8_file);
	}
	if(access(m3u_file,0) == 0)
	{
		remove(m3u_file);
	}
	fp = fopen(m3u8_file,"aw");
	if(!fp)
	{
		printf("can not create file:%s\n",m3u8_file);
		return HD_ERR_NG;
	}
	fprintf(fp,"#EXTM3U\n");
	fprintf(fp,"#EXT-X-VERSION:3\n");
	fprintf(fp,"#EXT-X-TARGETDURATION:%d\n",stream->record_time); // the same with hdal_flow.c
	fprintf(fp,"#EXT-X-MEDIA-SEQUENCE:%u\n",g_media_seq); // the same with HLS_RECORD_TIME
	fclose(fp);
	fp = NULL;
	fp = fopen(m3u_file,"aw");
	if(!fp)
	{
		printf("can not create file:%s\n",m3u_file);
		return HD_ERR_NG;
	}
	fprintf(fp,"#EXTM3U\n");
	fprintf(fp,"#EXT-X-VERSION:3\n");
	fprintf(fp,"#EXTINF:-1, stream\n");
	ret = get_current_ip(ip_addr);
	if(ret != HD_OK)
	{
		printf("get_current_ip fail ret:%d\n",ret);
		return HD_ERR_NG;
	}
	fprintf(fp,"http://%s:%d/%s\n",ip_addr, HLS_HTTP_PORT, M3U8_FILE_NAME);
	fclose(fp);
	return HD_OK;
}
#endif


HD_RESULT get_file_name( CHAR *file_path, UINT32 *file_path_len)
{

	CHAR tmp_path[256]={0};
	static UINT32 file_name_sequence_num=0;
	if(file_path == NULL)
	{
		printf("file_path pointer NULL error\n");
		return HD_ERR_NULL_PTR;
	}
	sprintf(tmp_path,"%s%s%u%s",HLS_DRIVE,HLS_PREFIX_NAME,file_name_sequence_num,HLS_EXTENSION_NAME);
	*file_path_len = strlen(tmp_path);
	strcpy(file_path,tmp_path);
	//printf("save data path:%s size:%d\n",file_path,*file_path_len);
	file_name_sequence_num ++;
	return HD_OK;
}

HD_RESULT filesys_close(void)
{
	int ret =0;
	ret = FileSys_CloseEx('A', 1000);
	if(ret != FST_STA_OK)
	{
		printf("FileSys_CloseEx fail ret:%d\r\n",ret);
		return HD_ERR_NG;
	}
	if(g_filesys_va)
	{
		ret = hd_common_mem_munmap((void *)g_filesys_va, HLS_POOL_SIZE_FILESYS);
		if (ret != HD_OK) {
			printf("hd_common_mem_munmap fail va:%x ret:%d\n",g_filesys_va,ret);
			return ret;
		}
		ret = hd_common_mem_release_block(g_filesys_blk);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail ret:%d\n",ret);
			return ret;
		}
	}


	printf("close filesys\n");
	return HD_OK;
}


HD_RESULT filesys_init(CHAR *folder){


	UINT32 			uiPoolAddr;
	HD_RESULT    		ret = HD_OK;
	FILE_TSK_INIT_PARAM     Param = {0};
	FS_HANDLE               StrgDXH;
	printf("filesys_init b\r\n");
	if(folder == NULL)
	{
		printf("folder NULL\n");
		return HD_ERR_NULL_PTR;
	}

	#if 0
		ret = vendor_common_mem_alloc_fixed_pool("filesys", &pa, (void **)&va, HLS_POOL_SIZE_FILESYS, DDR_ID0);
		if (ret != HD_OK) {
			return ret;
		}
	#else
		g_filesys_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, HLS_POOL_SIZE_FILESYS, DDR_ID0);
		if (HD_COMMON_MEM_VB_INVALID_BLK == g_filesys_blk)
		{
			printf("hd_common_mem_get_block fail\r\n");
			return HD_ERR_NG;
		}
		g_filesys_pa = hd_common_mem_blk2pa(g_filesys_blk);
		if (g_filesys_pa == 0) {
			printf("not get buffer, pa=%08x\r\n", (int)g_filesys_pa);
			return HD_ERR_NG;
		}
		g_filesys_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, g_filesys_pa, HLS_POOL_SIZE_FILESYS);
		if (g_filesys_va == 0) {
			printf("mem map fail\r\n");
			ret = hd_common_mem_munmap((void *)g_filesys_va, HLS_POOL_SIZE_FILESYS);
			if (ret != HD_OK) {
				printf("mem unmap fail\r\n");
				return ret;
			}
			return HD_ERR_NG;
		}
	#endif

 	memset(&Param, 0, sizeof(FILE_TSK_INIT_PARAM));
 	StrgDXH = (FS_HANDLE)sdio_getStorageObject(STRG_OBJ_FAT1);
	uiPoolAddr = g_filesys_va;
	Param.FSParam.WorkBuf = uiPoolAddr;
	Param.FSParam.WorkBufSize = (HLS_POOL_SIZE_FILESYS);
	// support exFAT
	Param.FSParam.bSupportExFAT   = FALSE;
	//Param.pDiskErrCB = (FileSys_CB)Card_InitCB;
	strcpy(g_root_folder,folder);
	strncpy(Param.FSParam.szMountPath, folder, sizeof(Param.FSParam.szMountPath) - 1); //only used by FsLinux
	Param.FSParam.szMountPath[sizeof(Param.FSParam.szMountPath) - 1] = '\0';
	Param.FSParam.MaxOpenedFileNum = HLS_MAX_OPENED_FILE_NUM;
	FileSys_InstallID(FileSys_GetOPS_Linux());
	if (FST_STA_OK != FileSys_Init(FileSys_GetOPS_Linux())) {
		printf("FileSys_Init failed\r\n");
		return HD_ERR_NG;
	}
	ret = FileSys_OpenEx('A', StrgDXH, &Param);
	if (FST_STA_OK != ret) {
		printf("FileSys_Open err %d\r\n", ret);
		return ret;
	}
	// call the function to wait init finish
	FileSys_WaitFinishEx('A');
	printf("filesys_init e\r\n");
	return ret;
}
