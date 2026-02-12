/**
	@brief Source file of vendor user-space net API.

	@file vendor_ai_net.c

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"

#include "vendor_common.h"
#include "vendor_ai_comm.h"
#include "vendor_ai.h"
#include "vendor_ai_plugin.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_dsp/vendor_ai_dsp.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_gen.h"

#include "vendor_ai_net_mem.h"
#include "vendor_ai_net_group.h"
#include "vendor_ai_net_debug.h"
#include "vendor_ai_net_cmd.h"
#include "kwrap/platform.h"
#include "kwrap/file.h"
#include "vendor_ai_comm_flow.h"

#if defined(__LINUX)
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include "vendor_ai_net/nn_net.h" // for NN_DLI
#if NN_DLI
#include "vendor_ai_net/nn_dli.h"
#endif

//=============================================================
#define __CLASS__ 				"[ai][lib][net]"
#include "vendor_ai_debug.h"
//=============================================================

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

//                                            set/get             set/get/start
//                                              ^ |                  ^ |
//                                              | v                  | v
//  [UNINIT] -- init   --> [INIT] -- open  --> [OPEN]  -- start --> [START]
//          <-- uninit --        <-- close --         <-- stop  --
//

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/
BOOL _vendor_ai_net_is_graph_job(VENDOR_AI_NET_JOB_OPT job_opt);
HD_RESULT _vendor_ai_net_get_iomem(UINT32 proc_id, UINT32 mctrl_id, GET_PORT_TYPE type, UINT32 port_id, VENDOR_AI_BUF *ai_buf);
HD_RESULT _vendor_ai_net_get_dest_layer_cnt_fmt(UINT32 proc_id, UINT32* cnt, UINT32* fmt);
#if NN_DLI
HD_RESULT _vendor_ai_net_nn_dli_parse_ext_info(UINT32 proc_id);
HD_RESULT _vendor_ai_net_nn_dli_parse_elementwise_parm(
	NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info);
HD_RESULT _vendor_ai_net_nn_dli_parse_resize_parm(
	NN_DLI_RESIZE_PARM *p_dli_resize_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info);
HD_RESULT _vendor_ai_net_nn_dli_parse_softmax_parm(
	NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info);
#endif
HD_RESULT _vendor_ais_split_combine_model_bin(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager);

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
VENDOR_AIS_FLOW_MEM_OFS g_ai_mem_ofs = {0};
#if defined(_BSP_NA51068_)
static VENDOR_AI_NET_MEM_INFO g_mem_info = {0};
#endif

#if CNN_AI_FASTBOOT
#define AI_FBOOT_PROC_PEROID_DEFALT     2
VENDOR_AI_ENGINE_PLUGIN pd_cfg_engine = {0};
UINT32 g_pd_bufsize = 0;
UINT32 g_fboot_rslt_size = 0;
UINT32 g_fboot_proc_period = AI_FBOOT_PROC_PEROID_DEFALT; //(deafult=2) do proc_net every 2 yuv frame for fastboot
VENDOR_AIS_FLOW_MAP_MEM_PARM g_fboot_mem = {0};           // fastboot_mem get from builtin
#endif

#define NUE2_MAX_WIDTH	4095
#define NUE2_MAX_HEIGHT	4095

extern UINT32 g_ai_support_net_max;
extern BOOL is_multi_process;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if CNN_AI_FASTBOOT
HD_RESULT vendor_ais_set_fastboot_proc_period(UINT32 frame_cnt)
{
	if (frame_cnt <= 0) {
		DBG_ERR("invalid frame_cnt(%lu) setting\r\n", frame_cnt);
		return HD_ERR_FAIL;
	}
	g_fboot_proc_period = frame_cnt;

	return HD_OK;
}

UINT16 _vendor_ai_net_make_check_sum(UINT8 *pData, UINT32 Len)
{
    UINT32 i, uiSum = 0;
    UINT16 *puiValue = (UINT16 *)pData;

    for (i = 0; i < (Len >> 1); i++) {
        uiSum += (*(puiValue + i) + i);
    }

    uiSum = ~((UINT16)(uiSum & 0x0000FFFF)) + 1;

    return (UINT16)uiSum;
}

UINT32 vendor_ai_net_get_fastboot_rslt_start_addr(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	UINT32 pd_buff_sz, fboot_rslt_addr;

	p_proc = _vendor_ai_info(proc_id);
	pd_buff_sz = p_proc->rsltbuf.size - g_fboot_rslt_size;
	fboot_rslt_addr = p_proc->rsltbuf.va + pd_buff_sz;

	return fboot_rslt_addr;
}

HD_RESULT _vendor_ai_net_dump_fastboot(VENDOR_AI_NET_INFO_PROC *p_proc, VENDOR_AIS_FLOW_KERL_START_MEM *p_mem_in_kerl)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AI_NET_CFG_WORKBUF 	 *p_rsltbuf;
	VENDOR_AI_NET_CFG_WORKBUF    pd_buff = {0}, fboot_rslt = {0};
	UINT32 data_addr, data_size;
	UINT32 load_size;
	INT ret, fd, len;
	CHAR modelbin[64];
	CHAR tmp_buf[128];
	CHAR dtsi_path[128]="/tmp/fastboot/nvt-na51055-fastboot-ai.dtsi";

	if (p_proc == NULL) {
		DBG_ERR("p_proc is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if (p_mem_in_kerl == NULL) {
		DBG_ERR("p_mem_in_kerl is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	p_mem_manager = &p_proc->mem_manager;
	p_rsltbuf = &p_proc->rsltbuf;

	// alloc pd_buff and fboot_rslt
	pd_buff.pa = p_rsltbuf->pa;
	pd_buff.va = p_rsltbuf->va;
	pd_buff.size = p_rsltbuf->size - g_fboot_rslt_size;
	fboot_rslt.pa = pd_buff.pa + pd_buff.size;
	fboot_rslt.va = pd_buff.va + pd_buff.size;
	fboot_rslt.size = g_fboot_rslt_size;

	/********* dump ai model bin *********/
	data_addr = p_mem_manager->kerl_parm.va;
	data_size = p_mem_manager->user_model.va + p_mem_manager->user_model.size - p_mem_manager->kerl_parm.va; // only (kerl + model) need to dump to bin file

	// for checksum complement
	{
		UINT16 *ptr16 = (UINT16 *)(data_addr + data_size);

		*ptr16 = 0;
		data_size += sizeof(UINT16);
		*ptr16 = _vendor_ai_net_make_check_sum((UINT8 *)data_addr, data_size);
	}

	// print mem layout
	printf("==========[mem layout]================================\n");
	printf("kerl_parm:   pa=0x%lx, va=0x%lx, size=0x%lx\n", p_mem_manager->kerl_parm.pa, p_mem_manager->kerl_parm.va, p_mem_manager->kerl_parm.size);
	printf("model:       pa=0x%lx, va=0x%lx, size=0x%lx\n", p_mem_manager->user_model.pa, p_mem_manager->user_model.va, p_mem_manager->user_model.size);
	printf("pd_buff:     pa=0x%lx, va=0x%lx, size=0x%lx\n", pd_buff.pa, pd_buff.va, pd_buff.size);
	printf("fboot_rslt:  pa=0x%lx, va=0x%lx, size=0x%lx\n", fboot_rslt.pa, fboot_rslt.va, fboot_rslt.size);
	printf("io_buff:     pa=0x%lx, va=0x%lx, size=0x%lx\n", p_mem_manager->user_buff.pa, p_mem_manager->user_buff.va, p_mem_manager->user_buff.size);
	printf("user_parm:   pa=0x%lx, va=0x%lx, size=0x%lx (Not used)\n", p_mem_manager->user_parm.pa, p_mem_manager->user_parm.va, p_mem_manager->user_parm.size);

	printf("==========[kernel space va mapping]================================\n");
	printf("kerl_start:  va=0x%lx <---> pa=0x%lx\n", p_mem_in_kerl->kerl_parm.va, p_mem_in_kerl->kerl_parm.pa);
	printf("iomem_start: va=0x%lx <---> pa=0x%lx\n", p_mem_in_kerl->user_buff.va, p_mem_in_kerl->user_buff.pa);
	
	printf("\n[write file range] >>>>>>>>>>>>> va: 0x%lx ~ 0x%lx (kerl_parm + model)\n", data_addr, data_addr+data_size);

	// write file
	snprintf(modelbin, 64-1, "/tmp/fastboot/dump_model.bin");
	if (modelbin != NULL) {
		fd = open(modelbin, O_WRONLY | O_CREAT, 0644);
		if(fd < 0){
	        printf("fail to create/open (%s)\n", modelbin);
	        return HD_ERR_FAIL;
	    }
	} else {
		printf("filepath is NULL\n");
		return HD_ERR_FAIL;
	}
	ret = write(fd, (VOID *)data_addr, data_size);
	close(fd);
	if ((UINT32)ret != data_size) {
		printf("fail to write %lu bytes to (%s), ret(%d) %s\n", data_size, modelbin, ret, strerror(errno));
	} else {
		printf("write %lu bytes to (%s) success !\n", data_size, modelbin);
	}

	/********* dump ai dtsi *********/
	fd = vos_file_open(dtsi_path, O_CREAT|O_WRONLY|O_SYNC, 0666);
	if ((VOS_FILE)(-1) == fd) {
		printf("open %s failure\r\n", tmp_buf);
		return HD_ERR_FAIL;
	}

	// [ai_model]
	len = snprintf(tmp_buf, sizeof(tmp_buf), "&fastboot {\r\n\tai_model {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	load_size = p_mem_manager->kerl_parm.size + p_mem_manager->user_model.size;
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tload_dst = <0x%lx 0x%lx>;\r\n", p_mem_manager->kerl_parm.pa, load_size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	// [ai_buf]
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n\tai_buf {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tkerl_parm = <0x%lx 0x%lx>;\r\n", p_mem_manager->kerl_parm.pa, p_mem_manager->kerl_parm.size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tuser_model = <0x%lx 0x%lx>;\r\n", p_mem_manager->user_model.pa, p_mem_manager->user_model.size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tpd_buff = <0x%lx 0x%lx>;\r\n", pd_buff.pa, pd_buff.size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tfboot_rslt = <0x%lx 0x%lx>;\r\n", fboot_rslt.pa, fboot_rslt.size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tio_buff = <0x%lx 0x%lx>;\r\n", p_mem_manager->user_buff.pa, p_mem_manager->user_buff.size);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	// [mem_in_kerl]
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n\tmem_in_kerl {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	if (p_mem_manager->kerl_parm.pa != p_mem_in_kerl->kerl_parm.pa) {
		DBG_ERR("pa is not match! kerl_parm(0x%lx), mem_in_kerl(0x%lx)\r\n", p_mem_manager->kerl_parm.pa, p_mem_in_kerl->kerl_parm.pa);
		return HD_ERR_FAIL;
	} 
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tkerl_parm = <0x%lx 0x%lx>;\r\n", p_mem_in_kerl->kerl_parm.pa, p_mem_in_kerl->kerl_parm.va);
	vos_file_write(fd, (void *)tmp_buf, len);

	if (p_mem_manager->user_buff.pa != p_mem_in_kerl->user_buff.pa) {
		DBG_ERR("pa is not match! user_buff(0x%lx), mem_in_kerl(0x%lx)\r\n", p_mem_manager->user_buff.pa, p_mem_in_kerl->user_buff.pa);
		return HD_ERR_FAIL;
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tio_buff = <0x%lx 0x%lx>;\r\n", p_mem_in_kerl->user_buff.pa, p_mem_in_kerl->user_buff.va);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	// [user_set]
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n\tuser_set {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tproc_period = <%lu>;\r\n", g_fboot_proc_period);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n};");
	vos_file_write(fd, (void *)tmp_buf, len);

	vos_file_close(fd);

	printf("dump (%s) success !\n", dtsi_path);

	return HD_OK;
}

HD_RESULT _vendor_ai_net_set_pd_plugin(void* p_param)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_ENGINE_PLUGIN *p_user = (VENDOR_AI_ENGINE_PLUGIN *) p_param;
	VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine;

	p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&pd_cfg_engine;
	if (p_user == 0) {
		DBG_ERR("cfg engine => clear!\r\n");
		p_cfg_engine->sign = 0;
		return HD_OK;
	}

	if (p_user->sign == 0) {
		DBG_ERR("cfg engine => invalid sign!\r\n");
		return HD_ERR_SIGN;
	}
	memcpy(p_cfg_engine, p_user, sizeof(VENDOR_AI_ENGINE_PLUGIN));

	return rv;
}

HD_RESULT _vendor_ai_net_get_pd_buf_size(UINT32 proc_id, UINT32 model_addr, UINT32 *buf_size)
{
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	HD_RESULT rv = HD_OK;

	rv = vendor_ais_get_net_info(&net_info, model_addr);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	proc_layer_num = p_head->mode_ctrl_num;

#if CNN_USER_POSTPROC
	VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&pd_cfg_engine; //CPU engine: user postproc
	rv = HD_ERR_NOT_SUPPORT;
	if (p_cfg_engine->get_cb != 0) {
		rv = p_cfg_engine->get_cb(proc_id, proc_layer_num, 0, 0, 0, VENDOR_AI_PLUGIN_BUFSIZE, 0, buf_size);
		g_pd_bufsize = *buf_size;
	} else {
		DBG_ERR("p_cfg_engine->getwork_cb is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
#endif

	return rv;
}

HD_RESULT _vendor_ai_net_get_fastboot_bufsize(UINT32 proc_id, UINT32 model_addr, UINT32 *p_parm_sz, UINT32 *p_model_sz, UINT32 *p_buf_sz)
{
	UINT32 pd_sz = 0;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	if (p_parm_sz == NULL || p_model_sz == NULL || p_buf_sz == NULL) {
		DBG_ERR("invalid input param !!\n");
		return HD_ERR_NULL_PTR;
	}

	// calc require size
	vendor_ais_cal_size(model_addr, p_parm_sz, p_model_sz, p_buf_sz);

	// add pd buff size
	_vendor_ai_net_get_pd_buf_size(proc_id, model_addr, &pd_sz);
	*p_buf_sz += pd_sz;

	// add fboot rslt buff size
	g_fboot_rslt_size = (PD_MAX_OUTNUM * sizeof(PD_RESULT) + 32) * FBOOT_RSLT_NUM;
	*p_buf_sz += g_fboot_rslt_size;

	return HD_OK;
}

#define AI_FASTBOOT_MAP_VA(addr, old_base, new_base)  (new_base + (addr - old_base)) // real_va = new_base + (offset)
static UINT32 g_backbone_out_indexs[3][2] = {{112, 113}, {126, 127}, {152, 155}}; // process index for pdcnn
HD_RESULT _vendor_ai_net_map_fastboot_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 old_base_va, UINT32 old_io_base_va)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AI_LL_HEAD *p_ll_head;
	NN_DATA *p_sai, *p_sao;
	UINT32 process_index, proc_layer_num;
	UINT32 imem_cnt, in_id;
	UINT32 omem_cnt, out_id;
	UINT32 new_base_va, new_io_base_va;
	HD_RESULT er = HD_OK;

	if ((p_mem == NULL) || (p_mem->kerl_parm.va == 0)) {
		DBG_ERR("invalid memory input\r\n");
		return HD_ERR_NG;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->kerl_parm.va);
	if (er != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num  = p_head->mode_ctrl_num;

	// get new va_base
	new_base_va = p_mem->kerl_parm.va;
	new_io_base_va = p_mem->user_buff.va;

	//DBG_DUMP("%s:%d [kerl_parm] old_base(0x%lx), new_base(0x%lx)\r\n", __func__, __LINE__, old_base_va, new_base_va);
	//DBG_DUMP("%s:%d [io_buff] old_base(0x%lx), new_base(0x%lx)\r\n", __func__, __LINE__, old_io_base_va, new_io_base_va);

	// map mctrl and ll_head
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				//printf("map mctrl/ll_head, process_index(%lu)\n", process_index);
				// map mctrl va
				p_mctrl[process_index].addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].addr, old_base_va, new_base_va);

				// map ll_head va
				p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
				p_ll_head->parm_addr = AI_FASTBOOT_MAP_VA(p_ll_head->parm_addr, old_base_va, new_base_va);
			}
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("invalid trig_src(%d), proc_idx(%lu)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	// map iomem
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			// map layer 0, 1 for normal mode dumpbuf (debug)
			// and map out layer for postprocess by g_backbone_out_indexs
			if (process_index == 0 || process_index == 1 || p_mctrl[process_index].tot_trig_eng_times > 0 || 
				process_index == g_backbone_out_indexs[0][0] || process_index == g_backbone_out_indexs[0][1] ||
				process_index == g_backbone_out_indexs[1][0] || process_index == g_backbone_out_indexs[1][1] ||
				process_index == g_backbone_out_indexs[2][0] || process_index == g_backbone_out_indexs[2][1]) {
				//printf("map iomem, process_index(%lu)\n", process_index);
				// imem_addr
				p_mctrl[process_index].iomem.imem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.imem_addr, old_base_va, new_base_va);
				imem_cnt = p_mctrl->iomem.imem_cnt;
				p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;

				for (in_id = 0; in_id < imem_cnt; in_id++) {
					//printf("\tmap sai, in_id(%lu)\n", in_id);
					if (process_index == 0) {
						p_sai[in_id].va = 0; // restore input va for in_buf_num calculation
					} else {
						p_sai[in_id].va = AI_FASTBOOT_MAP_VA(p_sai[in_id].va, old_io_base_va, new_io_base_va);
					}
				}

				// omem_addr
				p_mctrl[process_index].iomem.omem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.omem_addr, old_base_va, new_base_va);
				omem_cnt = p_mctrl->iomem.omem_cnt;
				p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;

				// sao_va
				for (out_id = 0; out_id < omem_cnt; out_id++) {
					if (p_sao[out_id].va != 0) {
						if ((UINT32)OUT_BUF_ATTR_GET(&p_mctrl[process_index], NN_OUT_BUF_ATTR_PRESERVE)) {
							//printf("\tmap sao, out_id(%lu)\n", out_id);
							p_sao[out_id].va = AI_FASTBOOT_MAP_VA(p_sao[out_id].va, old_io_base_va, new_io_base_va);
						}
					}
				}
			}
			break;
		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("invalid trig_src(%d), proc_idx(%lu)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_fix_fastboot_user_parm(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AI_LL_HEAD *p_ll_head;
	NN_DATA *p_sai, *p_sao;
	UINT32 process_index, proc_layer_num;
	UINT32 imem_cnt, in_id;
	UINT32 omem_cnt, out_id;
	UINT32 old_base_va, old_io_base_va;
	UINT32 new_base_va, new_io_base_va;
	HD_RESULT er = HD_OK;

	if ((p_mem == NULL) || (p_mem->kerl_parm.va == 0)) {
		DBG_ERR("invalid memory input\r\n");
		return HD_ERR_NG;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (er != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num  = p_head->mode_ctrl_num;

	// get old va_base (Because this newly user_parm was copy from builtin kerl_parm)
	old_base_va = p_mem->kerl_parm.va;
	old_io_base_va = p_mem->user_buff.va;

	// get new va_base
	new_base_va = p_mem->user_parm.va;
	new_io_base_va = p_mem->user_buff.va;

	//DBG_DUMP("%s:%d old_base(0x%lx), new_base(0x%lx)\r\n", __func__, __LINE__, old_base_va, new_base_va);
	//DBG_DUMP("%s:%d old_io_base_va(0x%lx), new_io_base_va(0x%lx)\r\n", __func__, __LINE__, old_io_base_va, new_io_base_va);

	// map new va
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				// map mctrl va
				p_mctrl[process_index].addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].addr, old_base_va, new_base_va);

				// map ll_head va
				p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl[process_index].addr;
				p_ll_head->parm_addr = AI_FASTBOOT_MAP_VA(p_ll_head->parm_addr, old_base_va, new_base_va);

				// map iomem input va
				p_mctrl[process_index].iomem.imem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.imem_addr, old_base_va, new_base_va);
				imem_cnt = p_mctrl->iomem.imem_cnt;
				p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
				for (in_id = 0; in_id < imem_cnt; in_id++) {
					p_sai[in_id].va = AI_FASTBOOT_MAP_VA(p_sai[in_id].va, old_io_base_va, new_io_base_va);
				}

				// map iomem output va
				p_mctrl[process_index].iomem.omem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.omem_addr, old_base_va, new_base_va);
				omem_cnt = p_mctrl->iomem.omem_cnt;
				p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;
				for (out_id = 0; out_id < omem_cnt; out_id++) {
					p_sao[out_id].va = AI_FASTBOOT_MAP_VA(p_sao[out_id].va, old_io_base_va, new_io_base_va);
				}
			}
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("invalid trig_src(%d), proc_idx(%lu)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_cfg_fastboot_model(VENDOR_AI_NET_CFG_PROC *p_cfg)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_fboot_mem = &g_fboot_mem;
	UINT32 fboot_base_va, fboot_io_base_va;
	UINT32 map_size;

	if (p_cfg == NULL) {
		DBG_ERR("p_proc == NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	rv = vendor_ais_get_fastboot_builtin_mem(p_fboot_mem);
	if (rv != HD_OK) {
		DBG_ERR("get fastboot_builtin_mem fail(%d)\r\n", rv);
		return HD_ERR_NG;
	}

	/*printf("======[mem get from builtin]===================\n");
	printf("    kerl_parm (0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->kerl_parm.pa, p_fboot_mem->kerl_parm.va, p_fboot_mem->kerl_parm.size);
	printf("    user_model(0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->user_model.pa, p_fboot_mem->user_model.va, p_fboot_mem->user_model.size);
	printf("    user_buff (0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->user_buff.pa, p_fboot_mem->user_buff.va, p_fboot_mem->user_buff.size);*/

	// backup fastboot va base
	fboot_base_va = p_fboot_mem->kerl_parm.va;
	fboot_io_base_va = p_fboot_mem->user_buff.va;

	// remap va as new base
	map_size = p_fboot_mem->kerl_parm.size + p_fboot_mem->user_model.size + p_fboot_mem->user_buff.size; // map range need to cover kerl_parm + user_model + user_buff
	p_fboot_mem->kerl_parm.va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_fboot_mem->kerl_parm.pa, map_size);
	p_fboot_mem->user_model.va = p_fboot_mem->kerl_parm.va + (p_fboot_mem->user_model.pa - p_fboot_mem->kerl_parm.pa);
	p_fboot_mem->user_buff.va = p_fboot_mem->kerl_parm.va + (p_fboot_mem->user_buff.pa - p_fboot_mem->kerl_parm.pa);

	/*printf("======[normal mode mem (user space)]===========================\n");
	printf("    kerl_parm (0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->kerl_parm.pa, p_fboot_mem->kerl_parm.va, p_fboot_mem->kerl_parm.size);
	printf("    user_model(0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->user_model.pa, p_fboot_mem->user_model.va, p_fboot_mem->user_model.size);
	printf("    user_buff (0x%lx, 0x%lx, 0x%lx)\n", p_fboot_mem->user_buff.pa, p_fboot_mem->user_buff.va, p_fboot_mem->user_buff.size);*/

	// fix all fastboot mem va
	rv = _vendor_ai_net_map_fastboot_mem(p_fboot_mem, fboot_base_va, fboot_io_base_va);
	if (rv != HD_OK) {
		DBG_ERR("fix fastboot_mem fail(%d)\r\n", rv);
		return HD_ERR_NG;
	}

	p_cfg->cfg_model.pa = p_fboot_mem->kerl_parm.pa;
	p_cfg->cfg_model.va = p_fboot_mem->kerl_parm.va;
	p_cfg->cfg_model.size = p_fboot_mem->kerl_parm.size + p_fboot_mem->user_model.size;
	p_cfg->cfg_share_model.pa = p_cfg->cfg_model.pa;
	p_cfg->cfg_share_model.va = p_cfg->cfg_model.va;
	p_cfg->cfg_share_model.size = p_cfg->cfg_model.size;

	/*printf("new cfg_model from builtin:\n");
	printf("    cfg_model      (0x%lx, 0x%lx, 0x%lx)\n", p_cfg->cfg_model.pa, p_cfg->cfg_model.va, p_cfg->cfg_model.size);
	printf("    cfg_share_model(0x%lx, 0x%lx, 0x%lx)\n", p_cfg->cfg_share_model.pa, p_cfg->cfg_share_model.va, p_cfg->cfg_share_model.size);*/

	return HD_OK;
}
#endif // end of "#if CNN_AI_FASTBOOT"

HD_RESULT _vendor_ai_net_debug_dump(UINT32 proc_id, UINT32 space, UINT32 item, CHAR *filepath)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	p_proc = _vendor_ai_info(proc_id);
	if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
		DBG_ERR("proc_id(%lu) is NOT opened/started yet !!\n", proc_id);
		return HD_ERR_NOT_OPEN;
	}

	return vendor_ai_net_debug_dump(proc_id, (NET_DBG_SPACE)space, (NET_DBG_ITEM)item, &p_proc->mem_manager, filepath);
}

HD_RESULT _vendor_ai_net_debug_layer(UINT32 proc_id, UINT32 layer_opt, CHAR *filedir)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	p_proc = _vendor_ai_info(proc_id);

	if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
		DBG_ERR("proc_id(%lu) is NOT opened/started yet !!\n", proc_id);
		return HD_ERR_NOT_OPEN;
	}

	return vendor_ai_net_debug_layer(proc_id, (NET_DBG_LAYER)layer_opt, &p_proc->mem_manager, filedir);
}

HD_RESULT _vendor_ai_net_debug_performance(UINT32 proc_id, CHAR *p_model_name, UINT64 *p_proc_time, DOUBLE p_cpu_loading)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
		DBG_ERR("proc_id(%lu) is NOT started yet !!\n", proc_id);
		return HD_ERR_NOT_OPEN;
	}

	return vendor_ai_net_debug_performance(proc_id, p_model_name, p_proc_time, p_cpu_loading);
}

HD_RESULT _vendor_ai_net_debug_get_memsize(UINT32 proc_id, UINT32* p_model_size, UINT32* p_initbuf_size, UINT32* p_workbuf_size)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
		DBG_ERR("proc_id(%lu) is NOT started yet !!\n", proc_id);
		return HD_ERR_NOT_OPEN;
	}

	return vendor_ai_net_debug_get_memsize(proc_id, p_model_size, p_initbuf_size, p_workbuf_size);
}


static INT _vendor_ai_net_verify_param_CFG_MODEL(VENDOR_AI_NET_CFG_MODEL *p_cfg_model, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_model->pa == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_MODEL: pa(%lu) is 0.", p_cfg_model->pa);
		goto exit;
	}

	if (p_cfg_model->va == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_MODEL: va(%lu) is 0.", p_cfg_model->va);
		goto exit;
	}

	if (p_cfg_model->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_MODEL: size(%lu) is 0.", p_cfg_model->size);
		goto exit;
	}

	return 0;
exit:
	return -1;

}

static INT _vendor_ai_net_verify_param_CFG_SHARE_MODEL(VENDOR_AI_NET_CFG_MODEL *p_cfg_model, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_model->pa == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_SHARE_MODEL: pa(%lu) is 0.", p_cfg_model->pa);
		goto exit;
	}

	if (p_cfg_model->va == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_SHARE_MODEL: va(%lu) is 0.", p_cfg_model->va);
		goto exit;
	}

	if (p_cfg_model->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_SHARE_MODEL: size(%lu) is 0.", p_cfg_model->size);
		goto exit;
	}

	return 0;
exit:
	return -1;

}

static INT _vendor_ai_net_verify_param_CFG_WORKBUF(VENDOR_AI_NET_CFG_WORKBUF *p_cfg_workbuf, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_workbuf->pa == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_WORKBUF: pa(%lu) is 0.", p_cfg_workbuf->pa);
		goto exit;
	}

	if (p_cfg_workbuf->va == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_WORKBUF: va(%lu) is 0.", p_cfg_workbuf->va);
		goto exit;
	}

	if (p_cfg_workbuf->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_WORKBUF: size(%lu) is 0.", p_cfg_workbuf->size);
		goto exit;
	}

	if (p_cfg_workbuf->pa % 32 != 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_WORKBUF: pa(0x%08x) is NOT 32x align !!\n", (UINT)p_cfg_workbuf->pa);
		goto exit;
	}
	return 0;
exit:
	return -1;

}
static INT _vendor_ai_net_verify_param_CFG_RONLYBUF(VENDOR_AI_NET_CFG_RONLYBUF *p_cfg_rbuf, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_rbuf->pa == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_RONLYBUF: pa(%lx) is 0.", (ULONG)p_cfg_rbuf->pa);
		goto exit;
	}

	if (p_cfg_rbuf->va == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_RONLYBUF: va(%lx) is 0.",(ULONG) p_cfg_rbuf->va);
		goto exit;
	}

	if (p_cfg_rbuf->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_RONLYBUF: size(%d) is 0.", (int)p_cfg_rbuf->size);
		goto exit;
	}
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
	if (p_cfg_rbuf->pa % 64 != 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_RONLYBUF: pa(0x%08x) is NOT 64x align !!\n", (UINT)p_cfg_rbuf->pa);
		goto exit;
	}
#else
	if (p_cfg_rbuf->pa % 32 != 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_CFG_RONLYBUF: pa(0x%08x) is NOT 32x align !!\n", (UINT)p_cfg_rbuf->pa);
		goto exit;
	}
#endif
	return 0;
exit:
	return -1;

}

static INT _vendor_ai_net_verify_param_CFG_INTLBUF(VENDOR_AI_NET_CFG_INTLBUF *p_cfg_intlbuf, CHAR *p_ret_string, INT max_str_len)
{
	if (p_cfg_intlbuf->pa == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_CFG_INTLBUF: pa(%lu) is 0.", p_cfg_intlbuf->pa);
		goto exit;
	}

	if (p_cfg_intlbuf->va == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_CFG_INTLBUF: va(%lu) is 0.", p_cfg_intlbuf->va);
		goto exit;
	}

	if (p_cfg_intlbuf->size == 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_CFG_INTLBUF: size(%lu) is 0.", p_cfg_intlbuf->size);
		goto exit;
	}

	if (p_cfg_intlbuf->pa % 32 != 0) {
		snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_CFG_INTLBUF: pa(0x%08x) is NOT 32x align !!\n", (UINT)p_cfg_intlbuf->pa);
		goto exit;
	}

	return 0;
exit:
	return -1;

}

