#include <stdio.h>
#include <string.h>
#include "detection.h"
#include "autoframing.h"

HD_IPOINT af_zoom_table [AUTOFRAMING_INDEX] = {
	{1920,1080},
	{1910,1072},
	{1900,1068},
	{1890,1060},
	{1880,1056},
	{1870,1048},
	{1860,1044},
	{1850,1040},
	{1840,1032},
	{1830,1028},
	{1820,1020},
	{1810,1016},
	{1800,1012},
	{1790,1004},
	{1780,1000},
	{1770,992},
	{1760,988},
	{1750,984},
	{1740,976},
	{1730,972},
	{1720,964},
	{1710,960},
	{1700,956},
	{1690,948},
	{1680,944},
	{1670,936},
	{1660,932},
	{1650,928},
	{1640,920},
	{1630,916},
	{1620,908},
	{1610,904},
	{1600,900},
	{1590,892},
	{1580,888},
	{1570,880},
	{1560,876},
	{1550,868},
	{1540,864},
	{1530,860},
	{1520,852},
	{1510,848},
	{1500,840},
	{1490,836},
	{1480,832},
	{1470,824},
	{1460,820},
	{1450,812},
	{1440,808},
	{1430,804},
	{1420,796},
	{1410,792},
	{1400,784},
	{1390,780},
	{1380,776},
	{1370,768},
	{1360,764},
	{1350,756},
	{1340,752},
	{1330,748},
	{1320,740},
	{1310,736},
	{1300,728},
	{1290,724},
	{1280,720},
	{1270,712},
	{1260,708},
	{1250,700},
	{1240,696},
	{1230,688},
	{1220,684},
	{1210,680},
	{1200,672},
	{1190,668},
	{1180,660},
	{1170,656},
	{1160,652},
	{1150,644},
	{1140,640},
	{1130,632},
	{1120,528},
	{1110,624},
	{1100,616},
	{1090,612},
	{1080,604},
	{1070,600},
	{1060,596},
	{1050,588},
	{1040,584},
	{1030,576},
	{1020,572},
	{1010,568},
	{1000,560},
	{990,556},
	{980,548},
	{970,544},
	{960,540},
	{950,532},
	{940,528},
	{930,520},
	{920,516},
	{910,508},
	{900,504},
	{890,500},
	{880,492},
	{870,488},
	{860,480},
	{850,476},
	{840,472},
	{830,464},
	{820,460},
	{810,452},
	{800,448},
	{790,444},
	{780,436},
	{770,432},
	{760,424},
	{750,420},
	{740,416},
	{730,408}
};


af_fdcnn_result af_fd_rlt[2] = {0};
af_pvcnn_result af_pv_rlt[2] = {0};
af_process_result af_proc_rlt[MAX_ALG_CNT] = {0};

UINT32 alg_buffer_write_flag = 0;
UINT32 alg_buffer_read_flag = 0;
UINT32 alg_pv_cnt0 = 0;
UINT32 alg_pv_cnt1 = 0;
UINT32 alg_fd_cnt0 = 0;
UINT32 alg_fd_cnt1 = 0;
UINT64 algSerialNum = 0;
//UINT32 alg_pv_framecnt = 0;
//UINT32 alg_fd_framecnt = 0;
INT32 alg_buf_id = 0;

int IsPvwithFD(INT32 x, INT32 y, INT32 w, INT32 h)
{
	UINT32 ret = 0, indx = 0;
	for (indx = 0;indx < MAX_ALG_CNT;indx ++){
		if (x + w > af_fd_rlt[alg_buf_id].face[indx].x &&
			af_fd_rlt[alg_buf_id].face[indx].x + af_fd_rlt[alg_buf_id].face[indx].w > x &&
			y + h > af_fd_rlt[alg_buf_id].face[indx].y &&
			af_fd_rlt[alg_buf_id].face[indx].y + af_fd_rlt[alg_buf_id].face[indx].h > y){
			ret = 1;
			break;
		}
	}

	return ret;
}

