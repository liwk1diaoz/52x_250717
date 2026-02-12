#ifndef _DETECTION_H_
#define _DETECTION_H_

#define MD		1	// face detection
#define PD		2	// people detection
#define FD		3	// face detection
#define FTG		4	// face tracking granding

#define MD_FPS			        30
#define PD_FPS			        5
#define FD_FPS			        10
#define MAX_DETECT_COUNT		16

#define ROTATE_MODE          	0  //0 for off, 1 for right 90 degree, 2 for flip horizontally

typedef struct _motion_result_ {
	unsigned char identity[4];
	unsigned int timestamp;
	unsigned int mb_w_num;
	unsigned int mb_h_num;
	unsigned char evt_map_addr[400];
} motion_result;

typedef struct _detection_info_ {
	unsigned int id;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	unsigned int tracking;
} detection_info;

typedef struct _people_result_ {
	unsigned char identity[4];
	unsigned int timestamp;
	unsigned int people_num;
	detection_info people[MAX_DETECT_COUNT];
} people_result;

typedef struct _face_result_ {
	unsigned char identity[4];
	unsigned int timestamp;
	unsigned int face_num;
	detection_info face[MAX_DETECT_COUNT];
} face_result;

typedef struct _face_tracking_info_ {
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	unsigned int id;
	// FTG clarity
	unsigned int luma; //< result face average luma range [0, 255]
	unsigned int clarity; //< result face average clarity range [0, 255]
	unsigned int contrast; //< result face average contrast range [0, 255]
	// FTG landmark
	unsigned int lstate;	//< if landmark success is 1, failed is -1;
	unsigned int yaw;	//< result face yaw angle
	unsigned int pitch;	//< result face pitch angle
	unsigned int roll;	//< result face roll angle
	unsigned int lscore;	//< result face landmark score range [-1024 1024]
	unsigned int lnumber;	//< result face landmark number
} face_tracking_info;
typedef struct _face_tracking_result_ {
	unsigned char identity[4];
	unsigned int timestamp;
	unsigned int has_land_mark;
	unsigned int face_num;
	face_tracking_info ftg[MAX_DETECT_COUNT];
} face_tracking_result;

int get_struct_size(unsigned int type, int *struct_len);
unsigned int get_timestamp_by_type(unsigned int type);
int copy_result_data(unsigned int type, unsigned char *data, int *len);
void generate_pdt(int num, unsigned int timestamp,unsigned int x_off,unsigned int y_off);
void generate_fdt(int num,unsigned int timestamp,unsigned int x_off,unsigned int y_off);
void init_detect_data(void);
void generate_fdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h);
void generate_pdt_idx(int index,unsigned int id, unsigned int x, unsigned int y, unsigned int w,unsigned int h);
void init_motion_detection(void);
void init_people_detection(void);
void init_face_detection(void);
void init_face_tracking_granding(void);
#endif