static INT _vendor_ai_net_verify_param_CFG_JOB_OPT(VENDOR_AI_NET_CFG_JOB_OPT *p_cfg_job_opt, CHAR *p_ret_string, INT max_str_len)
{
	switch (p_cfg_job_opt->method) {
		case VENDOR_AI_NET_JOB_OPT_LINEAR:
		case VENDOR_AI_NET_JOB_OPT_LINEAR_O1:
		case VENDOR_AI_NET_JOB_OPT_GRAPH:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O1:
			break;

		case VENDOR_AI_NET_JOB_OPT_GRAPH_O2:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O3:
		default:
			snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_JOB_OPT: method(%lu) is not support.", (UINT32)p_cfg_job_opt->method);
			goto exit;
	}

	if (p_cfg_job_opt->wait_ms < -1 || p_cfg_job_opt->wait_ms > 10000) {
		snprintf(p_ret_string, max_str_len,
			"VENDOR_AI_NET_PARAM_JOB_OPT: wait_ms(%lu) is out-of-range(-1 ~ 10000).", p_cfg_job_opt->wait_ms);
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT _vendor_ai_net_verify_param_CFG_BUF_OPT(VENDOR_AI_NET_CFG_BUF_OPT *p_cfg_buf_opt, CHAR *p_ret_string, INT max_str_len)
{
	switch (p_cfg_buf_opt->method) {
		case VENDOR_AI_NET_BUF_OPT_FULL:
		case VENDOR_AI_NET_BUF_OPT_SHRINK:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O1:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O2:
		case VENDOR_AI_NET_BUF_OPT_NONE:
			break;

		case VENDOR_AI_NET_BUF_OPT_SHRINK_O3:
		default:
			snprintf(p_ret_string, max_str_len,
				"VENDOR_AI_NET_PARAM_BUF_OPT: method(%lu) is not support.", (UINT32)p_cfg_buf_opt->method);
			goto exit;
	}

	return 0;
exit:
	return -1;
}

INT _vendor_ai_net_check_params_range(VENDOR_AI_NET_PARAM_ID param_id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	UINT32 layer_id = VENDOR_AI_GET_LAYER(param_id);
	UINT32 in_id    = VENDOR_AI_GET_IN(param_id);
	UINT32 out_id   = VENDOR_AI_GET_OUT(param_id);
	INT ret = 0;

	switch (param_id) {
	case VENDOR_AI_NET_PARAM_CFG_MODEL:
		ret = _vendor_ai_net_verify_param_CFG_MODEL((VENDOR_AI_NET_CFG_MODEL*) p_param, p_ret_string, max_str_len);
		break;

	case VENDOR_AI_NET_PARAM_CFG_SHAREMODEL:
		ret = _vendor_ai_net_verify_param_CFG_SHARE_MODEL((VENDOR_AI_NET_CFG_MODEL*) p_param, p_ret_string, max_str_len);
		break;

	case VENDOR_AI_NET_PARAM_CFG_JOB_OPT:
		ret = _vendor_ai_net_verify_param_CFG_JOB_OPT((VENDOR_AI_NET_CFG_JOB_OPT*) p_param, p_ret_string, max_str_len);
		break;

	case VENDOR_AI_NET_PARAM_CFG_BUF_OPT:
		ret = _vendor_ai_net_verify_param_CFG_BUF_OPT((VENDOR_AI_NET_CFG_BUF_OPT*) p_param, p_ret_string, max_str_len);
		break;

#if CNN_USER_POSTPROC
	case VENDOR_AI_NET_PARAM_CFG_USER_POSTPROC:
#endif
	case VENDOR_AI_NET_PARAM_RES_ID:
	case VENDOR_AI_NET_PARAM_RES_ID_IMM:
	case VENDOR_AI_NET_PARAM_RES_DIM:
	case VENDOR_AI_NET_PARAM_RES_DIM_IMM:
	case VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO:
	case VENDOR_AI_NET_PARAM_CUSTOM_INFO:
	case VENDOR_AI_NET_PARAM_BATCH_ID:
	case VENDOR_AI_NET_PARAM_BATCH_ID_IMM:
	case VENDOR_AI_NET_PARAM_BATCH_N:
	case VENDOR_AI_NET_PARAM_BATCH_N_IMM:
		break;

	case VENDOR_AI_NET_PARAM_CFG_RONLYBUF:
		ret = _vendor_ai_net_verify_param_CFG_RONLYBUF((VENDOR_AI_NET_CFG_RONLYBUF*) p_param, p_ret_string, max_str_len);
		break;
	case VENDOR_AI_NET_PARAM_CFG_WORKBUF:
		ret = _vendor_ai_net_verify_param_CFG_WORKBUF((VENDOR_AI_NET_CFG_WORKBUF*) p_param, p_ret_string, max_str_len);
		break;

	case VENDOR_AI_NET_PARAM_CFG_INTLBUF:
		ret = _vendor_ai_net_verify_param_CFG_INTLBUF((VENDOR_AI_NET_CFG_INTLBUF*) p_param, p_ret_string, max_str_len);
		break;

	default:
		//--- check if this is PARAM_IN() or PARAM_OUT() ---
		switch(VENDOR_AI_GET_PARAM_TYPE(param_id)) {
			case VENDOR_AI_PARAM_TYPE_LAYER:
				//
			break;

			case VENDOR_AI_PARAM_TYPE_IN:
				if ((layer_id == 0) && (in_id ==0)) {
					// Input param will be checked by _vendor_ai_net_check_input_format().
				}
			break;

			case VENDOR_AI_PARAM_TYPE_OUT:
				if ((layer_id == VENDOR_AI_MAXLAYER) && (out_id ==0)) {
					// ...
				}
			break;

			default:
				snprintf(p_ret_string, max_str_len,"param_id(%u) not support !!\r\n", param_id);
				return HD_ERR_PARAM;
			break;
		}
		break;
	}
	return ret;
}

INT _vendor_ai_net_check_params_range_on_open(UINT32 proc_id, CHAR *p_ret_string, INT max_str_len)
{
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;

	p_cfg = _vendor_ai_cfg(proc_id);

	// job_opt & buf_opt combination
	if (TRUE == _vendor_ai_net_is_graph_job(p_cfg->job_opt.method)) {
		if ((p_cfg->buf_opt.method == VENDOR_AI_NET_BUF_OPT_SHRINK_O1) || (p_cfg->buf_opt.method == VENDOR_AI_NET_BUF_OPT_SHRINK_O2)) {
			#if 0
			// error setting
			snprintf(p_ret_string, max_str_len,"job_opt.method(%lu) with buf_opt.method(%lu) is NOT support !!\r\n", job_opt, buf_opt);
			goto exit;
			#else
			// don't treat as error, but auto modify buf_opt(2/3) to buf_opt(1) instead
			//===> [Update] graph B2/B3 is implemented now, so remove following restrict. JIRA : NA51055-1758, NA51055-1277
			//vendor_ai_net_trace(proc_id, AI_BUF, "job_opt(%lu) with buf_opt(%lu) ... auto modify to buf_opt(1)\r\n", p_cfg->job_opt.method, p_cfg->buf_opt.method);
			//p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_SHRINK;
			#endif
		}
	}
	return 0;
#if 0
exit:
	return -1;
#endif
}

VOID _vendor_ai_net_fill_default_params(VENDOR_AI_NET_PARAM_ID param_id, VOID *p_param)
{
	switch (param_id) {
	case VENDOR_AI_NET_PARAM_CFG_MODEL:
		{
			VENDOR_AI_NET_CFG_MODEL *p_model_cfg = (VENDOR_AI_NET_CFG_MODEL *)p_param;
			p_model_cfg->pa   = 0;
			p_model_cfg->va   = 0;
			p_model_cfg->size = 0;
		}
	break;
	case VENDOR_AI_NET_PARAM_CFG_JOB_OPT:
		{
			VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)p_param;
			p_job_opt->method  = VENDOR_AI_NET_JOB_OPT_LINEAR;
			p_job_opt->wait_ms = -1;
			p_job_opt->schd_parm = 0;
		}
	break;
	case VENDOR_AI_NET_PARAM_CFG_BUF_OPT:
		{
			VENDOR_AI_NET_CFG_BUF_OPT *p_buf_opt = (VENDOR_AI_NET_CFG_BUF_OPT *)p_param;
			p_buf_opt->method = VENDOR_AI_NET_BUF_OPT_SHRINK;
			p_buf_opt->ddr_id = DDR_ID0;
		}
	break;
	default:
	break;
	}
}

VOID _vendor_ai_net_set_mem_ofs(UINT32 proc_id, UINT32 mem_ofs)
{
	g_ai_mem_ofs.net_id = proc_id;
	g_ai_mem_ofs.ofs = mem_ofs;
	//printf("vendor set_mem_ofs: proc_id(%lu) ofs(0x%lx)\r\n", g_ai_mem_ofs.net_id, g_ai_mem_ofs.ofs);
}

HD_RESULT _vendor_ai_net_fill_all_default_params(UINT32 proc_id)
{
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);

	//init values
	_vendor_ai_net_fill_default_params(VENDOR_AI_NET_PARAM_CFG_MODEL,   &p_cfg->cfg_model);
	_vendor_ai_net_fill_default_params(VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &p_cfg->job_opt);
	_vendor_ai_net_fill_default_params(VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &p_cfg->buf_opt);
		
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_INIT;  // [UNINT] -> [INIT]

	return HD_OK;
}

NN_GEN_MODEL_HEAD *_vendor_ai_net_get_head(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT rv;

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		goto err_exit;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		goto err_exit;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		goto err_exit;
	}

	p_head = net_info.p_head;
	if (p_head == NULL) {
		DBG_ERR("p_head is null\r\n");
		goto err_exit;
	}

	return p_head;

err_exit:
	return NULL;
}


NN_GEN_MODE_CTRL *_vendor_ai_net_get_mctrl(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODE_CTRL *p_mctrl;
	HD_RESULT rv;

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		goto err_exit;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		goto err_exit;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		goto err_exit;
	}


	p_mctrl = net_info.p_mctrl;
	if (p_mctrl == NULL) {
		DBG_ERR("p_mctrl is null\r\n");
		goto err_exit;
	}

	return p_mctrl;

err_exit:
	return NULL;
}

HD_RESULT _vendor_ai_net_check_input_start_address(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, VENDOR_AI_BUF* ai_buf, UINT32 mctrl_id, UINT32 port_id, UINT32 buf_count)
{
	ULONG start_address;

	if (p_mctrl[mctrl_id].eng == NN_GEN_ENG_NUE2) {
		if (buf_count == 2) {
			// input image channels are separated. check Y8/UV.
			if (ai_buf[0].fmt == HD_VIDEO_PXLFMT_Y8 && ai_buf[1].fmt == HD_VIDEO_PXLFMT_UV) {
				start_address = ai_buf[1].pa;
				if (start_address % 2) {
					DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, start_address(0x%lx)\r\n", proc_id, mctrl_id, port_id, start_address);
					goto err_exit;
				}
			}
		} else if (buf_count == 1) {
			// input image channels are not separated. check YUV420.
			if (ai_buf[0].fmt == HD_VIDEO_PXLFMT_YUV420) {
				start_address = ai_buf[0].pa + ai_buf[0].line_ofs * ai_buf[0].height;
				if (start_address % 2) {
					DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, start_address(0x%lx)\r\n", proc_id, mctrl_id, port_id, start_address);
					goto err_exit;
				}
			}
		}
	}

	return HD_OK;

err_exit:
	return HD_ERR_PARAM;
}

/* check for gentool 2.0 (model gen by old tool) */
HD_RESULT _vendor_ai_net_check_oldtool_input(UINT32 proc_id, VENDOR_AI_BUF* ai_buf, VENDOR_AI_BUF* in_buf, VENDOR_AI_BUF* out_buf, UINT32 mctrl_id, UINT32 port_id, UINT32 buf_count)
{
	HD_RESULT rv;
	NN_GEN_MODE_CTRL *p_mctrl;
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	VENDOR_AI_NET_CFG_MODEL *p_diff_model = NULL;

	// get mctrl
	p_mctrl = _vendor_ai_net_get_mctrl(proc_id);
	if (p_mctrl == NULL) {
		DBG_ERR("get p_mctrl fail\r\n");
		return HD_ERR_NG;
	}

	if (ai_buf->pa == 0) {
		DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx)\r\n", proc_id, mctrl_id, port_id, ai_buf->pa);
		goto err_exit;
	}

	if (FALSE == _vendor_ai_common_info()->not_check_input_align) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		if (ai_buf->pa % 64 != 0) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx) is NOT 64x align !!\n", proc_id, mctrl_id, port_id, ai_buf->pa);
			goto err_exit;
		}
#else
		if (ai_buf->pa % 32 != 0) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx) is NOT 32x align !!\n", proc_id, mctrl_id, port_id, ai_buf->pa);
			goto err_exit;
		}
#endif
	}

	rv = _vendor_ai_net_check_input_start_address(proc_id, p_mctrl, ai_buf, mctrl_id, port_id, buf_count);
	if (rv != HD_OK) {
		goto err_exit;
	}

	if (p_mctrl[mctrl_id].eng != NN_GEN_ENG_NUE2) { // CNN: feature-in
		// do nothing
	} else { // NUE2
		p_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.diff_model;
		if ((p_diff_model->pa == 0) || (p_diff_model->va == 0) || (p_diff_model->size == 0)) {
			if (ai_buf->width < out_buf->width) {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_w(%lu), out_w(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->width, out_buf->width);
				goto err_exit;
			}
			if (ai_buf->height < out_buf->height) {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_h(%lu), out_h(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->height, out_buf->height);
				goto err_exit;
			}
		}
	}

	return HD_OK;

err_exit:
	return HD_ERR_PARAM;
}

/* check for AI_Tool 3.0 (model gen by new tool) */
HD_RESULT _vendor_ai_net_check_newtool_input(UINT32 proc_id, VENDOR_AI_BUF* ai_buf, VENDOR_AI_BUF* in_buf, VENDOR_AI_BUF* out_buf, UINT32 mctrl_id, UINT32 port_id, UINT32 buf_count)
{
	HD_RESULT rv;
	NN_GEN_MODE_CTRL *p_mctrl;
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	VENDOR_AI_NET_CFG_MODEL *p_diff_model = NULL;

	// get mctrl
	p_mctrl = _vendor_ai_net_get_mctrl(proc_id);
	if (p_mctrl == NULL) {
		DBG_ERR("get p_mctrl fail\r\n");
		return HD_ERR_NG;
	}

	if (ai_buf->pa == 0) {
		DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx)\r\n", proc_id, mctrl_id, port_id, ai_buf->pa);
		goto err_exit;
	}

	if (FALSE == _vendor_ai_common_info()->not_check_input_align) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		if (ai_buf->pa % 64 != 0) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx) is NOT 64x align !!\n", proc_id, mctrl_id, port_id, ai_buf->pa);
			goto err_exit;
		}
#else
		if (ai_buf->pa % 32 != 0) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, pa(0x%lx) is NOT 32x align !!\n", proc_id, mctrl_id, port_id, ai_buf->pa);
			goto err_exit;
		}
#endif
	}

	rv = _vendor_ai_net_check_input_start_address(proc_id, p_mctrl, ai_buf, mctrl_id, port_id, buf_count);
	if (rv != HD_OK) {
		goto err_exit;
	}

	if (ai_buf->size == 0) {
		DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, size(0x%lx)\r\n", proc_id, mctrl_id, port_id, ai_buf->size);
		goto err_exit;
	}

	if (buf_count == 1) {
		if (ai_buf->channel != in_buf->channel) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_ch(%lu) != in_ch(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->channel, in_buf->channel);
			goto err_exit;
		}
	}

	if (buf_count == 1) {
		if (ai_buf->fmt != in_buf->fmt) {
			if ( (( ai_buf->fmt & 0xf0000000) == 0xa0000000) && ((ai_buf->fmt & 0xffff0000) == (in_buf->fmt & 0xffff0000 ))) {
				// do nothing
			} else {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_fmt(%u) != in_fmt(%u)\r\n", proc_id, mctrl_id, port_id, ai_buf->fmt, in_buf->fmt);
				goto err_exit;
			}
		}
	}

	if (p_mctrl[mctrl_id].eng != NN_GEN_ENG_NUE2) { // CNN: feature-in
		if (ai_buf->width != out_buf->width) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_w(%lu) != out_w(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->width, out_buf->width);
			goto err_exit;
		}

		if (ai_buf->height != out_buf->height) {
			DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_h(%lu) != out_h(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->height, out_buf->height);
			goto err_exit;
		}
	} else { // NUE2
		p_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.diff_model;
		if ((p_diff_model->pa == 0) || (p_diff_model->va == 0) || (p_diff_model->size == 0)) {
			if (ai_buf->width < out_buf->width) {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_w(%lu), out_w(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->width, out_buf->width);
				goto err_exit;
			}
			if (ai_buf->height < out_buf->height) {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_h(%lu), out_h(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->height, out_buf->height);
				goto err_exit;
			}
		} else {
            if ((ai_buf->width < p_cfg->diff_resinfo.curr_dim.w && ai_buf->height > p_cfg->diff_resinfo.curr_dim.h) ||
                (ai_buf->width > p_cfg->diff_resinfo.curr_dim.w && ai_buf->height < p_cfg->diff_resinfo.curr_dim.h)) {
				DBG_ERR("proc_id(%lu) layer[%lu] input[%lu] check fail, usr_w(%lu), usr_h(%lu), out_w(%lu), out_h(%lu)\r\n", proc_id, mctrl_id, port_id, ai_buf->width, ai_buf->height, p_cfg->diff_resinfo.curr_dim.w, p_cfg->diff_resinfo.curr_dim.h);
				DBG_ERR("When running multi-scale model, user width and height must be less than or equal to or greater than or equal to model width and height at the same time\r\n");
                goto err_exit;
			}
        }
	}

	return HD_OK;

err_exit:
	return HD_ERR_PARAM;
}

HD_RESULT _vendor_ai_net_check_input_format(UINT32 proc_id, VENDOR_AI_BUF* ai_buf, UINT32 mctrl_id, UINT32 port_id, UINT32 buf_count)
{
	HD_RESULT rv;
	VENDOR_AI_BUF in_buf = {0}, out_buf = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;

	// get layer in_buf / out_buf
	rv = _vendor_ai_net_get_iomem(proc_id, mctrl_id, GET_PORT_TYPE_IN, port_id, &in_buf);
	if (rv != HD_OK) {
		DBG_ERR("get first_layer_in fail(%d)\r\n", rv);
		return HD_ERR_NG;
	}
	rv = _vendor_ai_net_get_iomem(proc_id, mctrl_id, GET_PORT_TYPE_OUT, port_id, &out_buf);
	if (rv != HD_OK) {
		DBG_ERR("get first_layer_out fail(%d)\r\n", rv);
		return HD_ERR_NG;
	}

	// get head
	p_head = _vendor_ai_net_get_head(proc_id);
	if (p_head == NULL) {
		DBG_ERR("get p_head fail\r\n");
		return HD_ERR_NG;
	}

	// get mctrl
	p_mctrl = _vendor_ai_net_get_mctrl(proc_id);
	if (p_mctrl == NULL) {
		DBG_ERR("get p_mctrl fail\r\n");
		return HD_ERR_NG;
	}

#if 0 // debug
	DBG_DUMP("##user input:\n");
	DBG_DUMP("pa :        0x%lx\n", ai_buf->pa);
	DBG_DUMP("size :      0x%lx\n", ai_buf->size);
	DBG_DUMP("channel :   %lu\n", ai_buf->channel);
	DBG_DUMP("batch_num : %lu\n", ai_buf->batch_num);
	DBG_DUMP("fmt :       %u\n", ai_buf->fmt);
	DBG_DUMP("width :     %lu\n", ai_buf->width);
	DBG_DUMP("height :    %lu\n", ai_buf->height);
	DBG_DUMP("\n");
	DBG_DUMP("##[%lu] in_buf:\n");
	DBG_DUMP("pa :        0x%lx\n", in_buf.pa);
	DBG_DUMP("size :      0x%lx\n", in_buf.size);
	DBG_DUMP("channel :   %lu\n", in_buf.channel);
	DBG_DUMP("batch_num : %lu\n", in_buf.batch_num);
	DBG_DUMP("fmt :       %u\n", in_buf.fmt);
	DBG_DUMP("width :     %lu\n", in_buf.width);
	DBG_DUMP("height :    %lu\n", in_buf.height);
	DBG_DUMP("\n");
	DBG_DUMP("##[%lu] out_buf:\n");
	DBG_DUMP("pa :        0x%lx\n", out_buf.pa);
	DBG_DUMP("size :      0x%lx\n", out_buf.size);
	DBG_DUMP("channel :   %lu\n", out_buf.channel);
	DBG_DUMP("batch_num : %lu\n", out_buf.batch_num);
	DBG_DUMP("fmt :       %u\n", out_buf.fmt);
	DBG_DUMP("width :     %lu\n", out_buf.width);
	DBG_DUMP("height :    %lu\n", out_buf.height);
	DBG_DUMP("\n");
#endif

	if (((p_head->chip.gentool_vers & 0xFF000000)>>24) <= 0x15) { // check for gentool 2.0
		rv = _vendor_ai_net_check_oldtool_input(proc_id, ai_buf, &in_buf, &out_buf, mctrl_id, port_id, buf_count);
		if (rv != HD_OK) {
			DBG_ERR("check OLD tool input fail(%d)\r\n", rv);
			goto err_exit;
		}
	} else { // check for AI_Tool 3.0
		rv = _vendor_ai_net_check_newtool_input(proc_id, ai_buf, &in_buf, &out_buf, mctrl_id, port_id, buf_count);
		if (rv != HD_OK) {
			DBG_ERR("check NEW tool input fail(%d)\r\n", rv);
			goto err_exit;
		}
	}

	return HD_OK;

err_exit:
	return HD_ERR_PARAM;
}

HD_RESULT _vendor_ai_net_set_input_img_by_list(UINT32 proc_id, VENDOR_AI_BUF_LIST* ai_buf_list, UINT32 mctrl_id, UINT32 port_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_IMG_PARM_V2 img_parm_v2 = {0};
	//VENDOR_AI_BUF ai_buf[4] = {0};
	UINT32 i = 0;

	// check input validation
	rv = _vendor_ai_net_check_input_format(proc_id, ai_buf_list->buf_list, mctrl_id, port_id, ai_buf_list->buf_count);
	if (rv != HD_OK) {
		DBG_ERR("check_input_format fail(%d)\r\n", rv);
		return rv;
	}

	if (ai_buf_list->buf_count == 2) { // 2 for Y8/UV
		// set input_img settingg in lib
		memcpy(&p_proc->input_img, &ai_buf_list->buf_list[0], sizeof(VENDOR_AI_BUF));
		memcpy(&p_proc->input2_img, &ai_buf_list->buf_list[1], sizeof(VENDOR_AI_BUF));

		for (i = 0; i < ai_buf_list->buf_count; i++) {
			img_parm_v2.img_ch[i].pa          = ai_buf_list->buf_list[i].pa;
			img_parm_v2.img_ch[i].va          = ai_buf_list->buf_list[i].va;
			// combine fmt to ch[0]
			if (i == 0) {
				img_parm_v2.img_ch[i].fmt = HD_VIDEO_PXLFMT_YUV420;
			} else {
				img_parm_v2.img_ch[i].fmt = 0;
			}
			img_parm_v2.img_ch[i].width       = ai_buf_list->buf_list[i].width;
			img_parm_v2.img_ch[i].height      = ai_buf_list->buf_list[i].height;
			img_parm_v2.img_ch[i].channel     = ai_buf_list->buf_list[i].channel;
			img_parm_v2.img_ch[i].batch_num   = ai_buf_list->buf_list[i].batch_num;
			img_parm_v2.img_ch[i].line_ofs    = ai_buf_list->buf_list[i].line_ofs;
			img_parm_v2.img_ch[i].channel_ofs = ai_buf_list->buf_list[i].channel_ofs;
			img_parm_v2.img_ch[i].batch_ofs   = ai_buf_list->buf_list[i].batch_ofs;
#if AI_SUPPORT_MULTI_FMT
			img_parm_v2.img_ch[i].fmt_type    = 0;
#endif
		}

		rv = vendor_ais_net_input_init_v2(&img_parm_v2, proc_id, mctrl_id);
#if CNN_MULTI_INPUT
		if (rv == HD_OK) {
			if (p_proc->priv.src_layer.cnt > 0) {
				p_proc->priv.src_layer.cnt--;
			}
		}
#endif
	} else if (ai_buf_list->buf_count == 3) { // 3 for R8/G8/B8
		// set input_img settingg in lib
		memcpy(&p_proc->input_img, &ai_buf_list->buf_list[0], sizeof(VENDOR_AI_BUF));
		memcpy(&p_proc->input2_img, &ai_buf_list->buf_list[1], sizeof(VENDOR_AI_BUF));
		memcpy(&p_proc->input3_img, &ai_buf_list->buf_list[2], sizeof(VENDOR_AI_BUF));

		for (i = 0; i < ai_buf_list->buf_count; i++) {
			img_parm_v2.img_ch[i].pa          = ai_buf_list->buf_list[i].pa;
			img_parm_v2.img_ch[i].va          = ai_buf_list->buf_list[i].va;
			// combine fmt to ch[0]
			if (i == 0) {
				img_parm_v2.img_ch[i].fmt = (ai_buf_list->buf_list[0].fmt == HD_VIDEO_PXLFMT_R8)? HD_VIDEO_PXLFMT_RGB888_PLANAR : HD_VIDEO_PXLFMT_BGR888_PLANAR;
			} else {
				img_parm_v2.img_ch[i].fmt = 0;
			}
			img_parm_v2.img_ch[i].width       = ai_buf_list->buf_list[i].width;
			img_parm_v2.img_ch[i].height      = ai_buf_list->buf_list[i].height;
			img_parm_v2.img_ch[i].channel     = ai_buf_list->buf_list[i].channel;
			img_parm_v2.img_ch[i].batch_num   = ai_buf_list->buf_list[i].batch_num;
			img_parm_v2.img_ch[i].line_ofs    = ai_buf_list->buf_list[i].line_ofs;
			img_parm_v2.img_ch[i].channel_ofs = ai_buf_list->buf_list[i].channel_ofs;
			img_parm_v2.img_ch[i].batch_ofs   = ai_buf_list->buf_list[i].batch_ofs;
#if AI_SUPPORT_MULTI_FMT
			img_parm_v2.img_ch[i].fmt_type    = 0;
#endif
		}
		rv = vendor_ais_net_input_init_v2(&img_parm_v2, proc_id, mctrl_id);
#if CNN_MULTI_INPUT
		if (rv == HD_OK) {
			if (p_proc->priv.src_layer.cnt > 0) {
				p_proc->priv.src_layer.cnt--;
			}
		}
#endif
	} else {
		DBG_ERR("Not supported buf_count(%lu)\r\n", ai_buf_list->buf_count);
		return rv;
	}

	return rv;
}

HD_RESULT _vendor_ai_net_set_input_img(UINT32 proc_id, VENDOR_AI_BUF* ai_buf, UINT32 mctrl_id, UINT32 port_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AIS_IMG_PARM img_parm = {0};
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	UINT32 buf_count = 1;

	// check input validation
	rv = _vendor_ai_net_check_input_format(proc_id, ai_buf, mctrl_id, port_id, buf_count);
	if (rv != HD_OK) {
		return rv;
	}

	// set input_img settingg in lib
	memcpy(&p_proc->input_img, ai_buf, sizeof(VENDOR_AI_BUF));

	// call vendor_ais_net_input_init()
	img_parm.pa          = ai_buf->pa;
	img_parm.va          = ai_buf->va;
	img_parm.fmt         = ai_buf->fmt;
	img_parm.width       = ai_buf->width;
	img_parm.height      = ai_buf->height;
	img_parm.channel     = ai_buf->channel;
	img_parm.batch_num   = ai_buf->batch_num;
	img_parm.line_ofs    = ai_buf->line_ofs;
	img_parm.channel_ofs = ai_buf->channel_ofs;
	img_parm.batch_ofs   = ai_buf->batch_ofs;
#if AI_SUPPORT_MULTI_FMT
	img_parm.fmt_type    = 0;
#endif

	vendor_ais_lock_net(proc_id);
	rv = vendor_ais_net_input_init(&img_parm, proc_id, mctrl_id);
	vendor_ais_unlock_net(proc_id);

#if CNN_MULTI_INPUT
	if (rv == HD_OK) {
		if (p_proc->priv.src_layer.cnt > 0) {
			p_proc->priv.src_layer.cnt--;
		}
	}
#endif

	return rv;
}

#if !CNN_25_MATLAB
/**
   @input: proc_id / layer_name / output_layer_info / num_output_layer
   @output: out_path
*/
HD_RESULT _vendor_ais_find_layer_info_by_name(UINT32 proc_id, char *layer_name, NN_LAYER_OUTPUT_INFO *output_layer_info, UINT32 *out_path, UINT32 num_output_layer)
{
	UINT32 i = 0, maxlen = 0;
	NN_LAYER_OUTPUT_INFO *p_layer_info;

	if (layer_name == NULL) {
		DBG_ERR("layer_name is null\r\n");
		return HD_ERR_ABORT;
	}
	if (out_path == NULL) {
		DBG_ERR("out_path is null\r\n");
		return HD_ERR_NULL_PTR;
	}

	// check string size
	maxlen = sizeof(output_layer_info[i].layer_name);
	if (strlen(layer_name) > maxlen) {
		DBG_ERR("layer_name size(%u) exceeds limit(%lu) !!\r\n", strlen(layer_name), maxlen);
		return HD_ERR_LIMIT;
	}

	if (output_layer_info) {
		for(i = 0; i < num_output_layer; i++) {
			if(strcmp(layer_name, output_layer_info[i].layer_name) == 0) {
				p_layer_info = &output_layer_info[i];
				*out_path = (UINT32)p_layer_info->fusion_layer_index;
				return HD_OK;
			}
		}
	}

	return HD_OK;
}

HD_RESULT _vendor_ais_get_external_first_layer_info(UINT32 proc_id, NN_IN_OUT_FMT** input_info)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT rv = HD_OK;
	NN_GEN_MODEL_HEAD* p_head = NULL;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 tmp_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('F') | ((UINT32)('F')<<8) | ((UINT32)('I')<<16) | ((UINT32)('O')<<24));

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			*input_info = (NN_IN_OUT_FMT *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			return HD_OK;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}	

	return rv;
}

