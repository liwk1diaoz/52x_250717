#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "kwrap/nvt_type.h"
#include "kwrap/platform.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "HfsNvt/HfsNvtAPI.h"
#include <signal.h>
#include <pthread.h>
#include "hdal_flow.h"
#include "file_operation.h"

#define SAVED_ENCODE_TO_FILE 0

#define SAVED_AUDIO_ENCODE_TO_FILE 0

#define ENABLE_HTTP_MSG 0

#define SHOW_INFO 0

int exit_flag = 0;
VIDEO_RECORD g_stream_info={0};
static int only_hls=0;
static void signal_ctrl(int no)
{
	switch(no)
	{
		case SIGINT:
			printf("ready service\r\n");
			HfsNvt_Close();
			if(only_hls == 0)
			{
				hdal_stop(&g_stream_info);
				filesys_close();
				hdal_mem_uinit();
			}
			exit_flag =1;
			printf("finish hls signal_ctrl\r\n");
			break;
		case SIGPIPE:
			printf("get sigpipe signal\n");
			break;
		case SIGCHLD:
			printf("get SIGCHLD signal\n");
			break;
		case SIGUSR1:
			printf("get SIGUSR1 signal\n");
			break;
		default:
			break; 
	}

}

static INT32 getcustomData(CHAR *path, CHAR *argument, UINT32 bufAddr, UINT32 *bufSize, CHAR *mimeType, UINT32 segmentCount)
{
	printf("in getcustomData\n");
	return 0;
}

INT32 fileout_cb_func(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param)
{
	HD_FILEOUT_CB_EVENT event = cbinfo->cb_event;
	int ret=0;
	switch (event)
	{
		case HD_FILEOUT_CB_EVENT_NAMING:
			//printf("HD_FILEOUT_CB_EVENT_NAMING~~~~~~~~~~~~~~~~~~~~~~~~\n");
			ret = get_file_name(cbinfo->p_fpath, &cbinfo->fpath_size);
			if(ret !=0)
			{
				printf("get_file_name error ret:%d\r\n",ret);
				return ret;
			}			
			break;
		case HD_FILEOUT_CB_EVENT_OPENED:
			printf("HD_FILEOUT_CB_EVENT_OPENED\n");
			//cbinfo->alloc_size = g_stream_info.record_time * g_stream_info.enc_bitrate;
			//reserve more, follow for cardv _MovieMulti_FileOut_CB_Opened
			//cbinfo->alloc_size = cbinfo->alloc_size + (cbinfo->alloc_size/10);
			break;
		case HD_FILEOUT_CB_EVENT_CLOSED:
			//printf("HD_FILEOUT_CB_EVENT_CLOSED\n");
			#if 0
			ret = need_create_new_m3u8(cbinfo->p_fpath);
			if(ret == 1)
			{
				ret = create_m3u8_file();
				if(ret != HD_OK)
				{
					printf("create_m3u8_file fail ret:%d\r\n",ret);
					return ret;
				}
			}
			else if(ret == 0){}
			else
			{
				printf("check need_create_new_m3u8 fail ret:%d\n",ret);
				return HD_ERR_NG;
			}
			#endif
			ret = update_m3u8_file(cbinfo->p_fpath , &g_stream_info);
			if(ret !=0)
			{
				printf("update_m3u8_file error ret:%d\r\n",ret);
				return ret;
			}
			ret = need_delete_old_ts_file(cbinfo->p_fpath, &g_stream_info);
			if(ret == 1)
			{
				ret = delete_old_ts_file(cbinfo->p_fpath, &g_stream_info);
				if(ret != HD_OK)
				{
					printf("delete_old_ts_file fail ret:%d\r\n",ret);
					return ret;
				}
			}
			else if(ret ==0){}
			else
			{
				printf("check need_delete_old_ts_file fail ret:%d\n",ret);
				return HD_ERR_NG;
			}
			#if SHOW_INFO == 1
				system("cat /proc/hdal/venc/info");
			#endif

			break;
		case HD_FILEOUT_CB_EVENT_DELETE:
			//printf("HD_FILEOUT_CB_EVENT_DELETE\n");
			break;
		case HD_FILEOUT_CB_EVENT_FS_ERR:
			printf("HD_FILEOUT_CB_EVENT_FS_ERR\n");
			break;
		default:
			printf("fileout event error:%d\n",event);
			break;
	}

	return 0;
}

