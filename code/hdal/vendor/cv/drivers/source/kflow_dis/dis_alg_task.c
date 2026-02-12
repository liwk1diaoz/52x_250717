#include "dis_alg_task.h" //for KFLOW_TASK_FUNC



THREAD_HANDLE KFLOW_DIS_TSK_ID = 0;
SEM_HANDLE dis_semi_id;
DIS_FLOW_TASK_STATUS dis_status = DIS_TASK_STATUS_CLOSE;

#define PROF                           DISABLE

ER dis_alg_task_open(void)
{
    ER rt = E_OK;
    if (dis_status == DIS_TASK_STATUS_CLOSE) {

        dis_status = DIS_TASK_STATUS_OPEN;

    	OS_CONFIG_FLAG(dis_semi_id);

    	vos_flag_clr(dis_semi_id, FLGPTN_BIT_ALL); //clear all flag

    	THREAD_CREATE(KFLOW_DIS_TSK_ID, kflow_dis_task, NULL, "kflow_dis_tsk");
    	if (KFLOW_DIS_TSK_ID == 0) {
    		DBG_ERR("task create fail!\r\n");
    		return -1;
    	}
        THREAD_SET_PRIORITY(KFLOW_DIS_TSK_ID, KFLOW_DIS_TSK_PRI);
        THREAD_RESUME(KFLOW_DIS_TSK_ID);
    } else {
		DBG_ERR("dis status fail = %d, not close before open\r\n", dis_status);
		rt = E_SYS;
    }
	return rt;
}

ER dis_alg_task_close(void)
{
    ER rt = E_OK;
	FLGPTN flag = 0;

    if (dis_status == DIS_TASK_STATUS_START) {
		dis_status = DIS_TASK_STATUS_OPEN;
		wai_flg(&flag, dis_semi_id, FLGPTN_IDLE, TWF_ORW | TWF_CLR);
		vos_flag_set(dis_semi_id, FLGPTN_UNINIT);
	} else if (dis_status != DIS_TASK_STATUS_CLOSE) {
		dis_status = DIS_TASK_STATUS_OPEN;
	}

	if (dis_status == DIS_TASK_STATUS_OPEN) {
        dis_status = DIS_TASK_STATUS_CLOSE;
		vos_flag_clr(dis_semi_id, FLGPTN_IDLE); //clear IDLE before send cmd
	    vos_flag_set(dis_semi_id, FLGPTN_STOP); //send STOP cmd

        KFLOW_DIS_TSK_ID = 0; //clear task id

		rt = E_OK;
	} else {
		DBG_ERR("dis status fail %d\r\n", dis_status);
		rt = E_SYS;
	}

	return rt;
}

ER dis_alg_task_init(void)
{
	ER rt = E_OK;

    if (dis_status == DIS_TASK_STATUS_OPEN) {
        vos_flag_clr(dis_semi_id, FLGPTN_IDLE);
        vos_flag_set(dis_semi_id, FLGPTN_INIT); //send STOP cmd
        rt = E_OK;
    } else {
        DBG_ERR("dis status fail %d not open before start\r\n", dis_status);
        rt = E_SYS;
    }

	return rt;
}

ER dis_alg_task_trigger(void)
{
	ER rt = E_OK;
	FLGPTN flag = 0;
	
    if (dis_status == DIS_TASK_STATUS_START)  {
        wai_flg(&flag, dis_semi_id, FLGPTN_IDLE, TWF_ORW | TWF_CLR);
        vos_flag_set(dis_semi_id, FLGPTN_PROC);
    } else if (dis_status == DIS_TASK_STATUS_OPEN){
		DBG_ERR("dis status fail %d, must init before trigger at open status\r\n", dis_status);
		rt = E_SYS;
	} else if (dis_status == DIS_TASK_STATUS_PAUSE){
		//DBG_ERR("dis status fail %d, must resume before trigger at pause status\r\n", dis_status);
		//rt = E_SYS;
	} else {
		DBG_ERR("dis status fail %d\r\n", dis_status);
		rt = E_SYS;
	}

	return rt;
}


ER dis_alg_task_uninit(void)
{
	ER rt = E_OK;
	FLGPTN flag = 0;

    if (dis_status == DIS_TASK_STATUS_START) {
        wai_flg(&flag, dis_semi_id, FLGPTN_IDLE, TWF_ORW | TWF_CLR);
        vos_flag_set(dis_semi_id, FLGPTN_UNINIT); //send STOP cmd
    } else {
		DBG_ERR("dis status fail %d\r\n", dis_status);
		rt = E_SYS;
	}

	return rt;
}