#if CNN_FMT_V4
HD_RESULT _vendor_ais_config_ext_feature_in(NN_IN_OUT_BUF_INFO* p_ext_info, NN_DATA *p_saio)
{
	HD_RESULT rv = HD_OK;

	if ((p_ext_info->out_sign_bit_num & 0xf) == 2 && // 00001111(binary), lower 4 bits == "2" means special fmt
		(p_ext_info->out_bitdepth & 0xf) == NN_FEATURE) { // 00001111(binary), lower 4 bits for NN_IN_OUT_BUF_FMT
		UINT32 bit_sum = 0;

		// config sign_bit
		p_ext_info->out_sign_bit_num |= (p_saio->fmt.sign_bits << 4);

		// config int_bit_num
		p_ext_info->out_int_bit_num = p_saio->fmt.int_bits;

		// config frac_bit_num
		p_ext_info->out_frac_bit_num = p_saio->fmt.frac_bits;

		// config bitdepth
		bit_sum = p_saio->fmt.frac_bits + p_saio->fmt.int_bits + p_saio->fmt.sign_bits;
		switch (bit_sum) {
		case 8:
			p_ext_info->out_bitdepth |= (NN_BITDEPTH_8 << 4);
			break;
		case 16:
			p_ext_info->out_bitdepth |= (NN_BITDEPTH_16 << 4);
			break;
		case 32:
			p_ext_info->out_bitdepth |= (NN_BITDEPTH_32 << 4);
			break;
		default:
			DBG_ERR("Invalid bit_sum(0x%lu)\r\n", bit_sum);
			rv = HD_ERR_NG;
			goto exit;
			break;
		}

		//printf("config_ext_feature_in: sign_bit(0x%x), out_bitdepth(0x%x)\n", p_ext_info->out_sign_bit_num, p_ext_info->out_bitdepth);
	}
exit:
	return rv;
}
/**

The layout of external info is as below.

ext_head -> ----------------------------------------     ---           -----
            | size | tag |  ext_info  |  ext_info  |      ^              ^
            | ...                                  |   tmp_size          |
            |                                      |      v              |
            |---------------------------------------     ---
            | size2 | tag2 | ...                   |      ^         ext_total_size
            |                                      |   tmp_size2
            |                                      |      v              |
            |--------------------------------------|    ---              |
            | ...                                  |                     |
            |                                      |                     |
            |                                      |                     v
            ----------------------------------------                   -----
*/
HD_RESULT _vendor_ais_config_external_info(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_sai, *p_sao;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 ext_info_num = 1; // need to start from 1
	UINT32 dst_tag = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0;
	UINT32 is_oldtool;

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_mctrl = net_info.p_mctrl;
	p_head = net_info.p_head;

	// get oldtool/newtool version
	if (((p_head->chip.gentool_vers & 0xFF000000)>>24) <= 0x15) { // check for gentool 2.0
		is_oldtool = 1;
	} else {
		is_oldtool = 0;
	}

	// get layer info
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			for(UINT32 i = 0; i < num_output_layer; i++) {
				UINT32 buf_attr, mctrl_id, port_id;
				UINT32 imem_addr, omem_addr;

				// parse ext_id
				buf_attr = (ext_info[i].ext_id & 0xf0000000) >> 28;   // 0x9: input buffer, 0xa: output buffer
				port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;
				mctrl_id = (ext_info[i].ext_id & 0xffff);

				// debug
				/*printf("buf_attr(0x%lx) port_id(0x%lx) mctrl_id(0x%lx) ext_info_num(%lu)\n", buf_attr, port_id, mctrl_id, ext_info_num);
				printf("========= i(%d) ==============\n", (int)i);
				printf("layer_name = %s\n", ext_info[i].layer_name);
				printf("layer_type = %s\n", ext_info[i].layer_type);
				printf("caffe_layer_index = %d\n", ext_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", ext_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", ext_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", ext_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", ext_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", ext_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", ext_info[i].out_scale_ratio);
				printf("width = %d\n", ext_info[i].width);
				printf("height = %d\n", ext_info[i].height);
				printf("channel = %d\n", ext_info[i].channel);
				printf("batch = %d\n", ext_info[i].batch);
				printf("time = %d\n", ext_info[i].time);
				printf("out_lofs = %d\n", ext_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %lu\n", ext_info[i].out_batch_ofs);
				printf("out_time_ofs = %lu\n", ext_info[i].out_time_ofs);
				printf("ext_id(0x%lx)\n", ext_info[i].ext_id);
				printf("dataorder = %s\n", ext_info[i].data_order);
				printf("\n\n");*/

				// check va fixing
				imem_addr = p_mctrl[mctrl_id].iomem.imem_addr;
				if (imem_addr < p_mem->va) imem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it
				omem_addr = p_mctrl[mctrl_id].iomem.omem_addr;
				if (omem_addr < p_mem->va) omem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it

				// config to NN_DATA
				if (buf_attr == 0x9) {
					p_sai = (NN_DATA*)imem_addr;
					p_sai[port_id].fmt.reserved = ext_info_num & 0x0fffffff;

					// config feature_in (only for oldtool)
					if (is_oldtool == 1) {
						rv = _vendor_ais_config_ext_feature_in(&ext_info[i], &p_sai[port_id]);
						if (rv != HD_OK) {
							goto exit;
						}
					}
				} else if (buf_attr == 0xa) {
					p_sao = (NN_DATA*)omem_addr;
					p_sao[port_id].fmt.reserved = ext_info_num & 0x0fffffff;

					// config feature_in (only for oldtool)
					if (is_oldtool == 1) {
						rv = _vendor_ais_config_ext_feature_in(&ext_info[i], &p_sao[port_id]);
						if (rv != HD_OK) {
							goto exit;
						}
					}
				} else {
					DBG_ERR("Invalid buf_attr(0x%lx)\r\n", buf_attr);
					rv = HD_ERR_NG;
					goto exit;
				}

				// update ext_info_num
				ext_info_num++;
				if (ext_info_num > 128) {   // limit by p_sao[port_id].fmt.reserved is INT8
					DBG_ERR("Over limit!, ext_info_num > %u\r\n", 128);
					rv = HD_ERR_LIMIT;
					goto exit;
				}
				p_proc->priv.ext_info_cnt = ext_info_num;
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

exit:
	return rv;
}

HD_RESULT _vendor_ai_net_get_in_path_list(UINT32 proc_id, UINT32 *p_path_list)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 list_cnt = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('I') | ((UINT32)('L')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32 *p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 mctrl_id, port_id;

	if (p_path_list == NULL) {
		DBG_ERR("p_path_list NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			UINT32 *p_input_blob_head = (UINT32 *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			UINT32 input_blob_ofs = 0;
			UINT32 input_blob_num = 0;
			UINT32 input_blob_io_num = 0;
			UINT32 *p_input_blob_info = NULL;
			UINT32 i; // means input_blob_idx
			UINT32 j; // means input_io_idx of each blob

			// get input blob num
			input_blob_num = p_input_blob_head[0];

			for (i = 0; i < input_blob_num; i++) {
				// get input blob info
				input_blob_io_num = p_input_blob_head[2*(i+1) - 1];
				input_blob_ofs = p_input_blob_head[2*(i+1)];

				//DBG_DUMP("i(%lu) input_blob_io_num(0x%lx) input_blob_ofs(0x%lx)\n", i, input_blob_io_num, input_blob_ofs);
				p_input_blob_info = (UINT32 *)(((UINT32)p_input_blob_head) + input_blob_ofs);
				if (p_input_blob_info == NULL) {
					DBG_ERR("p_input_blob_info is NULL\r\n");
					return HD_ERR_NG;
				}

				// config in_path_list
				for (j = 0; j < input_blob_io_num; j++) {
					//DBG_DUMP("i(%lu) j(%lu) list_cnt(%lu)\n", i, j, list_cnt);
					mctrl_id = (p_input_blob_info[j] & 0xFFFFFF00)>>8; // higher 24-bits
					port_id = p_input_blob_info[j] & 0xFF;             // lower 8-bits
					p_path_list[list_cnt] = VENDOR_AI_NET_PARAM_IN(mctrl_id, port_id);
					//DBG_DUMP("p_input_blob_info(0x%lx) => mctrl_id(%lu) port_id(%lu) path_list(0x%lx)\n",
					//		p_input_blob_info[j], mctrl_id, port_id, p_path_list[list_cnt]);
					list_cnt++;
				}
			}

			return HD_OK;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}

	printf("get input blob info fail...\n");

	return HD_ERR_NG;
}

HD_RESULT _vendor_ai_net_get_out_path_list(UINT32 proc_id, UINT32 *p_path_list)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_sao;
	UINT32 proc_layer_num, mctrl_id;
	UINT32 omem_cnt, port_id, ext_info_num;
	UINT32 list_cnt = 0;
	UINT32 fusion_layer_idx_tmp = -1;
#if OUTPATH_TOOL_SORT
	UINT32 ext_id = 0;
#endif

	if (p_path_list == NULL) {
		DBG_ERR("p_path_list NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

#if OUTPATH_TOOL_SORT
	vendor_ai_net_trace(proc_id, AI_FLOW|AI_JOB|AI_BUF, "get_out_path_list() - priv.ext_info_cnt = %u\r\n", p_proc->priv.ext_info_cnt);
	for (ext_id = 1; ext_id <= p_proc->priv.ext_info_cnt; ext_id++) { // ext_id starts from 1
		// init tmp idx each ext_id searching loop
		fusion_layer_idx_tmp = -1;
		for (mctrl_id = 0; mctrl_id < proc_layer_num; mctrl_id++) {
			if ((UINT32)OUT_BUF_ATTR_GET(&p_mctrl[mctrl_id], NN_OUT_BUF_ATTR_PRESERVE)) {
				// checking for one layer has multiple same outbuf (in stripe cases)
				if(fusion_layer_idx_tmp != p_mctrl[mctrl_id].layer_index) {
					fusion_layer_idx_tmp = p_mctrl[mctrl_id].layer_index;
				} else {
					// the same outbuf should be skipped
					continue;
				}

				// if not fix yet(call this funciton before gen_init(), use fixed addr
				p_sao = (NN_DATA*)((p_mctrl[mctrl_id].iomem.omem_addr < p_mem->va)? (p_mctrl[mctrl_id].iomem.omem_addr+p_mem->va): (p_mctrl[mctrl_id].iomem.omem_addr));
				omem_cnt = p_mctrl[mctrl_id].iomem.omem_cnt;
				for (port_id = 0; port_id < omem_cnt; port_id++) {
					// get ext_info_num
					ext_info_num = p_sao[port_id].fmt.reserved & 0x0fffffff;
					if (ext_info_num == ext_id && ext_info_num > 0) {
						p_path_list[list_cnt] = VENDOR_AI_NET_PARAM_OUT(mctrl_id, port_id);
						vendor_ai_net_trace(proc_id, AI_FLOW|AI_JOB|AI_BUF, "get_out_path_list():\r\n");
						vendor_ai_net_trace(proc_id, AI_FLOW|AI_JOB|AI_BUF, "       ext_id = %u, path_list[%u] = 0x%x\r\n", ext_id, list_cnt, p_path_list[list_cnt]);
						list_cnt++;
					}
				}
			}
		}
	}
#else
	/* output path_list sorting by mctrl_id & port_id */

	// outbuf must satisfy 2 conditions below:
	// (1) preserve attribute is set
	// (2) ext_info_num > 0
	for (mctrl_id = 0; mctrl_id < proc_layer_num; mctrl_id++) {
		if ((UINT32)OUT_BUF_ATTR_GET(&p_mctrl[mctrl_id], NN_OUT_BUF_ATTR_PRESERVE)) {
			// checking for one layer has multiple same outbuf (in stripe cases)
			if(fusion_layer_idx_tmp != p_mctrl[mctrl_id].layer_index) {
				fusion_layer_idx_tmp = p_mctrl[mctrl_id].layer_index;
			} else {
				// the same outbuf should be skipped
				continue;
			}

			// if not fix yet(call this funciton before gen_init(), use fixed addr
			p_sao = (NN_DATA*)((p_mctrl[mctrl_id].iomem.omem_addr < p_mem->va)? (p_mctrl[mctrl_id].iomem.omem_addr+p_mem->va): (p_mctrl[mctrl_id].iomem.omem_addr));
			omem_cnt = p_mctrl[mctrl_id].iomem.omem_cnt;
			for (port_id = 0; port_id < omem_cnt; port_id++) {
				// get ext_info_num
				ext_info_num = p_sao[port_id].fmt.reserved & 0x0fffffff;
				if (ext_info_num > 0) {
					p_path_list[list_cnt] = VENDOR_AI_NET_PARAM_OUT(mctrl_id, port_id);
					list_cnt++;
				}
			}
		}
	}
#endif

	return rv;
}

HD_RESULT _vendor_ais_get_path_by_name(UINT32 proc_id, VENDOR_AI_BUF_NAME *p_info)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0;
	UINT32 maxlen = 0;
	UINT32 hit_cnt = 0;

	if (p_info == NULL) {
		DBG_ERR("p_info is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	// get layer info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			// find match name
			for(UINT32 i = 0; i < num_output_layer; i++) {

				// debug
				/*printf("========= i(%d) ==============\n", (int)i);
				printf("layer_name = %s\n", ext_info[i].layer_name);
				printf("layer_type = %s\n", ext_info[i].layer_type);
				printf("caffe_layer_index = %d\n", ext_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", ext_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", ext_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", ext_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", ext_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", ext_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", ext_info[i].out_scale_ratio);
				printf("width = %d\n", ext_info[i].width);
				printf("height = %d\n", ext_info[i].height);
				printf("channel = %d\n", ext_info[i].channel);
				printf("batch = %d\n", ext_info[i].batch);
				printf("time = %d\n", ext_info[i].time);
				printf("out_lofs = %d\n", ext_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %lu\n", ext_info[i].out_batch_ofs);
				printf("out_time_ofs = %lu\n", ext_info[i].out_time_ofs);
				printf("ext_id(0x%lx)\n", ext_info[i].ext_id);
				printf("dataorder = %s\n", ext_info[i].dataorder);
				printf("\n\n");*/

				// check string size
				maxlen = sizeof(ext_info[i].layer_name);
				if (strlen(p_info->name) > maxlen) {
					DBG_ERR("i(%lu), buf_name.name size(%u) exceeds limit(%lu) !!\r\n", i, strlen(p_info->name), maxlen);
					return HD_ERR_LIMIT;
				}

				if(strcmp(p_info->name, ext_info[i].layer_name) == 0) {
					UINT32 port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;
					UINT32 mctrl_id = (ext_info[i].ext_id & 0xffff);
					p_info->path_id = VENDOR_AI_NET_PARAM_OUT(mctrl_id, port_id);
					hit_cnt++;
					#if 0
					// debug
					printf("fount name: %s !!!!\n", p_info->name);
					printf("mctrl_id = 0x%lx, port_id = 0x%lx\n", mctrl_id, port_id);
					printf("path_id = 0x%lx\n", p_info->path_id);
					#endif
				}

			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	if (hit_cnt == 0) {
		// not found any match name
		rv = HD_ERR_NG;
		goto exit;
	}

exit:
	return rv;
}

HD_RESULT _vendor_ais_get_net_input_info_list(UINT32 proc_id, VENDOR_AI_NET_INPUT_INFO *p_info)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	VENDOR_AI_NET_INPUT_INFO *p_info_next;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT rv = HD_OK;
	HD_RESULT hd_ret = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag  = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 dst_tag2 = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('2')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0;
	NN_IN_OUT_BUF_INFO2* ext_info2 = NULL;

	if (p_info == NULL) {
		DBG_ERR("p_info is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	// get layer info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	// try to find dst_tag2 first
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag2) { // bit1 represent tag_type
			ext_info2 = (NN_IN_OUT_BUF_INFO2 *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info2 == NULL) {
				DBG_ERR("ext_info2 pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	// original dst_tag
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			// find match name
			p_info_next = p_info;
			for(UINT32 i = 0; i < num_output_layer; i++) {
				if((ext_info[i].ext_id & 0xf0000000) == 0x90000000) {
					memcpy(p_info_next->name, ext_info[i].layer_name, 192);
					p_info_next->width   = (ext_info2 == NULL)? ext_info[i+1].width  :ext_info2[i+1].width;
					p_info_next->height  = (ext_info2 == NULL)? ext_info[i+1].height :ext_info2[i+1].height;
					p_info_next->channel = (ext_info2 == NULL)? ext_info[i+1].channel:ext_info2[i+1].channel;
					p_info_next->batch   = (ext_info2 == NULL)? ext_info[i+1].batch  :ext_info2[i+1].batch;
					p_info_next++;
				}
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			hd_ret = vendor_ais_set_model_id(p_diff_resinfo);
			if (hd_ret == HD_ERR_ABORT) {
				return HD_ERR_ABORT;
			} else if (hd_ret != HD_OK) {
				DBG_ERR("find model id fail...\r\n");
				return HD_ERR_ABORT;
			}
			p_info->height = p_diff_resinfo->curr_dim.h;  
			p_info->channel = p_diff_resinfo->curr_dim.w; 
		}
	}
exit:
	return rv;
}

HD_RESULT _vendor_ais_get_external_sdk_param(VENDOR_AIS_FLOW_MEM_PARM *p_model, VENDOR_AI_NET_JOB_OPT *job_opt, VENDOR_AI_NET_BUF_OPT *buf_opt)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = p_model;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 dst_tag = (UINT32)((UINT32)('P') | ((UINT32)('D')<<8) | ((UINT32)('E')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32 *p_ex_head = NULL;
	UINT32 ex_total_size = 0;

	if (p_mem == NULL) {
		DBG_ERR("p_mem is NULL ?\r\n");
		return HD_ERR_PARAM;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			*job_opt = *((VENDOR_AI_NET_JOB_OPT*)((UINT32)p_tmp_head + 2*sizeof(UINT32)));
			*buf_opt = *((VENDOR_AI_NET_BUF_OPT*)((UINT32)p_tmp_head + 3*sizeof(UINT32)));
			return HD_OK;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}

	// nvt_model.bin without 'PDEF' extinfo block
	return HD_ERR_NOT_SUPPORT;
}

#if 0
HD_RESULT _vendor_ais_check_batch_num(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 dst_tag = (UINT32)((UINT32)('I') | ((UINT32)('L')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32 *p_ex_head = NULL;
	UINT32 ex_total_size = 0;

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			UINT32 *p_input_blob_head = (UINT32 *)(((uintptr_t)p_tmp_head) + 2*sizeof(UINT32));
			UINT32 input_blob_num = 0;
			UINT32 input_blob_io_num = 0;
			UINT32 i; // means input_blob_idx
			UINT32 first_batch = 0;

			// get input blob num
			input_blob_num = p_input_blob_head[0];
			//DBG_DUMP("check_batch_num: input_blob_num = %lu\r\n",input_blob_num);
			for (i = 0; i < input_blob_num; i++) {
				// get input blob info
				input_blob_io_num = p_input_blob_head[2*(i+1) - 1];
				//DBG_DUMP("blob(%lu): batch_num = %lu\r\n",i,input_blob_io_num);
				if(i == 0) {
					first_batch = input_blob_io_num;
				} else {
					if(input_blob_io_num != first_batch) {
						DBG_ERR("current batch_num(%lu) is not same as first batch num(%lu)\r\n",input_blob_io_num,first_batch);
						return HD_ERR_NOT_SUPPORT;
					}
				}
			}
			return HD_OK;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}

	return HD_ERR_NG;
}
#endif
#endif

#if !CNN_FMT_V4
HD_RESULT _vendor_ais_get_external_output_info(UINT32 proc_id, NN_LAYER_OUTPUT_INFO** output_layer_info, UINT32 *num_output_layer)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 tmp_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	// get layer info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			*output_layer_info = (NN_LAYER_OUTPUT_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (num_output_layer) {
				*num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32))/sizeof(NN_LAYER_OUTPUT_INFO);
			}

			// dump info
			/*for(UINT32 i = 0; i < *num_output_layer; i++){
				NN_LAYER_OUTPUT_INFO* dbg_info = (NN_LAYER_OUTPUT_INFO*)(*output_layer_info);
				printf("========= %d ==============\n", (int)i);
				printf("layer_name = %s\n", dbg_info[i].layer_name);
				printf("caffe_layer_index = %d\n", dbg_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", dbg_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", dbg_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", dbg_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", dbg_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", dbg_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", dbg_info[i].out_scale_ratio);
				printf("out_width = %d\n", dbg_info[i].out_width);
				printf("out_height = %d\n", dbg_info[i].out_height);
				printf("out_channel = %d\n", dbg_info[i].out_channel);
				printf("out_batch = %d\n", dbg_info[i].out_batch);
				printf("out_time = %d\n", dbg_info[i].out_time);
				printf("out_lofs = %d\n", dbg_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", dbg_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %lu\n", dbg_info[i].out_batch_ofs);
				printf("out_time_ofs = %lu\n", dbg_info[i].out_time_ofs);
			}*/
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	return HD_OK;
}
#endif

HD_RESULT _vendor_ai_net_yuvfmt_str2hdal(char *p_str, HD_VIDEO_PXLFMT *p_fmt)
{
	if (p_str == NULL) {
		DBG_ERR("p_str is NULL\n");
		return HD_ERR_NULL_PTR;
	}
	if (p_fmt == NULL) {
		DBG_ERR("p_fmt is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	// convert str to hdal
	if (strcmp(p_str, "YUV420") == 0) {
		*p_fmt = HD_VIDEO_PXLFMT_YUV420;
	} else if (strcmp(p_str, "BGR888") == 0) {
		*p_fmt = HD_VIDEO_PXLFMT_BGR888_PLANAR;
	} else {
		DBG_ERR("Unknown yuv_fmt(%s)\n", p_str);
		return HD_ERR_NG;
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_get_first_layer_input_info(UINT32 proc_id, VENDOR_AI_BUF *ai_buf)
{
	NN_IN_OUT_FMT *input_info = NULL;

	if (ai_buf == NULL) {
		DBG_ERR("ai_buf is NULL !!\n");
		return HD_ERR_NULL_PTR;
	}

	memset(ai_buf, 0, sizeof(VENDOR_AI_BUF));

	_vendor_ais_get_external_first_layer_info(proc_id, &input_info);

	if (input_info == NULL) {
		DBG_ERR("input_info is NULL !!\n");
		return HD_ERR_NULL_PTR;	
	}

	_vendor_ai_net_yuvfmt_str2hdal(input_info->in_fmt, &ai_buf->fmt);
	ai_buf->channel = input_info->in_channel;

	return HD_OK;
}

HD_RESULT _vendor_ai_net_get_first_layer_output_info(UINT32 proc_id, VENDOR_AI_BUF *ai_buf)
{
	NN_IN_OUT_FMT *output_info = NULL;

	if (ai_buf == NULL) {
		DBG_ERR("ai_buf is NULL !!\n");
		return HD_ERR_NULL_PTR;
	}

	memset(ai_buf, 0, sizeof(VENDOR_AI_BUF));

	_vendor_ais_get_external_first_layer_info(proc_id, &output_info);

	if (output_info == NULL) {
		DBG_ERR("input_info is NULL !!\n");
		return HD_ERR_NULL_PTR;	
	}

	_vendor_ai_net_yuvfmt_str2hdal(output_info->model_fmt, &ai_buf->fmt);
	ai_buf->width = output_info->model_width;
	ai_buf->height = output_info->model_height;
	ai_buf->batch_num = output_info->model_batch;
	ai_buf->channel = output_info->model_channel;

	return HD_OK;
}

HD_RESULT _vendor_ai_net_get_last_layer_labelnum(UINT32 proc_id, UINT32 *label_num)
{
	//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	UINT32 model_size = 0;
	HD_RESULT rv;

	if (label_num == NULL) {
		DBG_ERR("label_num is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}
	*label_num = 0;

	// check if MODEL is set by user
	p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
	if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL  for model first !!\r\n", proc_id);
		return HD_ERR_NO_CONFIG;
	}

	// parse memory layout from model bin file => we actually need user_parm & user_model infomation
	p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	model_size = vendor_ais_auto_alloc_mem(p_model_mem, &mem_manager_tmp);  // we only need user_parm + user_model actually
	if (model_size == 0) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) auto alloc mem fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	p_mem = &mem_manager_tmp.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	if (p_mctrl[proc_layer_num - 1].trig_src == NN_GEN_TRIG_COMMON 
		&& p_mctrl[proc_layer_num - 1].mode == NN_POSTPROC) {
		NN_POSTPROC_PARM *p_parm = (NN_POSTPROC_PARM *)(p_mem->va + p_mctrl[proc_layer_num - 1].addr); // user_parm->va + offset
		if (p_parm == NULL) {
			printf("p_parm == NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
#if AI_V4
		*label_num = p_parm->shape.channel;
#else
		*label_num = p_parm->channel;
#endif
	}

	return HD_OK;
}

#if CNN_INNER_POSTPROC

typedef enum {
	FMT_RGB = 0,
	FMT_BGR = 1,
	FMT_YUV420,
	FMT_YUV422,
	FMT_Y_ONLY,
	UV_PACKED,
} FMT;

HD_RESULT _vendor_ai_net_get_src_layer_cnt_fmt(UINT32 proc_id, UINT32* cnt, UINT32* fmt)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	//VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	//VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	//VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	//NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	//UINT32 proc_layer_num;
	//UINT32 model_size = 0;
	HD_RESULT rv;
	UINT32 in_fmt;

	// parse memory layout from model bin file => we actually need user_parm & user_model infomation
	//p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	//p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	//proc_layer_num = p_head->mode_ctrl_num;
	if (cnt) {
#if CNN_MULTI_INPUT 
		cnt[0] = vendor_ais_net_get_input_layer_index(*p_mem);
#else
		cnt[0] = 1;
#endif
	}
	in_fmt = (UINT32)IN_BUF_ATTR_GET(&p_mctrl[0],	NN_IN_BUF_ATTR_PREPROC_IN_FMT);
	//(UINT32)OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PREPROC_OUT_FMT)
	if (fmt) {
		switch (in_fmt) {
		case FMT_RGB:
		case FMT_BGR:
		case FMT_YUV420:
		case FMT_YUV422:
		case FMT_Y_ONLY:
		case UV_PACKED:
			fmt[0] = MAKEFOURCC('A','B','U','F');
			break;
		default:
			fmt[0] = 0;
			break;
		}
	}
	return rv;
}

HD_RESULT _vendor_ai_net_get_dest_layer_cnt_fmt(UINT32 proc_id, UINT32* cnt, UINT32* fmt)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	//VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	//VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	//VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	//UINT32 model_size = 0;
	HD_RESULT rv;
	NN_DATA *p_sao;
	UINT32 fusion_layer_idx_tmp = -1;
	UINT32 omem_cnt, port_id;
	UINT32 ext_info_num;
	UINT32 out_fmt;
	UINT32 out_cnt;
	UINT32 i;
	
	//parse memory layout from model bin file => we actually need user_parm & user_model infomation
	//p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	
	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	out_cnt = 0;
	out_fmt = 0;
#if CNN_USER_POSTPROC
	if (p_cfg->user_postproc.sign != 0) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_cfg->user_postproc); //CPU engine: user postproc
		out_cnt = 1;
		if (cnt)
			cnt[0] = out_cnt;
		if (p_cfg_engine->get_cb != 0) {
			rv = p_cfg_engine->get_cb(proc_id, proc_layer_num, p_cfg->user_postproc.sign, 0, 0, VENDOR_AI_PLUGIN_BUFTYPE, &out_fmt, 0);
			if (rv != HD_OK) {
				out_fmt = MAKEFOURCC('A','B','U','F');
				rv = HD_OK;
			}
		}
		if (fmt)
			fmt[0] = out_fmt;
		
		return rv;
	}
#endif

	// outbuf must satisfy 2 conditions below:
	// (1) preserve attribute is set
	// (2) ext_info_num > 0
	for (i = 0; i < proc_layer_num; i++) {
		if ((UINT32)OUT_BUF_ATTR_GET(&p_mctrl[i], NN_OUT_BUF_ATTR_PRESERVE)) {
			// checking for one layer has multiple same outbuf (in stripe cases)
			if(fusion_layer_idx_tmp != p_mctrl[i].layer_index) {
				fusion_layer_idx_tmp = p_mctrl[i].layer_index;
			} else {
				// the same outbuf should be skipped
				continue;
			}

			// if not fix yet(call this funciton before gen_init(), use fixed addr
			p_sao = (NN_DATA*)((p_mctrl[i].iomem.omem_addr < p_mem->va)? (p_mctrl[i].iomem.omem_addr+p_mem->va): (p_mctrl[i].iomem.omem_addr));
			omem_cnt = p_mctrl[i].iomem.omem_cnt;
			for (port_id = 0; port_id < omem_cnt; port_id++) {
				// get ext_info_num
				ext_info_num = p_sao[port_id].fmt.reserved & 0x0fffffff;
				if (ext_info_num > 0) {
					out_cnt++;
				}
			}
		}
	}
	if (cnt)
		cnt[0] = out_cnt;
	if (out_cnt == 1) {
		if (p_mctrl[proc_layer_num - 1].trig_src == NN_GEN_TRIG_COMMON) {
			NN_GEN_MODE_CTRL *last_layer = &p_mctrl[proc_layer_num - 1];
			UINT32 last_layer_parm = 0;
			//UINT32 buf_addr = 0;
			VENDOR_AI_COMMON_INFO* p_comm = _vendor_ai_common_info();
			VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_comm->engine_plug[0]); //CPU engine
			if (last_layer == NULL) {
				printf("last_layer == NULL\r\n");
				return HD_ERR_NULL_PTR;
			}
			last_layer_parm = ALIGN_CEIL_4(p_mem->va) + last_layer->addr; // user_parm->va + offset
			if (p_cfg_engine->get_cb != 0) {
				rv = p_cfg_engine->get_cb(proc_id, proc_layer_num - 1, last_layer->mode, (UINT32)last_layer, last_layer_parm, VENDOR_AI_PLUGIN_BUFTYPE, &out_fmt, 0);
				if (rv != HD_OK) {
					out_fmt = MAKEFOURCC('A','B','U','F');
					rv = HD_OK;
				}
			}
		} else {
			out_fmt = MAKEFOURCC('A','B','U','F');
		}
	} else {
		out_fmt = MAKEFOURCC('A','B','U','F');
	}
	if (fmt)
		fmt[0] = out_fmt;
	
	return rv;
}

HD_RESULT _vendor_ai_net_get_last_layer_cpu_buffer_size(UINT32 proc_id, UINT32* buf_size)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	//VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	//VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	//VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	//UINT32 model_size = 0;
	HD_RESULT rv;

	// parse memory layout from model bin file => we actually need user_parm & user_model infomation
	//p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

#if CNN_USER_POSTPROC
	if (p_cfg->user_postproc.sign != 0) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_cfg->user_postproc); //CPU engine: user postproc
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->get_cb != 0) {
			rv = p_cfg_engine->get_cb(proc_id, proc_layer_num, p_cfg->user_postproc.sign, 0, 0, VENDOR_AI_PLUGIN_BUFSIZE, 0, buf_size);
			#if CNN_AI_FASTBOOT
			*buf_size += g_fboot_rslt_size;
			#endif
			*buf_size = ALIGN_CEIL_32(*buf_size);
		}
		
		return rv;
	}
#endif

	if (p_mctrl[proc_layer_num - 1].trig_src != NN_GEN_TRIG_COMMON) {
		return HD_ERR_NOT_SUPPORT;
	}

	{
		NN_GEN_MODE_CTRL *last_layer = &p_mctrl[proc_layer_num - 1];
		UINT32 last_layer_parm = 0;
		//UINT32 buf_addr = 0;
		VENDOR_AI_COMMON_INFO* p_comm = _vendor_ai_common_info();
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_comm->engine_plug[0]); //CPU engine
		if (last_layer == NULL) {
			printf("last_layer == NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
		last_layer_parm = ALIGN_CEIL_4(p_mem->va) + last_layer->addr; // user_parm->va + offset
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->get_cb != 0) {
			rv = p_cfg_engine->get_cb(proc_id, proc_layer_num - 1, last_layer->mode, (UINT32)last_layer, last_layer_parm, VENDOR_AI_PLUGIN_BUFSIZE, 0, buf_size);
			*buf_size = ALIGN_CEIL_32(*buf_size);
		}
	}

	return rv;
}

HD_RESULT _vendor_ai_net_set_last_layer_cpu_buffer_addr(UINT32 proc_id, UINT32 buf_addr, UINT32 buf_size)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	//VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	//VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	//VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	//UINT32 model_size = 0;
	HD_RESULT rv;

	// parse memory layout from model bin file => we actually need user_parm & user_model infomation
	//p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

#if CNN_USER_POSTPROC
	if (p_cfg->user_postproc.sign != 0) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_cfg->user_postproc); //CPU engine: user postproc
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->set_cb != 0) {
			rv = p_cfg_engine->set_cb(proc_id, proc_layer_num, p_cfg->user_postproc.sign, 0, 0, VENDOR_AI_PLUGIN_BUFADDR, buf_addr, buf_size);
		}
		
		return rv;
	}
#endif

	if (p_mctrl[proc_layer_num - 1].trig_src != NN_GEN_TRIG_COMMON) {
		return HD_ERR_NOT_SUPPORT;
	}

	{
		NN_GEN_MODE_CTRL *last_layer = &p_mctrl[proc_layer_num - 1];
		UINT32 last_layer_parm = 0;
		//UINT32 buf_addr = 0;
		VENDOR_AI_COMMON_INFO* p_comm = _vendor_ai_common_info();
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_comm->engine_plug[0]); //CPU engine
		if (last_layer == NULL) {
			printf("last_layer == NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
		last_layer_parm = ALIGN_CEIL_4(p_mem->va) + last_layer->addr; // user_parm->va + offset
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->set_cb != 0)
			rv = p_cfg_engine->set_cb(proc_id, proc_layer_num - 1, last_layer->mode, (UINT32)last_layer, last_layer_parm, VENDOR_AI_PLUGIN_BUFADDR, buf_addr, buf_size);
	}

	return rv;
}

HD_RESULT _vendor_ai_net_get_last_layer_cpu_buffer_struct(UINT32 proc_id, UINT32* struct_addr, UINT32* struct_size)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	//VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	//VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	//VENDOR_AIS_FLOW_MEM_PARM *p_model_mem;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	//UINT32 model_size = 0;
	HD_RESULT rv;

	// parse memory layout from model bin file => we actually need user_parm & user_model infomation
	//p_model_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	
	//NET info
	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

#if CNN_USER_POSTPROC
	if (p_cfg->user_postproc.sign != 0) {
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_cfg->user_postproc); //CPU engine: user postproc
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->proc_cb != 0) {
			rv = p_cfg_engine->proc_cb(proc_id, proc_layer_num, p_cfg->user_postproc.sign, 0, 0);
			if (rv != HD_OK) {
				DBG_ERR("proc_cb fail...\r\n");
			}
		}
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->get_cb != 0) {
			rv = p_cfg_engine->get_cb(proc_id, proc_layer_num, p_cfg->user_postproc.sign, 0, 0, VENDOR_AI_PLUGIN_RESULT, struct_addr, struct_size);
			if (rv != HD_OK) {
				DBG_ERR("proc_cb VENDOR_AI_PLUGIN_RESULT fail...\r\n");
			}
		}
		
		return rv;
	}