INT32 bsmux_cb_func(CHAR *p_name, HD_BSMUX_CBINFO *cbinfo, UINT32 *param)
{
	HD_BSMUX_CB_EVENT event = cbinfo->cb_event;

	HD_PATH_ID path_id = 0;
	if(cbinfo->out_data){
		path_id = (HD_PATH_ID)cbinfo->out_data->pathID;
	}
	HD_RESULT ret = HD_OK;
	HD_FILEOUT_BUF *fout_buf = NULL;
	switch (event) {
		case HD_BSMUX_CB_EVENT_PUTBSDONE:
			//printf("HD_BSMUX_CB_EVENT_PUTBSDONE\n");
			break;
		case HD_BSMUX_CB_EVENT_COPYBSBUF:
			//printf("HD_BSMUX_CB_EVENT_COPYBSBUF\n");
			break;
		case HD_BSMUX_CB_EVENT_FOUTREADY:
			//printf("HD_BSMUX_CB_EVENT_FOUTREADY!\n");
			fout_buf = (HD_FILEOUT_BUF *)cbinfo->out_data;
                        ret = hd_fileout_push_in_buf(path_id, fout_buf, -1);
                        if (ret != HD_OK)
			{
                                printf("fileout push_in_buf fail\r\n");
                        }
			break;
		case HD_BSMUX_CB_EVENT_KICKTHUMB:
			//printf("HD_BSMUX_CB_EVENT_KICKTHUMB\n");
			break;
		case HD_BSMUX_CB_EVENT_CUT_COMPLETE:
			//printf("HD_BSMUX_CB_EVENT_CUT_COMPLETE\n");
			break;
		case HD_BSMUX_CB_EVENT_CLOSE_RESULT:
			//printf("HD_BSMUX_CB_EVENT_CLOSE_RESULT\n");
			break;
		default:
			printf("bsmux cb event type error :%d\r\n",event);
			break;
	}
	return 0;
}

