#include "detection.h"
#include "hd_type.h"

#define SENSORW 1920
#define SENSORH 1080
//update size condition

#define CENTERCHG 200
#define HIGHCHG 200
#define ZINDXCHG 5


#define MD		1	// face detection
#define PVCNN   PD  // PVCNN
#define FDCNN   FD  // FD
#define FTG		4	// face tracking granding
#define MAX_ALG_CNT 100
#define MIN_ALG_VALID_CNT 40 // frames
#define AUTOFRAMING_INDEX 120
#define PVCLOSE 100

#define AF_DATA_OK	1	
#define AF_DATA_ERR -1  
#define AF_DATA_DUP 0
#define ABS(s)  ((s) < 0 ? -(s) : (s))

typedef enum{
	AF_PINGPONG_INIT, //0
	AF_PINGPONG_READY, //1
	AF_PINGPONG_WRITE, //2
	AF_PINGPONG_READ, //3
	AF_PINGPONG_DONE, //4
	AF_PINGPONG_MAX
}AF_PINGPONG_STATUS;

typedef struct _det_info_ {
	INT32 id;
	INT32 x;
	INT32 y;
	INT32 w;
	INT32 h;
	INT32 tracking;
} det_info;

typedef struct _af_process_result_ {
	det_info process_result;
	INT32  validCnt;///< valid cnt
} af_process_result;

typedef struct _af_pvcnn_result_ {
	det_info people[MAX_ALG_CNT];
	AF_PINGPONG_STATUS  status;
	UINT64 sn;
} af_pvcnn_result;

typedef struct _af_fdcnn_result_ {
	det_info face[MAX_ALG_CNT];
	AF_PINGPONG_STATUS  status;
	UINT64 sn;
} af_fdcnn_result;

typedef struct _af_final_result_ {
	INT32  bufid;   ///< pp buf id
	INT32  cx;   ///< center x
	INT32  cy;   ///< center y
	INT32  zindx;///< zoom index
	INT32  x;       ///< X coordinate of the top-left point of the rectangle
	INT32  y;       ///< Y coordinate of the top-left point of the rectangle
	INT32  w;       ///< Rectangle width
	INT32  h;       ///< Rectangle height
} af_final_result;

void insert_pvcnn_result(unsigned int x, unsigned int y, unsigned int w,unsigned int h);
void insert_fdcnn_result(unsigned int x, unsigned int y, unsigned int w,unsigned int h);
int get_autoframing_result(af_final_result *af_rlt);
int IsPvwithFD(INT32 x, INT32 y, INT32 w, INT32 h);
int IsPVinProcess(INT32 x, INT32 y, INT32 w, INT32 h);
void init_autoframing_data(void);