#endif

	if (p_mctrl[proc_layer_num - 1].trig_src != NN_GEN_TRIG_COMMON) {
		return HD_ERR_NOT_SUPPORT;
	}

	{
		NN_GEN_MODE_CTRL *last_layer = &p_mctrl[proc_layer_num - 1];
		UINT32 last_layer_parm = 0;
		//UINT32 buf_addr = 0;
		VENDOR_AI_COMMON_INFO* p_comm = _vendor_ai_common_info();
		VENDOR_AI_ENGINE_PLUGIN* p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&(p_comm->engine_plug[0]); //CPU engine
		if (last_layer == NULL) {
			printf("last_layer == NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
		last_layer_parm = ALIGN_CEIL_4(p_mem->va) + last_layer->addr; // user_parm->va + offset
		rv = HD_ERR_NOT_SUPPORT;
		if (p_cfg_engine->get_cb != 0)
			rv = p_cfg_engine->get_cb(proc_id, proc_layer_num - 1, last_layer->mode, (UINT32)last_layer, last_layer_parm, VENDOR_AI_PLUGIN_RESULT, struct_addr, struct_size);
	}
	return rv;
}
#endif

#if CNN_FMT_V4
static HD_RESULT _vendor_ai_net_combine_buf_fmt(NN_IN_OUT_BUF_INFO *ext_info, VENDOR_AI_BUF *ai_buf, UINT32 out_sign_bit_num, UINT32 out_bitdepth, BOOL err_flag)
{
	// combine buf fmt
	switch (out_bitdepth) {
	case 8: { // 8-bits
			if (out_sign_bit_num == 1) {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT8;
			} else {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT8;
			}
			ai_buf->fmt |= (ext_info->out_int_bit_num & 0xff) << 8; // uint8 only have int (without frac)
			ai_buf->fmt |= (ext_info->out_frac_bit_num & 0xff);
			break;
		}
	case 16: { // 16-bits
			if (out_sign_bit_num == 1) {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT16;
			} else {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT16;
			}
			ai_buf->fmt |= (ext_info->out_int_bit_num & 0xff) << 8;
			ai_buf->fmt |= (ext_info->out_frac_bit_num & 0xff);
			break;
		}
	case 32: { // 32-bits
			if (out_sign_bit_num == 1) {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT32;
			} else {
				ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT32;
			}
			ai_buf->fmt |= (ext_info->out_int_bit_num & 0xff) << 8;
			ai_buf->fmt |= (ext_info->out_frac_bit_num & 0xff);
			break;
		}
	default:
		if (err_flag == TRUE) {
			printf("invalid out_bitdepth(%lu)!!\n", out_bitdepth);
			return HD_ERR_NOT_SUPPORT;
		} else {
			break;
		}
	}

	return HD_OK;
}

static HD_RESULT _vendor_ai_net_fill_ext_info(NN_IN_OUT_BUF_INFO *ext_info, NN_IN_OUT_BUF_INFO2 *ext_info2, VENDOR_AI_BUF *ai_buf)
{
	HD_RESULT rv;
	UINT32 out_sign_bit_num = 0, out_bitdepth = 0;

	if (!ext_info || !ai_buf) {
		DBG_ERR("Invalid input ptr null!\n");
		return HD_ERR_NULL_PTR;
	}

	// gen pixel format
	if ((ext_info->out_sign_bit_num & 0xf) == 2) { // 00001111(binary), lower 4 bits == "2" means special fmt
		switch (ext_info->out_bitdepth & 0xf) { // 00001111(binary), lower 4 bits for NN_IN_OUT_BUF_FMT
		case NN_RGB888:
			ai_buf->fmt = HD_VIDEO_PXLFMT_RGB888_PLANAR;
			break;
		case NN_BGR888:
			ai_buf->fmt = HD_VIDEO_PXLFMT_BGR888_PLANAR;
			break;
		case NN_YUV420:
			ai_buf->fmt = HD_VIDEO_PXLFMT_YUV420;
			break;
		case NN_Y_ONLY:
			ai_buf->fmt = HD_VIDEO_PXLFMT_Y8;
			break;
		case NN_UV_PAC:
			ai_buf->fmt = HD_VIDEO_PXLFMT_UV;
			break;
		case NN_FEATURE: // NN_FEATURE means io_buf result by the NN engine
		{
			UINT32 bitdepth_fmt = 0;

			// fmt 0 means AI_BUF or FEATURE_IN
			ai_buf->fmt = 0;

			// query real sign_bit and bitdepth_fmt
			out_sign_bit_num = (ext_info->out_sign_bit_num & 0xf0) >> 4; // 11110000(binary), higher 4 bits for value
			bitdepth_fmt = (ext_info->out_bitdepth & 0xf0) >> 4; // 11110000(binary), higher 4 bits for value

			// get bitdepth by fmt
			switch (bitdepth_fmt) {
			case NN_BITDEPTH_8:
				out_bitdepth = 8;
				break;
			case NN_BITDEPTH_16:
				out_bitdepth = 16;
				break;
			case NN_BITDEPTH_32:
				out_bitdepth = 32;
				break;
			default:
				DBG_ERR("invalid bitdepth_fmt(%lu)\n", bitdepth_fmt);
				rv = HD_ERR_NG;
				return rv;
			}

			rv = _vendor_ai_net_combine_buf_fmt(ext_info, ai_buf, out_sign_bit_num, out_bitdepth, 0); // err_flag=0: don't return error for backward compatible
			if (rv != HD_OK) {
				DBG_ERR("combine buf fmt fail(%u), NN_FEATURE\n", rv);
				return rv;
			}
			break;
		}
		default:
			DBG_ERR("unknown NN_IN_OUT_BUF_FMT format(%u)\n", (ext_info->out_bitdepth & 0x7)); // 00000111(binary), lower 3 bits for format
			return HD_ERR_NOT_SUPPORT;
		}
	} else { 
		out_sign_bit_num = ext_info->out_sign_bit_num;
		out_bitdepth = ext_info->out_bitdepth;
		rv = _vendor_ai_net_combine_buf_fmt(ext_info, ai_buf, out_sign_bit_num, out_bitdepth, 1);
		if (rv != HD_OK) {
			DBG_ERR("combine buf fmt fail(%u)\n", rv);
			return rv;
		}
	}

#if 0 // debug
	UINT32 buf_attr, mctrl_id, port_id;
	buf_attr = (ext_info->ext_id & 0xf0000000) >> 28;   // 0x9: input buffer, 0xa: output buffer
	port_id = (ext_info->ext_id & 0xfff0000) >> 16;
	mctrl_id = (ext_info->ext_id & 0xffff);
	
	printf("buf_attr(0x%lx) port_id(0x%lx) mctrl_id(0x%lx)\n", buf_attr, port_id, mctrl_id);
	printf("========= () ==============\n");
	printf("layer_name = %s\n", ext_info->layer_name);
	printf("layer_type = %s\n", ext_info->layer_type);
	printf("caffe_layer_index = %d\n", ext_info->caffe_layer_index);
	printf("fusion_layer_index = %d\n", ext_info->fusion_layer_index);
	printf("out_bitdepth = %d\n", ext_info->out_bitdepth);
	printf("out_sign_bit_num = %d\n", ext_info->out_sign_bit_num);
	printf("out_int_bit_num = %d\n", ext_info->out_int_bit_num);
	printf("out_frac_bit_num = %d\n", ext_info->out_frac_bit_num);
	printf("out_scale_ratio = %f\n", ext_info->out_scale_ratio);
	printf("width = %d\n", ext_info->width);
	printf("height = %d\n", ext_info->height);
	printf("channel = %d\n", ext_info->channel);
	printf("batch = %d\n", ext_info->batch);
	printf("time = %d\n", ext_info->time);
	printf("out_lofs = %d\n", ext_info->out_lofs);
	printf("out_ch_ofs = %d\n", ext_info->out_ch_ofs);
	printf("out_batch_ofs  = %lu\n", ext_info->out_batch_ofs);
	printf("out_time_ofs = %lu\n", ext_info->out_time_ofs);
	printf("ext_id(0x%lx)\n", ext_info->ext_id);
	printf("dataorder = %s\n", ext_info->data_order);
	printf("\n\n");
#endif

	// other info
	ai_buf->ddr_id = 0; // [TBD]
	ai_buf->width = (ext_info2 == NULL)? ext_info->width : ext_info2->width;
	ai_buf->height = (ext_info2 == NULL)? ext_info->height : ext_info2->height;
	ai_buf->channel = (ext_info2 == NULL)? ext_info->channel : ext_info2->channel;
	ai_buf->batch_num = (ext_info2 == NULL)? ext_info->batch : ext_info2->batch;
	ai_buf->line_ofs = (ext_info2 == NULL)? ext_info->out_lofs : ext_info2->out_lofs;
	ai_buf->channel_ofs = (ext_info2 == NULL)? ext_info->out_ch_ofs : ext_info2->out_ch_ofs;
	ai_buf->batch_ofs = ext_info->out_batch_ofs;
	ai_buf->time_ofs = ext_info->out_time_ofs;
	strcpy(ai_buf->layout, ext_info->data_order);
	ai_buf->name = ext_info->layer_name;
	ai_buf->op_name = ext_info->layer_type;
	ai_buf->scale_ratio = ext_info->out_scale_ratio;

	return HD_OK;
}
static HD_RESULT _vendor_ai_net_get_ext_info(UINT32 proc_id, UINT32 ext_info_num, VENDOR_AI_BUF *ai_buf)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 tmp_size = 0;
	UINT32 dst_tag  = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 dst_tag2 = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('2')<<24));
	HD_RESULT rv = HD_OK;
	NN_IN_OUT_BUF_INFO2* this_ext_info2 = NULL;

	if (ai_buf == NULL) {
		DBG_ERR("ai_buf is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	if (ext_info_num == 0) { // ext_info_num will be started from 1
		DBG_ERR("Invalid input ext_info_num(%lu)\n", ext_info_num);
		return HD_ERR_INV;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	
	// use ext_info_num to find external info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	// try to find dst_tag2 first
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag2) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO2* first_ext_info = (NN_IN_OUT_BUF_INFO2 *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			this_ext_info2 = (NN_IN_OUT_BUF_INFO2 *)(((UINT32)first_ext_info) + (ext_info_num-1) * sizeof(NN_IN_OUT_BUF_INFO2));

			if (this_ext_info2 == NULL) {
				DBG_ERR("this_ext_info is NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	// original dst_tag
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* first_ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
#if 1
			NN_IN_OUT_BUF_INFO* this_ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)first_ext_info) + (ext_info_num-1) * sizeof(NN_IN_OUT_BUF_INFO));
#else
			NN_IN_OUT_BUF_INFO* this_ext_info = &first_ext_info[ext_info_num];
#endif
			if (this_ext_info == NULL) {
				DBG_ERR("this_ext_info is NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}

			// fill AI_BUF info
			rv = _vendor_ai_net_fill_ext_info(this_ext_info, this_ext_info2, ai_buf);
			if (rv != HD_OK) {
				DBG_ERR("fill_ext_info fail, proc_id(%lu) ext_info_num(%lu)\r\n", proc_id, ext_info_num);
				goto exit;
			} else {
				goto exit;
			}
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

exit:
	return rv;
}
#else
static HD_RESULT _vendor_ai_net_get_nn_output_info(UINT32 mctrl_id, UINT32 port_id, VENDOR_AI_BUF *ai_buf)
{
	NN_LAYER_OUTPUT_INFO *output_layer_info = NULL;
	UINT32 num_output_layer = 0;

	if (ai_buf == NULL) {
		DBG_ERR("ai_buf is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	_vendor_ais_get_external_output_info(mctrl_id, &output_layer_info, &num_output_layer);

	// dump info
	/*printf("%s:%d dump info:\n", __func__, __LINE__);
	for(int i = 0; i < num_output_layer; i++){
		NN_LAYER_OUTPUT_INFO* dbg_info = output_layer_info;
		printf("========= %d ==============\n", (int)i);
		printf("layer_name = %s\n", dbg_info[i].layer_name);
		printf("caffe_layer_index = %d\n", dbg_info[i].caffe_layer_index);
		printf("fusion_layer_index = %d\n", dbg_info[i].fusion_layer_index);
		printf("reserve = %d\n", dbg_info[i].reserve);
		printf("out_bitdepth = %d\n", dbg_info[i].out_bitdepth);
		printf("out_sign_bit_num = %d\n", dbg_info[i].out_sign_bit_num);
		printf("out_int_bit_num = %d\n", dbg_info[i].out_int_bit_num);
		printf("out_frac_bit_num = %d\n", dbg_info[i].out_frac_bit_num);
		printf("out_scale_ratio = %f\n", dbg_info[i].out_scale_ratio);
		printf("out_width = %d\n", dbg_info[i].out_width);
		printf("out_height = %d\n", dbg_info[i].out_height);
		printf("out_channel = %d\n", dbg_info[i].out_channel);
		printf("out_batch = %d\n", dbg_info[i].out_batch);
		printf("out_time = %d\n", dbg_info[i].out_time);
		printf("out_lofs = %d\n", dbg_info[i].out_lofs);
		printf("out_ch_ofs = %d\n", dbg_info[i].out_ch_ofs);
		printf("out_batch_ofs  = %lu\n", dbg_info[i].out_batch_ofs);
		printf("out_time_ofs = %lu\n", dbg_info[i].out_time_ofs);
	}
	printf("\n\n");*/

	// gen pixel format
	switch (output_layer_info[port_id].out_bitdepth) {
		case 8: { // 8-bits
				if (output_layer_info[port_id].out_sign_bit_num == 1) {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT8;
				} else {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT8;
				}
				ai_buf->fmt |= output_layer_info[port_id].out_int_bit_num << 8; // uint8 only have int (without frac)
			}
			break;

		case 16: { // 16-bits
				if (output_layer_info[port_id].out_sign_bit_num == 1) {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT16;
				} else {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT16;
				}
				ai_buf->fmt |= output_layer_info[port_id].out_int_bit_num << 8;
				ai_buf->fmt |= output_layer_info[port_id].out_frac_bit_num;
			}
			break;

		case 32: { // 32-bits
				if (output_layer_info[port_id].out_sign_bit_num == 1) {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_SINT32;
				} else {
					ai_buf->fmt = HD_VIDEO_PXLFMT_AI_UINT32;
				}
				ai_buf->fmt |= output_layer_info[port_id].out_int_bit_num << 8;
				ai_buf->fmt |= output_layer_info[port_id].out_frac_bit_num;
			}
			break;

		default:
			printf("invalid out_bitdepth(%d)!!, port_id(%lu)\n", output_layer_info[port_id].out_bitdepth, port_id);
			break;
	}

	// other info
	ai_buf->ddr_id = 0; // [TBD]
	ai_buf->width = output_layer_info[port_id].out_width;
	ai_buf->height = output_layer_info[port_id].out_height;
	ai_buf->channel = output_layer_info[port_id].out_channel;
	ai_buf->batch_num = output_layer_info[port_id].out_batch;
	ai_buf->line_ofs = output_layer_info[port_id].out_lofs;
	ai_buf->channel_ofs = output_layer_info[port_id].out_ch_ofs;
	ai_buf->batch_ofs = output_layer_info[port_id].out_batch_ofs;
	ai_buf->time_ofs = output_layer_info[port_id].out_time_ofs;

	return HD_OK;
}
#endif // end of CNN_FMT_V4

#endif // end of !CNN_25_MATLAB

HD_RESULT _vendor_ai_net_get_iomem(UINT32 proc_id, UINT32 mctrl_id, GET_PORT_TYPE type, UINT32 port_id, VENDOR_AI_BUF *ai_buf)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	NN_GEN_MODEL_HEAD *p_head;
#if !CNN_25_MATLAB
	NN_DATA *p_sai, *p_sao;
#else
	NN_IOMEM *p_io_mem;
#endif
	NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	UINT32 layer_index = 0;
#endif
	NN_GEN_NET_INFO net_info = {0};
	UINT32 SAI_num = 0, SAO_num = 0;
	HD_RESULT rv = HD_OK;

	if (p_proc == NULL) {
		DBG_ERR("p_proc is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
#if CNN_25_MATLAB
	p_io_mem = net_info.p_io_mem;
#endif
	p_mctrl = net_info.p_mctrl;

	if (mctrl_id >= p_head->mode_ctrl_num) {
		DBG_ERR("given layer_id(%lu) exceed max_layer_num(%lu)\r\n", mctrl_id, p_head->mode_ctrl_num);
		return HD_ERR_USER;
	}

#if CNN_25_MATLAB
	layer_index = p_mctrl[mctrl_id].layer_index;
	SAI_num	= sizeof(p_io_mem[layer_index].SAI)  / sizeof(p_io_mem[layer_index].SAI[0]);
	SAO_num	= sizeof(p_io_mem[layer_index].SAO)  / sizeof(p_io_mem[layer_index].SAO[0]);
#else
	SAI_num = p_mctrl[mctrl_id].iomem.imem_cnt;
	SAO_num = p_mctrl[mctrl_id].iomem.omem_cnt;
#endif

	if (type == GET_PORT_TYPE_IN && port_id >= SAI_num) {
		DBG_ERR("given in_id(%lu) exceed SAI_num(%lu)\r\n", port_id, SAI_num);
		return HD_ERR_USER;
	}

	if (type == GET_PORT_TYPE_OUT && port_id >= SAO_num) {
		DBG_ERR("given out_id(%lu) exceed SAO_num(%lu)\r\n", port_id, SAO_num);
		return HD_ERR_USER;
	}

	{
		ai_buf->sign = MAKEFOURCC('A','B','U','F');
		if (type == GET_PORT_TYPE_IN) {
#if CNN_25_MATLAB
			ai_buf->pa   = p_io_mem[layer_index].SAI[port_id].pa;
			ai_buf->va   = p_io_mem[layer_index].SAI[port_id].va;
			ai_buf->size = p_io_mem[layer_index].SAI[port_id].size;
#else
			p_sai = (NN_DATA*)p_mctrl[mctrl_id].iomem.imem_addr;

			ai_buf->pa   = p_sai[port_id].pa;
			ai_buf->va   = p_sai[port_id].va;
			ai_buf->size = p_sai[port_id].size;

#if CNN_FMT_V4
			{
				// use mctrl_id and port_id to get ext_info_num
				UINT32 ext_info_num = p_sai[port_id].fmt.reserved & 0x0fffffff;
				if (ext_info_num > 0) {
					_vendor_ai_net_get_ext_info(proc_id, ext_info_num, ai_buf);
				}
			}
			//_vendor_ai_net_get_nn_input_info(proc_id, port_id, ai_buf); // [TODO] should change method: use (layer_id + buf_id) in NN DATA to link external zone
#endif
#endif
		}else if (type == GET_PORT_TYPE_OUT) {
#if CNN_25_MATLAB
			ai_buf->pa   = p_io_mem[layer_index].SAO[port_id].pa;
			ai_buf->va   = p_io_mem[layer_index].SAO[port_id].va;
			ai_buf->size = p_io_mem[layer_index].SAO[port_id].size;
#else
			// if not fix yet(call this funciton before gen_init(), use fixed addr
			p_sao = (NN_DATA*)((p_mctrl[mctrl_id].iomem.omem_addr < p_mem->va)? (p_mctrl[mctrl_id].iomem.omem_addr+p_mem->va): (p_mctrl[mctrl_id].iomem.omem_addr));

			ai_buf->pa   = p_sao[port_id].pa;
			ai_buf->va   = p_sao[port_id].va;
			ai_buf->size = p_sao[port_id].size;
#if CNN_FMT_V4
			{
				// use mctrl_id and port_id to get ext_info_num
				UINT32 ext_info_num = p_sao[port_id].fmt.reserved & 0x0fffffff;
				if (ext_info_num > 0) {
					_vendor_ai_net_get_ext_info(proc_id, ext_info_num, ai_buf);
				}
			}
#else
			_vendor_ai_net_get_nn_output_info(mctrl_id, port_id, ai_buf);
#endif

#endif
		} else {
			//...
		}
	}

	return HD_OK;
}

#if defined(_BSP_NA51068_)
HD_RESULT _vendor_ai_net_mempool_init(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_POOL_INFO mem_info = {0};
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);

	if (g_mem_info.init_done) {
		goto exit;
	} else {
		g_mem_info.init_done = 1;
	}
	mem_info.type = HD_COMMON_MEM_CNN_POOL;
	mem_info.ddr_id = p_cfg->buf_opt.ddr_id;
	ret = hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&mem_info);
	if (ret != HD_OK) {
		printf("hd_common_mem_get error\n");
		goto exit;
	}
	if (mem_info.start_addr == 0) {
		DBG_ERR("HD_COMMON_MEM_CNN_POOL start_addr is NULL\n");
		ret = HD_ERR_NG;
		goto exit;
	}
	g_mem_info.start_addr = mem_info.start_addr;
	g_mem_info.size = mem_info.blk_size * mem_info.blk_cnt;
	g_mem_info.offset = 0;

exit:
	return ret;
}

#if USE_GET_BLK
HD_RESULT _vendor_ai_net_mem_alloc(UINT32 *pa, void **va, UINT32 size, HD_COMMON_MEM_VB_BLK *blk)
{
	HD_RESULT ret = HD_OK;

	if (pa == NULL || va == NULL || blk == NULL || size == 0) {
		printf("%s: invalid param!\n", __func__);
		ret = HD_ERR_NULL_PTR;
		goto exit;
	}

	// get blk
	*blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == *blk) {
		printf("%s: get block fail\r\n", __func__);
		ret = HD_ERR_NG;
		goto exit;
	}

	// get pa
	*pa = hd_common_mem_blk2pa(*blk);
	if (*pa == 0) {
		printf("%s: blk2pa fail, blk = %#lx\r\n", __func__, *blk);
		hd_common_mem_release_block(*blk);
		ret = HD_ERR_NG;
		goto exit;
	}

	// get va
	/* Must use "HD_COMMON_MEM_MEM_TYPE_CACHE", or it will cause the cpu layer to perform inefficiently */
	*va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pa, size);
	if (*va == 0) {
		printf("%s: va = 0, mem_mmap fail\r\n", __func__);
		ret = hd_common_mem_munmap((void *)*va, size);
		if (ret != HD_OK) {
			printf("%s: mem_munmap fail\r\n", __func__);
			goto exit;
		}
		ret = HD_ERR_NG;
		goto exit;
	}

	//printf("%s:%d va(0x%lx), blk(0x%lx), size(0x%lx)\r\n", __func__, __LINE__, (UINT32)*va, *blk, size);

exit:
	return ret;
}

HD_RESULT _vendor_ai_net_mem_free(UINT32 phy_addr, void *virt_addr, UINT32 size, HD_COMMON_MEM_VB_BLK blk)
{
	HD_RESULT ret = HD_OK;

	//printf("%s:%d va(0x%lx), blk(0x%lx), size(0x%lx)\r\n", __func__, __LINE__, (UINT32)virt_addr, blk, size);

	// unmap va
	if (virt_addr) {
		ret = hd_common_mem_munmap(virt_addr, size);
		if (ret != HD_OK) {
			printf("%s: mem_munmap fail\n", __func__);
		}
	}

	// release blk
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		printf("%s: release_block fail, blk = 0x%lx\n", __func__, blk);
	}

	return ret;
}
#else
HD_RESULT _vendor_ai_net_mem_alloc(UINT32 *pa, void **va, UINT32 size)
{
	HD_RESULT ret = HD_OK;

	if (ALIGN_CEIL_16(g_mem_info.offset + size) > g_mem_info.size) {
		DBG_ERR("alloc buffer exceed max size(%lu), need(%lu)\n", g_mem_info.size, size);
		ret = HD_ERR_NG;
		goto exit;
	}

	*pa = g_mem_info.start_addr + g_mem_info.offset;
	*va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pa, size);
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)*va, size);
		if (ret != HD_OK) {
			printf("hd_common_mem_munmap fail\r\n");
			goto exit;
		}
		ret = HD_ERR_NG;
		goto exit;
	}
	g_mem_info.offset += ALIGN_CEIL_16(size);

exit:
	return ret;
}
#endif
#endif

VOID* _vendor_ai_net_get_usr_info(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	p_proc = _vendor_ai_info(proc_id);
	return p_proc->priv.usr_info;
}

UINT32 _vendor_ai_net_init_buf_calc_size(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager_tmp)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	UINT32 share_model_mode = _vendor_ai_common_info()->share_model_mode;
	UINT32 user_parm_size = p_mem_manager_tmp->user_parm.size;
	UINT32 group_buf_size = 0;
	if (p_proc->priv.b_simplify_bin == TRUE) {
		group_buf_size = 0;  // simplify BIN could only run on Job=1 (linear optimize), we DO NOT need group memory
	} else {
		group_buf_size = vendor_ai_net_group_calcbuffersize(proc_id, p_mem_manager_tmp->user_parm.va);
	}
	p_proc->priv.group_buf.size = group_buf_size;

	if (share_model_mode == 1) {
		return ALIGN_CEIL_16(user_parm_size) * 2 + ALIGN_CEIL_16(group_buf_size); // init_buf = user_parm + group_buf + kerl_parm
	} else {
		return ALIGN_CEIL_16(user_parm_size) + ALIGN_CEIL_16(group_buf_size);     // init_buf = user_parm + group_buf
	}
}

HD_RESULT _vendor_ai_net_init_buf_alloc(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager_tmp, UINT32 req_size)
{
	UINT32 pa = 0;
	void  *va = NULL;
	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
	HD_COMMON_MEM_VB_BLK blk = 0;
#endif

	if (req_size == 0) {
		DBG_ERR("proc_id(%lu) req_size = 0?\n", proc_id);
		return HD_ERR_FAIL;
	}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	CHAR   pool_name[24] = {0};
	snprintf(pool_name, 24, "ainet.proc[%lu].init", proc_id);
	ret = hd_common_mem_alloc((CHAR *)pool_name, &pa, &va, req_size, p_cfg->buf_opt.ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, req_size);
		return HD_ERR_NOBUF;
	}
#else
	#if USE_GET_BLK
	ret = _vendor_ai_net_mem_alloc(&pa, &va, req_size, &blk);
	#else
	_vendor_ai_net_mempool_init(proc_id);
	ret = _vendor_ai_net_mem_alloc(&pa, &va, req_size);
	#endif
	if (ret != HD_OK) {
		DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, req_size);
		return HD_ERR_NOBUF;
	}
#endif
	// backup alloc memory info ,so we can do free at close()
	p_proc->priv.init_buf.pa   = (UINT32)pa;
	p_proc->priv.init_buf.va   = (UINT32)va;
	p_proc->priv.init_buf.size = req_size;
#if defined(_BSP_NA51068_)
	p_proc->priv.init_buf.blk  = (INT32)blk;
#endif

	return HD_OK;
}

HD_RESULT _vendor_ai_net_init_buf_assign_range(UINT32 proc_id, VENDOR_AIS_FLOW_MEM_PARM *init_buf, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager_tmp, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_out)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	UINT32 user_parm_size = p_mem_manager_tmp->user_parm.size;
	UINT32 share_model_mode = _vendor_ai_common_info()->share_model_mode;
    VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
    VENDOR_AI_NET_CFG_MODEL *p_share_diff_model = NULL;

	// user_parm
	p_mem_out->user_parm.pa     = init_buf->pa;
	p_mem_out->user_parm.va     = init_buf->va;
	p_mem_out->user_parm.size   = user_parm_size;

	// group
	p_proc->priv.group_buf.pa   = ALIGN_CEIL_16(p_mem_out->user_parm.pa + user_parm_size);
	p_proc->priv.group_buf.va   = ALIGN_CEIL_16(p_mem_out->user_parm.va + user_parm_size);

	// copy content in original user_parm to newly alloc user_parm
	memcpy((VOID *)p_mem_out->user_parm.va, (VOID *)p_mem_manager_tmp->user_parm.va, user_parm_size);
	//hd_common_mem_flush_cache((VOID *)p_mem_out->user_parm.va, user_parm_size);
	vendor_common_mem_cache_sync((VOID *)p_mem_out->user_parm.va, user_parm_size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input

	if (share_model_mode == 0) {
		// use original user_parm (bin file) as currently kerl_parm
		p_mem_out->kerl_parm.pa     = p_mem_manager_tmp->user_parm.pa;
		p_mem_out->kerl_parm.va     = p_mem_manager_tmp->user_parm.va;
		p_mem_out->kerl_parm.size   = p_mem_manager_tmp->user_parm.size;
	} else if (share_model_mode == 1) {
		// kerl_parm is set after group 
		p_mem_out->kerl_parm.pa     = ALIGN_CEIL_16(p_proc->priv.group_buf.pa + p_proc->priv.group_buf.size);
		p_mem_out->kerl_parm.va     = ALIGN_CEIL_16(p_proc->priv.group_buf.va + p_proc->priv.group_buf.size);
		p_mem_out->kerl_parm.size   = p_mem_manager_tmp->kerl_parm.size;	
	} else if (share_model_mode == 2) {
        p_share_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.share_diff_model;
		// Set read only buf size
		p_proc->rbuf.size =  p_mem_manager_tmp->kerl_parm.size;	
		p_mem_out->kerl_parm.size   = p_mem_manager_tmp->kerl_parm.size;
        if ((p_share_diff_model->pa != 0) && (p_share_diff_model->va != 0) && (p_share_diff_model->size != 0)) {
            p_proc->rbuf.size += p_share_diff_model->size;
        }
	} else {
		DBG_ERR("unsupported share_model_mode(%d)...\r\n", (int)share_model_mode);
		return HD_ERR_PARAM;
	}

	return HD_OK;
}

UINT32 _vendor_ai_net_io_buf_calc_size(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	UINT32 user_buff_size = p_mem_manager->user_buff.size;

		return ALIGN_CEIL_32(user_buff_size);
}

HD_RESULT _vendor_ai_net_work_buf_alloc(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 result_size)
{
	UINT32 pa = 0;
	void  *va = NULL;
	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
#if defined(_BSP_NA51068_)
	HD_COMMON_MEM_VB_BLK blk = 0;
#endif

	UINT32 req_size = _vendor_ai_net_io_buf_calc_size(p_mem_manager);
	if (req_size == 0) {
		DBG_ERR("proc_id(%lu) req_size = 0?\n", proc_id);
		return HD_ERR_FAIL;
	}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	CHAR   pool_name[24] = {0};
	snprintf(pool_name, 24, "ainet.proc[%lu].work", proc_id);
#if CNN_INNER_POSTPROC
	ret = hd_common_mem_alloc((CHAR *)pool_name, &pa, &va, req_size + result_size, p_cfg->buf_opt.ddr_id);
#else
	ret = hd_common_mem_alloc((CHAR *)pool_name, &pa, &va, req_size, p_cfg->buf_opt.ddr_id);
#endif
	if (ret != HD_OK) {
		DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, req_size);
		return HD_ERR_NOBUF;
	}
#else
	#if USE_GET_BLK
	ret = _vendor_ai_net_mem_alloc(&pa, &va, req_size, &blk);
	#else
	ret = _vendor_ai_net_mem_alloc(&pa, &va, req_size);
	#endif
	if (ret != HD_OK) {
		DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, req_size);
		return HD_ERR_NOBUF;
	}
#endif
	// backup whole work memory info ,so we can do free at close()
	p_proc->priv.work_buf.pa   = (UINT32)pa;
	p_proc->priv.work_buf.va   = (UINT32)va;
	p_proc->priv.work_buf.size = req_size + result_size;
#if defined(_BSP_NA51068_)
	p_proc->priv.work_buf.blk  = (INT32)blk;
#endif

	// alloc all priv memory layout
	p_proc->priv.rslt_buf.pa = p_proc->priv.work_buf.pa;
	p_proc->priv.rslt_buf.va = p_proc->priv.work_buf.va;
	p_proc->priv.rslt_buf.size = result_size;
	p_proc->priv.io_buf.pa = p_proc->priv.rslt_buf.pa + p_proc->priv.rslt_buf.size;
	p_proc->priv.io_buf.va = p_proc->priv.rslt_buf.va + p_proc->priv.rslt_buf.size;
	p_proc->priv.io_buf.size = req_size;

	return HD_OK;
}

HD_RESULT _vendor_ai_net_io_buf_assign_range(VENDOR_AIS_FLOW_MEM_PARM *io_buf, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_out)
{

	if (io_buf->pa % 32 != 0) {
		DBG_WRN("io_buf start address pa(0x%08x) is NOT 32x align !!\n", (UINT)io_buf->pa);
	}

	p_mem_out->user_buff.pa     = io_buf->pa;
	p_mem_out->user_buff.va     = io_buf->va;
	
	return HD_OK;
}

HD_RESULT _vendor_ai_net_rbuf_assign_range(VENDOR_AI_NET_INFO_PROC *p_proc, UINT32 proc_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_out = &p_proc->mem_manager ;
	UINT32 share_model_mode = _vendor_ai_common_info()->share_model_mode;
	UINT32 kerl_parm_size   = p_mem_out->kerl_parm.size;
    VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
    VENDOR_AI_NET_CFG_MODEL *p_share_diff_model = NULL;
    VENDOR_AI_NET_CFG_MODEL *p_diff_model = NULL;

    p_share_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.share_diff_model;
    p_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.diff_model;

	if (share_model_mode == 2) {
        if ((p_share_diff_model->pa != 0) && (p_share_diff_model->va != 0) && (p_share_diff_model->size != 0)) {
            if (p_proc->rbuf.pa && p_proc->rbuf.va && (p_proc->rbuf.size >= (kerl_parm_size + p_share_diff_model->size))) {
    #if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                if (p_proc->rbuf.pa% 64 != 0) {
                    DBG_WRN("io_buf start address pa(0x%lx) is NOT 64x align !!\n", (ULONG)p_proc->rbuf.pa);
                }
    #else
                if (p_proc->rbuf.pa % 32 != 0) {
                    DBG_WRN("io_buf start address pa(0x%lx) is NOT 32x align !!\n", (ULONG)p_proc->rbuf.pa);
                }
    #endif
                p_mem_out->kerl_parm.pa = p_proc->rbuf.pa; //Work buf only put user_buff
                p_mem_out->kerl_parm.va = p_proc->rbuf.va;

                p_diff_model->pa = p_proc->rbuf.pa + kerl_parm_size;
                p_diff_model->va = p_proc->rbuf.va + kerl_parm_size;
                p_diff_model->size = p_share_diff_model->size;
                memcpy((VOID *)p_diff_model->va, (VOID *)p_share_diff_model->va, p_diff_model->size);

                // Save to priv
                p_proc->priv.rbuf_buf.pa   = p_proc->rbuf.pa;
                p_proc->priv.rbuf_buf.va   = p_proc->rbuf.va;
                p_proc->priv.rbuf_buf.size = p_proc->rbuf.size;
                vendor_ai_net_trace(proc_id, AI_BUF, "start() - ronly buf alloc - io buf, pa = %#lx, va = %#lx, size = %lu\r\n",
                                p_proc->priv.rbuf_buf.pa, p_proc->priv.rbuf_buf.va, p_proc->priv.rbuf_buf.size);

            } else {
                return HD_ERR_BAD_DATA ; 
            }
        } else {
            if (p_proc->rbuf.pa && p_proc->rbuf.va && (p_proc->rbuf.size >= kerl_parm_size)) {
    #if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
                if (p_proc->rbuf.pa% 64 != 0) {
                    DBG_WRN("io_buf start address pa(0x%lx) is NOT 64x align !!\n", (ULONG)p_proc->rbuf.pa);
                }
    #else
                if (p_proc->rbuf.pa % 32 != 0) {
                    DBG_WRN("io_buf start address pa(0x%lx) is NOT 32x align !!\n", (ULONG)p_proc->rbuf.pa);
                }
    #endif
                p_mem_out->kerl_parm.pa = p_proc->rbuf.pa; //Work buf only put user_buff
                p_mem_out->kerl_parm.va = p_proc->rbuf.va;

                // Save to priv
                p_proc->priv.rbuf_buf.pa   = p_proc->rbuf.pa;
                p_proc->priv.rbuf_buf.va   = p_proc->rbuf.va;
                p_proc->priv.rbuf_buf.size = p_proc->rbuf.size;
                vendor_ai_net_trace(proc_id, AI_BUF, "start() - ronly buf alloc - io buf, pa = %#lx, va = %#lx, size = %lu\r\n",
                                p_proc->priv.rbuf_buf.pa, p_proc->priv.rbuf_buf.va, p_proc->priv.rbuf_buf.size);

            } else {
                return HD_ERR_BAD_DATA ; 
            }
        }
	} 

	return HD_OK;
}

HD_RESULT _vendor_ai_net_check_tool_iobuf_cache_align(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	HD_RESULT rv = HD_OK;
	NN_GEN_MODEL_HEAD *p_head = NULL;
	UINT32 idx = 0, idx2 = 0;
	UINT32 omem_addr=0, omem_cnt=0;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = NULL;
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	NN_DATA *p_sao = NULL;

	p_mem = &p_mem_manager->user_parm;

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head  = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	for (idx = 0; idx < p_head->mode_ctrl_num; idx++) {
		omem_addr = p_mctrl[idx].iomem.omem_addr;
		omem_cnt  = p_mctrl[idx].iomem.omem_cnt;
		p_sao     = (NN_DATA *)((omem_addr < p_mem->va)? (p_mem->va + omem_addr):(omem_addr));
		if (p_mctrl[idx].eng == NN_GEN_ENG_NUE2) continue;

		for (idx2 = 0; idx2 < omem_cnt; idx2++) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
			if (p_sao[idx2].pa % 64 != 0) {
			       DBG_ERR("proc_id(%lu) <TOOL> mctrl[%lu] sao[%lu] check fail, pa(0x%lx) is NOT 64x align !!\n", proc_id, idx, idx2, p_sao[idx2].pa);
			       rv = HD_ERR_BAD_DATA;
			}
			if (p_sao[idx2].size % 64 != 0) {
			       DBG_ERR("proc_id(%lu) <TOOL> mctrl[%lu] sao[%lu] check fail, size(%lu) is NOT 64x align !!\n", proc_id, idx, idx2, p_sao[idx2].size);
			       rv = HD_ERR_BAD_DATA;
			}
#else
			if (p_sao[idx2].pa % 32 != 0) {
			       DBG_ERR("proc_id(%lu) <TOOL> mctrl[%lu] sao[%lu] check fail, pa(0x%lx) is NOT 32x align !!\n", proc_id, idx, idx2, p_sao[idx2].pa);
			       rv = HD_ERR_BAD_DATA;
			}
			if (p_sao[idx2].size % 32 != 0) {
			       DBG_ERR("proc_id(%lu) <TOOL> mctrl[%lu] sao[%lu] check fail, size(%lu) is NOT 32x align !!\n", proc_id, idx, idx2, p_sao[idx2].size);
			       rv = HD_ERR_BAD_DATA;
			}
#endif

		}
	}

	return rv;
}

