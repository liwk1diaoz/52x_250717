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


void init_motion_detection(void)
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

void init_people_detection(void)
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
    generate_pdt_idx(0,0,0,0,0,0);
    generate_pdt_idx(1,0,0,0,0,0);
    generate_pdt_idx(2,0,0,0,0,0);
    generate_pdt_idx(3,0,0,0,0,0);
    generate_pdt_idx(4,0,0,0,0,0);
    generate_pdt_idx(5,0,0,0,0,0);
    generate_pdt_idx(6,0,0,0,0,0);
    generate_pdt_idx(7,0,0,0,0,0);
    generate_pdt_idx(8,0,0,0,0,0);
    generate_pdt_idx(9,0,0,0,0,0);
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
 
void generate_pdt(int num, unsigned int timestamp,unsigned int x_off,unsigned int y_off)
{
	if(gpd_result == NULL)
	{
        printf("[%s] gpd_result is NULL\n",__func__);
        return;
	}
	gpd_result->timestamp = timestamp;
	if(num > MAX_DETECT_COUNT)
		num = MAX_DETECT_COUNT;
	gpd_result->people_num = num;
#if 0
	generate_pdt_idx(0,332,100 + x_off,100 + y_off,100,400);
    generate_pdt_idx(1,2,400 + x_off,0 + y_off,200,400);
    generate_pdt_idx(2,323,500 + x_off,200 + y_off,300,400);
    generate_pdt_idx(3,24,600+ x_off,20 + y_off,200,100);
    generate_pdt_idx(4,51,700 + x_off,30 + y_off,200,200);
    generate_pdt_idx(5,46,600 + x_off,500 + y_off,200,100);
    generate_pdt_idx(6,37,50+ x_off,600 + y_off,200,300);
    generate_pdt_idx(7,82,140 + x_off,800 + y_off,200,200);
    generate_pdt_idx(8,19,320 + x_off,700 + y_off,10,10);	
    generate_pdt_idx(9,222,600+ x_off,140 + y_off,30,10);
#endif
	return ;
}

void init_face_detection(void)
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

void generate_fdt(int num,unsigned int timestamp,unsigned int x_off,unsigned int y_off)
{
	if(gfd_result == NULL)
	{	
		printf("[%s] fdt_result is NULL\n",__func__);
		return;
	}
	
    gfd_result->timestamp = timestamp;
    if(num > MAX_DETECT_COUNT)
        num = MAX_DETECT_COUNT;
    gfd_result->face_num = num;
#if 0

    generate_fdt_idx(0,223,0 + x_off,0 + y_off,100,40);
    generate_fdt_idx(1,2,0 + x_off,0 + y_off,20,400);
    generate_fdt_idx(2,323,0 + x_off,0 + y_off,30,400);
    generate_fdt_idx(3,24,0 + x_off,0 + y_off,400,10);
    generate_fdt_idx(4,51,0 + x_off,0 + y_off,400,20);
    generate_fdt_idx(5,46,0 + x_off,0 + y_off,40,300);
    generate_fdt_idx(6,37,0 + x_off,0 + y_off,40,40);
    generate_fdt_idx(7,82,0 + x_off,0 + y_off,20,200);
    generate_fdt_idx(8,19,0 + x_off,0 + y_off,10,100);
    generate_fdt_idx(9,222,0 + x_off,0 + y_off,30,100);
#endif

}
void init_face_tracking_granding(void)
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