ER dis_alg_task_pause(void)
{
	ER rt = E_OK;
	FLGPTN flag = 0;

	if (dis_status == DIS_TASK_STATUS_START) {
		dis_status = DIS_TASK_STATUS_PAUSE;
		wai_flg(&flag, dis_semi_id, FLGPTN_IDLE, TWF_ORW | TWF_CLR);
		vos_flag_set(dis_semi_id, FLGPTN_PAUSE);
		rt = E_OK;
	} else {
		DBG_ERR("dis status fail %d!!\r\n", dis_status);
		rt = E_SYS;
	}

	return rt;
}

ER dis_alg_task_resume(void)
{
	ER rt = E_OK;

	if (dis_status == DIS_TASK_STATUS_PAUSE) {
		dis_status = DIS_TASK_STATUS_START;
		//vos_flag_clr(dis_semi_id, FLGPTN_IDLE);
		//vos_flag_set(dis_semi_id, FLGPTN_PROC);
		rt = E_OK;
	} else {
		DBG_ERR("dis status fail %d!!\r\n", dis_status);
		rt = E_SYS;
	}

	return rt;
}

ER dis_alg_task_reset(void)
{
	ER rt = E_OK;

	dis_alg_task_pause();
	
	nvt_dis_reset();

	dis_alg_task_resume();


	return rt;
}

DIS_FLOW_TASK_STATUS dis_alg_task_status(void)
{
	return dis_status;
}


THREAD_DECLARE(kflow_dis_task, arglist)
{
	FLGPTN flag = 0;
	BOOL is_continue = 1;
    ER ret = E_OK;

#if PROF
    static struct timeval tstart, tend;
    static UINT64 cur_time = 0, sum_time = 0;
    static UINT32 icount = 0;
#endif

	THREAD_ENTRY();

	while (is_continue) {
		set_flg(dis_semi_id, FLGPTN_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&flag, dis_semi_id, FLGPTN_INIT | FLGPTN_PROC | FLGPTN_STOP | FLGPTN_PAUSE | FLGPTN_UNINIT, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		
		DBG_IND("flag=%d, dis_semi_id = %d\r\n", flag, dis_semi_id);

		if (flag & FLGPTN_STOP) {
			is_continue = 0;
            rel_flg(dis_semi_id);
			continue;
		}
 		if (flag & FLGPTN_INIT) {
            ret = nvt_dis_init();
            if (ret != E_OK) {
                DBG_ERR("err:nvt_dis_init error %d\r\n",ret);
                break;
            }
            dis_status = DIS_TASK_STATUS_START;
			continue;
		}
		if (flag & FLGPTN_PROC) {
            #if PROF
                do_gettimeofday(&tstart);
            #endif
            ret = nvt_dis_process();
            if (ret != E_OK) {
                DBG_ERR("err:nvt_dis_only_calc_vector error %d\r\n",ret);
                break;
            }
            #if PROF
                do_gettimeofday(&tend);
                cur_time = (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
                sum_time += cur_time;
                //mean_time = sum_time/(++icount);
                DBG_DUMP("[nvt_dis_only_calc_vector] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time, sum_time, ++icount);
            #endif
            continue;
		}
        if (flag & FLGPTN_UNINIT) {
            ret = nvt_dis_uninit();
            if (ret != E_OK) {
                DBG_ERR("err:nvt_dis_uninit error %d\r\n",ret);
                break;
            }
            dis_status = DIS_TASK_STATUS_OPEN;
			continue;
		}
		/*if (flag & FLGPTN_PAUSE) {
            dis_status = DIS_TASK_STATUS_PAUSE;
			continue;
		}*/
	}

	THREAD_RETURN(0);
}

ER dis_alg_task_set(DIS_ALG_PARAM *dis_alg_set)
{
	ER rt = E_OK;

    if(dis_status != DIS_TASK_STATUS_OPEN){
        DBG_ERR("dis_alg_task_set err! status must be DIS_TASK_STATUS_OPEN %d, not %d", DIS_TASK_STATUS_OPEN, dis_status);
        rt = E_OBJ;
    }else{
        rt = nvt_dis_set(dis_alg_set);
    }

	return rt;
}

ER dis_alg_task_get(DIS_ALG_PARAM *dis_alg_get)
{
	ER rt = E_OK;

    rt = nvt_dis_get(dis_alg_get);


	return rt;
}