HD_RESULT _vendor_ai_net_alloc_memory(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, VENDOR_AI_NET_BUF_OPT buf_opt_method, VENDOR_AI_NET_JOB_OPT job_opt_method)
{
	return _vendor_ai_net_mem_alloc_mem(proc_id, p_mem_manager, buf_opt_method, job_opt_method);
}

BOOL _vendor_ai_net_is_shrink_buf(VENDOR_AI_NET_BUF_OPT buf_opt)
{
	switch (buf_opt) {
		case VENDOR_AI_NET_BUF_OPT_SHRINK:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O1:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O2:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O3:
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL _vendor_ai_net_is_optimize_job(VENDOR_AI_NET_JOB_OPT job_opt)
{
	switch (job_opt) {
		// with optimize level ( connect llcmd depends on graph result )
		case VENDOR_AI_NET_JOB_OPT_LINEAR_O1:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O1:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O2:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O3:
			return TRUE;

		// keep llcmd same as bin file (every llcmd seperate)
		case VENDOR_AI_NET_JOB_OPT_LINEAR:
		case VENDOR_AI_NET_JOB_OPT_GRAPH:
		default:
			return FALSE;
	}
}

BOOL _vendor_ai_net_is_linear_job(VENDOR_AI_NET_JOB_OPT job_opt)
{
	switch (job_opt) {
		case VENDOR_AI_NET_JOB_OPT_LINEAR:
		case VENDOR_AI_NET_JOB_OPT_LINEAR_O1:
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL _vendor_ai_net_is_graph_job(VENDOR_AI_NET_JOB_OPT job_opt)
{
	switch (job_opt) {
		case VENDOR_AI_NET_JOB_OPT_GRAPH:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O1:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O2:
		case VENDOR_AI_NET_JOB_OPT_GRAPH_O3:
			return TRUE;
		default:
			return FALSE;
	}
}

HD_RESULT _vendor_ai_net_pars_diff_batch(UINT32 proc_id)   // for BATCH_ID_IMM & BATCH_N_IMM
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts=0, ts_new=0;

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
	// check if VENDOR_AI_BATCH_INFO is set by user
	{
		VENDOR_AI_BATCH_INFO *p_batch_info = NULL;
		p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
		if (p_batch_info->enable) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			rv = vendor_ais_pars_diff_batch(&p_proc->mem_manager, p_batch_info, proc_id);
			if (rv != HD_OK) {
				DBG_ERR("_vendor_ai_net_pars_diff_batch FAIL !!!!\r\n");
				return rv;
			}
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				vendor_ai_net_trace(proc_id, AI_PERF, "IMM - >>> ts_pars_diff_batch time = %llu\r\n", ts_new-ts);
			}
		}
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_unpars_diff_batch(UINT32 proc_id)   // for BATCH_ID_IMM & BATCH_N_IMM
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts=0, ts_new=0;

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
	// check if VENDOR_AI_BATCH_INFO is set by user
	{
		VENDOR_AI_BATCH_INFO *p_batch_info = NULL;
		p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
		if (p_batch_info->enable) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			rv = vendor_ais_unpars_diff_batch(&p_proc->mem_manager, p_batch_info, proc_id);
			if (rv != HD_OK) {
				DBG_ERR("_vendor_ai_net_unpars_diff_batch FAIL !!!!\r\n");
				return rv;
			}
			p_batch_info->enable = 0;
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				vendor_ai_net_trace(proc_id, AI_PERF, "IMM - >>> ts_unpars_diff_batch time = %llu\r\n", ts_new-ts);
			}
		}
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_pars_diff_mem(UINT32 proc_id)   // for RES_ID_IMM & RES_DIM_IMM
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts=0, ts_new=0;

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			rv = vendor_ais_pars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				DBG_ERR("vendor_ais_pars_diff_mem FAIL !!!!\r\n");
				return rv;
			}
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> ts_pars_diff_mem time = %llu\r\n", ts_new-ts);
			}
		}
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_unpars_diff_mem(UINT32 proc_id)   // for RES_ID_IMM & RES_DIM_IMM
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts=0, ts_new=0;

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			rv = vendor_ais_unpars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				DBG_ERR("vendor_ais_unpars_diff_mem FAIL !!!!\r\n");
				return rv;
			}
			p_diff_resinfo->curr_id = 0;
            p_diff_resinfo->curr_dim.h = 0;
            p_diff_resinfo->curr_dim.w = 0;
            p_diff_resinfo->new_id = 0;
            p_diff_resinfo->new_dim.h = 0;
            p_diff_resinfo->new_dim.w = 0;
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> ts_unpars_diff_mem time = %llu\r\n", ts_new-ts);
			}
		}
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_print_err_neq_init(UINT32 proc_id, VENDOR_AI_PROC_STATE status, CHAR *p_str)
{
    if (status == VENDOR_AI_PROC_STATE_UNINIT) {
        DBG_ERR("proc_id(%lu): Current status is not init yet, please set %s after call vendor_ai_init()\r\n", proc_id, p_str);
        return HD_ERR_UNINIT;
    } else if (status == VENDOR_AI_PROC_STATE_OPEN){
		DBG_ERR("proc_id(%lu): Current status is already open, please set %s before call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_ALREADY_OPEN;
    } else if (status == VENDOR_AI_PROC_STATE_START){
		DBG_ERR("proc_id(%lu): Current status is already start, please set %s before call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_ALREADY_START;
    }
	return HD_OK;
}

HD_RESULT _vendor_ai_net_print_err_neq_open(UINT32 proc_id, VENDOR_AI_PROC_STATE status, CHAR *p_str)
{
    if (status == VENDOR_AI_PROC_STATE_UNINIT) {
        DBG_ERR("proc_id(%lu): Current status is not init yet, please set %s after call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_UNINIT;
    } else if (status == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("proc_id(%lu): Current status is not open yet, please set %s after call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_INIT;
    } else if (status == VENDOR_AI_PROC_STATE_START){
		DBG_ERR("proc_id(%lu): Current status is already start, please set %s before call vendor_ai_net_start()\r\n", proc_id, p_str);
        return HD_ERR_ALREADY_START;
    }
	return HD_OK;
}

HD_RESULT _vendor_ai_net_print_err_neq_start(UINT32 proc_id, VENDOR_AI_PROC_STATE status, CHAR *p_str)
{
    if (status == VENDOR_AI_PROC_STATE_UNINIT) {
        DBG_ERR("proc_id(%lu): Current status is not init yet, please set %s after call vendor_ai_net_start()\r\n", proc_id, p_str);
        return HD_ERR_UNINIT;
    } else if (status == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("proc_id(%lu): Current status is not open yet, please set %s after call vendor_ai_net_start()\r\n", proc_id, p_str);
        return HD_ERR_INIT;
    } else if (status == VENDOR_AI_PROC_STATE_OPEN){
		DBG_ERR("proc_id(%lu): Current status is not start yet, please set %s after call vendor_ai_net_start()\r\n", proc_id, p_str);
        return HD_ERR_NOT_START;
    }
	return HD_OK;
}

HD_RESULT _vendor_ai_net_print_err_neq_open_and_start(UINT32 proc_id, VENDOR_AI_PROC_STATE status, CHAR *p_str)
{
    if (status == VENDOR_AI_PROC_STATE_UNINIT) {
        DBG_ERR("proc_id(%lu): Current status is not init yet, please set %s after call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_UNINIT;
    } else if (status == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("proc_id(%lu): Current status is not open yet, please set %s after call vendor_ai_net_open()\r\n", proc_id, p_str);
        return HD_ERR_INIT;
    } 
	return HD_OK;
}

HD_RESULT vendor_ai_net_reset_context (UINT32 proc_id)
{
#if (NN_USER_HEAP == 1)
#else
	//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_INFO_PROC *p_proc;
	p_proc = _vendor_ai_info(proc_id);
	
	memset(p_proc, 0, sizeof(VENDOR_AI_NET_INFO_PROC));
#endif
	// set default parameters
	_vendor_ai_net_fill_all_default_params(proc_id);
	
	return HD_OK;
}

HD_RESULT vendor_ai_net_reset_state (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);

	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START) {
		DBG_WRN("ERROR: proc_id(%lu) is NOT stop/close yet, before uninit!\r\n", proc_id);
		rv = HD_ERR_ALREADY_START;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN) {
		DBG_WRN("ERROR: proc_id(%lu) is NOT close yet, before uninit!\r\n", proc_id);
		rv = HD_ERR_ALREADY_OPEN;
	} else {
		// set status
		_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_UNINIT; // [INIT]/[UNINIT] -> [UNINT]
	}
	return rv;
}

HD_RESULT vendor_ai_net_set (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID param_id, void* p_param)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT32 layer_id = VENDOR_AI_GET_LAYER(param_id);
	UINT32 in_id    = VENDOR_AI_GET_IN(param_id);
	UINT32 out_id   = VENDOR_AI_GET_OUT(param_id);

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	if (p_param == NULL) {
		DBG_ERR("proc_id(%lu) p_param is NULL !!\n", proc_id);
		return HD_ERR_NULL_PTR;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	// check input parameter valid
	{
		CHAR msg_string[256];
		if (_vendor_ai_net_check_params_range(param_id, p_param, msg_string, 256) < 0) {
			DBG_ERR("Wrong value. %s, id=%d\n", msg_string, param_id);
			return HD_ERR_PARAM;
		}
	}

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
		
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("could only be called after INIT, please call vendor_ai_net_init() first\r\n");
		return HD_ERR_UNINIT;
	}

	switch (param_id) {
		case VENDOR_AI_NET_PARAM_CFG_MODEL:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
				CHAR *cfg_type = "CFG_MODEL";
				rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_NET_CFG_MODEL *p_user = (VENDOR_AI_NET_CFG_MODEL *) p_param;
				VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
				VENDOR_AI_NET_CFG_MODEL *p_cfg_share_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_share_model;
				vendor_ai_net_trace(proc_id, AI_FLOW|AI_JOB|AI_BUF, "set() - MODEL = (0x%08x, 0x%08x, %d)\r\n", (UINT)p_user->pa, (UINT)p_user->va, p_user->size);
				
				memcpy(p_cfg_model, p_user, sizeof(VENDOR_AI_NET_CFG_MODEL));       // user_parm
				memcpy(p_cfg_share_model, p_user, sizeof(VENDOR_AI_NET_CFG_MODEL)); // user_model
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_SHAREMODEL:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
				CHAR *cfg_type = "CFG_SHAREMODEL";
				rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
				if (p_cfg_model->pa == 0 || p_cfg_model->va == 0 || p_cfg_model->size == 0 ) {
					DBG_ERR("set CFG_SHAREMODEL is FAILED : could only be set after CFG_MODEL is set !!\r\n");
					return HD_ERR_USER;
				}
			}
			{
				VENDOR_AI_NET_CFG_MODEL *p_user = (VENDOR_AI_NET_CFG_MODEL *) p_param;
				VENDOR_AI_NET_CFG_MODEL *p_cfg_share_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_share_model;
				vendor_ai_net_trace(proc_id, AI_FLOW|AI_JOB|AI_BUF, "set() - SHAREMODEL = (0x%08x, 0x%08x, %d)\r\n", (UINT)p_user->pa, (UINT)p_user->va, p_user->size);

				memcpy(p_cfg_share_model, p_user, sizeof(VENDOR_AI_NET_CFG_MODEL)); // user_model
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_JOB_OPT:
			if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT) {
				VENDOR_AI_NET_CFG_JOB_OPT *p_user = (VENDOR_AI_NET_CFG_JOB_OPT *) p_param;
				VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;
				vendor_ai_net_trace(proc_id, AI_GRP|AI_JOB, "set() - JOB_OPT = %d %d\r\n", p_user->method, p_user->wait_ms);
				
				memcpy(p_job_opt, p_user, sizeof(VENDOR_AI_NET_CFG_JOB_OPT));
			}
			else {
				//only update schd_parm
				VENDOR_AI_NET_CFG_JOB_OPT *p_user = (VENDOR_AI_NET_CFG_JOB_OPT *) p_param;
				VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;

				p_job_opt->schd_parm = p_user->schd_parm;
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_BUF_OPT:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
				CHAR *cfg_type = "CFG_BUF_OPT";
				rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_NET_CFG_BUF_OPT *p_user = (VENDOR_AI_NET_CFG_BUF_OPT *) p_param;
				VENDOR_AI_NET_CFG_BUF_OPT *p_buf_opt = (VENDOR_AI_NET_CFG_BUF_OPT *)&p_cfg->buf_opt;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - BUF_OPT = %d %d\r\n", p_user->method, p_user->ddr_id);
				
				memcpy(p_buf_opt, p_user, sizeof(VENDOR_AI_NET_CFG_BUF_OPT));
			}
			break;

#if CNN_USER_POSTPROC
		case VENDOR_AI_NET_PARAM_CFG_USER_POSTPROC:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
				CHAR *cfg_type = "CFG_USER_POSTPROC";
				rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_ENGINE_PLUGIN *p_user = (VENDOR_AI_ENGINE_PLUGIN *) p_param;
				VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine;

				vendor_ai_net_trace(proc_id, AI_BUF, "set() - USER_POSTPROC = %08x %d\r\n", p_user);

				p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&p_cfg->user_postproc;
				if (p_user == 0) {
					DBG_ERR("cfg engine => clear!\r\n");
					p_cfg_engine->sign = 0;
					return HD_OK;
				}
				
				if (p_user->sign == 0) {
					DBG_ERR("cfg engine => invalid sign!\r\n");
					return HD_ERR_SIGN;
				}
				memcpy(p_cfg_engine, p_user, sizeof(VENDOR_AI_ENGINE_PLUGIN));
			}
			break;
#endif
			
		case VENDOR_AI_NET_PARAM_RES_ID:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "RES_ID";
				rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_model = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - RES_ID = %#lu\r\n", *p_user);
				p_diff_model->new_id = *p_user;
				memcpy(&p_diff_model->new_dim, &p_diff_model->curr_dim, sizeof(HD_DIM));
			}
			break;

		case VENDOR_AI_NET_PARAM_RES_ID_IMM:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "RES_ID_IMM";
				rv = _vendor_ai_net_print_err_neq_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - RES_ID_IMM\r\n");
				if ((rv = _vendor_ai_net_unpars_diff_mem(proc_id)) != HD_OK) return rv;
				if ((rv = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_ID, p_param)) != HD_OK) return rv;
				if ((rv = _vendor_ai_net_pars_diff_mem(proc_id)) != HD_OK) return rv;
			}
			break;

		case VENDOR_AI_NET_PARAM_RES_DIM:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "RES_DIM";
				rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				HD_DIM *p_user = (HD_DIM *) p_param;
				VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_model = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - RES_DIM = (%#08x, %#08x)\r\n", (UINT)p_user->w, (UINT)p_user->h);
				if ((p_user->w == 0) || (p_user->h == 0) || (p_user->w > NUE2_MAX_WIDTH) ||(p_user->h > NUE2_MAX_HEIGHT)) {
					DBG_ERR("set() - RES_DIM = (%d, %d) fail\r\n", (UINT)p_user->w, (UINT)p_user->h);
					return HD_ERR_LIMIT;
				}
				memcpy(&p_diff_model->new_dim, p_user, sizeof(HD_DIM));
				p_diff_model->new_id = p_diff_model->curr_id;
			}
			break;

		case VENDOR_AI_NET_PARAM_RES_DIM_IMM:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "RES_DIM_IMM";
				rv = _vendor_ai_net_print_err_neq_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - RES_DIM_IMM\r\n");
				if ((rv = _vendor_ai_net_unpars_diff_mem(proc_id)) != HD_OK) return rv;
				if ((rv = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_DIM, p_param)) != HD_OK) return rv;
				if ((rv = _vendor_ai_net_pars_diff_mem(proc_id)) != HD_OK) return rv;
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_ID:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "BATCH_ID";
				rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_BATCH_INFO *p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - BATCH_ID = %#lu\r\n", *p_user);
				p_batch_info->id = *p_user;
				p_batch_info->enable = 1;
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_ID_IMM:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "BATCH_ID_IMM";
				rv = _vendor_ai_net_print_err_neq_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - BATCH_ID_IMM\r\n");
				if ((rv = _vendor_ai_net_unpars_diff_batch(proc_id)) != HD_OK) return rv;
				if ((rv = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_BATCH_ID, p_param)) != HD_OK) return rv;
				if ((rv = _vendor_ai_net_pars_diff_batch(proc_id)) != HD_OK) return rv;
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_N:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "BATCH_N";
				rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_BATCH_INFO *p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info; 
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - BATCH_N = %#lu\r\n", *p_user);
				p_batch_info->num = *p_user;
				p_batch_info->enable = 1;
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_N_IMM:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "BATCH_N_IMM";
				rv = _vendor_ai_net_print_err_neq_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - BATCH_N_IMM\r\n");
				if ((rv = _vendor_ai_net_unpars_diff_batch(proc_id)) != HD_OK) return rv;
				if ((rv = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_BATCH_N, p_param)) != HD_OK) return rv;
				if ((rv = _vendor_ai_net_pars_diff_batch(proc_id)) != HD_OK) {
                    VENDOR_AI_BATCH_INFO *p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info; 
                    memset(p_batch_info, 0 ,sizeof(VENDOR_AI_BATCH_INFO));
                    return rv;
                }
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
				CHAR *cfg_type = "CFG_MODEL_RESINFO";
				rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_NET_CFG_MODEL *p_user = (VENDOR_AI_NET_CFG_MODEL *) p_param;
				VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - MODEL_RESINFO = (0x%08x, 0x%08x, %d)\r\n", (UINT)p_user->pa, (UINT)p_user->va, p_user->size);

				if(_vendor_ai_common_info()->share_model_mode == 2) {
                    memcpy(&p_diff_resinfo->share_diff_model, p_user, sizeof(VENDOR_AI_NET_CFG_MODEL));
                } else {
                    memcpy(&p_diff_resinfo->diff_model, p_user, sizeof(VENDOR_AI_NET_CFG_MODEL));
                }
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_WORKBUF:
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) {
				CHAR *cfg_type = "CFG_WORKBUF";
				rv = _vendor_ai_net_print_err_neq_open(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				VENDOR_AI_NET_CFG_WORKBUF *p_user = (VENDOR_AI_NET_CFG_WORKBUF *) p_param;
				VENDOR_AI_NET_CFG_WORKBUF *p_workbuf = (VENDOR_AI_NET_CFG_WORKBUF *)&p_proc->workbuf;
				vendor_ai_net_trace(proc_id, AI_BUF, "set() - WORKBUF = (0x%08x, 0x%08x, %d)\r\n", (UINT)p_user->pa, (UINT)p_user->va, p_user->size);

#if CNN_INNER_POSTPROC
				if (p_user->size < p_proc->iobuf.size + p_proc->rsltbuf.size) {
					DBG_ERR("size of workbuf is not enough\r\n");
					return HD_ERR_NOBUF;
				}
#else
				if (p_user->size < p_proc->iobuf.size) {
					DBG_ERR("size of workbuf is not enough\r\n");
					return HD_ERR_NOBUF;
				}
#endif
				p_workbuf->pa = p_user->pa;  //only copy pa/va/size from user // (x) memcpy(p_workbuf, p_user, sizeof(VENDOR_AI_NET_CFG_WORKBUF));
				p_workbuf->va = p_user->va;
				p_workbuf->size = p_user->size;

#if CNN_INNER_POSTPROC
				if (p_proc->rsltbuf.size > 0) {
					HD_RESULT r1;

					// rsltbuf should put before io_buff
					p_proc->rsltbuf.pa = p_user->pa;
					p_proc->rsltbuf.va = p_user->va;
					p_proc->iobuf.pa = p_proc->rsltbuf.pa + p_proc->rsltbuf.size;
					p_proc->iobuf.va = p_proc->rsltbuf.va + p_proc->rsltbuf.size;

					r1 = _vendor_ai_net_set_last_layer_cpu_buffer_addr(proc_id, p_proc->rsltbuf.va, p_proc->rsltbuf.size);
					if (r1 != HD_OK) {
						DBG_ERR("addr of rsltbuf is not enough\r\n");
						return r1;
					}
				} else {
					// without rsltbuf, iobuf should be whole workbuf
					p_proc->iobuf.pa = p_user->pa;
					p_proc->iobuf.va = p_user->va;
				}
#endif
			}
			break;
			case VENDOR_AI_NET_PARAM_CFG_RONLYBUF:
				if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) {
					CHAR *cfg_type = "CFG_RONLYBUF";
					rv = _vendor_ai_net_print_err_neq_open(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
					return rv;
				}
				{
					VENDOR_AI_NET_CFG_RONLYBUF *p_user = (VENDOR_AI_NET_CFG_RONLYBUF *) p_param;
					VENDOR_AI_NET_CFG_RONLYBUF *p_rbuf = (VENDOR_AI_NET_CFG_RONLYBUF *)&p_proc->rbuf;
					vendor_ai_net_trace(proc_id, AI_BUF, "set() - RONLYBUF = (0x%lx, 0x%lx, %d)\r\n", (ULONG)p_user->pa, (ULONG)p_user->va, p_user->size);
						


					if (p_user->size < p_proc->rbuf.size ) {
						DBG_ERR("size of ronlybuf is not enough\r\n");
						return HD_ERR_NOBUF;
					}

					p_rbuf->pa = p_user->pa;  //only copy pa/va/size from user // (x) memcpy(p_rbuf, p_user, sizeof(VENDOR_AI_NET_CFG_RONLYBUF));
					p_rbuf->va = p_user->va;
					p_rbuf->size = p_user->size;
				}
				break;	
			case VENDOR_AI_NET_PARAM_CFG_INTLBUF:
				if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT) {
					CHAR *cfg_type = "CFG_INTLBUF";
					rv = _vendor_ai_net_print_err_neq_init(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
					return rv;
				}
				{
					VENDOR_AI_NET_CFG_INTLBUF *p_user = (VENDOR_AI_NET_CFG_INTLBUF *) p_param;
//					VENDOR_AI_NET_CFG_INTLBUF *p_intlbuf = (VENDOR_AI_NET_CFG_INTLBUF *)&p_cfg->intlbuf;
					vendor_ai_net_trace(proc_id, AI_BUF, "set() - INTLBUF = (0x%lx, 0x%lx, %d)\r\n", (ULONG)p_user->pa, (ULONG)p_user->va, p_user->size);

					p_cfg->intlbuf.pa = p_user->pa;
					p_cfg->intlbuf.va = p_user->va;
				}
				break;
				
		case VENDOR_AI_NET_PARAM_CUSTOM_INFO:
			if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
				CHAR *cfg_type = "CUSTOM_INFO";
				rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
				return rv;
			}
			{
				p_proc = _vendor_ai_info(proc_id);
				p_proc->priv.usr_info = p_param;
			}
			break;
		default:
			//--- check if this is PARAM_IN() or PARAM_OUT() ---
			switch(VENDOR_AI_GET_PARAM_TYPE(param_id)) {
				case VENDOR_AI_PARAM_TYPE_LAYER:
				{
					//
					vendor_ai_net_trace(proc_id, AI_JOB, "set() - LAYER[%d] = %08x\r\n", layer_id, (void*)p_param);
					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						CHAR *cfg_type = "TYPE_LAYER";
						rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
						return rv;
					}
				}
				break;

				case VENDOR_AI_PARAM_TYPE_IN:
				{
					UINT64 ts = 0, ts_new = 0;
					UINT64 ts_inputset= 0;
					VENDOR_AI_BUF_LIST *buf_list = (VENDOR_AI_BUF_LIST *)p_param;

					vendor_ai_net_trace(proc_id, AI_BUF, "set() - LAYER[%d].IN[%d] = %08x\r\n", layer_id, in_id, (void*)p_param);
					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						CHAR *cfg_type = "TYPE_IN";
						rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
						return rv;
					}
					if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
						ts = hd_gettime_us();
					}

					if (buf_list->sign == MAKEFOURCC('A','L','S','T')) {
						// input by ai_buf_list (Y8/UV or R8/G8/B8)
						rv = _vendor_ai_net_set_input_img_by_list(proc_id, (VENDOR_AI_BUF_LIST*)p_param, layer_id, in_id);
					} else {
						// input by ai_buf
						rv = _vendor_ai_net_set_input_img(proc_id, (VENDOR_AI_BUF*)p_param, layer_id, in_id);
					}

					if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
						ts_new = hd_gettime_us();
						ts_inputset = ts_new-ts;
						vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "set() - inputbuf set time = %llu\r\n", ts_inputset);
					}
				}
				break;

				case VENDOR_AI_PARAM_TYPE_OUT:
				{
					if ((layer_id == VENDOR_AI_MAXLAYER) && (out_id ==0)) {
						// ...
						vendor_ai_net_trace(proc_id, AI_BUF, "set() - LAYER[%d].OUT[%d] = %08x\r\n", layer_id, out_id, (void*)p_param);
						if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
							CHAR *cfg_type = "TYPE_OUT";
							rv = _vendor_ai_net_print_err_neq_open_and_start(proc_id, _vendor_ai_state(proc_id)[0], cfg_type);
							return rv;
						}
					}
				}
				break;

				default:
				{
					DBG_ERR("param_id(%u) not support !!\r\n", param_id);
					return HD_ERR_PARAM;
				}
				break;
			}
			break;
	}

//exit:
	
	return rv;
}

HD_RESULT vendor_ai_net_get (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID param_id, void* p_param)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT32 layer_id = VENDOR_AI_GET_LAYER(param_id);
	UINT32 in_id    = VENDOR_AI_GET_IN(param_id);
	UINT32 out_id   = VENDOR_AI_GET_OUT(param_id);

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("could only be called after INIT, please call vendor_ai_net_init() first\r\n");
		return HD_ERR_UNINIT;
	}

	switch (param_id) {
		case VENDOR_AI_NET_PARAM_STATE:
			{
				if (p_param) {
					UINT32 *p_user = (UINT32 *) p_param;
					p_user[0] = (_vendor_ai_state(proc_id)[0]);
				}
			}
			break;
			
		case VENDOR_AI_NET_PARAM_CFG_MODEL:
			{
				VENDOR_AI_NET_CFG_MODEL *p_user = (VENDOR_AI_NET_CFG_MODEL *) p_param;
				VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
				memcpy(p_user, p_cfg_model, sizeof(VENDOR_AI_NET_CFG_MODEL));
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_SHAREMODEL:
			{
				VENDOR_AI_NET_CFG_MODEL *p_user = (VENDOR_AI_NET_CFG_MODEL *) p_param;
				VENDOR_AI_NET_CFG_MODEL *p_cfg_share_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_share_model;
				memcpy(p_user, p_cfg_share_model, sizeof(VENDOR_AI_NET_CFG_MODEL));
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_JOB_OPT:
			{
				VENDOR_AI_NET_CFG_JOB_OPT *p_user = (VENDOR_AI_NET_CFG_JOB_OPT *) p_param;
				VENDOR_AI_NET_CFG_JOB_OPT *p_job_opt = (VENDOR_AI_NET_CFG_JOB_OPT *)&p_cfg->job_opt;
				memcpy(p_user, p_job_opt, sizeof(VENDOR_AI_NET_CFG_JOB_OPT));
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_BUF_OPT:
			{
				VENDOR_AI_NET_CFG_BUF_OPT *p_user = (VENDOR_AI_NET_CFG_BUF_OPT *) p_param;
				VENDOR_AI_NET_CFG_BUF_OPT *p_buf_opt = (VENDOR_AI_NET_CFG_BUF_OPT *)&p_cfg->buf_opt;
				memcpy(p_user, p_buf_opt, sizeof(VENDOR_AI_NET_CFG_BUF_OPT));
			}
			break;

		case VENDOR_AI_NET_PARAM_INFO:
#if CNN_MULTI_INPUT
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN && _vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
				DBG_ERR("invalid state(%d), could only be called after OPEN, please call vendor_ai_net_open() first\r\n", _vendor_ai_state(proc_id)[0]);
				return HD_ERR_NOT_OPEN;
			}
#else
			if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) {
				DBG_ERR("could only be called after OPEN, please call vendor_ai_net_open() first\r\n");
				return HD_ERR_NOT_OPEN;
			}
#endif
			{
				VENDOR_AI_NET_INFO *p_info = (VENDOR_AI_NET_INFO *) p_param;
				if (p_info == NULL) {
					DBG_ERR("p_info is null\r\n");
					return HD_ERR_NULL_PTR;
				}
				
				p_info->model_part1_size = p_proc->mem_manager.user_parm.size;
				p_info->model_part2_size = p_proc->mem_manager.user_model.size;
				p_info->model_layer_cnt = 0;
				p_info->model_bind_cnt = 0;
				p_info->model_buf_cnt = 0;
				p_info->model_dep_cnt = 0;

				// job status
				p_info->total_job_cnt = 0;
				p_info->total_bind_cnt = 0;
				p_info->in_job_cnt = 0;
				p_info->out_job_cnt = 0;
				
				// buffer status
				p_info->total_buf_cnt = 0;
				rv = _vendor_ai_net_get_src_layer_cnt_fmt(proc_id, &p_info->in_buf_cnt, &p_info->in_buf_type);
				if (rv != HD_OK) {
					p_info->in_buf_cnt = 0;
					p_info->in_buf_type = 0;
				}
				rv = _vendor_ai_net_get_dest_layer_cnt_fmt(proc_id, &p_info->out_buf_cnt, &p_info->out_buf_type);
				if (rv != HD_OK) {
					p_info->out_buf_cnt = 0;
					p_info->out_buf_type = 0;
				}

				p_info->in_buf_size = 0;
				p_info->io_buf_size = 0;
				p_info->out_buf_size = 0;

#if 0 //TODO ------- porting to get net info
				VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
				ER er = E_OK;
				NN_GEN_NET_INFO net_info = {0};
				NN_GEN_MODEL_HEAD *p_head;
				
				er = vendor_ais_get_net_info(&net_info, net_mem.va);
				if (er != E_OK) {
					DBG_ERR("vendor_ais_get_net_info fail...\r\n");
					return er;
				}
			
				p_head = net_info.p_head;
			
				if (1) {
					UINT32 model_head_size, mctrl_size, id_list_size, external_size, bufinfo_size, io_mem_addr_size, parm_size;
					UINT32 model_size;
			
					p_proc->info.model_layer_cnt = p_head->mode_ctrl_num;
					p_proc->info.model_bind_cnt = p_head->layer_id_list_size/sizeof(UINT32);
					p_proc->info.model_buf_cnt = p_head->iomem_size/sizeof(NN_IOMEM);
					//p_proc->info.model_dep_cnt = 0;
			
					// HEAD
					model_head_size 	= ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD));
					// JOB CNT
					mctrl_size			= ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * proc_layer_num);
					// JOB BIND CNT
					id_list_size		= p_head->layer_id_list_size;
					// IO BUFFER ADDIITONAL DESC
					external_size		= p_head->external_size;
					// IO BUFFER REGION
					bufinfo_size		= p_head->bufinfo_size;
					// IO BUFFER BASIC DESC
					io_mem_addr_size	= ALIGN_CEIL_4(p_head->iomem_size);
					// JOB WORKLOAD (APP or LLCMD)
					parm_size			= ALIGN_CEIL_4(p_head->parm_size);
					
					p_proc->info.model_part1_size = model_head_size + mctrl_size + id_list_size + external_size + bufinfo_size + io_mem_addr_size + parm_size;
			
					// WEIGHT DESC + WEIGHT BUFFER
					model_size			= ALIGN_CEIL_4(p_head->model_size);
					
					p_proc->info.model_part2_size = model_size;  ///< -model part2 size (weight-buffer)
			
					p_proc->info.model_buf_size = p_proc->info.model_part1_size + p_proc->info.model_part2_size;
				}
				
				//after open
				if (1) {
					HD_RESULT rv = HD_OK;
					p_proc->info.total_buf_cnt = 0;
					rv = _vendor_ai_net_get_src_layer_cnt_fmt(proc_id, &p_proc->info.in_buf_cnt, &p_proc->info.in_buf_type);
					if (rv != HD_OK) {
						p_proc->info.in_buf_cnt = 0;
						p_proc->info.in_buf_type = 0;
					}
					rv = _vendor_ai_net_get_dest_layer_cnt_fmt(proc_id, &p_proc->info.out_buf_cnt, &p_proc->info.out_buf_type);
					if (rv != HD_OK) {
						p_proc->info.out_buf_cnt = 0;
						p_proc->info.out_buf_type = 0;
					}
					/*
					UINT32 io_buff_size;
					// IO BUFFER
					io_buff_size		= ALIGN_CEIL_4(p_head->io_buff_size);
					
					p_proc->work_buf_size;	   ///< work buffer size
					p_proc->in_buf_size;	   ///< -in buffer size
					p_proc->io_buf_size;	   ///< -io buffer size
					p_proc->out_buf_size;	   ///< -out buffer size
					p_proc->misc_buf_size;	   ///< -misc buffer size (result,....)
					*/
				}
			
				//after start
				if (1) {
					p_proc->info.total_job_cnt = JOB_CNT;
					p_proc->info.total_bind_cnt = BIND_CNT;
					p_proc->info.in_job_cnt = src_count;
					p_proc->info.out_job_cnt = dest_count;
				}
				