int IsPVinProcess(INT32 x, INT32 y, INT32 w, INT32 h)
{
	INT32 notfound = -1, indx = 0;
	
	//printf("IsPVinProcess %u %u %u %u \r\n", x,y,w,h);
	for (indx = 0; indx < MAX_ALG_CNT; indx ++){
	//printf("IsPVinProcess %u %u %u %u \r\n", af_proc_rlt[indx].process_result.x, af_proc_rlt[indx].process_result.y, af_proc_rlt[indx].process_result.w, af_proc_rlt[indx].process_result.h);	
	
#if 0

	printf("IsPVinProcess if (%d %d %d %d) \r\n", 
		af_proc_rlt[indx].process_result.x,
		af_proc_rlt[indx].process_result.y,
		af_proc_rlt[indx].process_result.w,
		af_proc_rlt[indx].process_result.h);

	printf("IsPVinProcess if ABS(%d %d %d %d) \r\n", 
		ABS(af_proc_rlt[indx].process_result.x - x),
		ABS(af_proc_rlt[indx].process_result.y - y),
		ABS((af_proc_rlt[indx].process_result.x + af_proc_rlt[indx].process_result.w) - (x + w)),
		ABS((af_proc_rlt[indx].process_result.y + af_proc_rlt[indx].process_result.h) - (y + h)));
#endif
		if (ABS(af_proc_rlt[indx].process_result.x - x) < PVCLOSE && 
			ABS(af_proc_rlt[indx].process_result.y - y) < PVCLOSE && 
			ABS((af_proc_rlt[indx].process_result.x + af_proc_rlt[indx].process_result.w) - (x + w)) < PVCLOSE && 
			ABS((af_proc_rlt[indx].process_result.y + af_proc_rlt[indx].process_result.h) - (y + h)) < PVCLOSE){
			return indx;
		}
	}

	return notfound;
}
/*
void set_alg_framecnt(unsigned int type)
{
	switch (type) {
	case PVCNN:
		if (alg_pv_framecnt < MAX_ALG_CNT)
			alg_pv_framecnt++;
		else
			alg_pv_framecnt = 0;
			
		break;
	case FDCNN:
		if (alg_fd_framecnt < MAX_ALG_CNT)
			alg_fd_framecnt++;
		else
			alg_fd_framecnt = 0;
		break;
		
	default:
		printf("get struct size: detect type(%d) error!\n", type);
		ret = -1;
		break;
	}

}
*/
void insert_pvcnn_result(unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
	af_pvcnn_result *ppvcnn = NULL;
	af_pvcnn_result *ppvcnn_other = NULL;

	if(af_pv_rlt[0].sn <= af_pv_rlt[1].sn){
		if(af_pv_rlt[0].status != AF_PINGPONG_READ){
			ppvcnn = &af_pv_rlt[0];
			ppvcnn_other = &af_pv_rlt[1];
			if (ppvcnn_other->status == AF_PINGPONG_WRITE)
				ppvcnn_other->status = AF_PINGPONG_READY;
			ppvcnn->status = AF_PINGPONG_WRITE;
			ppvcnn->people[alg_pv_cnt0].x = (INT32)x;
			ppvcnn->people[alg_pv_cnt0].y = (INT32)y;
			ppvcnn->people[alg_pv_cnt0].w = (INT32)w;
			ppvcnn->people[alg_pv_cnt0].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
	//printf("[1] c0: %d, c1: %d sn0: %lld sn1: %lld s0: %d s1: %d\r\n",alg_pv_cnt0, alg_pv_cnt1, af_pv_rlt[0].sn, af_pv_rlt[1].sn, af_pv_rlt[0].status, af_pv_rlt[1].status);
			alg_pv_cnt0++;
			//algSerialNum++;
		} else {
			if(af_pv_rlt[1].status != AF_PINGPONG_READ){
				ppvcnn = &af_pv_rlt[1];
				ppvcnn_other = &af_pv_rlt[0];
				if (ppvcnn_other->status == AF_PINGPONG_WRITE)
					ppvcnn_other->status = AF_PINGPONG_READY;
				ppvcnn->status = AF_PINGPONG_WRITE;
				ppvcnn->people[alg_pv_cnt1].x = (INT32)x;
				ppvcnn->people[alg_pv_cnt1].y = (INT32)y;
				ppvcnn->people[alg_pv_cnt1].w = (INT32)w;
				ppvcnn->people[alg_pv_cnt1].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
	//printf("[2] c0: %d, c1: %d sn0: %lld sn1: %lld s0: %d s1: %d\r\n",alg_pv_cnt0, alg_pv_cnt1, af_pv_rlt[0].sn, af_pv_rlt[1].sn, af_pv_rlt[0].status, af_pv_rlt[1].status);
				alg_pv_cnt1++;
				//algSerialNum++;
			} else {
				printf("[1]af_pv_rlt Flag error buf1 flag:%d	buf2 flag:%d\r\n",af_pv_rlt[0].status,af_pv_rlt[1].status);
				printf("[1]I: %d RY: %d R: %d W: %d\r\n",AF_PINGPONG_INIT, AF_PINGPONG_READY, AF_PINGPONG_READ, AF_PINGPONG_WRITE);
			}
		}
	} else {
		if(af_pv_rlt[1].status != AF_PINGPONG_READ){
			ppvcnn = &af_pv_rlt[1];
			ppvcnn_other = &af_pv_rlt[0];
			if (ppvcnn_other->status == AF_PINGPONG_WRITE)
				ppvcnn_other->status = AF_PINGPONG_READY;
			ppvcnn->status = AF_PINGPONG_WRITE;
			ppvcnn->people[alg_pv_cnt1].x = (INT32)x;
			ppvcnn->people[alg_pv_cnt1].y = (INT32)y;
			ppvcnn->people[alg_pv_cnt1].w = (INT32)w;
			ppvcnn->people[alg_pv_cnt1].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
	//printf("[3] c0: %d, c1: %d sn0: %lld sn1: %lld s0: %d s1: %d\r\n",alg_pv_cnt0, alg_pv_cnt1, af_pv_rlt[0].sn, af_pv_rlt[1].sn, af_pv_rlt[0].status, af_pv_rlt[1].status);
			alg_pv_cnt1++;
			//algSerialNum++;
		} else {
			if(af_pv_rlt[0].status != AF_PINGPONG_READ){
				ppvcnn = &af_pv_rlt[0];
				ppvcnn_other = &af_pv_rlt[1];
				if (ppvcnn_other->status == AF_PINGPONG_WRITE)
					ppvcnn_other->status = AF_PINGPONG_READY;
				ppvcnn->status = AF_PINGPONG_WRITE;
				ppvcnn->people[alg_pv_cnt0].x = (INT32)x;
				ppvcnn->people[alg_pv_cnt0].y = (INT32)y;
				ppvcnn->people[alg_pv_cnt0].w = (INT32)w;
				ppvcnn->people[alg_pv_cnt0].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
	//printf("[4] c0: %d, c1: %d sn0: %lld sn1: %lld s0: %d s1: %d\r\n",alg_pv_cnt0, alg_pv_cnt1, af_pv_rlt[0].sn, af_pv_rlt[1].sn, af_pv_rlt[0].status, af_pv_rlt[1].status);
				alg_pv_cnt0++;
				//algSerialNum++;
			} else {
				printf("[2]af_pv_rlt Flag error buf1 flag:%d	buf2 flag:%d\r\n",af_pv_rlt[0].status,af_pv_rlt[1].status);
				printf("[2]I: %d RY: %d R: %d W: %d\r\n",AF_PINGPONG_INIT, AF_PINGPONG_READY, AF_PINGPONG_READ, AF_PINGPONG_WRITE);
			}
		}
	}
	
	if (alg_pv_cnt0 >= MAX_ALG_CNT){
		ppvcnn->status = AF_PINGPONG_READY;
		algSerialNum++;
		ppvcnn->sn = algSerialNum;
		alg_pv_cnt0 = 0;
	}
	if (alg_pv_cnt1 >= MAX_ALG_CNT){
		ppvcnn->status = AF_PINGPONG_READY;
		algSerialNum++;
		ppvcnn->sn = algSerialNum;
		alg_pv_cnt1 = 0;
	}
//	printf("algSN: %lld pv_cnt0: %d pv_cnt1: %d 0s: %d 1s: %d 0sn: %d 1sn: %d\r\n", algSerialNum, alg_pv_cnt0, alg_pv_cnt1, af_pv_rlt[0].status, af_pv_rlt[1].status, af_pv_rlt[0].sn, af_pv_rlt[1].sn);
}

