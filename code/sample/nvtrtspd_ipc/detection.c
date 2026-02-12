#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include "detection.h"

int system_h;
int system_w;
face_result *gfd_result;

motion_result *gmd_result;

people_result *gpd_result;

face_tracking_result *gftg_result;

int get_struct_size(unsigned int type, int *struct_len)
{
	int ret = 0;

	switch (type) {
	case MD:
		*struct_len += sizeof(motion_result) + 4;
		break;
	case PD:
		*struct_len += sizeof(people_result) + 4;
		break;
	case FD:
		*struct_len += sizeof(face_result) + 4;
		break;
	case FTG:
		*struct_len += sizeof(face_tracking_result) + 4;
		break;
	default:
		printf("get struct size: detect type(%d) error!\n", type);
		ret = -1;
		break;
	}
	return ret;
}

#if 0
unsigned int get_timestamp_by_type(unsigned int type)
{
	unsigned int timestamp;
	switch (type) {
	case MD:
		timestamp = gftg_result->timestamp;
		break;
	case PD:
		timestamp = gpd_result->timestamp;
		break;
	case FD:
		timestamp = gfd_result->timestamp;
		break;
	case FTG:
		timestamp = gftg_result->timestamp;
		break;
	default:
		timestamp = -1;
		printf("get timestamp: detect type(%d) error!\n", type);
		break;
	}
	return timestamp;
}
#endif
int copy_result_data(unsigned int type, unsigned char *data, int *len)
{
	int ret = 0;

	switch (type) {
	case MD:
		memcpy(data + *len, (unsigned char *)gmd_result, sizeof(motion_result));
		*len += sizeof(motion_result);
		break;
	case PD:
#if 0
		for (i = 0; i < (int)gpd_result->people_num; i++) {
			printf("People id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gpd_result->people[i].id, gpd_result->people[i].x, gpd_result->people[i].y, gpd_result->people[i].w, gpd_result->people[i].h);
		}
#endif
		memcpy(data + *len, (unsigned char *)gpd_result, sizeof(people_result));
		*len += sizeof(people_result);
		break;
	case FD:
#if 0
		for (i = 0; i < (int)gfd_result->face_num; i++) {
			printf("Face id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gfd_result->face[i].id, gfd_result->face[i].x, gfd_result->face[i].y, gfd_result->face[i].w, gfd_result->face[i].h);

		}
#endif
		memcpy(data + *len, (unsigned char *)gfd_result, sizeof(face_result));
		*len += sizeof(face_result);
		break;
	case FTG:
#if 0
		for (i = 0; i < (int)gftg_result->face_num; i++) {
			printf("FTG id(%d) x(%u)  y(%u) width(%u) height(%u)\n", gftg_result->ftg[i].id, gftg_result->ftg[i].x, gftg_result->ftg[i].y, gftg_result->ftg[i].w, gftg_result->ftg[i].h);

		}
#endif
		memcpy(data + *len, (unsigned char *)gftg_result, sizeof(face_tracking_result));
		*len += sizeof(face_tracking_result);
		break;
	default:
		printf("copy_result_data: detect type(%d) error!\n", type);
		ret = -1;
		break;
	}
	return ret;
}


static void init_motion_detection(void)
{
	gmd_result = (motion_result *)malloc(sizeof(motion_result));
	memset(gmd_result, 0xff, sizeof(motion_result));
	gmd_result->identity[0] = 0x4D; //'M'
	gmd_result->identity[1] = 0x4F; //'O'
	gmd_result->identity[2] = 0x54; //'T'
	gmd_result->identity[3] = 0x4E; //'N'
	gmd_result->mb_w_num = system_w / 32;
	gmd_result->mb_h_num = system_h / 32;
}

static void init_people_detection(void)
{
	gpd_result = (people_result *)malloc(sizeof(people_result));
	if(gpd_result == NULL)
	{
		printf("[%s] gpd_result is NULL\n",__func__);
		return;
	}
	memset(gpd_result, 0xff, sizeof(people_result));
	gpd_result->identity[0] = 0x50; //'P'
	gpd_result->identity[1] = 0x50; //'P'
	gpd_result->identity[2] = 0x4C; //'L'
	gpd_result->identity[3] = 0x45; //'E'
}


void generate_pdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
	//printf("index = %d,id=%d,x=%x,y=%d,w=%d,h=%d\n",index,id,x,y,w,h);
	gpd_result->people[index].id = id;
    gpd_result->people[index].x = x;
    gpd_result->people[index].y = y;
    gpd_result->people[index].w = w;
    gpd_result->people[index].h = h;
}

//void generate_pdt(int num, unsigned int timestamp,unsigned int x_off,unsigned int y_off)
void generate_pdt(int num, short delay_fps,short trace_fps,unsigned int x_off,unsigned int y_off)
{
	if(gpd_result == NULL)
	{
        printf("[%s] gpd_result is NULL\n",__func__);
        return;
	}
	//gpd_result->timestamp = timestamp;
	gpd_result->trace_fps = trace_fps;
	gpd_result->delay_fps = delay_fps;
	if(num > MAX_DETECT_COUNT)
		num = MAX_DETECT_COUNT;
	gpd_result->people_num = num;
	return ;
}

static void init_face_detection(void)
{
	gfd_result = (face_result *)malloc(sizeof(face_result));
	memset(gfd_result, 0xff, sizeof(face_result));
	gfd_result->identity[0] = 0x46; //'F'
	gfd_result->identity[1] = 0x41; //'A'
	gfd_result->identity[2] = 0x43; //'C'
	gfd_result->identity[3] = 0x45; //'E'
}

void generate_fdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h)
{
	gfd_result->face[index].id = id;
    gfd_result->face[index].x = x;
    gfd_result->face[index].y = y;
    gfd_result->face[index].w = w;
    gfd_result->face[index].h = h;
}

//void generate_fdt(int num,unsigned int timestamp,unsigned int x_off,unsigned int y_off)
void generate_fdt(int num,short delay_fps,short trace_fps,unsigned int x_off,unsigned int y_off)
{
	if(gfd_result == NULL)
	{
		printf("[%s] fdt_result is NULL\n",__func__);
		return;
	}

    //gfd_result->timestamp = timestamp;
	gfd_result->delay_fps = delay_fps;
	gfd_result->trace_fps = trace_fps;
    if(num > MAX_DETECT_COUNT)
        num = MAX_DETECT_COUNT;
    gfd_result->face_num = num;

}
static void init_face_tracking_granding(void)
{

	gftg_result = (face_tracking_result *)malloc(sizeof(face_tracking_result));
	memset(gftg_result, 0x0, sizeof(face_tracking_result));
	gftg_result->identity[0] = 0x46; //'F'
	gftg_result->identity[1] = 0x54; //'T'
	gftg_result->identity[2] = 0x47; //'G'
	gftg_result->identity[3] = 0x52; //'R'

}

void init_detect_data(void)
{
	init_motion_detection();
	init_people_detection();
	init_face_detection();
	init_face_tracking_granding();
	return ;
}