#endif
				
			}
			break;

		case VENDOR_AI_NET_PARAM_IN_PATH_LIST:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
					DBG_ERR("could only be called after OPEN\r\n");
					return HD_ERR_NOT_OPEN;
				}

				rv = _vendor_ai_net_get_in_path_list(proc_id, p_user);
				if (rv != HD_OK) {
					DBG_ERR("get_in_path_list fail(%d)\r\n", rv);
					return rv;
				}
			}
			break;

		case VENDOR_AI_NET_PARAM_OUT_PATH_LIST:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
					DBG_ERR("could only be called after OPEN\r\n");
					return HD_ERR_NOT_OPEN;
				}

				rv = _vendor_ai_net_get_out_path_list(proc_id, p_user);
				if (rv != HD_OK) {
					DBG_ERR("get_out_path_list fail(%d)\r\n", rv);
					return rv;
				}
			}
			break;
			
		case VENDOR_AI_NET_PARAM_OUT_PATH_BY_NAME:
			{
#if !CNN_25_MATLAB
#if CNN_FMT_V4
				VENDOR_AI_BUF_NAME *p_info = (VENDOR_AI_BUF_NAME *) p_param;
				if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
					DBG_ERR("could only be called after OPEN\r\n");
					return HD_ERR_NOT_OPEN;
				}

				rv = _vendor_ais_get_path_by_name(proc_id, p_info);
				if (rv != HD_OK) {
					DBG_ERR("get_path_by_name fail(%d)\r\n", rv);
					return rv;
				}
#else
				VENDOR_AI_BUF_NAME *p_info = (VENDOR_AI_BUF_NAME *) p_param;
				NN_LAYER_OUTPUT_INFO *output_layer_info = NULL;
				UINT32 num_output_layer = 0;

				if (p_info == NULL) {
					DBG_ERR("p_info is null\r\n");
					return HD_ERR_NULL_PTR;
				}

				_vendor_ais_get_external_output_info(proc_id, &output_layer_info, &num_output_layer);

				rv = _vendor_ais_find_layer_info_by_name(proc_id, p_info->name, output_layer_info, &p_info->path_id, num_output_layer);
				if (rv != HD_OK) {
					DBG_ERR("find_layer_info_by_name fail(%d)\r\n", rv);
					return rv;
				}
#endif
#else
				DBG_ERR("Only support for cgen version!!!\r\n");
#endif
			}
			break;

		case VENDOR_AI_NET_PARAM_RES_ID:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_model = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
				vendor_ai_net_trace(proc_id, AI_BUF, "get() - RES_ID = %#lu\r\n", p_diff_model->curr_id);
				*p_user = p_diff_model->curr_id;
			}
			break;

		case VENDOR_AI_NET_PARAM_RES_DIM:
			{
				HD_DIM *p_user = (HD_DIM *) p_param;
				VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_model = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
				vendor_ai_net_trace(proc_id, AI_BUF, "get() - RES_DIM = (%#08x, %#08x)\r\n", (UINT)p_diff_model->curr_dim.w, (UINT)p_diff_model->curr_dim.h);

				memcpy(p_user, &p_diff_model->curr_dim, sizeof(HD_DIM));
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_ID:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_BATCH_INFO *p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
				vendor_ai_net_trace(proc_id, AI_BUF, "get() - BATCH_ID = %#lu\r\n", p_batch_info->id);
				*p_user = p_batch_info->id;
			}
			break;

		case VENDOR_AI_NET_PARAM_BATCH_N:
			{
				UINT32 *p_user = (UINT32 *) p_param;
				VENDOR_AI_BATCH_INFO *p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
				vendor_ai_net_trace(proc_id, AI_BUF, "get() - BATCH_N = %#lu\r\n", p_batch_info->num);
				*p_user = p_batch_info->num;
			}
			break;

		case VENDOR_AI_NET_PARAM_CFG_WORKBUF:
			{
				VENDOR_AI_NET_CFG_WORKBUF *p_user = (VENDOR_AI_NET_CFG_WORKBUF *) p_param;
				VENDOR_AI_NET_CFG_WORKBUF *p_workbuf = (VENDOR_AI_NET_CFG_WORKBUF *)&p_proc->iobuf;
				vendor_ai_net_trace(proc_id, AI_BUF, "get() - WORKBUF = (0x%08x, 0x%08x, %d)\r\n", (UINT)p_workbuf->pa, (UINT)p_workbuf->va, p_workbuf->size);

				if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
					DBG_ERR("could only be called after OPEN\r\n");
					return HD_ERR_NOT_OPEN;
				}
				memcpy(p_user, p_workbuf, sizeof(VENDOR_AI_NET_CFG_WORKBUF));
#if CNN_INNER_POSTPROC
				if (1) {
					UINT32 buffer_size = 0;
					HD_RESULT r1;
					r1 = _vendor_ai_net_get_last_layer_cpu_buffer_size(proc_id, &buffer_size);
					if (r1 == HD_OK) {
						p_proc->rsltbuf.size = buffer_size;
						p_user->size += p_proc->rsltbuf.size;
					}
				}
#endif
			}
			break;
		case VENDOR_AI_NET_PARAM_CFG_RONLYBUF:
			{
				VENDOR_AI_NET_CFG_RONLYBUF *p_user = (VENDOR_AI_NET_CFG_RONLYBUF *) p_param;
				VENDOR_AI_NET_CFG_RONLYBUF *p_rbuf = (VENDOR_AI_NET_CFG_RONLYBUF *)&p_proc->rbuf;
					vendor_ai_net_trace(proc_id, AI_BUF, "get() - RONLYBUF = (0x%lx, 0x%lx, %d)\r\n", (ULONG)p_rbuf->pa, (ULONG)p_rbuf->va, p_rbuf->size);

					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						DBG_ERR("could only be called after OPEN\r\n");
						return HD_ERR_NOT_OPEN;
					}
					memcpy(p_user, p_rbuf, sizeof(VENDOR_AI_NET_CFG_RONLYBUF));	
			}
			break;
		case VENDOR_AI_NET_PARAM_CFG_INTLBUF:
				{
					VENDOR_AI_NET_CFG_INTLBUF *p_user = (VENDOR_AI_NET_CFG_INTLBUF *) p_param;
					VENDOR_AI_NET_CFG_INTLBUF *p_intlbuf = (VENDOR_AI_NET_CFG_INTLBUF *)&p_cfg->intlbuf;
					VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
					UINT32 share_model_mode = _vendor_ai_common_info()->share_model_mode;
					NN_GEN_NET_INFO net_info = {0};
					UINT32 mode_ctrl_num = 0;
					UINT32 require_sz = 0, parm_sz = 0, model_sz = 0, buf_sz = 0;

					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_INIT)) {
						DBG_ERR("could only be called after INIT\r\n");
						return HD_ERR_ALREADY_OPEN;
					}
					if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
						DBG_ERR("please set CFG_MODEL before get CFG_INTLBUF\r\n");
						return HD_ERR_NG;
					}

					if (vendor_ais_cal_size(p_cfg_model->va, &parm_sz, &model_sz, &buf_sz) != HD_OK) {
						DBG_ERR("vendor_ais_cal_size fail\r\n");
						return HD_ERR_NG;
					}
					if (vendor_ais_get_net_info(&net_info, p_cfg_model->va) != HD_OK) {
						DBG_ERR("vendor_ais_get_net_info fail\r\n");
						return HD_ERR_NG;
					}
					mode_ctrl_num = net_info.p_head->mode_ctrl_num;

					require_sz += sizeof(VENDOR_AI_NET_MCTRL_ENTRY) * mode_ctrl_num;
					require_sz += sizeof(VENDOR_AI_NET_LLGROUP) * mode_ctrl_num;
					require_sz += sizeof(VENDOR_AI_NET_MEM_ENTRY) * mode_ctrl_num * 2;

					p_intlbuf->pa = 0;
					p_intlbuf->va = 0;
					p_intlbuf->size = ALIGN_CEIL_16(parm_sz) + ALIGN_CEIL_16(require_sz); // init_buf = user_parm + group buf
					if (share_model_mode == 1) {
						p_intlbuf->size += ALIGN_CEIL_16(parm_sz); // put kerl_parm, too  // init_buf = user_parm + group buf + kerl_parm
					}
					vendor_ai_net_trace(proc_id, AI_BUF, "get() - INTLBUF = (0x%lx, 0x%lx, %d)\r\n", (ULONG)p_intlbuf->pa, (ULONG)p_intlbuf->va, p_intlbuf->size);
					memcpy(p_user, p_intlbuf, sizeof(VENDOR_AI_NET_CFG_INTLBUF));
				}
				break;

		/*case VENDOR_AI_NET_PARAM_FIRST_LAYER_INPUT:
			{
#if !CNN_25_MATLAB
#if !CNN_FMT_V4
				VENDOR_AI_BUF *p_user = (VENDOR_AI_BUF *) p_param;
				rv = _vendor_ai_net_get_first_layer_input_info(proc_id, p_user);
				if (rv != HD_OK) {
					DBG_ERR("get_first_layer_input_info fail(%d)\r\n", rv);
				}
#else
				DBG_ERR("V4 not support!!!\r\n");
#endif
#else
				DBG_ERR("Only support for cgen version!!!\r\n");
#endif
			}
			break;*/

		/*case VENDOR_AI_NET_PARAM_FIRST_LAYER_OUTPUT:
			{
#if !CNN_25_MATLAB
#if !CNN_FMT_V4
				VENDOR_AI_BUF *p_user = (VENDOR_AI_BUF *) p_param;
				rv = _vendor_ai_net_get_first_layer_output_info(proc_id, p_user);
				if (rv != HD_OK) {
					DBG_ERR("get_first_layer_output_info fail(%d)\r\n", rv);
				}
#else
				DBG_ERR("V4 not support!!!\r\n");
#endif
#else
				DBG_ERR("Only support for cgen version!!!\r\n");
#endif
			}
			break;*/

		case VENDOR_AI_NET_PARAM_LAST_LAYER_LABELNUM:
			{
				UINT32 *p_label_num = (UINT32 *) p_param;
				if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
					DBG_ERR("could only be called after OPEN\r\n");
					return HD_ERR_NOT_OPEN;
				}
				
				rv = _vendor_ai_net_get_last_layer_labelnum(proc_id, p_label_num);
				if (rv != HD_OK) {
					DBG_ERR("get_last_layer_labelnum fail(%d)\r\n", rv);
				}
			}
			break;

		default:
			//--- check if this is PARAM_IN() or PARAM_OUT() ---
			switch(VENDOR_AI_GET_PARAM_TYPE(param_id)) {
				case VENDOR_AI_PARAM_TYPE_LAYER:
				{
					//
					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						DBG_ERR("could only be called after OPEN\r\n");
						return HD_ERR_NOT_OPEN;
					}
				}
				break;

				case VENDOR_AI_PARAM_TYPE_IN:
				{
					VENDOR_AI_BUF *p_user = (VENDOR_AI_BUF *) p_param;
					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						DBG_ERR("could only be called after OPEN\r\n");
						return HD_ERR_NOT_OPEN;
					}

					rv = _vendor_ai_net_get_iomem(proc_id, layer_id, GET_PORT_TYPE_IN, in_id, p_user);
					if (rv != HD_OK) goto exit;
				}
				break;

				case VENDOR_AI_PARAM_TYPE_OUT:
				{
					VENDOR_AI_BUF *p_user = (VENDOR_AI_BUF *) p_param;
					if ((_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) && (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START)) {
						DBG_ERR("could only be called after OPEN\r\n");
						return HD_ERR_NOT_OPEN;
					}
					
#if CNN_INNER_POSTPROC
					if ((layer_id == VENDOR_AI_MAXLAYER) && (out_id ==0) && (p_proc->rsltbuf.size > 0)) {
						UINT32 struct_addr = 0;
						UINT32 struct_size = 0;
						HD_RESULT r1;
						//p_proc->rsltbuf.pa
						//p_proc->rsltbuf.va
						//p_proc->rsltbuf.size
						r1 = _vendor_ai_net_get_last_layer_cpu_buffer_struct(proc_id, &struct_addr, &struct_size);
						if (r1 == HD_OK) {
							memcpy((void*)p_param, (void*)struct_addr, struct_size);
						}
						return r1;
					} if ((layer_id == VENDOR_AI_MAXLAYER) && (out_id ==0) && (p_proc->rsltbuf.size == 0)) {
						rv = HD_ERR_PATH;
						if (rv != HD_OK) goto exit;
					}else
#endif
					{
						rv = _vendor_ai_net_get_iomem(proc_id, layer_id, GET_PORT_TYPE_OUT, out_id, p_user);
						if (rv != HD_OK) goto exit;
					}
					
				}
				break;

				default:
				{
					DBG_ERR("param_id(%u) not support !!\r\n", param_id);
					return HD_ERR_PARAM;
				}
				break;
			}
			break;
	}
exit:
	return rv;
}

HD_RESULT _vendor_ai_net_open_normal (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK, rv2 = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	VENDOR_AI_NET_CFG_MODEL *p_cfg_share_model = NULL;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = NULL;
	VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	UINT32 model_size = 0;

	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_1st = 0;
	UINT64 ts_open = 0;
	UINT64 ts_initbuf = 0, ts_groupjob = 0, ts_iomem = 0;
    VENDOR_AI_NET_CFG_MODEL *p_diff_model = NULL;
    VENDOR_AI_NET_CFG_MODEL *p_share_diff_model = NULL;
	
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - begin\r\n");

	//--- check input parameter valid(those who can only be checked on OPEN) ---
	{
		CHAR msg_string[256];
		if (_vendor_ai_net_check_params_range_on_open(proc_id, msg_string, 256) < 0) {
			DBG_ERR("Wrong value. %s\n", msg_string);
			return HD_ERR_PARAM;
		}
	}

	vendor_ais_lock_net(proc_id);
#if (NET_OPEN_HEAP == 1)
	rv = _vendor_ai_alloc_proc(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOMEM;
	}
#endif

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

#if CNN_AI_FASTBOOT
	if (vendor_ais_is_fastboot()) {
		// get model from fastboot
		rv = _vendor_ai_net_cfg_fastboot_model(p_cfg);
		if (rv != HD_OK) {
#if (NET_OPEN_HEAP == 1)
			_vendor_ai_free_proc(proc_id);
#endif
			DBG_ERR("config fastboot_model fail(%d)\r\n", rv);
			return HD_ERR_NG;
		}
	}
#endif

	// check if MODEL is set by user
	p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
	if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL  for model first !!\r\n", proc_id);
		return HD_ERR_NO_CONFIG;
	}
	p_cfg_share_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_share_model;
	if ((p_cfg_share_model->pa == 0) || (p_cfg_share_model->va == 0) || (p_cfg_share_model->size == 0)) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL  for model first !!\r\n", proc_id); // [1] CFG_SHAREMODEL must set after CFG_MODEL is set [2] CFG_SHAREMODEL already check valid while set...so if pa/va/size still=0, must CFG_MODEL is not set yet !
		return HD_ERR_NO_CONFIG;
	}

	// check chip
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - check chip - begin\r\n");
	if (HD_OK != vendor_ais_net_gen_chk_vers((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model, proc_id)) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) input model is NOT supported by this CHIP !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	}
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - check chip - end\r\n");

	// check if MODEL is fresh loaded (offset-type addr)  // [ IVOT_N12047_CO-340 ] : user may stop->close->set_model->open without reload model file to memory. The addr will NOT be offset-type, but is absolute-type which will cause Segmentation Fault later !!
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - check model fresh loaded - begin\r\n");
	if (HD_OK != _vendor_ai_net_gen_chk_model_is_fresh_loaded((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model)) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) input model check fresh-loaded fail !!\r\n        The content of input model set by VENDOR_AI_NET_PARAM_CFG_MODEL is NOT fresh load from file. Please reload model file first !!\r\n        Possibly reason that cause this error => user call [stop->close->set model->open] just use previous model address without reload model file !!\r\n", proc_id);
		return HD_ERR_NO_CONFIG;
	}
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - check model fresh loaded - end\r\n");

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_1st = ts = hd_gettime_us();
	}

	// check simplify bin (id_list_size = 0 is simplify bin, with NO graph information)
	if (0 == vendor_ais_net_gen_get_id_list_size((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model)) {
		if (p_cfg->buf_opt.method != VENDOR_AI_NET_BUF_OPT_NONE) {
			DBG_WRN("Running simplify model bin ... buf_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_BUF_OPT_NONE !!\r\n", (int)p_cfg->buf_opt.method);
			p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_NONE;
		}
		if (p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR_O1) {
			DBG_WRN("Running simplify model bin ... job_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_JOB_OPT_LINEAR_O1 !!\r\n", (int)p_cfg->job_opt.method);
			p_cfg->job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
		}
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_GRP|AI_JOB|AI_BUF, "open() - <<<<<<<<<<<<<< !!! RUNNING SIMPLIFY MODEL BIN !!! >>>>>>>>>>>>>>\r\n");
		p_proc->priv.b_simplify_bin = TRUE;
	} else {
		VENDOR_AI_NET_JOB_OPT ext_job_opt=1;
		VENDOR_AI_NET_BUF_OPT ext_buf_opt=1;
		// check if model with extinfo 'PDEF' to suggest job_opt/buf_opt
		if (HD_OK == _vendor_ais_get_external_sdk_param((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model, &ext_job_opt, &ext_buf_opt)) {
			// job_opt
			if ((int)ext_job_opt != (-100)) {
				vendor_ai_net_trace(proc_id, AI_FLOW, "open() - extinfo update job_opt => update job_opt to model set = %d\r\n", (int)ext_job_opt);
				p_cfg->job_opt.method = ext_job_opt;
			} else {
				vendor_ai_net_trace(proc_id, AI_FLOW, "open() - extinfo update job_opt => model job_opt = (-100), ignore update...\r\n");
			}
			// buf_opt
			if ((int)ext_buf_opt != (-100)) {
				vendor_ai_net_trace(proc_id, AI_FLOW, "open() - extinfo update buf_opt => update buf_opt to model set = %d\r\n", (int)ext_buf_opt);
				p_cfg->buf_opt.method = ext_buf_opt;
			} else {
				vendor_ai_net_trace(proc_id, AI_FLOW, "open() - extinfo update buf_opt => model buf_opt = (-100), ignore update...\r\n");
			}
		} else {
			vendor_ai_net_trace(proc_id, AI_FLOW, "open() - extinfo for job_opt/buf_opt is not found in this model, ignore update...\r\n");
		}
		p_proc->priv.b_simplify_bin = FALSE;
	}

#if (DBG_OUT_DUMP == 1)
	p_cfg->job_opt.method = 0;
	p_cfg->job_opt.wait_ms = -1;
	printf("open() - >>> FORCE (job_opt.method=0, wait_ms=-1) for dump out!\r\n");
	p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_FULL;
	printf("open() - >>> FORCE (buf_opt.method = full) for dump out!\n");
#endif
#if (DBG_REG_DUMP == 1)
	p_cfg->job_opt.method = 0;
	p_cfg->job_opt.wait_ms = -1;
	printf("open() - >>> FORCE (job_opt.method=0, wait_ms=-1) for dump reg!\r\n");
#endif
#if (DBG_TIME_DUMP == 1)
	p_cfg->job_opt.method = 0;
	p_cfg->job_opt.wait_ms = -1;
	printf("open() - >>> FORCE (job_opt.method=0, wait_ms=-1) for dump time!\r\n");
#endif
	// parse memory layout from model bin file => we actually only need user_parm infomation
	p_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	model_size = vendor_ais_auto_alloc_mem(p_mem, &mem_manager_tmp);  // we only need user_parm actually
	if (model_size == 0) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) auto alloc mem fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

    if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP){
        // 1 = original model_bin value
        vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_IOMEM,  &mem_manager_tmp, "/mnt/sd/ori_iomem.txt");
        vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL,  &mem_manager_tmp, "/mnt/sd/ori_mctrl.txt");
        vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_AIPARM, &mem_manager_tmp, "/mnt/sd/ori_aiparm.txt");
        vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_LLCMD,  &mem_manager_tmp, "/mnt/sd/ori_llcmd.txt");
    }

    //check if model is combine bin, then split it (nvt_model.bin + stripe_model.bin)
    rv = _vendor_ais_split_combine_model_bin(proc_id, &mem_manager_tmp);
    if (rv != HD_OK) {
		DBG_ERR("_vendor_ais_split_combine_model_bin fail...\r\n");
		return rv;
	}

    if(_vendor_ai_common_info()->share_model_mode == 2) {
        p_share_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.share_diff_model;
        if ((p_share_diff_model->pa != 0) && (p_share_diff_model->va != 0) && (p_share_diff_model->size != 0)) {
            if (p_cfg->buf_opt.method != VENDOR_AI_NET_BUF_OPT_NONE) {
                DBG_WRN("Running multi-scale model bin ... buf_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_BUF_OPT_NONE !!\r\n", (int)p_cfg->buf_opt.method);
                vendor_ai_net_trace(proc_id, AI_FLOW, "open() - Running multi-scale model bin => update buf_opt to VENDOR_AI_NET_BUF_OPT_NONE\r\n");
                p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_NONE;
            }
            if (p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR_O1 && p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR) {
                DBG_WRN("Running multi-scale model bin ... job_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_JOB_OPT_LINEAR_O1 !!\r\n", (int)p_cfg->job_opt.method);
                vendor_ai_net_trace(proc_id, AI_FLOW, "open() - Running multi-scale model bin => update job_opt to VENDOR_AI_NET_JOB_OPT_LINEAR_O1\r\n");
                p_cfg->job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
            }
            vendor_ai_net_trace(proc_id, AI_FLOW|AI_GRP|AI_JOB|AI_BUF, "open() - <<<<<<<<<<<<<< !!! RUNNING MULTI-SCALE MODEL BIN !!! >>>>>>>>>>>>>>\r\n");
        }
    } else {
        p_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.diff_model;
        if ((p_diff_model->pa != 0) && (p_diff_model->va != 0) && (p_diff_model->size != 0)) {
            if (p_cfg->buf_opt.method != VENDOR_AI_NET_BUF_OPT_NONE) {
                DBG_WRN("Running multi-scale model bin ... buf_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_BUF_OPT_NONE !!\r\n", (int)p_cfg->buf_opt.method);
                vendor_ai_net_trace(proc_id, AI_FLOW, "open() - Running multi-scale model bin => update buf_opt to VENDOR_AI_NET_BUF_OPT_NONE\r\n");
                p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_NONE;
            }
            if (p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR_O1 && p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR) {
                DBG_WRN("Running multi-scale model bin ... job_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_JOB_OPT_LINEAR_O1 !!\r\n", (int)p_cfg->job_opt.method);
                vendor_ai_net_trace(proc_id, AI_FLOW, "open() - Running multi-scale model bin => update job_opt to VENDOR_AI_NET_JOB_OPT_LINEAR_O1\r\n");
                p_cfg->job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
            }
            vendor_ai_net_trace(proc_id, AI_FLOW|AI_GRP|AI_JOB|AI_BUF, "open() - <<<<<<<<<<<<<< !!! RUNNING MULTI-SCALE MODEL BIN !!! >>>>>>>>>>>>>>\r\n");
        }
    }

    if (vendor_ais_check_diff_batch_mem(&mem_manager_tmp)) {
		if (p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR_O1 && p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR) {
			DBG_WRN("Running dynamic-batch model bin ... job_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_JOB_OPT_LINEAR_O1 !!\r\n", (int)p_cfg->job_opt.method);
			vendor_ai_net_trace(proc_id, AI_FLOW, "open() - Running dynamic-batch model bin => update job_opt to VENDOR_AI_NET_JOB_OPT_LINEAR_O1\r\n");
            p_cfg->job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
		}
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_GRP|AI_JOB|AI_BUF, "open() - <<<<<<<<<<<<<< !!! RUNNING DYNAMIC-BATCH MODEL BIN !!! >>>>>>>>>>>>>>\r\n");
    }
    
	if (p_proc->priv.init_buf_alloc == 0) {

		UINT32 req_size = _vendor_ai_net_init_buf_calc_size(proc_id, &mem_manager_tmp);
		vendor_ai_net_trace(proc_id, AI_BUF, "open() - init buf alloc - begin, req_size = %lu\r\n", req_size);
		if (p_cfg->intlbuf.pa && p_cfg->intlbuf.va && (p_cfg->intlbuf.size >= req_size)) { // user already set intlbuf
			// whole init_buf
			p_proc->priv.init_buf.pa   = p_cfg->intlbuf.pa;
			p_proc->priv.init_buf.va   = p_cfg->intlbuf.va;
			p_proc->priv.init_buf.size = p_cfg->intlbuf.size;

		} else { // need to alloc intlbuf by hdal
			// alloc  init buffer for (user_parm + vendor_ai_net_group) for given proc_id
			rv = _vendor_ai_net_init_buf_alloc(proc_id, &mem_manager_tmp, req_size);
			if (rv != HD_OK) {
		#if (NET_OPEN_HEAP == 1)
				_vendor_ai_free_proc(proc_id);
		#endif
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "open() - init buf alloc - user init buf, pa = %#lx, va = %#lx, size = %lu\r\n",
							p_proc->priv.init_buf.pa, p_proc->priv.init_buf.va, p_proc->priv.init_buf.size);

		p_proc->priv.init_buf_alloc = 1;
	}

	// assign init buffer memory manager for this proc_id
	_vendor_ai_net_init_buf_assign_range(proc_id, &p_proc->priv.init_buf, &mem_manager_tmp, &p_proc->mem_manager); // [1] this will copy all content for user_parm ( bin file -> new alloc user_parm )  [2] (if VENDOR_AI_CFG_SHAREMODEL_MODE == 0) using original (user_parm in bin_file) as kerl_parm !!

	vendor_ai_net_trace(proc_id, AI_BUF, "open() - init buf alloc - end\r\n");
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_initbuf = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "open() - >>> initbuf time = %llu\r\n", ts_initbuf);
	}
#if NET_TABLE_HEAP
	//alloc g_proc_idx_map_table
	vendor_ais_net_alloc_map_table(proc_id, &mem_manager_tmp);
#endif
	
#if CNN_528_PSW
{
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		vendor_ai_net_trace(proc_id, AI_JOB|AI_BUF, "proc_id(%lu) open with job-opt=%d (wait_ms = %ld), buf-opt=%d (ddr_id=%d)!\r\n", proc_id, (int)p_cfg->job_opt.method, p_cfg->job_opt.wait_ms, (int)p_cfg->buf_opt.method, (int)p_cfg->buf_opt.ddr_id);
		ts = ts_new;
	}
	vendor_ai_net_trace(proc_id, AI_GRP, "open() - group_job - begin\r\n");

	// set group memory & proc
	if (p_proc->priv.b_simplify_bin == FALSE) {
		rv = vendor_ai_net_group_setbuffer(proc_id, p_proc->priv.group_buf.va, p_proc->mem_manager.user_parm.va);
		if (rv != HD_OK) {
			goto open_init_mem_exit;
		}
		rv = vendor_ai_net_group_proc(proc_id, p_cfg->job_opt.method);
		if (rv != HD_OK) {
			goto open_init_mem_exit;
		}

		if (TRUE == _vendor_ai_net_is_optimize_job(p_cfg->job_opt.method)) {
			rv = vendor_ai_net_group_llcmd_fix(proc_id); // connect llcmd depends on graph result
			if (rv != HD_OK) {
				goto open_init_mem_exit;
			}
		}
	} else {
		vendor_ai_net_trace(proc_id, AI_GRP, "open() - group_job - SKIP GROUP_PROC() ... because this is SIMPLIFY MODEL BIN\r\n");
	}
	vendor_ai_net_trace(proc_id, AI_GRP, "open() - group_job - end\r\n");
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_groupjob = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_GRP|AI_PERF, "open() - >>> groupjob time = %llu\r\n", ts_groupjob);

		ts = ts_new;
	}
	vendor_ai_net_trace(proc_id, AI_BUF, "open() - iomem - begin\r\n");

#if NN_DLI
	rv = _vendor_ai_net_nn_dli_parse_ext_info(proc_id);
	if (rv != HD_OK) {
		goto open_init_mem_exit;
	}
#endif

	if (p_cfg->buf_opt.method == VENDOR_AI_NET_BUF_OPT_NONE) {
		// DO NOT alloc memory => just use IO_MEM / AI_PARM come from vsgen(already on model bin)
		p_proc->mem_manager.user_buff.size = mem_manager_tmp.user_buff.size;
#if CNN_AI_FASTBOOT
		if (vendor_ais_is_fastboot()) {
			// fix newly user_parm
			rv = _vendor_ai_net_fix_fastboot_user_parm(&p_proc->mem_manager);
			if (rv != HD_OK) {
				goto open_init_mem_exit;
			}
		}
#endif
		vendor_ai_net_trace(proc_id, AI_BUF, "proc_id(%lu) user_buffer size = %lu (using buffer space already in model bin) \r\n", proc_id, p_proc->mem_manager.user_buff.size);
		// old tool may generate model with iobuf != 32/64x align addr/size, have to check here !!
		rv = _vendor_ai_net_check_tool_iobuf_cache_align(proc_id, &p_proc->mem_manager);
		if (rv != HD_OK) {
			goto open_init_mem_exit;
		}
	} else {
		// alloc memory => modify IO_MEM / AI_PARM
		rv = _vendor_ai_net_alloc_memory(proc_id, &p_proc->mem_manager, p_cfg->buf_opt.method, p_cfg->job_opt.method);  // this will update p_proc->mem_manager.user_buff.size & p_head->io_buff_size
		if (rv != HD_OK) {
			goto open_init_mem_exit;
		}
	}

	p_proc->iobuf.size = _vendor_ai_net_io_buf_calc_size(&p_proc->mem_manager);
	p_proc->iobuf.pa = p_proc->iobuf.va = 0;
	p_proc->rsltbuf.size = 0;
	p_proc->rsltbuf.pa = p_proc->rsltbuf.va = 0;
	vendor_ai_net_trace(proc_id, AI_BUF, "open() - iomem - end, size = %lu\r\n", p_proc->iobuf.size);
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_iomem = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "open() - >>> iomem time = %llu\r\n", ts_iomem);
	}
}
#endif

	// parse memory layout from shared_model bin file => we actually only need user_model  information
	p_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_share_model;
	model_size = vendor_ais_auto_alloc_mem(p_mem, &mem_manager_tmp);  // we only need user_model actually
	if (model_size == 0) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) auto alloc mem fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	p_proc->mem_manager.user_model.pa    = mem_manager_tmp.user_model.pa;
	p_proc->mem_manager.user_model.va    = mem_manager_tmp.user_model.va;
	p_proc->mem_manager.user_model.size  = mem_manager_tmp.user_model.size;

#if CNN_FMT_V4
	// config external info
#if CNN_AI_FASTBOOT
	if (vendor_ais_is_fastboot() == DISABLE) {
#endif
		_vendor_ais_config_external_info(proc_id);
#if CNN_AI_FASTBOOT
	}
#endif
#endif

	// set open
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN;   // [INIT] -> [OPEN]

	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "open() - end\r\n");
    /////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_open = ts_new-ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "open() - open time = %llu\r\n", ts_open);
	}
	
	return HD_OK;

/*
open_work_mem_exit:
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)
	if (p_proc->priv.io_buf_alloc == 1) {
		if ((p_proc->workbuf.pa == 0) && (p_proc->workbuf.va == 0)) {
			rv = hd_common_mem_free((UINT32)p_proc->priv.work_buf.pa, (void *)p_proc->priv.work_buf.va);
			if (rv != HD_OK) {
				DBG_ERR("proc_id(%lu) free work memory fail, ret = %d\n", proc_id, rv);
			}
		}
		p_proc->priv.io_buf_alloc = 0;
	}
#endif
*/

open_init_mem_exit:
	rv2 = HD_OK;
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)
	rv2 = hd_common_mem_free((UINT32)p_proc->priv.init_buf.pa, (void *)p_proc->priv.init_buf.va);
	if (rv2 != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv2);
	}
#else
	#if USE_GET_BLK
	rv2 = _vendor_ai_net_mem_free(p_proc->priv.init_buf.pa, (void *)p_proc->priv.init_buf.va,
							p_proc->priv.init_buf.size, (HD_COMMON_MEM_VB_BLK)p_proc->priv.init_buf.blk);
	if (rv2 != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv2);
	}
	#endif
#endif
	if (rv == HD_OK && rv2 != HD_OK) rv = rv2;
	
#if (NET_OPEN_HEAP == 1)
	_vendor_ai_free_proc(proc_id);
#endif
	vendor_ais_unlock_net(proc_id);

	return rv;
}

