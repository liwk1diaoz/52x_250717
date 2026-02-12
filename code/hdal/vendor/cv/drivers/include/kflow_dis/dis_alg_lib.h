#ifndef _NVT_DIS_API_H_
#define _NVT_DIS_API_H_

#include "dis_alg_platform.h"

/**
	DIS status
*/
//@{
typedef enum {
	DIS_TASK_STATUS_CLOSE   = 0,
	DIS_TASK_STATUS_OPEN    = 1,
	DIS_TASK_STATUS_START   = 2,
	DIS_TASK_STATUS_RUN     = 3,
	DIS_TASK_STATUS_PAUSE   = 4,
	ENUM_DUMMY4WORD(DIS_FLOW_TASK_STATUS)
} DIS_FLOW_TASK_STATUS;
//@}

/**
	DIS SUBSAMPLE INPUT
*/
//@{
typedef enum {
	DIS_ALG_SUBSAMPLE_SEL_1X = 0,	    /* use all pixel for motion vector */
	DIS_ALG_SUBSAMPLE_SEL_2X = 1,		/* use 1/2 pixel for motion vector */
	DIS_ALG_SUBSAMPLE_SEL_4X = 2,		/* use 1/4 pixel for motion vector, only support 528*/
	DIS_ALG_SUBSAMPLE_SEL_8X = 3,		/* use 1/8 pixel for motion vector, only support 528*/
	ENUM_DUMMY4WORD(DIS_ALG_SUBSAMPLE_SEL)
} DIS_ALG_SUBSAMPLE_SEL;
//@}


/**
    DIS ALG PARAMETER
*/
//@{
typedef struct {
	DIS_ALG_SUBSAMPLE_SEL subsample_sel;      ///< <set/get>image size for mv
	UINT32                scale_up_ratio;     ///< <set/get>mv scale up ratio, default: 1000
	IPOINT                global_mv;          ///< <get>output MV, mv_out = ((mv*scale_up_ration)<<12)/size/1000
    BOOL                  global_mv_valid;    ///< <get>output MV is valid or not
	UINT32 frm_cnt;		                      ///< <set/get>corresponding raw frame cnt, should be set in dis_alg_task_get
} DIS_ALG_PARAM;
//@}


/**
    Get the working buffer size that nvt_dis need
    @return UINT32: the working buffer size that nvt_dis need.
*/
extern UINT32	 nvt_dis_cal_buffer_size(void);

/**
    Set the working buffer size that nvt_dis need.
    @param mem: struct should have addr and size.
    @return ER: E_OK for ok. other for error.
*/
extern ER        nvt_dis_buffer_set(void* mem);

/**
    Init buff size need by nvt_dis, and enable dis_eth.
    @param p_buf: the address for buff.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_init(void);
/**
    Uninit nvt_dis.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_uninit(void);

/**
    Process the nvt_dis.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_process(void);

/**
    Set nvt_dis parameter.
    @param p_dis_alg_set_info: the address of struct DIS_ALG_PARAM.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_set(void *p_dis_alg_set_info);

/**
    Get nvt_dis parameter.
    @param p_dis_alg_set_info: the address of struct DIS_ALG_PARAM.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_get(void *p_dis_alg_get_info);

/**
    Reset nvt_dis mv.
    @return ER, E_OK for ok. other for error.
*/
extern ER        nvt_dis_reset(void);

/**
    Open dis task.
    Status: DIS_TASK_STATUS_CLOSE -> DIS_TASK_STATUS_OPEN
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_open(void);

/**
    Close dis task.
    Status: Other -> DIS_TASK_STATUS_CLOSE
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_close(void);

/**
    Trigger dis task.
    Trigger once do dis_process once.
    Status: DIS_TASK_STATUS_START -> DIS_TASK_STATUS_RUN, when trigger finish DIS_TASK_STATUS_RUN->DIS_TASK_STATUS_START
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_trigger(void);

/**
    Init dis task.
    Status: DIS_TASK_STATUS_OPEN -> DIS_TASK_STATUS_START(when init finish)
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_init(void);

/**
    Uninit dis task.
    Status: DIS_TASK_STATUS_START -> DIS_TASK_STATUS_OPEN(when uninit finish)
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_uninit(void);

/**
    Pause dis task.
    Status: DIS_TASK_STATUS_START -> DIS_TASK_STATUS_PAUSE
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_pause(void);

/**
    Resume dis task.
    Status: DIS_TASK_STATUS_PAUSE -> DIS_TASK_STATUS_START
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_resume(void);

/**
    Reset dis task.
    @return ER, E_OK for ok. other for error.
*/
extern ER        dis_alg_task_reset(void);

/**
    Set dis task info after init. Status need DIS_TASK_STATUS_START
    @param dis_alg_set: the struct DIS_ALG_PARAM for set.
    @return ER, E_OK for ok. E_OBJ for not ready. other for error.
*/
extern ER        dis_alg_task_set(DIS_ALG_PARAM *dis_alg_set);

/**
    Get dis task info after trigger. Status need DIS_TASK_STATUS_START
    @param dis_alg_get: the struct DIS_ALG_PARAM for get.
    @return ER, E_OK for ok. E_OBJ for not ready. other for error.
*/
extern ER        dis_alg_task_get(DIS_ALG_PARAM *dis_alg_get);


/**
    Get dis task status.
    @param dis_alg_get: the struct DIS_ALG_PARAM for get.
    @return ER, E_OK for ok. other for error.
*/
extern DIS_FLOW_TASK_STATUS dis_alg_task_status(void);

extern ER       dis_init_module(UINT32 id_list);
extern ER       dis_uninit_module(void);


#endif