void insert_fdcnn_result(unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
	af_fdcnn_result *pfdcnn = NULL;
	if(af_fd_rlt[0].sn <= af_fd_rlt[1].sn){
		if(af_fd_rlt[0].status != AF_PINGPONG_READ){
			pfdcnn = &af_fd_rlt[0];
			pfdcnn->status = AF_PINGPONG_WRITE;
			pfdcnn->face[alg_fd_cnt0].x = (INT32)x;
			pfdcnn->face[alg_fd_cnt0].y = (INT32)y;
			pfdcnn->face[alg_fd_cnt0].w = (INT32)w;
			pfdcnn->face[alg_fd_cnt0].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
//	printf("[1]insert_fdcnn_result c0: %d, c1: %d %u %u %u %u \r\n",alg_fd_cnt0, alg_fd_cnt1, x,y,w,h);
			alg_fd_cnt0++;
			algSerialNum++;
		} else {
			if(af_fd_rlt[1].status != AF_PINGPONG_READ){
				pfdcnn = &af_fd_rlt[1];
				pfdcnn->status = AF_PINGPONG_WRITE;
				pfdcnn->face[alg_fd_cnt1].x = (INT32)x;
				pfdcnn->face[alg_fd_cnt1].y = (INT32)y;
				pfdcnn->face[alg_fd_cnt1].w = (INT32)w;
				pfdcnn->face[alg_fd_cnt1].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
//	printf("[1]insert_fdcnn_result c1: %d, c1: %d %u %u %u %u \r\n",alg_fd_cnt0, alg_fd_cnt1, x,y,w,h);
				alg_fd_cnt1++;
				algSerialNum++;
			} else {
				printf("[1]af_fd_rlt Flag error buf1 flag:%d	buf2 flag:%d\r\n",af_pv_rlt[0].status,af_pv_rlt[1].status);
				printf("[1]I: %d RY: %d R: %d W: %d\r\n",AF_PINGPONG_INIT, AF_PINGPONG_READY, AF_PINGPONG_READ, AF_PINGPONG_WRITE);
			}
		}
	} else {
		if(af_fd_rlt[1].status != AF_PINGPONG_READ){
			pfdcnn = &af_fd_rlt[0];
			pfdcnn->status = AF_PINGPONG_WRITE;
			pfdcnn->face[alg_fd_cnt0].x = (INT32)x;
			pfdcnn->face[alg_fd_cnt0].y = (INT32)y;
			pfdcnn->face[alg_fd_cnt0].w = (INT32)w;
			pfdcnn->face[alg_fd_cnt0].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
//	printf("[1]insert_fdcnn_result c0: %d, c1: %d %u %u %u %u \r\n",alg_fd_cnt0, alg_fd_cnt1, x,y,w,h);
			alg_fd_cnt1++;
			algSerialNum++;
		} else {
			if(af_fd_rlt[0].status != AF_PINGPONG_READ){
				pfdcnn = &af_fd_rlt[1];
				pfdcnn->status = AF_PINGPONG_WRITE;
				pfdcnn->face[alg_fd_cnt1].x = (INT32)x;
				pfdcnn->face[alg_fd_cnt1].y = (INT32)y;
				pfdcnn->face[alg_fd_cnt1].w = (INT32)w;
				pfdcnn->face[alg_fd_cnt1].h = (INT32)h;
//if (x != 1 && y != 1 &&w != 1 &&h != 1)
//	printf("[1]insert_fdcnn_result c1: %d, c1: %d %u %u %u %u \r\n",alg_fd_cnt0, alg_fd_cnt1, x,y,w,h);
				alg_fd_cnt0++;
				algSerialNum++;
			} else {
				printf("[2]af_fd_rlt Flag error buf1 flag:%d	buf2 flag:%d\r\n",af_pv_rlt[0].status,af_pv_rlt[1].status);
				printf("[2]I: %d RY: %d R: %d W: %d\r\n",AF_PINGPONG_INIT, AF_PINGPONG_READY, AF_PINGPONG_READ, AF_PINGPONG_WRITE);
			}
		}
	}
	if (alg_fd_cnt0 >= MAX_ALG_CNT){
		pfdcnn->status = AF_PINGPONG_READY;
		pfdcnn->sn = algSerialNum;
		alg_fd_cnt0 = 0;
	}
	if (alg_fd_cnt1 >= MAX_ALG_CNT){
		pfdcnn->status = AF_PINGPONG_READY;
		pfdcnn->sn = algSerialNum;
		alg_fd_cnt1 = 0;
	}
}