#if CNN_AI_FASTBOOT
HD_RESULT _vendor_ai_net_open_fastboot (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = NULL;
	VENDOR_AIS_FLOW_MEM_PARM pd_buff = {0};
	VENDOR_AIS_FLOW_MAP_MEM_PARM mem_manager_tmp = {0};
	UINT32 model_size = 0;

	vendor_ai_net_trace(proc_id, AI_FLOW, "open_fastboot() - begin\r\n");

	//--- check input parameter valid(those who can only be checked on OPEN) ---
	{
		CHAR msg_string[256];
		if (_vendor_ai_net_check_params_range_on_open(proc_id, msg_string, 256) < 0) {
			DBG_ERR("Wrong value. %s\n", msg_string);
			return HD_ERR_PARAM;
		}
	}

	vendor_ais_lock_net(proc_id);
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	// check if MODEL is set by user
	p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
	if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL  for model first !!\r\n", proc_id);
		return HD_ERR_NO_CONFIG;
	}

	// check chip
	vendor_ai_net_trace(proc_id, AI_FLOW, "open_fastboot() - check chip - begin\r\n");
	if (HD_OK != vendor_ais_net_gen_chk_vers((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model, proc_id)) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("ERROR : proc_id(%lu) input model is NOT supported by this CHIP !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	}
	vendor_ai_net_trace(proc_id, AI_FLOW, "open_fastboot() - check chip - end\r\n");

	// check simplify bin (id_list_size = 0 is simplify bin, with NO graph information)
	if (0 == vendor_ais_net_gen_get_id_list_size((VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model)) {
		if (p_cfg->buf_opt.method != VENDOR_AI_NET_BUF_OPT_NONE) {
			DBG_WRN("Running simplify model bin ... buf_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_BUF_OPT_NONE !!\r\n", (int)p_cfg->buf_opt.method);
			p_cfg->buf_opt.method = VENDOR_AI_NET_BUF_OPT_NONE;
		}
		if (p_cfg->job_opt.method != VENDOR_AI_NET_JOB_OPT_LINEAR_O1) {
			DBG_WRN("Running simplify model bin ... job_opt.method(%d) is invalid => auto set to VENDOR_AI_NET_JOB_OPT_LINEAR_O1 !!\r\n", (int)p_cfg->job_opt.method);
			p_cfg->job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
		}
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_GRP|AI_JOB|AI_BUF, "open_fastboot() - <<<<<<<<<<<<<< !!! RUNNING SIMPLIFY MODEL BIN !!! >>>>>>>>>>>>>>\r\n");
		p_proc->priv.b_simplify_bin = TRUE;
	} else {
		p_proc->priv.b_simplify_bin = FALSE;
	}

	// parse memory layout from model bin file
	p_mem = (VENDOR_AIS_FLOW_MEM_PARM *)p_cfg_model;
	model_size = vendor_ais_auto_alloc_mem(p_mem, &mem_manager_tmp);
	if (model_size == 0) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) auto alloc mem fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	// tmp -> p_proc
	p_proc->mem_manager.user_parm.pa    = mem_manager_tmp.user_parm.pa;
	p_proc->mem_manager.user_parm.va    = mem_manager_tmp.user_parm.va;
	p_proc->mem_manager.user_parm.size  = mem_manager_tmp.user_parm.size;

	p_proc->mem_manager.user_model.pa   = mem_manager_tmp.user_model.pa;
	p_proc->mem_manager.user_model.va   = mem_manager_tmp.user_model.va;
	p_proc->mem_manager.user_model.size = mem_manager_tmp.user_model.size;

	p_proc->mem_manager.user_buff.pa    = mem_manager_tmp.user_buff.pa;
	p_proc->mem_manager.user_buff.va    = mem_manager_tmp.user_buff.va;
	p_proc->mem_manager.user_buff.size  = mem_manager_tmp.user_buff.size;

	p_proc->mem_manager.kerl_parm.pa    = mem_manager_tmp.kerl_parm.pa;
	p_proc->mem_manager.kerl_parm.va    = mem_manager_tmp.kerl_parm.va;
	p_proc->mem_manager.kerl_parm.size  = mem_manager_tmp.kerl_parm.size;
	printf("==========[original mem]==========\n");
	printf("user_parm: pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_parm.pa, p_proc->mem_manager.user_parm.va, p_proc->mem_manager.user_parm.size);
	printf("model:     pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_model.pa, p_proc->mem_manager.user_model.va, p_proc->mem_manager.user_model.size);
	printf("io_buff:   pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_buff.pa, p_proc->mem_manager.user_buff.va, p_proc->mem_manager.user_buff.size);
	printf("kerl_parm: pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.kerl_parm.pa, p_proc->mem_manager.kerl_parm.va, p_proc->mem_manager.kerl_parm.size);

	// copy user_parm to kerl_parm
	printf("copy user_parm(0x%lx) to kerl_parm(0x%lx), size(0x%lx)\n",
		p_proc->mem_manager.user_parm.va, p_proc->mem_manager.kerl_parm.va, p_proc->mem_manager.user_parm.size);
	if (p_proc->mem_manager.kerl_parm.va && p_proc->mem_manager.user_parm.va && p_proc->mem_manager.user_parm.size) {
		memcpy((VOID *)p_proc->mem_manager.kerl_parm.va, (VOID *)p_proc->mem_manager.user_parm.va, p_proc->mem_manager.user_parm.size);
	} else {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("Invalid input !\n");
		return HD_ERR_NG;
	}

	VENDOR_AIS_FLOW_MEM_PARM tmp_mem = {0};

	// switch user_parm and kerl_parm
	printf("before swapping user_parm(0x%lx/0x%lx/0x%lx) and kerl_parm(0x%lx/0x%lx/0x%lx)\n",
		p_proc->mem_manager.user_parm.pa, p_proc->mem_manager.user_parm.va, p_proc->mem_manager.user_parm.size,
		p_proc->mem_manager.kerl_parm.pa, p_proc->mem_manager.kerl_parm.va, p_proc->mem_manager.kerl_parm.size);

	// swap user_parm and kerl_parm
	tmp_mem.pa                           = p_proc->mem_manager.user_parm.pa;
	tmp_mem.va                           = p_proc->mem_manager.user_parm.va;
	tmp_mem.size                         = p_proc->mem_manager.user_parm.size;
	p_proc->mem_manager.user_parm.pa     = p_proc->mem_manager.kerl_parm.pa;
	p_proc->mem_manager.user_parm.va     = p_proc->mem_manager.kerl_parm.va;
	p_proc->mem_manager.user_parm.size   = p_proc->mem_manager.kerl_parm.size;
	p_proc->mem_manager.kerl_parm.pa     = tmp_mem.pa;
	p_proc->mem_manager.kerl_parm.va     = tmp_mem.va;
	p_proc->mem_manager.kerl_parm.size   = tmp_mem.size;

	// alloc pd_buff
	pd_buff.pa                           = ALIGN_CEIL_32(p_proc->mem_manager.user_model.pa + p_proc->mem_manager.user_model.size);
	pd_buff.va                           = ALIGN_CEIL_32(p_proc->mem_manager.user_model.va + p_proc->mem_manager.user_model.size);
	pd_buff.size                         = g_pd_bufsize;

	// alloc io_buff
	p_proc->mem_manager.user_buff.pa     = ALIGN_CEIL_32(pd_buff.pa + pd_buff.size);
	p_proc->mem_manager.user_buff.va     = ALIGN_CEIL_32(pd_buff.va + pd_buff.size);
	p_proc->mem_manager.user_buff.size   = p_proc->mem_manager.user_buff.size - pd_buff.size;

	printf("==========[fastboot mem]==========\n");
	printf("kerl_parm: pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.kerl_parm.pa, p_proc->mem_manager.kerl_parm.va, p_proc->mem_manager.kerl_parm.size);
	printf("model:     pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_model.pa, p_proc->mem_manager.user_model.va, p_proc->mem_manager.user_model.size);
	printf("pd_buff:   pa=0x%lx, va=0x%lx, size=0x%lx\n", pd_buff.pa, pd_buff.va, pd_buff.size);
	printf("io_buff:   pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_buff.pa, p_proc->mem_manager.user_buff.va, p_proc->mem_manager.user_buff.size);
	printf("user_parm: pa=0x%lx, va=0x%lx, size=0x%lx\n", p_proc->mem_manager.user_parm.pa, p_proc->mem_manager.user_parm.va, p_proc->mem_manager.user_parm.size);

	// config pd plugin
	{
		VENDOR_AI_ENGINE_PLUGIN *p_user = (VENDOR_AI_ENGINE_PLUGIN *)&pd_cfg_engine;
		VENDOR_AI_ENGINE_PLUGIN *p_cfg_engine = (VENDOR_AI_ENGINE_PLUGIN *)&p_cfg->user_postproc;

		if (p_user == 0) {
			DBG_ERR("cfg engine => clear!\r\n");
			p_cfg_engine->sign = 0;
			return HD_OK;
		}

		if (p_user->sign == 0) {
			DBG_ERR("cfg engine => invalid sign!\r\n");
			return HD_ERR_SIGN;
		}
		memcpy(p_cfg_engine, p_user, sizeof(VENDOR_AI_ENGINE_PLUGIN));
	}

	// set open
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN;	// [INIT] -> [OPEN]

	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "open_fastboot() - end\r\n");

	return rv;
}
#endif

HD_RESULT vendor_ai_net_open (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	// check status
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init yet, please call vendor_ai_net_init() first !!!\r\n");
		return HD_ERR_UNINIT;
	} else if ((_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN) ||
				(_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START)){
		DBG_ERR("module is already opened...ignore request !!\r\n");
		return HD_ERR_ALREADY_OPEN;
	}

	rv = _vendor_ai_net_open_normal(proc_id);

	return rv;
}

HD_RESULT _vendor_ai_net_close_normal (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;

	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_1st = 0;
	UINT64 ts_close = 0;
	UINT64 ts_initbuf = 0, ts_workbuf = 0, ts_memmap = 0;

	vendor_ai_net_trace(proc_id, AI_FLOW, "close() - begin\r\n");

	vendor_ais_lock_net(proc_id);
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);
	if (p_proc->gen_init & AI_KERL_MEM_INIT){
		vendor_ai_net_trace(proc_id, AI_FLOW, "close() - uninit_job_graph\r\n");
		vendor_ai_uninit_job_graph(proc_id);

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_1st = ts = hd_gettime_us();
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "close() - mem unmap - begin\r\n");
		rv = vendor_ais_net_gen_uninit(proc_id);
		if (rv != HD_OK) {
			vendor_ais_unlock_net(proc_id);
			DBG_ERR("proc_id(%lu) net gen uninit fail=%d\r\n", proc_id, rv);
			return rv;
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "close() - mem unmap - end\r\n");
		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_memmap = ts_new-ts;
			vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "close() - >>> memmap time = %llu\r\n", ts_memmap);
			
			ts = ts_new;
		}
	}
#if NET_TABLE_HEAP
	//free g_proc_idx_map_table
	vendor_ais_net_free_map_table(proc_id);
#endif
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - init buf free - begin\r\n");
	// free private buffer for given proc_id
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)

	if (p_proc->priv.init_buf_alloc == 1) {
		if ((p_cfg->intlbuf.pa == 0) && (p_cfg->intlbuf.va == 0)) { // means user not set initbuf
			rv = hd_common_mem_free(p_proc->priv.init_buf.pa, (void*)p_proc->priv.init_buf.va);
			if (rv != HD_OK) {
				vendor_ais_unlock_net(proc_id);
				DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv);
				return rv;
			}
		}
		p_proc->priv.init_buf_alloc = 0;
	}
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - init buf free - end\r\n");

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_initbuf = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "close() - >>> initbuf time = %llu\r\n", ts_initbuf);
		
	    ts = ts_new;
	}
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - work buf free - begin\r\n");
	if (p_proc->priv.io_buf_alloc == 1) {
		if ((p_proc->workbuf.pa == 0) && (p_proc->workbuf.va == 0)) { // means user not set workbuf
			rv = hd_common_mem_free((UINT32)p_proc->priv.work_buf.pa, (void *)p_proc->priv.work_buf.va); // need to free buf by hdal
			if (rv != HD_OK) {
				DBG_ERR("proc_id(%lu) free work memory fail, ret = %d\n", proc_id, rv);
			}
		}
		p_proc->priv.io_buf_alloc = 0;
	}
#else
	#if USE_GET_BLK
	rv = _vendor_ai_net_mem_free(p_proc->priv.init_buf.pa, (void *)p_proc->priv.init_buf.va,
								 p_proc->priv.init_buf.size, (HD_COMMON_MEM_VB_BLK)p_proc->priv.init_buf.blk);
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv);
	}
	#endif

	if (p_proc->priv.init_buf_alloc == 1) {
		if ((p_cfg->intlbuf.pa == 0) && (p_cfg->intlbuf.va == 0)) { // means user not set initbuf
			pa = _vendor_ai_get_init_buf_pa(proc_id);
			if ((pa == 0) && p_proc->priv.init_buf.pa) {
				rv = _vendor_ai_net_mem_free(p_proc->priv.init_buf.pa, (void*)p_proc->priv.init_buf.va, p_proc->priv.init_buf.size, (HD_COMMON_MEM_VB_BLK)p_proc->priv.init_buf.blk);
				if (rv != HD_OK) {
					vendor_ais_unlock_net(proc_id);
					DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv);
					return rv;
				}
			} else {
				rv = vendor_pcie_mem_munmap((void*)p_proc->priv.init_buf.va, p_proc->priv.init_buf.size);
				if (rv != HD_OK) {
					vendor_ais_unlock_net(proc_id);
					DBG_ERR("proc_id(%lu) munmap init memory fail, ret = %d\n", proc_id, rv);
					return rv;
				}
			}
		}
		p_proc->priv.init_buf_alloc = 0;
	}
	
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - init buf free - end\r\n");
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_initbuf = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "close() - >>> initbuf time = %llu\r\n", ts_initbuf);
		
	    ts = ts_new;
	}
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - work buf free - begin\r\n");

	#if USE_GET_BLK
	if (p_proc->priv.io_buf_alloc == 1) {
		if ((p_proc->workbuf.pa == 0) && (p_proc->workbuf.va == 0)) { // means user not set workbuf
			rv = _vendor_ai_net_mem_free(p_proc->priv.work_buf.pa, (void *)p_proc->priv.work_buf.va,
										 p_proc->priv.work_buf.size, (HD_COMMON_MEM_VB_BLK)p_proc->priv.work_buf.blk); // need to free buf by hdal
			if (rv != HD_OK) {
				DBG_ERR("proc_id(%lu) free work memory fail, ret = %d\n", proc_id, rv);
			}
		}
		p_proc->priv.io_buf_alloc = 0;
	}
	#endif
#endif
	vendor_ai_net_trace(proc_id, AI_BUF, "close() - work buf free - end\r\n");
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_workbuf = ts_new-ts;
		vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "close() - >>> workbuf time = %llu\r\n", ts_workbuf);
		
	    //ts = ts_new;
	}
	
	// reset all variable for this proc_id
	{
		INT b_is_used_backup = 0;

		vendor_ai_comm_lock();
		b_is_used_backup = p_cfg->b_is_used; // backup  b_is_used
		memset(p_cfg, 0, sizeof(VENDOR_AI_NET_CFG_PROC));
		memset(p_proc, 0, sizeof(VENDOR_AI_NET_INFO_PROC));
		p_cfg->b_is_used = b_is_used_backup; // restore b_is_used
		vendor_ai_comm_unlock();

		_vendor_ai_net_fill_all_default_params(proc_id);
	}

	// set close
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_INIT;  // [OPEN] -> [INIT]

#if (NET_OPEN_HEAP == 1)
	_vendor_ai_free_proc(proc_id);
#endif
	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "close() - end\r\n");
    /////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_close = ts_new-ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "close() - close time = %llu\r\n", ts_close);
	}	
	
	return rv;
}

HD_RESULT _vendor_ai_net_close_fastboot (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;

	vendor_ai_net_trace(proc_id, AI_FLOW, "close_fastboot() - begin\r\n");

	vendor_ais_lock_net(proc_id);
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	vendor_ai_net_trace(proc_id, AI_BUF, "close_fastboot() - mem unmap - begin\r\n");
	rv = vendor_ais_net_gen_uninit(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		DBG_ERR("proc_id(%lu) net gen uninit fail=%d\r\n", proc_id, rv);
		return rv;
	}
	vendor_ai_net_trace(proc_id, AI_BUF, "close_fastboot() - mem unmap - end\r\n");

	// reset all variable for this proc_id
	{
		INT b_is_used_backup = 0;

		vendor_ai_comm_lock();
		b_is_used_backup = p_cfg->b_is_used; // backup  b_is_used
		memset(p_cfg, 0, sizeof(VENDOR_AI_NET_CFG_PROC));
		memset(p_proc, 0, sizeof(VENDOR_AI_NET_INFO_PROC));
		p_cfg->b_is_used = b_is_used_backup; // restore b_is_used
		vendor_ai_comm_unlock();

		_vendor_ai_net_fill_all_default_params(proc_id);
	}

	// set close
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_INIT;  // [OPEN] -> [INIT]

	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "close_fastboot() - end\r\n");

	return rv;
}

HD_RESULT vendor_ai_net_close (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START) {
		//DBG_WRN("module is START now, auto call vendor_ai_net_stop() before attemp to close.... !! \r\n");
		//if (HD_OK !=vendor_ai_net_stop(proc_id)) {
		//vendor_ais_unlock_net(proc_id);
		//	DBG_ERR("auto call vendor_ai_net_stop() fail ...!!\r\n");
		//	return HD_ERR_NG;
		//}
	    DBG_ERR("module is NOT stop yet... ignore close request !! \r\n");
		return HD_ERR_ALREADY_START;
	} else if ((_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) ||
	           (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT)) {
	    DBG_ERR("module is NOT open yet... ignore close request !! \r\n");
		return HD_ERR_NOT_OPEN;
	}

	rv = _vendor_ai_net_close_normal(proc_id);

	return rv;
}

#if (NEW_AI_FLOW == 1)
/*
static HD_RESULT _vendor_ai_net_JOB_done(UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

//NET info
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
	model_mem.va    = p_proc->mem_manager.user_parm.va;
	model_mem.size  = p_cfg->cfg_model.size;

	return vendor_ais_net_builtin_JOB_done(proc_id, job_id, model_mem);
}
*/
static HD_RESULT _vendor_ai_net_CPU_exec(UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};
	
//NET info
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
	model_mem.va    = p_proc->mem_manager.user_parm.va;
	model_mem.size  = p_cfg->cfg_model.size;

#if NN_DLI
	if ((job_id & 0xff000000) == 0xff000000) {	//start
		job_id &= ~0xff000000;
		return vendor_ais_net_builtin_CPU_init(proc_id, job_id, model_mem);
	}
	if ((job_id & 0xfe000000) == 0xfe000000) {	//stop
		job_id &= ~0xff000000;
		return vendor_ais_net_builtin_CPU_exit(proc_id, job_id, model_mem);
	}
#endif

	return vendor_ais_net_builtin_CPU_exec(proc_id, job_id, model_mem);
}

static HD_RESULT _vendor_ai_net_DSP_exec(UINT32 proc_id, UINT32 job_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

//NET info
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
	model_mem.va    = p_proc->mem_manager.user_parm.va;
	model_mem.size  = p_cfg->cfg_model.size;

	return vendor_ais_net_builtin_DSP_exec(proc_id, job_id, model_mem);
}
#endif

/* duplication of vendor_ai_net_start() */
HD_RESULT vendor_ai_net_start_dupl(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0, ts_1st = 0;
	UINT64 ts_start = 0, ts_workbuf = 0, ts_memmap = 0;

	p_proc = _vendor_ai_info(proc_id);

	vendor_ai_net_trace(proc_id, AI_FLOW, "start() - begin\r\n");

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & (AI_JOB|AI_PERF)) {
		_vendor_ai_dump_engine();
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_1st = ts = hd_gettime_us();
	}

	vendor_ais_lock_net(proc_id);
#if (NEW_AI_FLOW == 1)
	//vendor_ais_net_reg_JOB(_vendor_ai_net_JOB_done);
	vendor_ai_cpu_thread_reg_cb(_vendor_ai_net_CPU_exec);
	vendor_ai_dsp_thread_reg_cb(_vendor_ai_net_DSP_exec);
#endif

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT init & open yet, please call vendor_ai_net_init() and vendor_ai_net_open() first !!!\r\n");
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT open yet, please call vendor_ai_net_open() first !!!\r\n");
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START){
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("module is already start, ignore start request....\r\n");
		return HD_ERR_ALREADY_START;
	}

	// assign read only buffer for this proc_id	
	rv =_vendor_ai_net_rbuf_assign_range(p_proc, proc_id);
	if (rv != HD_OK) {
		DBG_ERR("ERROR : proc_id(%lu)  rbuf is NOT set yet, please rbuf first !!!\r\n", proc_id);
		return rv;
	}

	if (p_proc->priv.io_buf_alloc == 0) {
		UINT32 req_size = _vendor_ai_net_io_buf_calc_size(&p_proc->mem_manager);
		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts = hd_gettime_us();
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - begin, req_size = %lu\r\n", req_size);

		if (p_proc->workbuf.pa && p_proc->workbuf.va && (p_proc->workbuf.size >= req_size)) { // user already set workbuf
			// whole work_buf
			p_proc->priv.work_buf.pa   = p_proc->workbuf.pa;
			p_proc->priv.work_buf.va   = p_proc->workbuf.va;
			p_proc->priv.work_buf.size = p_proc->workbuf.size;
			// rslt_buf
			p_proc->priv.rslt_buf.pa   = p_proc->rsltbuf.pa;
			p_proc->priv.rslt_buf.va   = p_proc->rsltbuf.va;
			p_proc->priv.rslt_buf.size   = p_proc->rsltbuf.size;
			// io_buf
			p_proc->priv.io_buf.pa   = p_proc->iobuf.pa;
			p_proc->priv.io_buf.va   = p_proc->iobuf.va;
			p_proc->priv.io_buf.size   = p_proc->iobuf.size;

#if CNN_AI_FASTBOOT
			if (vendor_ais_is_fastboot()) {
				// need to replace with fastboot fixed mem
				p_proc->priv.io_buf.va = g_fboot_mem.user_buff.va;
			}
#endif

		} else { // need to alloc workbuf by hdal
			if( _vendor_ai_common_info()->share_model_mode == 2 ){
				DBG_ERR("ERROR : proc_id(%lu)  workbuf is NOT set yet, please workbuf first under share_mode 2 !!!\r\n", proc_id);
				return HD_ERR_BAD_DATA ; 
			}
#if CNN_INNER_POSTPROC
			if (1) {
				UINT32 buffer_size = 0;
				HD_RESULT r1;
				r1 = _vendor_ai_net_get_last_layer_cpu_buffer_size(proc_id, &buffer_size);
				if (r1 == HD_OK) {
					p_proc->rsltbuf.size = buffer_size;
				}
			}
#endif
			// alloc working buffer for (rslt_buf) + (io_buf)             for given proc_id (if VENDOR_AI_CFG_SHAREMODEL_MODE == 0)
			// alloc working buffer for (rslt_buf) + (kerl_parm + io_buf) for given proc_id (if VENDOR_AI_CFG_SHAREMODEL_MODE == 1)
			rv = _vendor_ai_net_work_buf_alloc(proc_id, &p_proc->mem_manager, p_proc->rsltbuf.size);
			if (rv != HD_OK) {
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
#if CNN_INNER_POSTPROC
			if (p_proc->rsltbuf.size > 0) {
				HD_RESULT r1;
				// rsltbuf should put before io_buff
				p_proc->rsltbuf.pa = p_proc->priv.work_buf.pa;
				p_proc->rsltbuf.va = p_proc->priv.work_buf.va;
				p_proc->iobuf.pa = p_proc->rsltbuf.pa + p_proc->rsltbuf.size;
				p_proc->iobuf.va = p_proc->rsltbuf.va + p_proc->rsltbuf.size;

				r1 = _vendor_ai_net_set_last_layer_cpu_buffer_addr(proc_id, p_proc->rsltbuf.va, p_proc->rsltbuf.size);
				if (r1 != HD_OK) {
					DBG_ERR("addr of rsltbuf is not enough\r\n");
					return r1;
				}
			}
#endif
		}

		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - io buf, pa = %#lx, va = %#lx, size = %lu\r\n",
							p_proc->priv.io_buf.pa, p_proc->priv.io_buf.va, p_proc->priv.io_buf.size);

		// assign io buffer for this proc_id
		_vendor_ai_net_io_buf_assign_range(&p_proc->priv.io_buf, &p_proc->mem_manager);

		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - end\r\n");

		if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP) {
			// 2 = SDK group done, iomem alloc done (fix 0x0CCCCCCC to offset value)
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_IOMEM, &p_proc->mem_manager, "/mnt/sd/ori_iomem2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL, &p_proc->mem_manager, "/mnt/sd/ori_mctrl2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_AIPARM, &p_proc->mem_manager, "/mnt/sd/ori_aiparm2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_LLCMD,  &p_proc->mem_manager, "/mnt/sd/ori_llcmd2.txt");

			if (p_proc->priv.b_simplify_bin == FALSE) {
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL_ENTRY, &p_proc->mem_manager, "/mnt/sd/group_mctrl_entry.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_GROUP, &p_proc->mem_manager, "/mnt/sd/group.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MEM_ENTRY, &p_proc->mem_manager, "/mnt/sd/group_mem_list.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_GROUP_NODE, &p_proc->mem_manager, "/mnt/sd/dot_group_graph.txt");
			}

			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_BUF_NODE, &p_proc->mem_manager, "/mnt/sd/dot_buf_graph.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_LAYER_NODE, &p_proc->mem_manager, "/mnt/sd/dot_layer_graph.txt");
		}

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_workbuf = ts_new - ts;
			vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> workbuf time = %llu\r\n", ts_workbuf);

			ts = ts_new;
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - mem map - begin\r\n");

		// map user/kernel memory
		rv = vendor_ais_net_gen_init(p_proc->mem_manager, proc_id);
		if (rv != HD_OK) {
			DBG_ERR("proc_id(%lu) net gen init fail=%d\r\n", proc_id, rv);
			goto gen_init_fail;
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - mem map - end\r\n");

#if CNN_AI_FASTBOOT
		// dump model bin and dtsi for fastboot
		if (vendor_ais_get_fastboot_dump_enable(0)) {
			VENDOR_AIS_FLOW_KERL_START_MEM kerl_start_mem = {0};
			// get kerl_parm va from ioctl, should get after gen_init() finished
			kerl_start_mem.proc_id = proc_id;

			vendor_ais_get_kerl_start_mem(&kerl_start_mem);
			/*printf("====== kerl_start mem =======\r\n");
			printf("kerl_parm: pa(0x%lx), va(0x%lx) size(0x%lx)\r\n", kerl_start_mem.kerl_parm.pa, kerl_start_mem.kerl_parm.va, kerl_start_mem.kerl_parm.size);
			printf("user_buff: pa(0x%lx), va(0x%lx) size(0x%lx)\r\n", kerl_start_mem.user_buff.pa, kerl_start_mem.user_buff.va, kerl_start_mem.user_buff.size);*/

			// dump model bin and dtsi
			rv = _vendor_ai_net_dump_fastboot(p_proc, &kerl_start_mem);
			if (rv != HD_OK) {
				DBG_ERR("proc_id(%lu) dump fastboot model bin fail(%d)\r\n", proc_id, rv);
				return rv;
			}
		}
#endif

		if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP) {
			// 3 = after vendor_ais_net_gen_init(), SDK fix offset to absolute memory address
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_IOMEM, &p_proc->mem_manager, "/mnt/sd/ori_iomem3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL, &p_proc->mem_manager, "/mnt/sd/ori_mctrl3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_AIPARM, &p_proc->mem_manager, "/mnt/sd/ori_aiparm3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_LLCMD,  &p_proc->mem_manager, "/mnt/sd/ori_llcmd3.txt");
		}
		//force set w=0, h=0 in layer[1] llcmd, to test error handing of engine timeout
		//vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_CRASH,  &p_proc->mem_manager, 0);
		//vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_KERL, NET_DBG_ITEM_CRASH,  &p_proc->mem_manager, 0);

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_memmap = ts_new - ts;
			vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> memmap time = %llu\r\n", ts_memmap);
		}

		vendor_ai_net_trace(proc_id, AI_FLOW, "start() - init_job_graph\r\n");
		vendor_ai_init_job_graph(proc_id);
		p_proc->priv.io_buf_alloc = 1;
	}

	// check if MODEL is set by user
	{
		VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
		if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
			vendor_ais_unlock_net(proc_id);
			DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL	for model first !!\r\n", proc_id);
			rv = HD_ERR_NO_CONFIG;
			goto gen_init_fail;
		}
	}

	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_mem start\r\n");
			rv = vendor_ais_pars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				goto gen_init_fail;
			}
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_mem end\r\n");
		}
	}

#if (NEW_AI_FLOW == 1)
	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;

		rv = vendor_ais_start_net(model_mem, proc_id, p_cfg->job_opt.method, p_cfg->job_opt.wait_ms, p_cfg->buf_opt.ddr_id); // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
		if (rv != HD_OK) {
			goto gen_init_fail;
		}
	}
#endif

#if CNN_MULTI_INPUT
	// get source layer information
	{
		UINT32 cnt = 0, fmt = 0;
		rv = _vendor_ai_net_get_src_layer_cnt_fmt(proc_id, &cnt, &fmt);
		p_proc->priv.src_layer.cnt = (INT32)cnt;
		p_proc->priv.src_layer.in_buf_num = cnt;
		if (rv != HD_OK) {
			DBG_ERR("proc_id(%lu) get src layer cnt/fmt fail(%d)\r\n", proc_id, rv);
			return HD_ERR_FAIL;
		}
	}
#endif

	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_START;
	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "start() - end\r\n");

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_start = ts_new - ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "start() - start time = %llu\r\n", ts_start);
	}
	return rv;

gen_init_fail:
	if (vendor_ais_net_gen_uninit(proc_id) != HD_OK) {
		DBG_ERR("proc_id(%lu) vendor_ais_net_gen_uninit fail\r\n", proc_id);
	}


		return rv;
}


HD_RESULT vendor_ai_net_start (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0, ts_1st = 0;
	UINT64 ts_start = 0, ts_workbuf = 0, ts_memmap = 0, ts_pars_diff_mem = 0;

	p_proc = _vendor_ai_info(proc_id);

	vendor_ai_net_trace(proc_id, AI_FLOW, "start() - begin\r\n");

	
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & (AI_JOB|AI_PERF)) {
	    _vendor_ai_dump_engine();
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_1st = ts = hd_gettime_us();
	}

	vendor_ais_lock_net(proc_id);
#if (NEW_AI_FLOW == 1)
	//vendor_ais_net_reg_JOB(_vendor_ai_net_JOB_done);
	vendor_ai_cpu_thread_reg_cb(_vendor_ai_net_CPU_exec);
	vendor_ai_dsp_thread_reg_cb(_vendor_ai_net_DSP_exec);
#endif

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT init & open yet, please call vendor_ai_net_init() and vendor_ai_net_open() first !!!\r\n");
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT open yet, please call vendor_ai_net_open() first !!!\r\n");
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START){
			vendor_ais_unlock_net(proc_id);
		DBG_ERR("module is already start, ignore start request....\r\n");
		return HD_ERR_ALREADY_START;
	}

	// assign read only buffer for this proc_id	
	rv =_vendor_ai_net_rbuf_assign_range(p_proc, proc_id);
	if (rv != HD_OK) {
		DBG_ERR("ERROR : proc_id(%lu)  rbuf is NOT set yet, please rbuf first !!!\r\n", proc_id);
		return rv;
	}


	if (p_proc->priv.io_buf_alloc == 0) {
		UINT32 req_size = _vendor_ai_net_io_buf_calc_size(&p_proc->mem_manager);
		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts = hd_gettime_us();
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - begin, req_size = %lu\r\n", req_size);

		if (p_proc->workbuf.pa && p_proc->workbuf.va && (p_proc->workbuf.size >= req_size)) { // user already set workbuf
			// whole work_buf
			p_proc->priv.work_buf.pa   = p_proc->workbuf.pa;
			p_proc->priv.work_buf.va   = p_proc->workbuf.va;
			p_proc->priv.work_buf.size = p_proc->workbuf.size;
			// rslt_buf
			p_proc->priv.rslt_buf.pa   = p_proc->rsltbuf.pa;
			p_proc->priv.rslt_buf.va   = p_proc->rsltbuf.va;
			p_proc->priv.rslt_buf.size   = p_proc->rsltbuf.size;
			// io_buf
			p_proc->priv.io_buf.pa   = p_proc->iobuf.pa;
			p_proc->priv.io_buf.va   = p_proc->iobuf.va;
			p_proc->priv.io_buf.size   = p_proc->iobuf.size;

#if CNN_AI_FASTBOOT
			if (vendor_ais_is_fastboot()) {
				// need to replace with fastboot fixed mem
				p_proc->priv.io_buf.va = g_fboot_mem.user_buff.va;
			}
#endif

		} else { // need to alloc workbuf by hdal
            if( _vendor_ai_common_info()->share_model_mode == 2 ){
                DBG_ERR("ERROR : proc_id(%lu)  workbuf is NOT set yet, please workbuf first under share_mode 2 !!!\r\n", proc_id);
                return HD_ERR_BAD_DATA ; 
            }
#if CNN_INNER_POSTPROC
			if (1) {
				UINT32 buffer_size = 0;
				HD_RESULT r1;
				r1 = _vendor_ai_net_get_last_layer_cpu_buffer_size(proc_id, &buffer_size);
				if (r1 == HD_OK) {
					p_proc->rsltbuf.size = buffer_size;
				}
			}
#endif
			// alloc working buffer for (rslt_buf) + (io_buf) for given proc_id
			rv = _vendor_ai_net_work_buf_alloc(proc_id, &p_proc->mem_manager, p_proc->rsltbuf.size);
			if (rv != HD_OK) {
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
#if CNN_INNER_POSTPROC
			if (p_proc->rsltbuf.size > 0) {
				HD_RESULT r1;
				// rsltbuf should put before io_buff
				p_proc->rsltbuf.pa = p_proc->priv.work_buf.pa;
				p_proc->rsltbuf.va = p_proc->priv.work_buf.va;
				p_proc->iobuf.pa = p_proc->rsltbuf.pa + p_proc->rsltbuf.size;
				p_proc->iobuf.va = p_proc->rsltbuf.va + p_proc->rsltbuf.size;

				r1 = _vendor_ai_net_set_last_layer_cpu_buffer_addr(proc_id, p_proc->rsltbuf.va, p_proc->rsltbuf.size);
				if (r1 != HD_OK) {
					DBG_ERR("addr of rsltbuf is not enough\r\n");
					return r1;
				}
			}
#endif
		}

		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - io buf, pa = %#lx, va = %#lx, size = %lu\r\n",
							p_proc->priv.io_buf.pa, p_proc->priv.io_buf.va, p_proc->priv.io_buf.size);

		// assign io buffer for this proc_id
		_vendor_ai_net_io_buf_assign_range(&p_proc->priv.io_buf, &p_proc->mem_manager);

		vendor_ai_net_trace(proc_id, AI_BUF, "start() - work buf alloc - end\r\n");

		if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP) {
			// 2 = SDK group done, iomem alloc done (fix 0x0CCCCCCC to offset value)
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_IOMEM, &p_proc->mem_manager, "/mnt/sd/ori_iomem2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL, &p_proc->mem_manager, "/mnt/sd/ori_mctrl2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_AIPARM, &p_proc->mem_manager, "/mnt/sd/ori_aiparm2.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_LLCMD,  &p_proc->mem_manager, "/mnt/sd/ori_llcmd2.txt");

			if (p_proc->priv.b_simplify_bin == FALSE) {
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL_ENTRY, &p_proc->mem_manager, "/mnt/sd/group_mctrl_entry.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_GROUP, &p_proc->mem_manager, "/mnt/sd/group.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MEM_ENTRY, &p_proc->mem_manager, "/mnt/sd/group_mem_list.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_GROUP_NODE, &p_proc->mem_manager, "/mnt/sd/dot_group_graph.txt");
			}

			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_BUF_NODE, &p_proc->mem_manager, "/mnt/sd/dot_buf_graph.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_DOT_LAYER_NODE, &p_proc->mem_manager, "/mnt/sd/dot_layer_graph.txt");
		}

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_workbuf = ts_new - ts;
			vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> workbuf time = %llu\r\n", ts_workbuf);

			ts = ts_new;
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - mem map - begin\r\n");

		// map user/kernel memory
		rv = vendor_ais_net_gen_init(p_proc->mem_manager, proc_id);
		if (rv != HD_OK) {
			DBG_ERR("proc_id(%lu) net gen init fail=%d\r\n", proc_id, rv);
			goto gen_init_fail;
		}
		vendor_ai_net_trace(proc_id, AI_BUF, "start() - mem map - end\r\n");