#if(HLS_AUDIO_ENABLE == 1)
static void *get_a_enc_data(void *arg)
{
	HD_AUDIO_BS  data_pull = {0};
	HD_RESULT ret = HD_OK;


	#if SAVED_AUDIO_ENCODE_TO_FILE == 1
		HD_VIDEOENC_BUFINFO phy_buf_main;
		UINT32 vir_addr_main;
		// query physical address of bs buffer ( this can ONLY query after hd_audioenc_start() is called !! )
		hd_audioenc_get(g_stream_info.a_enc_path, HD_AUDIOENC_PARAM_BUFINFO, &phy_buf_main);
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
		#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
		CHAR file_path[256]={0};
		FILE *fp = NULL;
		sprintf(file_path,"%s/test_audio_file.aac",g_stream_info.file_folder);
		printf("data save to :%s\n",file_path);
		fp = fopen(file_path,"wb");
		if(fp == NULL)
		{
			printf("file open path:%s fail",file_path);
		}
	#endif
	while (exit_flag == 0) {

		// pull data
		ret = hd_audioenc_pull_out_buf(g_stream_info.a_enc_path, &data_pull,1000 ); // -1 = blocking mode
		if(ret == HD_OK)
		{
			#if SAVED_AUDIO_ENCODE_TO_FILE == 1
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr);
				UINT32 size = data_pull.size;
				if (fp) fwrite(ptr, 1, size, fp);
				if (fp) fflush(fp);

			#endif
			ret = hd_bsmux_push_in_buf_audio(g_stream_info.bsmux_path,  &data_pull, 100);
			if (ret != HD_OK) {
				printf("hd_bsmux_push_in_buf_audio fail, ret(%d)\r\n", ret);
			}
			// release data
			ret = hd_audioenc_release_out_buf(g_stream_info.a_enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("hd_audioenc_release_out_buf fail, ret(%d)\r\n", ret);
			}
		}
		else
		{
			printf("get audio data fail ret:%d\n",ret);
		}

	}
	#if SAVED_AUDIO_ENCODE_TO_FILE == 1
		// mummap for bs buffer
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		// close output file
		if (fp) fclose(fp);
	#endif
	return 0;

}
#endif
static void *get_enc_data(void *arg)
{

	HD_VIDEOENC_BS  data_pull;
	HD_RESULT ret = HD_OK;

	#if SAVED_ENCODE_TO_FILE == 1
		HD_VIDEOENC_BUFINFO phy_buf_main;
		UINT32 vir_addr_main;
		// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
		hd_videoenc_get(g_stream_info.enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
		vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
		#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
		UINT32 j;
		CHAR file_path[256]={0};
		FILE *fp = NULL;
		sprintf(file_path,"%s/test_file.h264",g_stream_info.file_folder);
		printf("data save to :%s\n",file_path);
		fp = fopen(file_path,"wb");
		if(fp == NULL)
		{
			printf("file open path:%s fail",file_path);
		}
	#endif

	//set bsmux param for create ts data

	while(exit_flag == 0)
	{
		ret = hd_videoenc_pull_out_buf(g_stream_info.enc_path, &data_pull, 1000); // -1 = blocking mode, now set 1000ms
		if(ret == HD_OK)
		{


			#if SAVED_ENCODE_TO_FILE == 1
			for (j=0; j< data_pull.pack_num; j++) {
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
				UINT32 len = data_pull.video_pack[j].size;
					if (fp) fwrite(ptr, 1, len, fp);
					if (fp) fflush(fp);			
//				printf("encode j:%d ptr:%x phy:%x len:%x\n",j,ptr,data_pull.video_pack[j].phy_addr,len);

			}
			#endif
			/////do bsmux
			ret = hd_bsmux_push_in_buf_video(g_stream_info.bsmux_path, &data_pull,100);
			if (ret != HD_OK) {
				printf("hd_bsmux_push_in_buf_video error=%d !!\r\n", ret);
			}
			//end bsmux
			ret = hd_videoenc_release_out_buf(g_stream_info.enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("enc_release error=%d !!\r\n", ret);
			}
		}
		else
		{
			printf("get data fail ret:%d\r\n",ret);
		}
	}
	printf("stop get data\n");

	#if SAVED_ENCODE_TO_FILE == 1
		// mummap for bs buffer
		if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
		// close output file
		if (fp) fclose(fp);
	#endif
	return 0;
}

#if ENABLE_HTTP_MSG == 1
static INT32 xExamHfs_headerCb(UINT32 headerAddr, UINT32 headerSize, CHAR *filepath, CHAR *mimeType, void *reserved){
	char *headerstr;
	headerstr = (char *)headerAddr;
	printf("hfs===============\n\n");
	printf("http headerstr=%s\n",headerstr);
	if(filepath)
	{
		printf("filepath:%s\n",filepath);
	}
	if(mimeType)
	{
		printf("mimeType:%s\n",mimeType);
	}
	return 0;
}
#endif
ER start_hfs(int http_port, char *root_folder)
{

	HFSNVT_OPEN  hfsObj = {0};
	char         customstr[8] = {0};
	hfsObj.portNum = http_port;
	hfsObj.httpsPortNum = HLS_HTTPS_PORT;
	hfsObj.threadPriority = 6;
	if(root_folder != NULL && strlen(root_folder)!=0)
	{
		strncpy(hfsObj.rootdir, root_folder, strlen(root_folder));
		hfsObj.rootdir[strlen(root_folder)] ='\0';
	}
	hfsObj.sockbufSize = 51200;
	hfsObj.sharedMemAddr = 0;
	hfsObj.sharedMemSize = 0;
#if ENABLE_HTTP_MSG == 1
	hfsObj.headerCb = xExamHfs_headerCb;
#endif
	strncpy(hfsObj.customStr, customstr, 7);
	hfsObj.customStr[8]='\0';
	hfsObj.getCustomData = (HFSNVT_GET_CUSTOM_CB *)getcustomData;
	printf("start hfs\n");
	ER ret=0; 
	ret = HfsNvt_Open(&hfsObj);

	return ret;
}

int main(int argc, char **argv)
{
	signal(SIGINT, signal_ctrl);
	signal(SIGCHLD, signal_ctrl);
	signal(SIGUSR1, signal_ctrl);
	signal(SIGPIPE, signal_ctrl);
	printf("set signal\n");
	pthread_t  get_data_thread_id;
	
	#if (HLS_AUDIO_ENABLE == 1)
		pthread_t  get_a_data_thread_id;
	#endif
	int http_port=HLS_HTTP_PORT;
	char root_folder[256]={0};
	HD_RESULT hd_ret =0;
	ER ret;
	sprintf(root_folder,"/mnt/sd");

	g_stream_info.bsmux_cb_fun = bsmux_cb_func;
	g_stream_info.fileout_cb_fun = fileout_cb_func;
	g_stream_info.record_time = HLS_RECORD_TIME;
	g_stream_info.max_num_of_file_in_m3u8 = HLS_M3U8_INCLUDE_MAX_FILE; 
	sprintf(g_stream_info.file_folder,"%s",root_folder);

	if(argc >= 2)
	{
		only_hls = atoi(argv[1]);
		//only start http file server, you should put the m3u8 files and ts files in the folder(/mnt/sd)
	}
	if(argc >= 3)
	{
		g_stream_info.record_time = atoi(argv[2]);
	}
	if(argc >= 4)
	{
		g_stream_info.max_num_of_file_in_m3u8 = atoi(argv[3]);
	}
	printf("=============\n");
	printf("http port: %d\n",http_port);
	printf("http root folder: %s\n",root_folder);
	printf("record time:%d\n",g_stream_info.record_time);
	printf("max number of ts file in m3u8:%d\n",g_stream_info.max_num_of_file_in_m3u8);
	//start http file server
	ret = start_hfs(http_port, root_folder);
	if(ret != E_OK)
	{
		printf("start http file server fail ret:%d\n",ret);
		return ret;
	}
	if(only_hls == 1){

		while(exit_flag == 0)
		{
			sleep(1);
		}
	}
	else
	{
		// create hdal flow to get data

		hd_ret = hdal_init(&g_stream_info);
		if(hd_ret != HD_OK)
		{
			printf("hdal init fail ret:%d\r\n",hd_ret);
			return hd_ret;
		}
		//file sys init
		hd_ret = filesys_init(root_folder);
		if(hd_ret != HD_OK)
		{
			printf("filesys_init dail ret:%d\n",hd_ret);
			return hd_ret;
		}
		//create m3ub file
		ret = create_m3u8_file(&g_stream_info);
		if(ret != HD_OK)
		{
			printf("create_m3u8_file fail ret:%d\r\n",ret);
			return ret;
		}
		//create get encode data thread
		ret = pthread_create(&get_data_thread_id, NULL, get_enc_data, NULL);
		if (ret < 0) {
			printf("create get data thread failed\n");
			return ret;
		}
		#if(HLS_AUDIO_ENABLE == 1)
			//create get audio encode data thread
			ret = pthread_create(&get_a_data_thread_id, NULL, get_a_enc_data, NULL);
			if (ret < 0) {
				printf("create get audio data thread failed\n");
				return ret;
			}
			pthread_join(get_a_data_thread_id, NULL);
		#endif
		pthread_join(get_data_thread_id, NULL);
	}

	printf("nvthls stop\n");

	return 0;
}
