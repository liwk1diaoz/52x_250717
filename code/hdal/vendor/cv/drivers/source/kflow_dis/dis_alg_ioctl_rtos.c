#include "dis_alg_ioctl.h"
#include "dis_alg_lib.h"
#include "nvt_dis.h"
#include "kflow_dis_version.h"



#if defined(__FREERTOS)
int nvt_dis_alg_ioctl(int fd, unsigned int uiCmd, void *p_arg)
{
	int ret = 0;
    CHAR version_info[32] = KFLOW_DIS_VERSION;
	UINT32                dis_only_proc_mv    = 0;
	DIS_ALG_PARAM         dis_alg_info        = {0};
	DIS_IPC_INIT          mem                 = {0};
	DIS_BLKSZ             dis_alg_blksz       = {0};
    DIS_MDS_DIM           dis_alg_mds_dim     = {0};
    DIS_MV_INFO_SIZE      dis_alg_mv_info     = {0};



	nvt_dbg(IND, "cmd:%x\n", uiCmd);


	switch (uiCmd) {
	/* Add other operations here */

	case DIS_FLOW_IOC_INIT:
		 nvt_dis_init( );
		 break;

	case DIS_FLOW_IOC_UNINIT:
		 nvt_dis_uninit( );
		 break;

    case DIS_FLOW_IOC_SET_PARAM_IN:
         memcpy(&dis_alg_info, p_arg, sizeof(DIS_ALG_PARAM));
         nvt_dis_set((void *)&dis_alg_info);
         break;

	case DIS_FLOW_IOC_GET_PARAM_IN:
		nvt_dis_get((void *)&dis_alg_info);
        memcpy(p_arg, &dis_alg_info, sizeof(DIS_ALG_PARAM));
		break;

	case DIS_FLOW_IOC_SET_IMG_DMA_IN:
        memcpy(&mem, p_arg, sizeof(DIS_IPC_INIT));
		nvt_dis_buffer_set((void*)&mem);
		break;

	case DIS_FLOW_IOC_SET_ONLY_MV_EN:
        memcpy(&dis_only_proc_mv, p_arg, sizeof(UINT32));
		//dis_set_only_mv(dis_only_proc_mv);
		break;

	case DIS_FLOW_IOC_GET_ONLY_MV_EN:
		//dis_only_proc_mv = dis_get_only_mv();
		memcpy(p_arg, &dis_only_proc_mv, sizeof(UINT32));
		break;

	case DIS_FLOW_IOC_GET_MV_MAP_OUT:
        dis_get_ready_motionvec(&dis_alg_mv_info);
        memcpy(p_arg, &dis_alg_mv_info, sizeof(DIS_MV_INFO_SIZE));
		break;

	case DIS_FLOW_IOC_GET_MV_MDS_DIM:
		dis_alg_mds_dim = dis_get_mds_dim( );
        memcpy(p_arg, &dis_alg_mds_dim, sizeof(DIS_MDS_DIM));
		break;

	case DIS_FLOW_IOC_SET_MV_BLOCKS_DIM:
        memcpy(&dis_alg_blksz, p_arg, sizeof(DIS_BLKSZ));
        dis_set_blksize(dis_alg_blksz);
		break;
	case DIS_FLOW_IOC_GET_MV_BLOCKS_DIM:
		dis_alg_blksz = dis_get_blksize_info();
        memcpy(p_arg, &dis_alg_blksz, sizeof(DIS_BLKSZ));
		break;
	case DIS_FLOW_IOC_TRIGGER:
		nvt_dis_process();
		break;

    //GET KLOW VERSION
    case DIS_FLOW_IOC_GET_VER:
        memcpy(p_arg, &version_info, sizeof(KFLOW_DIS_VERSION));
        break;

	default :
		break;
	}
	return ret;
}

#endif