#if CNN_AI_FASTBOOT
		// dump model bin and dtsi for fastboot
		if (vendor_ais_get_fastboot_dump_enable(0)) {
			VENDOR_AIS_FLOW_KERL_START_MEM kerl_start_mem = {0};
			// get kerl_parm va from ioctl, should get after gen_init() finished
			kerl_start_mem.proc_id = proc_id;

			vendor_ais_get_kerl_start_mem(&kerl_start_mem);
			/*printf("====== kerl_start mem =======\r\n");
			printf("kerl_parm: pa(0x%lx), va(0x%lx) size(0x%lx)\r\n", kerl_start_mem.kerl_parm.pa, kerl_start_mem.kerl_parm.va, kerl_start_mem.kerl_parm.size);
			printf("user_buff: pa(0x%lx), va(0x%lx) size(0x%lx)\r\n", kerl_start_mem.user_buff.pa, kerl_start_mem.user_buff.va, kerl_start_mem.user_buff.size);*/

			// dump model bin and dtsi
			rv = _vendor_ai_net_dump_fastboot(p_proc, &kerl_start_mem);
			if (rv != HD_OK) {
				DBG_ERR("proc_id(%lu) dump fastboot model bin fail(%d)\r\n", proc_id, rv);
				return rv;
			}
		}
#endif

		if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_NETINFO_DUMP) {
			// 3 = after vendor_ais_net_gen_init(), SDK fix offset to absolute memory address
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_IOMEM, &p_proc->mem_manager, "/mnt/sd/ori_iomem3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_MCTRL, &p_proc->mem_manager, "/mnt/sd/ori_mctrl3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_AIPARM, &p_proc->mem_manager, "/mnt/sd/ori_aiparm3.txt");
			vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_LLCMD,  &p_proc->mem_manager, "/mnt/sd/ori_llcmd3.txt");
		}
		//force set w=0, h=0 in layer[1] llcmd, to test error handing of engine timeout
		//vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_USER, NET_DBG_ITEM_CRASH,  &p_proc->mem_manager, 0);
		//vendor_ai_net_debug_dump(proc_id, NET_DBG_SPACE_KERL, NET_DBG_ITEM_CRASH,  &p_proc->mem_manager, 0);

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_memmap = ts_new - ts;
			vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> memmap time = %llu\r\n", ts_memmap);
		}
		
		vendor_ai_net_trace(proc_id, AI_FLOW, "start() - init_job_graph\r\n");
		vendor_ai_init_job_graph(proc_id);
		p_proc->priv.io_buf_alloc = 1;
	}

	// check if MODEL is set by user
	{
		VENDOR_AI_NET_CFG_MODEL *p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
		if ((p_cfg_model->pa == 0) || (p_cfg_model->va == 0) || (p_cfg_model->size == 0)) {
			vendor_ais_unlock_net(proc_id);
			DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_CFG_MODEL	for model first !!\r\n", proc_id);
			rv = HD_ERR_NO_CONFIG;
			goto gen_init_fail;
		}
	}

	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_mem start\r\n");
			rv = vendor_ais_pars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				goto gen_init_fail;
			}
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_mem end\r\n");
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				ts_pars_diff_mem = ts_new - ts;
				vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> pars_diff_mem time = %llu\r\n", ts_pars_diff_mem);
			}
		}
	}

	// check if VENDOR_AI_BATCH_INFO is set by user
	{
		VENDOR_AI_BATCH_INFO *p_batch_info = NULL;
		p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
		if (p_batch_info->enable) {
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_batch start\r\n");
			rv = vendor_ais_pars_diff_batch(&p_proc->mem_manager, p_batch_info, proc_id);
			if (rv != HD_OK) {
				goto gen_init_fail;
			}
			vendor_ai_net_trace(proc_id, AI_FLOW, "start() - pars_diff_batch end\r\n");
		}
	}

#if (NEW_AI_FLOW == 1)
	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;

		rv = vendor_ais_start_net(model_mem, proc_id, p_cfg->job_opt.method, p_cfg->job_opt.wait_ms, p_cfg->buf_opt.ddr_id); // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
		if (rv != HD_OK) {
			goto gen_init_fail;
		}
	}
#endif

#if CNN_MULTI_INPUT
	// get source layer information
	{
		UINT32 cnt = 0, fmt = 0;
		rv = _vendor_ai_net_get_src_layer_cnt_fmt(proc_id, &cnt, &fmt);
		p_proc->priv.src_layer.cnt = (INT32)cnt;
		p_proc->priv.src_layer.in_buf_num = cnt;
		if (rv != HD_OK) {
			DBG_ERR("proc_id(%lu) get src layer cnt/fmt fail(%d)\r\n", proc_id, rv);
			return HD_ERR_FAIL;
		}
	}
#endif

	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_START;
	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "start() - end\r\n");

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_start = ts_new - ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "start() - start time = %llu\r\n", ts_start);
	}
	return rv;

gen_init_fail:
	if (vendor_ais_net_gen_uninit(proc_id) != HD_OK) {
		DBG_ERR("proc_id(%lu) vendor_ais_net_gen_uninit fail\r\n", proc_id);
	}
	return rv;
}

static HD_RESULT _vendor_ai_net_push (UINT32 proc_id, BOOL b_sync)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_1st = 0;
	UINT64 ts_proc = 0;
	UINT64 ts_workclr = 0;

	vendor_ai_net_trace(proc_id, AI_FLOW, "proc() - begin\r\n");
	
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
    	ts_1st = ts = hd_gettime_us();
	}
	if (b_sync) vendor_ais_lock_net(proc_id);
	
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		if (b_sync) vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT init & open & start yet, please call vendor_ai_net_init() & vendor_ai_net_open() & vendor_ai_net_start() first !!!\r\n");
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
		if (b_sync) vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT open & start yet, please call vendor_ai_net_open() & vendor_ai_net_start() first !!!\r\n");
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN){
		if (b_sync) vendor_ais_unlock_net(proc_id);
		DBG_ERR("FATAL ERROR : module is NOT start yet, please call vendor_ai_net_start() first !!!\r\n");
		return HD_ERR_NOT_START;
	}

	// check if input image is set by user
	{
		VENDOR_AI_BUF *p_input_img = (VENDOR_AI_BUF *)&p_proc->input_img;
		// p_input_img->va is optional, only used if layer 0 is CPU layer
		if (p_input_img->pa == 0) {
			if (b_sync) vendor_ais_unlock_net(proc_id);
			DBG_ERR("ERROR : proc_id(%lu) please set VENDOR_AI_NET_PARAM_IN(layer_id, in_id) for input image first !!\r\n", proc_id);
			return HD_ERR_NO_CONFIG;
		}
	}

	// proc + input_uninit
	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;

#if (DBG_OUT_DUMP == 1)
	vendor_ai_cmd_set_iomem_debug(proc_id, "clear_iobuf", "on");
#endif
//DEBUG: set iobuf to zero before proc
if (vendor_ai_cmd_get_iomem_debug(proc_id) & VENDOR_AI_NET_CMD_IOMEM_CLEAR_IOBUF){
		printf("proc() - >>> proc_id(%lu) clear io buffer... !!\n", proc_id);
		// clear io buffer
		memset((VOID *)p_proc->mem_manager.user_buff.va, 0, p_proc->mem_manager.user_buff.size);
		//hd_common_mem_flush_cache((VOID *)p_proc->mem_manager.user_buff.va, p_proc->mem_manager.user_buff.size);
		vendor_common_mem_cache_sync((VOID *)p_proc->mem_manager.user_buff.va, p_proc->mem_manager.user_buff.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
}
		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_workclr = ts_new-ts;
			vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "proc() - >>> workbuf clr time = %llu\r\n", ts_workclr);
		}		
		// proc
#if (NEW_AI_FLOW == 1)
		rv = vendor_ais_push_net(model_mem, proc_id, p_cfg->job_opt.method); // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
#else
		rv = vendor_ais_push_net(model_mem, proc_id);  // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
#endif
	}
	
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_proc = ts_new-ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "proc() - proc time = %llu\r\n", ts_proc);
	}
	if (rv != HD_OK) {
		if (b_sync) vendor_ais_unlock_net(proc_id);
	}
	return rv;
}

static HD_RESULT _vendor_ai_net_pull (UINT32 proc_id, BOOL b_sync)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_1st = 0;
	UINT64 ts_proc = 0;
	UINT64 ts_inputclr = 0;

	vendor_ai_net_trace(proc_id, AI_FLOW, "proc() - begin\r\n");
	
	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_1st = ts = hd_gettime_us();
	}
	
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;

		// proc
#if (NEW_AI_FLOW == 1)
		rv = vendor_ais_pull_net(model_mem, proc_id, p_cfg->job_opt.wait_ms); // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
#else
		rv = vendor_ais_pull_net(model_mem, proc_id);  // no longer need rslt_mem, this is handled by vendor_ai_cpu_postproc now => just pass {0} struct into
#endif

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts = hd_gettime_us();
		}
		// input uninit
		vendor_ais_net_input_uninit(proc_id);
		//restore src_layer.cnt
		p_proc->priv.src_layer.cnt = p_proc->priv.src_layer.in_buf_num;

		if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
			ts_new = hd_gettime_us();
			ts_inputclr = ts_new-ts;
			vendor_ai_net_trace(proc_id, AI_BUF|AI_PERF, "proc() - >>> inputbuf clr time = %llu\r\n", ts_inputclr);
		}	
	}
	if (b_sync) vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "proc() - end\r\n");

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_proc = ts_new-ts_1st;
		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "proc() - proc time = %llu\r\n", ts_proc);
	}
	
	return rv;
}

HD_RESULT vendor_ai_net_proc (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	p_proc = _vendor_ai_info(proc_id);

	if (p_proc == NULL) {
		DBG_ERR("Cannot find ai_info by proc_id(%lu)\r\n", proc_id);
		return HD_ERR_FAIL;
	}
	if (p_proc->priv.src_layer.in_buf_num == 0) {
		p_proc->priv.src_layer.in_buf_num = 1;  //set 1 to uninit input
		vendor_ais_net_input_uninit(proc_id);
		DBG_ERR("Cannot find input buffer by proc_id(%lu), please set VENDOR_AI_NET_PARAM_IN first and just before proc.\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	rv = _vendor_ai_net_push(proc_id, 1);
	if (rv != HD_OK) {
		return rv;
	}
	rv = _vendor_ai_net_pull(proc_id, 1);
	return rv;
}

HD_RESULT vendor_ai_net_proc_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID in_param_id, void* p_in_buf, VENDOR_AI_NET_PARAM_ID out_param_id, void* p_out_buf)
{
	HD_RESULT rv;
	
	vendor_ais_lock_net(proc_id);
	
	if (in_param_id == VENDOR_AI_NET_PARAM_IN_BUF_LIST) {
		DBG_ERR("proc_id(%lu) - IN_BUF_LIST not support yet !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		rv = vendor_ai_net_set(proc_id, in_param_id, p_in_buf);
	}
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	
	rv = _vendor_ai_net_push(proc_id, 0);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	rv = _vendor_ai_net_pull(proc_id, 0);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}

	if (out_param_id == VENDOR_AI_NET_PARAM_OUT_BUF_LIST) {
		DBG_ERR("proc_id(%lu) - OUT_BUF_LIST not support yet !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		rv = vendor_ai_net_get(proc_id, out_param_id, p_out_buf);
	}
	
	vendor_ais_unlock_net(proc_id);

	return rv;
}

HD_RESULT vendor_ai_net_push_in_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID in_param_id, void* p_in_buf, INT32 wait_ms)
{
	HD_RESULT rv;
	
	vendor_ais_lock_net(proc_id);
	
	if (in_param_id == VENDOR_AI_NET_PARAM_IN_BUF_LIST) {
		DBG_ERR("proc_id(%lu) - IN_BUF_LIST not support yet !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		rv = vendor_ai_net_set(proc_id, in_param_id, p_in_buf);
	}
	
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) - push_in_buf set fail %d !!\r\n", proc_id, rv);
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	
	rv = _vendor_ai_net_push(proc_id, 0);
	
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) - push_in_buf push fail %d !!\r\n", proc_id, rv);
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	return rv;
}

HD_RESULT vendor_ai_net_pull_out_buf (UINT32 proc_id, VENDOR_AI_NET_PARAM_ID out_param_id, void* p_out_buf, INT32 wait_ms)
{
	HD_RESULT rv;
	
	rv = _vendor_ai_net_pull(proc_id, 0);
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) - pull_out_buf pull fail %d !!\r\n", proc_id, rv);
		vendor_ais_unlock_net(proc_id);
		return rv;
	}

	if (out_param_id == VENDOR_AI_NET_PARAM_OUT_BUF_LIST) {
		DBG_ERR("proc_id(%lu) - OUT_BUF_LIST not support yet !!\r\n", proc_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		rv = vendor_ai_net_get(proc_id, out_param_id, p_out_buf);
	}
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) - pull_out_buf get fail %d !!\r\n", proc_id, rv);
	}

	vendor_ais_unlock_net(proc_id);

	return rv;
}

/* duplication of vendor_ai_net_stop() */
HD_RESULT vendor_ai_net_stop_dupl (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0;
	UINT64 ts_stop = 0;
	
	vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - begin\r\n");

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts = hd_gettime_us();
	}
	vendor_ais_lock_net(proc_id);
	p_proc = _vendor_ai_info(proc_id);
p_cfg = _vendor_ai_cfg(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
		DBG_ERR("module is NOT start yet, ignore stop request.... \r\n");
	vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_START;
	}

	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_mem start\r\n");
			rv = vendor_ais_unpars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				return rv;
			}
			p_diff_resinfo->curr_id = 0;
            p_diff_resinfo->curr_dim.h = 0;
            p_diff_resinfo->curr_dim.w = 0;
            p_diff_resinfo->new_id = 0;
            p_diff_resinfo->new_dim.h = 0;
            p_diff_resinfo->new_dim.w = 0;
			
			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_mem end\r\n");
		}
	}

#if (NEW_AI_FLOW == 1)
	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;
		rv = vendor_ais_stop_net(model_mem, proc_id);
		if (rv != HD_OK) {
			return rv;
		}
	}
#endif
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN; // [START] -> [OPEN]
	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - end\r\n");
	/////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_stop = ts_new-ts;

		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "stop() - stop time = %llu\r\n", ts_stop);
	}


	return rv;
}

HD_RESULT vendor_ai_net_stop (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT64 ts = 0, ts_new = 0, ts_unpars_diff_mem = 0;
	UINT64 ts_stop = 0;

	// reload max resolution setting
	if (0) {
		HD_DIM max_dim = {0};
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_cfg = _vendor_ai_cfg(proc_id);
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			vendor_ais_get_multiscale_max_dim(proc_id, &max_dim);
			vendor_ai_net_stop_dupl(proc_id);
			vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_DIM, &max_dim);
			vendor_ai_net_start_dupl(proc_id);
		}
	}

	vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - begin\r\n");

	if (_vendor_ai_is_init() == FALSE) {
		DBG_ERR("proc_id(%lu) NOT init yet !!\n", proc_id);
		return HD_ERR_UNINIT;
	}

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts = hd_gettime_us();
	}
	vendor_ais_lock_net(proc_id);
	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
		DBG_ERR("module is NOT start yet, ignore stop request.... \r\n");
	vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_START;
	}

	// check if VENDOR_AI_BATCH_INFO is set by user
	{
		VENDOR_AI_BATCH_INFO *p_batch_info = NULL;
		p_batch_info = (VENDOR_AI_BATCH_INFO *)&p_cfg->batch_info;
		if (p_batch_info->enable) {
			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_batch start\r\n");
			rv = vendor_ais_unpars_diff_batch(&p_proc->mem_manager, p_batch_info, proc_id);
			if (rv != HD_OK) {
				return rv;
			}
            p_batch_info->enable = 0;
			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_batch end\r\n");
		}
	}
	
	// check if DIFF_MODEL_RESINFO is set by user
	{
		VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo = NULL;
		p_diff_resinfo = (VENDOR_AI_DIFF_MODEL_RESINFO *)&p_cfg->diff_resinfo;
		if ((p_diff_resinfo->diff_model.pa != 0) && (p_diff_resinfo->diff_model.va != 0) && (p_diff_resinfo->diff_model.size != 0)) {
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts = hd_gettime_us();
			}
			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_mem start\r\n");
			rv = vendor_ais_unpars_diff_mem(&p_proc->mem_manager, p_diff_resinfo, proc_id);
			if (rv != HD_OK) {
				return rv;
			}
            p_diff_resinfo->curr_id = 0;
            p_diff_resinfo->curr_dim.h = 0;
            p_diff_resinfo->curr_dim.w = 0;
            p_diff_resinfo->new_id = 0;
            p_diff_resinfo->new_dim.h = 0;
            p_diff_resinfo->new_dim.w = 0;

			vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - unpars_diff_mem end\r\n");
			if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
				ts_new = hd_gettime_us();
				ts_unpars_diff_mem = ts_new - ts;
				vendor_ai_net_trace(proc_id, AI_BUF | AI_PERF, "start() - >>> ts_unpars_diff_mem time = %llu\r\n", ts_unpars_diff_mem);
			}
		}
	}

	//if user set input but not call proc, input will be uninit here
	// input uninit
	vendor_ais_net_input_uninit(proc_id);
	//restore src_layer.cnt
	p_proc->priv.src_layer.cnt = p_proc->priv.src_layer.in_buf_num;

#if (NEW_AI_FLOW == 1)
	{
		VENDOR_AIS_FLOW_MEM_PARM model_mem = {0};

		model_mem.pa    = p_proc->mem_manager.user_parm.pa;    // original vendor_ais_proc_net() pass model bin start address, but vendor_ais_proc_net() acturally need user_parm only, assign newly alloc user_parm
		model_mem.va    = p_proc->mem_manager.user_parm.va;
		model_mem.size  = p_cfg->cfg_model.size;
		rv = vendor_ais_stop_net(model_mem, proc_id);
		if (rv != HD_OK) {
			return rv;
		}
	}
#endif
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN; // [START] -> [OPEN]
	vendor_ais_unlock_net(proc_id);
	vendor_ai_net_trace(proc_id, AI_FLOW, "stop() - end\r\n");
	/////////////////////////////////////////////////////////////////////////////
	if (vendor_ai_net_get_trace(proc_id) & AI_PERF) {
		ts_new = hd_gettime_us();
		ts_stop = ts_new-ts;

		vendor_ai_net_trace(proc_id, AI_FLOW|AI_PERF, "stop() - stop time = %llu\r\n", ts_stop);
	}


	return rv;
}

#if NN_DLI
HD_RESULT _vendor_ai_net_nn_dli_parse_ext_info(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager = NULL;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = NULL;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head = NULL;
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 tmp_size = 0;
	HD_RESULT rv = HD_OK;
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header = NULL;
	NN_DLI_TENSOR_INFO *p_tensor_info = NULL;
	NN_DLI_QUANTIZATION_INFO_HEADER *p_quant_info_header = NULL;
	NN_DLI_QUANTIZATION_INFO *p_quant_info = NULL;
	UINTPTR va_ofs = 0;

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	// use ext_info_num to find external info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	// Try to find tensor info and quantization info header
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINTPTR)p_ex_head) + tmp_size);
		if (p_tmp_head[0] == 0) { // bit0 represent size
			break;
		}
		if (p_tmp_head[1] == NN_DLI_TENSOR_INFO_HEADER_SIGN) { // bit1 represent tag_type
			p_tensor_info_header = (NN_DLI_TENSOR_INFO_HEADER *)p_tmp_head;
		} else if (p_tmp_head[1] == NN_DLI_QUANTIZATION_INFO_HEADER_SIGN) { // bit1 represent tag_type
			p_quant_info_header = (NN_DLI_QUANTIZATION_INFO_HEADER *)p_tmp_head;
		} else {
			// Do nothing.
		}

		if(p_tensor_info_header != NULL && p_quant_info_header != NULL) {
			// Found both
			break;
		}

		tmp_size += p_tmp_head[0]; // add tmp_size, move to next
	}

	// Get Tensor Info array
	if(p_tensor_info_header != NULL) {
		p_tensor_info = (NN_DLI_TENSOR_INFO *)(((UINTPTR)p_tensor_info_header) + sizeof(NN_DLI_TENSOR_INFO_HEADER));
	}

	// Get Quantization Info array
	if(p_quant_info_header != NULL) {
		p_quant_info = (NN_DLI_QUANTIZATION_INFO *)((UINTPTR)(p_quant_info_header) + sizeof(NN_DLI_QUANTIZATION_INFO_HEADER));
	}

	// Connect Layer Parm and Tensor Info
	p_mctrl = net_info.p_mctrl;
	va_ofs = ALIGN_CEIL_4(p_mem->va);

	for (UINT32 process_index = 0; process_index < p_head->mode_ctrl_num; process_index++) {
		UINTPTR p_parm = p_mctrl[process_index].addr + va_ofs;
		rv = HD_OK;
		switch(p_mctrl[process_index].mode) {
		case NN_DLI_SQRT:
		case NN_DLI_EXP:
		case NN_DLI_LOG:
		case NN_DLI_SIN:
		case NN_DLI_FLOOR:
		case NN_DLI_ROUND:
		case NN_DLI_DIV:
		case NN_DLI_POW:
		case NN_DLI_EQUAL:
		case NN_DLI_GREATER:
		case NN_DLI_LESS:
			{
				NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm = (NN_DLI_ElEMENTWISE_PARM *)(p_parm);
				rv = _vendor_ai_net_nn_dli_parse_elementwise_parm(
					p_dli_elementwise_parm, p_tensor_info_header, p_tensor_info);
			}
			break;
		case NN_DLI_RESIZE:
			{
				NN_DLI_RESIZE_PARM *p_dli_resize_parm = (NN_DLI_RESIZE_PARM *)(p_parm);
				rv = _vendor_ai_net_nn_dli_parse_resize_parm(
					p_dli_resize_parm, p_tensor_info_header, p_tensor_info);
			}
			break;
		case NN_DLI_SOFTMAX:
			{
				NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm = (NN_DLI_SOFTMAX_PARM *)(p_parm);
				rv = _vendor_ai_net_nn_dli_parse_softmax_parm(
					p_dli_softmax_parm, p_tensor_info_header, p_tensor_info);
			}
			break;
		default:
			break;
		}
		if(rv != HD_OK)
			return rv;
	}

	// Connect Tensor Info and Quantization Info
	if(p_tensor_info_header) {
		for(UINT32 i = 0; i < p_tensor_info_header->nums; i++) {
			if(p_tensor_info[i].quant_info_nums > 0) {
				if(p_quant_info == NULL) {
					DBG_ERR("Can't find NN_DLI Quantization Info...\r\n");
					return HD_ERR_NG;
				}
				if(p_tensor_info[i].quant_info_va >= p_quant_info_header->nums) {
					DBG_ERR("Invalid NN_DLI quant_info_va %d\r\n", p_tensor_info[i].quant_info_va);
					return HD_ERR_NG;
				}
				p_tensor_info[i].quant_info_va = (UINTPTR)(&p_quant_info[p_tensor_info[i].quant_info_va]);
			}
		}
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_nn_dli_parse_elementwise_parm(
	NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info)
{
	// Check Tensor Info
	if(p_tensor_info_header == NULL || p_tensor_info == NULL) {
		DBG_ERR("NN_DLI Tensor Info is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check parm
	if(p_dli_elementwise_parm == NULL) {
		DBG_ERR("NN_DLI Elementwise Parm is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check sign
	if(p_dli_elementwise_parm->sign != NN_DLI_ElEMENTWISE_PARM_SIGN) {
		DBG_ERR("Invalid NN_DLI Elementwise Parm sign %08lX, check sign %08lX...\r\n",
			p_dli_elementwise_parm->sign, NN_DLI_ElEMENTWISE_PARM_SIGN);
		return HD_ERR_NG;
	}

	// Find input1's tensor info
	if(p_dli_elementwise_parm->input1_info_va < p_tensor_info_header->nums) {
		p_dli_elementwise_parm->input1_info_va = (UINTPTR)(&p_tensor_info[p_dli_elementwise_parm->input1_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Elementwise Parm input1_info_va %d\r\n", p_dli_elementwise_parm->input1_info_va);
		return HD_ERR_NG;
	}

	// Find input2's tensor info
	if(p_dli_elementwise_parm->input2_info_va < p_tensor_info_header->nums) {
		p_dli_elementwise_parm->input2_info_va = (UINTPTR)(&p_tensor_info[p_dli_elementwise_parm->input2_info_va]);
	} else if(p_dli_elementwise_parm->input2_info_va == UINTPTR_MAX) {
		// Do nothing
	} else {
		DBG_ERR("Invalid NN_DLI Elementwise Parm input2_info_va %d\r\n", p_dli_elementwise_parm->input2_info_va);
		return HD_ERR_NG;
	}

	// Find output's tensor info
	if(p_dli_elementwise_parm->output_info_va < p_tensor_info_header->nums) {
		p_dli_elementwise_parm->output_info_va = (UINTPTR)(&p_tensor_info[p_dli_elementwise_parm->output_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Elementwise Parm output_info_va %d\r\n", p_dli_elementwise_parm->output_info_va);
		return HD_ERR_NG;
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_nn_dli_parse_resize_parm(
	NN_DLI_RESIZE_PARM *p_dli_resize_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info)
{
	// Check Tensor Info
	if(p_tensor_info_header == NULL || p_tensor_info == NULL) {
		DBG_ERR("NN_DLI Tensor Info is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check parm
	if(p_dli_resize_parm == NULL) {
		DBG_ERR("NN_DLI Resize Parm is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check sign
	if(p_dli_resize_parm->sign != NN_DLI_RESIZE_PARM_SIGN) {
		DBG_ERR("Invalid NN_DLI Resize Parm sign %08lX, check sign %08lX...\r\n",
			p_dli_resize_parm->sign, NN_DLI_RESIZE_PARM_SIGN);
		return HD_ERR_NG;
	}

	// Find input's tensor info
	if(p_dli_resize_parm->input_info_va < p_tensor_info_header->nums) {
		p_dli_resize_parm->input_info_va = (UINTPTR)(&p_tensor_info[p_dli_resize_parm->input_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Resize Parm input_info_va %d\r\n", p_dli_resize_parm->input_info_va);
		return HD_ERR_NG;
	}

	// Find output's tensor info
	if(p_dli_resize_parm->output_info_va < p_tensor_info_header->nums) {
		p_dli_resize_parm->output_info_va = (UINTPTR)(&p_tensor_info[p_dli_resize_parm->output_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Resize Parm output_info_va %d\r\n", p_dli_resize_parm->output_info_va);
		return HD_ERR_NG;
	}

	return HD_OK;
}

HD_RESULT _vendor_ai_net_nn_dli_parse_softmax_parm(
	NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm,
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header,
	NN_DLI_TENSOR_INFO *p_tensor_info)
{
	// Check Tensor Info
	if(p_tensor_info_header == NULL || p_tensor_info == NULL) {
		DBG_ERR("NN_DLI Tensor Info is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check parm
	if(p_dli_softmax_parm == NULL) {
		DBG_ERR("NN_DLI Softmax Parm is NULL...\r\n");
		return HD_ERR_NG;
	}

	// Check sign
	if(p_dli_softmax_parm->sign != NN_DLI_SOFTMAX_PARM_SIGN) {
		DBG_ERR("Invalid NN_DLI Softmax Parm sign %08lX, check sign %08lX...\r\n",
			p_dli_softmax_parm->sign, NN_DLI_SOFTMAX_PARM_SIGN);
		return HD_ERR_NG;
	}

	// Find input's tensor info
	if(p_dli_softmax_parm->input_info_va < p_tensor_info_header->nums) {
		p_dli_softmax_parm->input_info_va = (UINTPTR)(&p_tensor_info[p_dli_softmax_parm->input_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Softmax Parm input_info_va %d\r\n", p_dli_softmax_parm->input_info_va);
		return HD_ERR_NG;
	}

	// Find output's tensor info
	if(p_dli_softmax_parm->output_info_va < p_tensor_info_header->nums) {
		p_dli_softmax_parm->output_info_va = (UINTPTR)(&p_tensor_info[p_dli_softmax_parm->output_info_va]);
	} else {
		DBG_ERR("Invalid NN_DLI Softmax Parm output_info_va %d\r\n", p_dli_softmax_parm->output_info_va);
		return HD_ERR_NG;
	}

	return HD_OK;
}

#endif

HD_RESULT _vendor_ais_split_combine_model_bin(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	//VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	//VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('M') | ((UINT32)('D')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;

    VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
    VENDOR_AI_NET_CFG_MODEL *p_cfg_model = NULL;
    VENDOR_AI_NET_CFG_MODEL *p_share_diff_model = NULL;
    VENDOR_AI_NET_CFG_MODEL *p_diff_model = NULL;
    VENDOR_AI_NET_CFG_MODEL *p_output_info = NULL;

    UINT64 nvt_mode_bin_filesize = 0;
    UINT64 diff_bin_filesize = 0;

    p_cfg_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->cfg_model;
    p_share_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.share_diff_model;
    p_diff_model = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.diff_model;
    p_output_info = (VENDOR_AI_NET_CFG_MODEL *)&p_cfg->diff_resinfo.output_info;

	//p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
	// get layer info
	p_head = net_info.p_head;

	// get layer info
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_MODEL_INFO* model_info = (NN_MODEL_INFO *)(((uintptr_t)p_tmp_head) + 2*sizeof(UINT32));
			if (model_info == NULL) {
				DBG_ERR("model_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
            nvt_mode_bin_filesize = model_info->nvt_mode_bin_filesize;
            diff_bin_filesize = model_info->diff_bin_filesize;
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}
    if(nvt_mode_bin_filesize != 0 && diff_bin_filesize != 0) {
        if(_vendor_ai_common_info()->share_model_mode == 2) {
            p_share_diff_model->pa = p_cfg_model->pa + nvt_mode_bin_filesize;
            p_share_diff_model->va = p_cfg_model->va + nvt_mode_bin_filesize;
            p_share_diff_model->size = diff_bin_filesize;
            p_output_info->pa = p_share_diff_model->pa + diff_bin_filesize;
            p_output_info->va = p_share_diff_model->va + diff_bin_filesize;
        } else {
            p_diff_model->pa = p_cfg_model->pa + nvt_mode_bin_filesize;
            p_diff_model->va = p_cfg_model->va + nvt_mode_bin_filesize;
            p_diff_model->size = diff_bin_filesize;
            p_output_info->pa = p_diff_model->pa + diff_bin_filesize;
            p_output_info->va = p_diff_model->va + diff_bin_filesize;
        }
        vendor_ai_net_trace(proc_id, AI_FLOW, "open() - split combine model , nvt_mode_bin_filesize = %llu\r\n", nvt_mode_bin_filesize);
        vendor_ai_net_trace(proc_id, AI_FLOW, "open() - split combine model , diff_bin_filesize = %llu\r\n", diff_bin_filesize);
    }

exit:
	return rv;
}
