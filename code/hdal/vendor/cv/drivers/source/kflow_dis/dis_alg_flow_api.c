#include "dis_alg_flow_api.h"
#include "dis_alg_lib.h"
#include "kflow_dis_version.h"

#include "kwrap/task.h"


#define PROF                           DISABLE
#define DEBUG_SUBIN                    DISABLE
#define msleep(x)                      vos_task_delay_ms(x)

extern BOOL g_dump_global_mv;
extern UINT32 g_dis_isp_id;
extern UINT32 g_dis_scale_up_ratio;
extern UINT32 g_dis_subsample_level;


#ifdef DEBUG_KDRV_DIS_ALG
int exam_kflow_dis_task_init(unsigned char argc, char **p_argv)
{
	int ret = 0;
	struct	vos_mem_cma_info_t buf_info = {0};
	VOS_MEM_CMA_HDL vos_mem_id;

	UINT32 mem_base = 0;
	UINT32 mem_end = 0;
	UINT32 mem_size = 0;//0x14800000;


    MEM_RANGE   share_mem;

    DIS_ALG_PARAM dis_parm_in;

    UINT32 subin_parm = 0;


#if PROF
    static struct timeval tstart, tend;
    static UINT64 cur_time = 0, sum_time = 0;
    static UINT32 icount = 0;
#endif

#if DEBUG_SUBIN
#if defined(__FREERTOS)
    sscanf(p_argv[0], "%ld", &subin_parm);
#else
    sscanf(p_argv[0], "%d", &subin_parm);
#endif
#endif


	mem_size = nvt_dis_cal_buffer_size();

	if (0 != vos_mem_init_cma_info(&buf_info, VOS_MEM_CMA_TYPE_CACHE, mem_size)) {
		nvt_dbg(ERR, "vos_mem_init_cma_info: init buffer fail. \r\n");
		return -1;
    } else {
		vos_mem_id = vos_mem_alloc_from_cma(&buf_info);
		if (NULL == vos_mem_id) {
			nvt_dbg(ERR, "get buffer fail\n");
            return -1;
    	}
	}

	mem_base = (UINT32)buf_info.vaddr;
	mem_end = mem_base + mem_size;

    DBG_ERR("mem_size = 0x%x, mem_end = 0x%x\n\r", mem_size, mem_end);

    share_mem.addr = mem_base;
    share_mem.size = mem_size;

    nvt_dis_buffer_set((void*)&share_mem);

    if(subin_parm == 0) {
        dis_parm_in.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_1X;
    } else if(subin_parm == 1) {
        dis_parm_in.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_2X;
    } else if(subin_parm == 2) {
        dis_parm_in.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_4X;
    } else if(subin_parm == 3) {
        dis_parm_in.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_8X;
    }

    dis_parm_in.scale_up_ratio = 1100;

    if(dis_alg_task_set(&dis_parm_in) != E_OK){
        DBG_ERR("dis alg set error\n\r");
    }

    dis_alg_task_init();

    return ret;
}

int exam_kflow_dis_task_reset(unsigned char argc, char **p_argv)
{
	int ret = 0;
	
    while(dis_alg_task_status()!= DIS_TASK_STATUS_START){
        DBG_ERR("dis_alg_task_init: %d\n\r", dis_alg_task_status());
    }
	
    dis_alg_task_reset();

    return ret;
}

int exam_kflow_dis_task_get(unsigned char argc, char **p_argv)
{
	int ret = 0;

    DIS_ALG_PARAM dis_parm_out;

    UINT32 frm_cnt = 300;
    UINT32 idx = 1;


#if DEBUG_SUBIN
#if defined(__FREERTOS)
    sscanf(p_argv[0], "%ld", &frm_cnt);
#else
    sscanf(p_argv[0], "%d", &frm_cnt);
#endif
#endif

   while(idx) {
       dis_parm_out.frm_cnt = frm_cnt;
       if(dis_alg_task_get(&dis_parm_out) == E_OK){
            DBG_IND("dis_parm_out:subsample_sel: %d\n\r",dis_parm_out.subsample_sel);
            DBG_IND("dis_parm_out:scale_up_ratio: %d\n\r",dis_parm_out.scale_up_ratio);
            DBG_ERR("dis_parm_out:out_roi_mv:(%d, %d), valid: %d\n\r",dis_parm_out.global_mv.x,dis_parm_out.global_mv.y, dis_parm_out.global_mv_valid);
            DBG_ERR("dis_parm_out:frame_count: %d\n\r",dis_parm_out.frm_cnt);
            frm_cnt += 1;
        }
   }

    return ret;
}

int exam_kflow_dis_task_uninit(unsigned char argc, char **p_argv)
{
	int ret = 0;

    while(dis_alg_task_status()!= DIS_TASK_STATUS_START){
        DBG_ERR("dis_alg_task_init: %d\n\r", dis_alg_task_status());
    }
    dis_alg_task_uninit();

    return ret;
}
#endif

int exam_kflow_dis_read_version(unsigned char argc, char **pargv)
{

	CHAR* kflow_dis_version;
	kflow_dis_version = KFLOW_DIS_VERSION;
	DBG_DUMP("kflow_dis_version:%s\n", kflow_dis_version);

	return 0;
}

int exam_kflow_dis_dump_global_mv_start(unsigned char argc, char **pargv)
{

	g_dump_global_mv = TRUE;

	return 0;
}

int exam_kflow_dis_dump_global_mv_end(unsigned char argc, char **pargv)
{

	g_dump_global_mv = FALSE;

	return 0;
}

int exam_kflow_dis_dump_param_in(unsigned char argc, char **pargv)
{

	DBG_DUMP("kflow_dis: <isp_id: %d>, <ratio: %d>, <subsample: %d> \n\r", g_dis_isp_id, g_dis_scale_up_ratio, g_dis_subsample_level);
	return 0;
}
int exam_kflow_dis_dump_buffsize(unsigned char argc, char **pargv)
{
	DBG_DUMP("kflow_dis: <buffsize: 0x%x> \n\r", nvt_dis_cal_buffer_size());
	return 0;
}