/*
return
-1: pp buffer error
0: duplicate result
1: ok
*/

int get_autoframing_result(af_final_result *af_fnal_rlt){
	UINT32 indx = 0, validpvcnt = 0, zoomindx = 0, updaterlt = 0;
	INT32 pindx = 0,centerX, centerY;
	af_pvcnn_result *ppvcnn = NULL;
	det_info *d_info = NULL;
	memset(&af_proc_rlt[0], 0, sizeof(af_process_result)*MAX_ALG_CNT);
	af_final_result af_tmp_rlt;
	
	if(af_pv_rlt[0].sn <= af_pv_rlt[1].sn){
		if(af_pv_rlt[0].status == AF_PINGPONG_READY){
			ppvcnn = &af_pv_rlt[0];
			alg_buf_id = 0;
		}
		else{
			if(af_pv_rlt[1].status == AF_PINGPONG_READY){
				ppvcnn = &af_pv_rlt[1];
				alg_buf_id = 1;
			}
			else{
printf("[1]AF_DATA_ERR: %u %u\r\n", af_pv_rlt[0].status, af_pv_rlt[1].status);
				return AF_DATA_ERR;
			}
		}
	} else {
		if(af_pv_rlt[1].status == AF_PINGPONG_READY){
			ppvcnn = &af_pv_rlt[1];
			alg_buf_id = 1;
		}
		else{
			if(af_pv_rlt[0].status == AF_PINGPONG_READY){
				ppvcnn = &af_pv_rlt[0];
				alg_buf_id = 0;
			}
			else{
printf("[2]AF_DATA_ERR: %u %u\r\n", af_pv_rlt[0].status, af_pv_rlt[1].status);
				return AF_DATA_ERR;
			}
		}
	}
	if (af_fnal_rlt->bufid == alg_buf_id)
		return AF_DATA_DUP;

	ppvcnn->status = AF_PINGPONG_READ;
	
	for (indx = 0; indx < MAX_ALG_CNT; indx ++){
		//1. get a pv result
		d_info = &ppvcnn->people[indx];
		//2. check if it already close to another in process rlt
		
		if (d_info->x == 1 && d_info->y == 1 &&d_info->w == 1 &&d_info->h == 1)//magic rlt
			continue;
		
		pindx = IsPVinProcess(d_info->x, d_info->y, d_info->w, d_info->h);
//printf("alg_buf_id: %d pindx: %d ppvcnn->people[%d]: %u %u %u %u\r\n", 
//	alg_buf_id, pindx, indx, d_info->x, d_info->y, d_info->w, d_info->h);
		if (pindx >= 0){
			//yes, process rlt++
			if (af_proc_rlt[pindx].validCnt){
				af_proc_rlt[pindx].validCnt++;
			} else {
				af_proc_rlt[pindx].process_result.x = d_info->x;
				af_proc_rlt[pindx].process_result.y = d_info->y;
				af_proc_rlt[pindx].process_result.w = d_info->w;
				af_proc_rlt[pindx].process_result.h = d_info->h;
				af_proc_rlt[pindx].validCnt++;
			}
		} else {
			//no, check if fd with it
			//if (IsPvwithFD(d_info->x, d_info->y, d_info->w, d_info->h)){
				//yes, save to tos process rlt 
				pindx = 0;
				for (pindx = 0; pindx < MAX_ALG_CNT; pindx ++){

					if (af_proc_rlt[pindx].validCnt == 0){
						af_proc_rlt[pindx].process_result.x = d_info->x;
						af_proc_rlt[pindx].process_result.y = d_info->y;
						af_proc_rlt[pindx].process_result.w = d_info->w;
						af_proc_rlt[pindx].process_result.h = d_info->h;
						af_proc_rlt[pindx].validCnt++;
#if 0
if (d_info->x != 1 && d_info->y != 1 &&d_info->w != 1 &&d_info->h != 1)
	printf("af_proc_rlt[%d] %u %u %u %u %u\r\n", pindx, af_proc_rlt[pindx].validCnt, 
		af_proc_rlt[pindx].process_result.x,
		af_proc_rlt[pindx].process_result.y,
		af_proc_rlt[pindx].process_result.w,
		af_proc_rlt[pindx].process_result.h);
#endif
						break;
						}
					}
				//} 
			//} else {
				//no drop it
			//}
		}
	}

	ppvcnn->status = AF_PINGPONG_READY;
	
	//start from center
	af_tmp_rlt.x = 960;
	af_tmp_rlt.y = 540;
	af_tmp_rlt.w = 10;
	af_tmp_rlt.h = 10;
	for (pindx = 0; pindx < MAX_ALG_CNT; pindx ++){

		if (af_proc_rlt[pindx].validCnt <= MIN_ALG_VALID_CNT){
			continue;
		}
	printf("af_proc_rlt[%d] %d %d %d %d validCnt %d\r\n", pindx,
		af_proc_rlt[pindx].process_result.x, af_proc_rlt[pindx].process_result.y, 
		af_proc_rlt[pindx].process_result.w, af_proc_rlt[pindx].process_result.h,
		af_proc_rlt[pindx].validCnt);
		validpvcnt++;
		if (af_proc_rlt[pindx].process_result.x < af_tmp_rlt.x)
			af_tmp_rlt.x = af_proc_rlt[pindx].process_result.x;
		if (af_proc_rlt[pindx].process_result.y < af_tmp_rlt.y)
			af_tmp_rlt.y = af_proc_rlt[pindx].process_result.y;
		
		if ((af_proc_rlt[pindx].process_result.x + af_proc_rlt[pindx].process_result.w) > af_tmp_rlt.w)
			af_tmp_rlt.w = af_proc_rlt[pindx].process_result.x + af_proc_rlt[pindx].process_result.w;
		if ((af_proc_rlt[pindx].process_result.y + af_proc_rlt[pindx].process_result.h) > af_tmp_rlt.h)
			af_tmp_rlt.h = af_proc_rlt[pindx].process_result.y + af_proc_rlt[pindx].process_result.h;
	}

	//find center
	if (validpvcnt){
		af_tmp_rlt.w = af_tmp_rlt.w - af_tmp_rlt.x;
		af_tmp_rlt.h = af_tmp_rlt.h - af_tmp_rlt.y;
		centerX = af_tmp_rlt.x + (af_tmp_rlt.w / 2);
		centerY = af_tmp_rlt.y + (af_tmp_rlt.h / 2);
	} else {
		centerX = af_fnal_rlt->cx;
		centerY = af_fnal_rlt->cy;
	}

	printf("[1]af_tmp_rlt %d %d %d %d validpvcnt %u, updaterlt: %u\r\n", af_tmp_rlt.x,af_tmp_rlt.y,af_tmp_rlt.w,af_tmp_rlt.h,validpvcnt, updaterlt);

	//check if cneter change
	if (ABS (centerX - af_fnal_rlt->cx) > CENTERCHG)
		updaterlt = 1;
	if (ABS (centerY - af_fnal_rlt->cy) > CENTERCHG)
		updaterlt = 1;

	//find zoom index
	if (validpvcnt){
		for (zoomindx = 0;zoomindx < AUTOFRAMING_INDEX - 1;zoomindx ++){
			if (af_tmp_rlt.h + HIGHCHG > af_zoom_table[zoomindx].y )
			    break;
		}
	} else {
	    zoomindx = 0;
	}
	
	//check if size change
	if (ABS (af_fnal_rlt->zindx - zoomindx) > ZINDXCHG){
		updaterlt = 1;
	}


	//re-center for zoom sizes
	af_tmp_rlt.x = centerX - (af_zoom_table[zoomindx].x / 2);
	af_tmp_rlt.y = centerY - (af_zoom_table[zoomindx].y / 2);
	af_tmp_rlt.w = af_zoom_table[zoomindx].x;
	af_tmp_rlt.h = af_zoom_table[zoomindx].y;
	printf("[2]af_tmp_rlt %d %d %d %d validpvcnt %u, updaterlt: %u\r\n", af_tmp_rlt.x,af_tmp_rlt.y,af_tmp_rlt.w,af_tmp_rlt.h,validpvcnt, updaterlt);


	//boundary checking (TODO)
	if (af_tmp_rlt.x + af_tmp_rlt.w > SENSORW){
		af_tmp_rlt.w = af_tmp_rlt.w - (af_tmp_rlt.x + af_tmp_rlt.w - SENSORW) - 4;
		af_tmp_rlt.x = af_tmp_rlt.x - (af_tmp_rlt.x + af_tmp_rlt.w - SENSORW);
	}
	if (af_tmp_rlt.y + af_tmp_rlt.h > SENSORH){
		af_tmp_rlt.h = af_tmp_rlt.h - (af_tmp_rlt.y + af_tmp_rlt.h - SENSORH) - 4;
		af_tmp_rlt.y = af_tmp_rlt.y - (af_tmp_rlt.y + af_tmp_rlt.h - SENSORH);
	}

	if (af_tmp_rlt.x < 0)
		af_tmp_rlt.x = 0;
	if (af_tmp_rlt.y < 0)
		af_tmp_rlt.y = 0;

	printf("[3]af_tmp_rlt %d %d %d %d validpvcnt %u, updaterlt: %u\r\n", af_tmp_rlt.x,af_tmp_rlt.y,af_tmp_rlt.w,af_tmp_rlt.h,validpvcnt, updaterlt);

	if (updaterlt){
		af_fnal_rlt->bufid = alg_buf_id;
		af_fnal_rlt->zindx = zoomindx;
		af_fnal_rlt->cx = centerX;
		af_fnal_rlt->cy = centerY;
		af_fnal_rlt->x = af_tmp_rlt.x;
		af_fnal_rlt->y = af_tmp_rlt.y;
		af_fnal_rlt->w = af_tmp_rlt.w;
		af_fnal_rlt->h = af_tmp_rlt.h;
		return AF_DATA_OK;
	} else {
		return AF_DATA_DUP;
	}
}

void init_autoframing_data(void)
{
	printf("[1]init_autoframing_datainit_autoframing_data\r\n");
	af_fd_rlt[0].status = AF_PINGPONG_INIT;
	af_fd_rlt[1].status = AF_PINGPONG_INIT;
	af_pv_rlt[0].status = AF_PINGPONG_INIT;
	af_pv_rlt[1].status = AF_PINGPONG_INIT;
}
//TODO: boundary checking
//TODO: no pv, reset to all frame

