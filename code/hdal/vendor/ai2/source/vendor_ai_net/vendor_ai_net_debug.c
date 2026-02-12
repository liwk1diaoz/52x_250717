/**
	@brief Source file of vendor sdk debug.

	@file vendor_ai_net_debug.c

	@ingroup vendor_ai_net_debug

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
#include "kwrap/nvt_type.h"
#include "kwrap/error_no.h"
#include "vendor_ai_net_debug.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_net_group.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "vendor_common.h" // for vendor_common_mem_cache_sync()
#include "vendor_ai_comm_flow.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

// Output ANSI color
#define _ANSI_BN_ 	"\x1B[1;30m" 	//~N -- for Bright-Gray string      "30"
#define _ANSI_BR_ 	"\x1B[1;31m" 	//~R -- for Bright-Red string       "31"
#define _ANSI_BG_ 	"\x1B[1;32m" 	//~G -- for Bright-Green string     "32"
#define _ANSI_BY_ 	"\x1B[1;33m" 	//~Y -- for Bright-Yellow string    "33"
#define _ANSI_BB_ 	"\x1B[1;34m" 	//~B -- for Bright-Blue string      "34"
#define _ANSI_BM_ 	"\x1B[1;35m" 	//~M -- for Bright-Magenta string   "35"
#define _ANSI_BC_ 	"\x1B[1;36m" 	//~C -- for Bright-Cyan string      "36"
#define _ANSI_BW_ 	"\x1B[1;37m" 	//~W -- for Bright-White string     "37"
#define _ANSI_N_ 		"\x1B[0;30m" 	//^N -- for Gray (Normal) string    "30"
#define _ANSI_R_ 		"\x1B[0;31m" 	//^R -- for Red string              "31"
#define _ANSI_G_ 		"\x1B[0;32m" 	//^G -- for Green string            "32"
#define _ANSI_Y_ 		"\x1B[0;33m" 	//^Y -- for Yellow string           "33"
#define _ANSI_B_ 		"\x1B[0;34m" 	//^B -- for Blue string             "34"
#define _ANSI_M_ 		"\x1B[0;35m" 	//^M -- for Magenta string          "35"
#define _ANSI_C_ 		"\x1B[0;36m" 	//^C -- for Cyan string             "36"
#define _ANSI_W_ 		"\x1B[0;37m" 	//^W -- for White string            "37"
#define _ANSI_0_		"\x1B[0m" 	//(Reset)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                         */
/*-----------------------------------------------------------------------------*/

static UINT32 g_all_trace = 0;
#if (NET_DBG_HEAP == 1)
static UINT32 fix_proc_trace[MAX_PROC_CNT] = {0};
#endif
static UINT32 *g_proc_trace;
extern UINT32 g_ai_support_net_max;
static CHAR msg_f1[] = _ANSI_W_"ai: \"proc[%d]\": %s";
static CHAR msg_g1[] = _ANSI_C_"ai: \"proc[%d]\": %s";
static CHAR msg_j1[] = _ANSI_Y_"ai: \"proc[%d]\": %s";
static CHAR msg_b1[] = _ANSI_M_"ai: \"proc[%d]\": %s";
static CHAR msg_p1[] = _ANSI_G_"ai: \"proc[%d]\": %s";
static CHAR msg_r1[] = _ANSI_R_"ai: \"proc[%d]\": %s";

UINT32 vendor_ai_net_get_trace(UINT32 proc_id)
{
	if (g_proc_trace) return g_proc_trace[proc_id];
	return 0;
}


void vendor_ai_net_trace(UINT32 proc_id, UINT32 class_bits, const char *fmtstr, ...)
{
	UINT32 trace;
	if (proc_id == 0xffffffff) trace = g_all_trace;
	else trace = g_proc_trace[proc_id];
	
	if (trace & class_bits) {
		
		CHAR new_fmtstr[256] = {0};
		va_list marker;

		if (class_bits & AI_FLOW)
			snprintf(new_fmtstr, 256, msg_f1, proc_id, fmtstr);
		if (class_bits & AI_GRP)
			snprintf(new_fmtstr, 256, msg_g1, proc_id, fmtstr);
		if (class_bits & AI_JOB)
			snprintf(new_fmtstr, 256, msg_j1, proc_id, fmtstr);
		if (class_bits & AI_BUF)
			snprintf(new_fmtstr, 256, msg_b1, proc_id, fmtstr);
		if (class_bits & AI_PERF)
			snprintf(new_fmtstr, 256, msg_p1, proc_id, fmtstr);
		if (class_bits & AI_RES)
			snprintf(new_fmtstr, 256, msg_r1, proc_id, fmtstr);

		va_start(marker, fmtstr);

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
		//coverity[dereference]
		debug_vprintf(new_fmtstr, marker);
#else
//#ifdef __KERNEL__
		debug_vprintf(new_fmtstr, marker);
//#else
//		if (marker) {
//			debug_vprintf(new_fmtstr, marker);
//		}
//#endif
#endif

		va_end(marker);
	}
}

void vendor_ai_net_set_trace_class(UINT32 proc_id, UINT32 class_mask)
{
	if (proc_id == 0xffffffff) {
		g_all_trace = class_mask;
		printf("ai - all.trace = %08x\r\n", (unsigned int)g_all_trace);
	} else {
		g_proc_trace[proc_id] = class_mask;
		printf("ai - proc[%lu].trace = %08x\r\n", proc_id, (unsigned int)g_proc_trace[proc_id]);
	}
}

INT _cvt_trig_mode_name(NN_MODE mode, CHAR *p_ret_string, INT max_str_len)
{
	switch (mode) {
		case NN_CONV:           snprintf(p_ret_string, max_str_len, "CONV");             break;
		case NN_DECONV:         snprintf(p_ret_string, max_str_len, "DECONV");           break;
		case NN_MATMUL:         snprintf(p_ret_string, max_str_len, "MATMUL");           break;
		case NN_SVM:            snprintf(p_ret_string, max_str_len, "SVM");              break;
		case NN_ROIPOOLING:     snprintf(p_ret_string, max_str_len, "ROIPOLL");          break;
		case NN_ELTWISE:        snprintf(p_ret_string, max_str_len, "ELTWISE");          break;
		case NN_REORGANIZATION: snprintf(p_ret_string, max_str_len, "REORGAN");          break;
		case NN_RESHAPE:        snprintf(p_ret_string, max_str_len, "RESHAPE");          break;
		case NN_PROPOSAL:       snprintf(p_ret_string, max_str_len, "PROPOSAL");         break;
		case NN_POSTPROC:       snprintf(p_ret_string, max_str_len, "POSTPROC");         break;
		case NN_SOFTMAX:        snprintf(p_ret_string, max_str_len, "SOFTMAX");          break;
		case NN_FC:             snprintf(p_ret_string, max_str_len, "FC");               break;
		case NN_PREPROC:        snprintf(p_ret_string, max_str_len, "PREPROC");          break;
		case NN_FC_POST:        snprintf(p_ret_string, max_str_len, "FC_POST");          break;
		case NN_POOL:           snprintf(p_ret_string, max_str_len, "POOL");             break;
		case NN_BNSCALE:        snprintf(p_ret_string, max_str_len, "BNSCALE");          break;
		case NN_CUSTOMER:       snprintf(p_ret_string, max_str_len, "CUSTOMER");         break;
		case NN_ANCHOR:         snprintf(p_ret_string, max_str_len, "ANCHOR");           break;
		case NN_UPSAMPLE:       snprintf(p_ret_string, max_str_len, "UPSAMPLE");         break;
		//case NN_CUSTOMER:       snprintf(p_ret_string, max_str_len, "CUSTOMER");         break;
		case NN_SCALEUP:        snprintf(p_ret_string, max_str_len, "SCALEUP");          break;
#if (USE_NEON || (!CNN_25_MATLAB))
		case NN_PRELU:          snprintf(p_ret_string, max_str_len, "PRELU");            break;
		case NN_SIGMOID:        snprintf(p_ret_string, max_str_len, "SIGMOID");          break;
		case NN_PRIORBOX:       snprintf(p_ret_string, max_str_len, "PRIORBOX");         break;
		case NN_DETOUT:         snprintf(p_ret_string, max_str_len, "DETOUT");           break;
#endif
#if AI_V4
		case NN_DEPTHWISE:      snprintf(p_ret_string, max_str_len, "DEPWISE");          break;
#endif
        case NN_FP2FIX:         snprintf(p_ret_string, max_str_len, "FP2FIX");           break;
        case NN_LSTM:           snprintf(p_ret_string, max_str_len, "LSTM");             break;
        case NN_REVERSE:        snprintf(p_ret_string, max_str_len, "REVERSE");          break;
        case NN_NORM:           snprintf(p_ret_string, max_str_len, "NORM");             break;
		case NN_BN:             snprintf(p_ret_string, max_str_len, "BN");             break;
		case NN_CORR:           snprintf(p_ret_string, max_str_len, "CORR");             break;
		case NN_LAYER_NORMALIZATION:           snprintf(p_ret_string, max_str_len, "NN_LAYER_NORMALIZATION");             break; 
#if NN_DLI
		case NN_DLI_SQRT:       snprintf(p_ret_string, max_str_len, "NN_DLI_SQRT");      break;
		case NN_DLI_DIV:        snprintf(p_ret_string, max_str_len, "NN_DLI_DIV");       break;
		case NN_DLI_EXP:        snprintf(p_ret_string, max_str_len, "NN_DLI_EXP");       break;
		case NN_DLI_RESIZE:     snprintf(p_ret_string, max_str_len, "NN_DLI_RESIZE");    break;
		case NN_DLI_LOG:        snprintf(p_ret_string, max_str_len, "NN_DLI_LOG");       break;
		case NN_DLI_POW:        snprintf(p_ret_string, max_str_len, "NN_DLI_POW");       break;
		case NN_DLI_SIN:        snprintf(p_ret_string, max_str_len, "NN_DLI_SIN");       break;
		case NN_DLI_EQUAL:      snprintf(p_ret_string, max_str_len, "NN_DLI_EQUAL");     break;
		case NN_DLI_GREATER:    snprintf(p_ret_string, max_str_len, "NN_DLI_GREATER");   break;
		case NN_DLI_LESS:       snprintf(p_ret_string, max_str_len, "NN_DLI_LESS");      break;
		case NN_DLI_FLOOR:      snprintf(p_ret_string, max_str_len, "NN_DLI_FLOOR");     break;
		case NN_DLI_ROUND:      snprintf(p_ret_string, max_str_len, "NN_DLI_ROUND");     break;
		case NN_DLI_SOFTMAX:    snprintf(p_ret_string, max_str_len, "NN_DLI_SOFTMAX");   break;
#endif
		default:
			snprintf(p_ret_string, max_str_len, "xxx");   break;
			return (-1);
	}
	return 0;
}


INT _cvt_trig_src_name(NN_GEN_TRIG_SRC src, CHAR *p_ret_string, INT max_str_len)
{
	switch (src) {
		case NN_GEN_TRIG_APP_AI_DRV: snprintf(p_ret_string, max_str_len, "APP");   break;
		case NN_GEN_TRIG_LL_AI_DRV:  snprintf(p_ret_string, max_str_len, "LL");    break;
		case NN_GEN_TRIG_COMMON:     snprintf(p_ret_string, max_str_len, "COMM");  break;
		default:
			snprintf(p_ret_string, max_str_len, "xxx");   break;
			return (-1);
	}
	return 0;
}

INT _cvt_eng_name(NN_GEN_ENG_TYPE eng, CHAR *p_ret_string, INT max_str_len)
{
	switch (eng) {
		case NN_GEN_ENG_VPE:   snprintf(p_ret_string, max_str_len, "VPE");   break;
		case NN_GEN_ENG_CNN:   snprintf(p_ret_string, max_str_len, "CNN");   break;
		case NN_GEN_ENG_CNN2:  snprintf(p_ret_string, max_str_len, "CNN2");  break;
		case NN_GEN_ENG_NUE:   snprintf(p_ret_string, max_str_len, "NUE");   break;
		case NN_GEN_ENG_NUE2:  snprintf(p_ret_string, max_str_len, "NUE2");  break;
		case NN_GEN_ENG_CPU:   snprintf(p_ret_string, max_str_len, "CPU");   break;
		case NN_GEN_ENG_DSP:   snprintf(p_ret_string, max_str_len, "DSP");   break;
		case NN_GEN_ENG_UNKNOWN:   snprintf(p_ret_string, max_str_len, "???");   break;
		default:
			snprintf(p_ret_string, max_str_len, "xxx");   break;
			return (-1);
	}
	return 0;
}

UINT32 _cvt_user_va(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 va)
{
	if (space == NET_DBG_SPACE_USER) {
		return va;
	} else if (space == NET_DBG_SPACE_KERL){
		UINT32 pa = vendor_ais_kerl_parm_va2pa(va, proc_id);
		return (pa-p_mem_manager->kerl_parm.pa)+p_mem_manager->kerl_parm.va;
	} else {
		return 0;
	}
}

HD_RESULT _vendor_ai_net_debug_dump_mem_manager(FILE *fd, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	fprintf(fd, "mem_mamager, user_parm.va = 0x%08x\n",  (UINT)p_mem_manager->user_parm.va);
	fprintf(fd, "mem_mamager, user_model.va = 0x%08x\n", (UINT)p_mem_manager->user_model.va);
	fprintf(fd, "mem_mamager, user_buff.va = 0x%08x\n",  (UINT)p_mem_manager->user_buff.va);
	fprintf(fd, "mem_mamager, kerl_parm.va = 0x%08x\n",  (UINT)p_mem_manager->kerl_parm.va);
	fprintf(fd, "mem_mamager, user_parm.pa = 0x%08x\n",  (UINT)p_mem_manager->user_parm.pa);
	fprintf(fd, "mem_mamager, user_model.pa = 0x%08x\n", (UINT)p_mem_manager->user_model.pa);
	fprintf(fd, "mem_mamager, user_buff.pa = 0x%08x\n",  (UINT)p_mem_manager->user_buff.pa);
	fprintf(fd, "mem_mamager, kerl_parm.pa = 0x%08x\n\n",(UINT)p_mem_manager->kerl_parm.pa);

	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_dump_iomem(NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd) 
{
	NN_GEN_MODEL_HEAD *p_head;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	UINT32 idx = 0, idx2 = 0;
	UINT32 max_buf_sz = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	//dump mem_manager
	_vendor_ai_net_debug_dump_mem_manager(fd, p_mem_manager);

	p_head = net_info.p_head;
#if CNN_25_MATLAB
	p_io_mem = net_info.p_io_mem;
#endif
	{
		fprintf(fd, "[IOMEM]\nmode_ctrl_num = %lu, layer_num = %lu, parm_size = %lu, model_size = %lu, io_buf_size = %lu\n", 
			p_head->mode_ctrl_num,
			p_head->layer_num,
			p_head->parm_size,
			p_head->model_size,
			p_head->io_buff_size);
	}
#if CNN_25_MATLAB
	for (idx = 0; idx < p_head->layer_num; idx++) {
		fprintf(fd, "---- Layer(%lu) ----\n", idx);
		for (idx2 = 0; idx2 < NN_IMEM_NUM; idx2++) {
			fprintf(fd, "  SAI[%lu] = %8lu, pa = 0x%08x, va = 0x%08x\n", idx2, p_io_mem[idx].SAI[idx2].size, (UINT)p_io_mem[idx].SAI[idx2].pa, (UINT)p_io_mem[idx].SAI[idx2].va);
			if (max_buf_sz < p_io_mem[idx].SAI[idx2].size) {
				max_buf_sz = p_io_mem[idx].SAI[idx2].size;
			}
		}
		for (idx2 = 0; idx2 < NN_OMEM_NUM; idx2++) {
			fprintf(fd, "  SAO[%lu] = %8lu, pa = 0x%08x, va = 0x%08x\n", idx2, p_io_mem[idx].SAO[idx2].size, (UINT)p_io_mem[idx].SAO[idx2].pa, (UINT)p_io_mem[idx].SAO[idx2].va);
			if (max_buf_sz < p_io_mem[idx].SAO[idx2].size) {
				max_buf_sz = p_io_mem[idx].SAO[idx2].size;
			}
		}
	}
#else
	{
		UINT32 imem_cnt=0, omem_cnt=0;
		NN_DATA *p_sai=NULL, *p_sao=NULL;
		UINT32 last_layer_id = (UINT32)(-1);
		NN_GEN_MODE_CTRL *p_mctrl = net_info.p_mctrl;

		for (idx = 0; idx < p_head->mode_ctrl_num; idx++) {
			if (p_mctrl[idx].layer_index == last_layer_id) {
				continue;
			} else {
				last_layer_id = p_mctrl[idx].layer_index;
			}
			imem_cnt = p_mctrl[idx].iomem.imem_cnt;
			omem_cnt = p_mctrl[idx].iomem.omem_cnt;
			p_sai    = (NN_DATA *)(p_mctrl[idx].iomem.imem_addr);
			p_sao    = (NN_DATA *)(p_mctrl[idx].iomem.omem_addr);
			
			fprintf(fd, "---- Layer(%lu) ----\n", p_mctrl[idx].layer_index);
			for (idx2 = 0; idx2 < imem_cnt; idx2++) {
				fprintf(fd, "  SAI[%lu] = %8lu, pa = 0x%08x, va = 0x%08x\n", idx2, p_sai[idx2].size, (UINT)p_sai[idx2].pa, (UINT)p_sai[idx2].va);
				if (max_buf_sz < p_sai[idx2].size) {
					max_buf_sz = p_sai[idx2].size;
				}
			}
			for (idx2 = 0; idx2 < omem_cnt; idx2++) {
				fprintf(fd, "  SAO[%lu] = %8lu, pa = 0x%08x, va = 0x%08x\n", idx2, p_sao[idx2].size, (UINT)p_sao[idx2].pa, (UINT)p_sao[idx2].va);
				if (max_buf_sz < p_sao[idx2].size) {
					max_buf_sz = p_sao[idx2].size;
				}
			}
		}
	}
#endif
	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_dump_mctrl(NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd, UINT32 fixed_offset)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}
	
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	//dump mem_manager
	_vendor_ai_net_debug_dump_mem_manager(fd, p_mem_manager);

	p_head = net_info.p_head;

	{
		fprintf(fd, "[MCTRL]\nmode_ctrl_num = %lu, layer_num = %lu, parm_size = %lu, model_size = %lu, io_buf_size = %lu\n", 
			p_head->mode_ctrl_num,
			p_head->layer_num,
			p_head->parm_size,
			p_head->model_size,
			p_head->io_buff_size);
	}
	
	{	
		NN_GEN_MODE_CTRL *p_mctrl = net_info.p_mctrl;
		CHAR  src_name[8];
		CHAR  eng_name[8];
		CHAR  mode_name[10];
#if CNN_528_PSW
#if CNN_25_MATLAB
		fprintf(fd, "==================================================================================================================================================================================================================================================================================\n");
		fprintf(fd, "mc_idx  trig_src        eng        mode            layer_index  nn_layer_index        addr      size  tot_trig_eng_times  i_buf[0]  i_buf[1]  i_buf[2]  o_buf[0]  o_buf[1]  o_size[0]  o_size[1]  stri_in[0]  stri_in[1]  stri_in[2]  stri_out[0]  stri_out[1]  idea_cycle  mc_idx\n");
		fprintf(fd, "==================================================================================================================================================================================================================================================================================\n");
#else
#if CNN_FMT_V4
		fprintf(fd, "===============================================================================================================================================================================================================================================================================\n");
		fprintf(fd, "mc_idx  trig_src        eng        mode            layer_index  nn_layer_index        addr      size  tot_trig_eng_times  ibuf_addr  ibuf_cnt   obuf_addr  obuf_cnt   imem_addr  imem_cnt   omem_addr  omem_cnt  in1_src  is_img  in_fmt  out_fmt  preserve  idea_cycle  mc_idx\n");
		fprintf(fd, "===============================================================================================================================================================================================================================================================================\n");
#else
#if CNN_CGEN_NEW_TMP_BUF
		fprintf(fd, "============================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================\n");
		fprintf(fd, "mc_idx  trig_src        eng        mode            layer_index  nn_layer_index        addr      size  tot_trig_eng_times  i_buf[0]  i_buf[1]  i_buf[2]  i_buf[3]  o_buf[0]  o_buf[1]  o_buf[2]  o_buf[3]  o_buf[4]  o_buf[5]  o_buf[6]  o_buf[7]  o_size[0]  o_size[1]  o_size[2]  o_size[3]  o_size[4]  o_size[5]  o_size[6]  o_size[7]  stri_in[0]  stri_in[1]  stri_in[2]  stri_in[3]  stri_out[0]  stri_out[1]  stri_out[2]  stri_out[3]  stri_out[4]  stri_out[5]  stri_out[6]  stri_out[7]   imem_addr  imem_cnt   omem_addr  omem_cnt  in1_src  is_img  in_fmt  out_fmt  preserve  idea_cycle  mc_idx\n");
		fprintf(fd, "============================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================\n");
#else
		fprintf(fd, "==========================================================================================================================================================================================================================================================================================================================================================================\n");
		fprintf(fd, "mc_idx  trig_src        eng        mode            layer_index  nn_layer_index        addr      size  tot_trig_eng_times  i_buf[0]  i_buf[1]  i_buf[2]  o_buf[0]  o_buf[1]  o_size[0]  o_size[1]  stri_in[0]  stri_in[1]  stri_in[2]  stri_out[0]  stri_out[1]   imem_addr  imem_cnt   omem_addr  omem_cnt  in1_src  is_img  in_fmt  out_fmt  preserve  idea_cycle  mc_idx\n");
		fprintf(fd, "===========================================================================================================================================================================================================================================================================================================================================================================\n");
#endif
#endif//CNN_FMT_V4
#endif
#else
		fprintf(fd, "========================================================================================================================\n");
		fprintf(fd, "mc_idx  trig_src        eng        mode            layer_index  nn_layer_index        addr      size  tot_trig_eng_times\n");
		fprintf(fd, "========================================================================================================================\n");
#endif
		for (idx = 0; idx < p_head->mode_ctrl_num; idx ++) {
			_cvt_trig_src_name(p_mctrl[idx].trig_src, src_name, 8);
			_cvt_eng_name(p_mctrl[idx].eng, eng_name, 8);
			_cvt_trig_mode_name(p_mctrl[idx].mode, mode_name, 10);
#if CNN_528_PSW
#if CNN_25_MATLAB
			fprintf(fd, "%6lu  %8u(%-4s)  %3u(%-4s)  %4lu(%-8s)  %11lu  %14lu  0x%08x  %8lu  %18lu  %8ld  %8ld  %8ld  %8ld  %8ld  %9ld  %9ld  0x%08x  0x%08x  0x%08x   0x%08x   0x%08x  %10lu  %6lu\n",
#else
#if CNN_FMT_V4
			fprintf(fd, "%6lu  %8u(%-4s)  %3u(%-4s)  %4lu(%-8s)  %11lu  %14lu  0x%08x  %8lu  %18lu  0x%08x  %8lu  0x%08x  %8lu  0x%08x  %8lu  0x%08x  %8lu  %7lu  %6lu  %6lu  %6lu  %8lu  %10lu  %6lu\n",
#else
#if CNN_CGEN_NEW_TMP_BUF
			fprintf(fd, "%6lu  %8u(%-4s)  %3u(%-4s)  %4lu(%-8s)  %11lu  %14lu  0x%08x  %8lu  %18lu  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %8ld  %9ld  %9ld  %9ld  %9ld  %9ld  %9ld  %9ld  %9ld  0x%08x  0x%08x  0x%08x  0x%08x   0x%08x   0x%08x   0x%08x   0x%08x   0x%08x   0x%08x   0x%08x   0x%08x  0x%08x  %8lu  0x%08x  %8lu  %7lu  %6lu  %6lu  %6lu  %8lu  %10lu  %6lu\n",
#else
			fprintf(fd, "%6lu  %8u(%-4s)  %3u(%-4s)  %4lu(%-8s)  %11lu  %14lu  0x%08x  %8lu  %18lu  %8ld  %8ld  %8ld  %8ld  %8ld  %9ld  %9ld  0x%08x  0x%08x  0x%08x   0x%08x   0x%08x  0x%08x  %8lu  0x%08x  %8lu  %7lu  %6lu  %6lu  %6lu  %8lu  %10lu  %6lu\n",
#endif
#endif//CNN_FMT_V4
#endif//CNN_25_MATLAB
#else
			fprintf(fd, "%6lu  %8u(%-4s)  %3u(%-4s)  %4lu(%-8s)  %11lu  %14lu  0x%08x  %8lu  %18lu\n",
#endif
				idx,
				p_mctrl[idx].trig_src,
				src_name,
				p_mctrl[idx].eng,
				eng_name,
				p_mctrl[idx].mode,
				mode_name,
				p_mctrl[idx].layer_index,
				p_mctrl[idx].nn_layer_index,
				(UINT)p_mctrl[idx].addr,
				p_mctrl[idx].size,
#if CNN_528_PSW
				p_mctrl[idx].tot_trig_eng_times,
#if CNN_FMT_V4
				(UINT)(p_mctrl[idx].in_bufinfo_addr - fixed_offset),
				p_mctrl[idx].in_bufinfo_cnt,
				(UINT)(p_mctrl[idx].out_bufinfo_addr - fixed_offset),
				p_mctrl[idx].out_bufinfo_cnt,
#else
				(INT32)IN_BUF_INDEX(&p_mctrl[idx], 0),  // p_mctrl[idx].in_buf_index[0],
				(INT32)IN_BUF_INDEX(&p_mctrl[idx], 1),  // p_mctrl[idx].in_buf_index[1],
				(INT32)IN_BUF_INDEX(&p_mctrl[idx], 2),  // p_mctrl[idx].in_buf_index[2],
#if CNN_CGEN_NEW_TMP_BUF
				(INT32)IN_BUF_INDEX(&p_mctrl[idx], 3),  // p_mctrl[idx].in_buf_index[3],
#endif
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 0), // p_mctrl[idx].out_buf_index[0],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 1), // p_mctrl[idx].out_buf_index[1],
#if CNN_CGEN_NEW_TMP_BUF
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 2), // p_mctrl[idx].out_buf_index[2],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 3), // p_mctrl[idx].out_buf_index[3],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 4), // p_mctrl[idx].out_buf_index[4],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 5), // p_mctrl[idx].out_buf_index[5],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 6), // p_mctrl[idx].out_buf_index[6],
				(INT32)OUT_BUF_INDEX(&p_mctrl[idx], 7), // p_mctrl[idx].out_buf_index[7],
#endif
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 0),  // p_mctrl[idx].output_buffsize[0],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 1),  // p_mctrl[idx].output_buffsize[1],
#if CNN_CGEN_NEW_TMP_BUF
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 2),  // p_mctrl[idx].output_buffsize[2],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 3),  // p_mctrl[idx].output_buffsize[3],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 4),  // p_mctrl[idx].output_buffsize[4],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 5),  // p_mctrl[idx].output_buffsize[5],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 6),  // p_mctrl[idx].output_buffsize[6],
				(INT32)OUT_BUF_SIZE(&p_mctrl[idx], 7),  // p_mctrl[idx].output_buffsize[7],
#endif
				(UINT)IN_BUF_OFFSET(&p_mctrl[idx], 0),  // p_mctrl[idx].stripe_inaddr[0],
				(UINT)IN_BUF_OFFSET(&p_mctrl[idx], 1),  // p_mctrl[idx].stripe_inaddr[1],
				(UINT)IN_BUF_OFFSET(&p_mctrl[idx], 2),  // p_mctrl[idx].stripe_inaddr[2],
#if CNN_CGEN_NEW_TMP_BUF
				(UINT)IN_BUF_OFFSET(&p_mctrl[idx], 3),  // p_mctrl[idx].stripe_inaddr[3],
#endif
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 0), // p_mctrl[idx].stripe_outaddr[0],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 1), // p_mctrl[idx].stripe_outaddr[1],
#if CNN_CGEN_NEW_TMP_BUF
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 2), // p_mctrl[idx].stripe_outaddr[2],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 3), // p_mctrl[idx].stripe_outaddr[3],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 4), // p_mctrl[idx].stripe_outaddr[4],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 5), // p_mctrl[idx].stripe_outaddr[5],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 6), // p_mctrl[idx].stripe_outaddr[6],
				(UINT)OUT_BUF_OFFSET(&p_mctrl[idx], 7), // p_mctrl[idx].stripe_outaddr[7],
#endif
#endif//CNN_FMT_V4
#if !CNN_25_MATLAB
				(UINT)(p_mctrl[idx].iomem.imem_addr - fixed_offset),
				p_mctrl[idx].iomem.imem_cnt,
				(UINT)(p_mctrl[idx].iomem.omem_addr - fixed_offset),
				p_mctrl[idx].iomem.omem_cnt,
				(UINT32)IN_BUF_ATTR_GET(&p_mctrl[idx],  NN_IN_BUF_ATTR_ELTWISE_IN_SRC),   //(UINT32)p_mctrl[idx].eltwise_in1_src,
				(UINT32)IN_BUF_ATTR_GET(&p_mctrl[idx],  NN_IN_BUF_ATTR_CONV_IN_ISIMG),    //(UINT32)p_mctrl[idx].cnn_in_isimg,
				(UINT32)IN_BUF_ATTR_GET(&p_mctrl[idx],  NN_IN_BUF_ATTR_PREPROC_IN_FMT),   //(UINT32)p_mctrl[idx].preproc_in_fmt,
				(UINT32)OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PREPROC_OUT_FMT), //(UINT32)p_mctrl[idx].preproc_out_fmt,
				(UINT32)OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PRESERVE),        //(UINT32)p_mctrl[idx].is_preserve,
#endif
				p_mctrl[idx].idea_cycle,
				idx
#else
				p_mctrl[idx].tot_trig_eng_times
#endif
				);
		}
	}

	// bufinfo (buf id/size/offset)
#if CNN_FMT_V4
	{
		NN_GEN_MODE_CTRL *p_mctrl = net_info.p_mctrl;
		UINT32 buf_num=0, idx2=0;

		fprintf(fd, "\n\n");
		fprintf(fd, "========================================================================================================================\n");
		fprintf(fd, "mc_idx  i_buf_num  buf_id      offset       size  is_tmp\n");
		fprintf(fd, "        o_buf_num  buf_id      offset       size  is_tmp\n");
		fprintf(fd, "========================================================================================================================\n");

		for (idx = 0; idx < p_head->mode_ctrl_num; idx ++) {
			// in_buf_info
			buf_num = IN_BUF_NUM_REAL(&p_mctrl[idx]);
			fprintf(fd, "%6lu  %9lu  =>\n", idx, (UINT32)buf_num);

			for (idx2 = 0; idx2 < buf_num; idx2++) {
				fprintf(fd, "                   %6ld  0x%08x\n", (INT32)IN_BUF_INDEX(&p_mctrl[idx], idx2), (UINT)IN_BUF_OFFSET(&p_mctrl[idx], idx2));
			}
			fprintf(fd, "\n");

			// out_buf_info
			buf_num = OUT_BUF_NUM_REAL(&p_mctrl[idx]);
			fprintf(fd, "        %9lu  =>\n", (UINT32)buf_num);

			for (idx2 = 0; idx2 < buf_num; idx2++) {
				fprintf(fd, "                   %6ld  0x%08x  %9ld  %6lu\n", (INT32)OUT_BUF_INDEX(&p_mctrl[idx], idx2), (UINT)OUT_BUF_OFFSET(&p_mctrl[idx], idx2), (INT32)OUT_BUF_SIZE(&p_mctrl[idx], idx2), (UINT32)OUT_BUF_IS_TMP(&p_mctrl[idx], idx2));
			}

			fprintf(fd, "----------------------------------------------------------\n");
		}
	}
#endif

	// graph
#if CNN_528_PSW
	{
		NN_GEN_MODE_CTRL *p_mctrl = net_info.p_mctrl;
		UINT32 id_list_start_addr = (UINT32)net_info.p_id_list;
		UINT32 start_ofs = 0, end_ofs = 0, list_ofs = 0, list_num = 0, idx2 = 0;

		fprintf(fd, "\n\n");
		fprintf(fd, "========================================================================================================================\n");
		fprintf(fd, "mc_idx  prev_num  prev_idx_addr  prev_layer_list\n");
		fprintf(fd, "        next_num  next_idx_addr  next_layer_list\n");
		fprintf(fd, "========================================================================================================================\n");

		for (idx = 0; idx < p_head->mode_ctrl_num; idx ++) {
			// prev
			list_num = p_mctrl[idx].prev_num;
			list_ofs = p_mctrl[idx].prev_layer_idx_addr;

			fprintf(fd, "%6lu  %8lu     0x%08x  ", idx, list_num, (UINT)list_ofs);

			start_ofs = list_ofs;
			end_ofs   = list_ofs + sizeof(UINT32)*list_num;
			for (idx2 = start_ofs; idx2 < end_ofs; idx2 += sizeof(UINT32)) {
				fprintf(fd, "%lu  ", *(UINT32 *)(id_list_start_addr + idx2));
			}
			fprintf(fd, "\n");

			// next
			list_num = p_mctrl[idx].next_num;
			list_ofs = p_mctrl[idx].next_layer_idx_addr;

			fprintf(fd, "        %8lu     0x%08x  ", list_num, (UINT)list_ofs);

			start_ofs = list_ofs;
			end_ofs   = list_ofs + sizeof(UINT32)*list_num;
			for (idx2 = start_ofs; idx2 < end_ofs; idx2 += sizeof(UINT32)) {
				fprintf(fd, "%lu  ", *(UINT32 *)(id_list_start_addr + idx2));
			}

			fprintf(fd, "\n------------------------------------------------\n");
		}
	}
#endif
	return HD_OK;
}

VOID _vendor_ai_net_debug_dump_aiparm_app(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_manager, FILE *fd, UINT32 head_va, UINT32 parm_va_ofs)
{
	UINT32 ai_head_addr = _cvt_user_va(proc_id, space, p_mem_manager, head_va);
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)ai_head_addr;
	UINT32 ai_parm_addr = _cvt_user_va(proc_id, space, p_mem_manager, p_head->parm_addr);
	if (ai_parm_addr < parm_va_ofs) ai_parm_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

	fprintf(fd, "  [ModeCtrl->addr]parm_addr = 0x%08x => [AI_APP_HEAD] mode = %u, parm = (0x%08x, %lu), stripe = 0x%08x \n", (UINT)head_va, p_head->mode, (UINT)p_head->parm_addr, p_head->parm_size, (UINT)p_head->stripe_head_addr);
	
	
	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)ai_parm_addr;
			fprintf(fd, "      <NEURAL> in_addr = 0x%08x, out0_addr = 0x%08x, out1_addr = 0x%08x, in_interm_addr = 0x%08x, out_interm_addr = 0x%08x, tmp_buf_addr = 0x%08x, conv.weight_addr = 0x%08x, conv.bias_addr = 0x%08x, conv.fcd.quanti_kmeans_addr = 0x%08x, conv.fcd.p_vlc_code = 0x%08x, conv.fcd.p_vlc_valid = 0x%08x, conv.fcd.p_vlc_ofs = 0x%08x, norm.bn_scl.bn_scale_addr = 0x%08x, elt.addr = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out0_addr, (UINT)p_parm->out1_addr, (UINT)p_parm->in_interm_addr, (UINT)p_parm->out_interm_addr, (UINT)p_parm->tmp_buf_addr, (UINT)p_parm->conv.weight_addr, (UINT)p_parm->conv.bias_addr, (UINT)p_parm->conv.fcd.quanti_kmeans_addr, (UINT)p_parm->conv.fcd.p_vlc_code, (UINT)p_parm->conv.fcd.p_vlc_valid, (UINT)p_parm->conv.fcd.p_vlc_ofs, (UINT)p_parm->norm.bn_scl.bn_scale_addr, (UINT)p_parm->elt.addr);
		}
		break;
	case AI_MODE_ROIPOOL: {
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)ai_parm_addr;
			fprintf(fd, "      <ROIPOOL> in_addr = 0x%08x, out_addr = 0x%08x, roi_ker.roi_addr = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out_addr, (UINT)p_parm->roi_ker.roi_addr);
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)ai_parm_addr;
			fprintf(fd, "      <SVM> in_addr = 0x%08x, out_addr = 0x%08x, svm_ker.sv_addr = 0x%08x, svm_ker.alpha_addr = 0x%08x, fcd.quanti_kmeans_addr = 0x%08x, fcd.p_vlc_code = 0x%08x, fcd.p_vlc_valid = 0x%08x, fcd.p_vlc_ofs = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out_addr, (UINT)p_parm->svm_ker.sv_addr, (UINT)p_parm->svm_ker.alpha_addr, (UINT)p_parm->fcd.quanti_kmeans_addr, (UINT)p_parm->fcd.p_vlc_code, (UINT)p_parm->fcd.p_vlc_valid, (UINT)p_parm->fcd.p_vlc_ofs);
		}
		break;
	case AI_MODE_FC: {
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)(ai_parm_addr);
			fprintf(fd, "      <FC> in_addr = 0x%08x, out_addr = 0x%08x, in_interm_addr = 0x%08x, out_interm_addr = 0x%08x, fc_ker.weight_addr = 0x%08x, fc_ker.bias_addr = 0x%08x, fcd.quanti_kmeans_addr = 0x%08x, fcd.p_vlc_code = 0x%08x, fcd.p_vlc_valid = 0x%08x, fcd.p_vlc_ofs = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out_addr, (UINT)p_parm->in_interm_addr, (UINT)p_parm->out_interm_addr, (UINT)p_parm->fc_ker.weight_addr, (UINT)p_parm->fc_ker.bias_addr, (UINT)p_parm->fcd.quanti_kmeans_addr, (UINT)p_parm->fcd.p_vlc_code, (UINT)p_parm->fcd.p_vlc_valid, (UINT)p_parm->fcd.p_vlc_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)ai_parm_addr;
			fprintf(fd, "      <PERMUTE> in_addr = 0x%08x, out_addr = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out_addr);
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)ai_parm_addr;
			fprintf(fd, "      <REORG> in_addr = 0x%08x, out_addr = 0x%08x\n", (UINT)p_parm->in_addr, (UINT)p_parm->out_addr);
		}
		break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)(ai_parm_addr);
			fprintf(fd, "      <PREPROC> in_addr[0] = 0x%08x, in_addr[1] = 0x%08x, in_addr[2] = 0x%08x, out_addr[0] = 0x%08x, out_addr[1] = 0x%08x, out_addr[2] = 0x%08x\n", (UINT)p_parm->in_addr[0], (UINT)p_parm->in_addr[1], (UINT)p_parm->in_addr[2], (UINT)p_parm->out_addr[0], (UINT)p_parm->out_addr[1], (UINT)p_parm->out_addr[2]);
		}
		break;
	default:
		break;
	}
}

VOID _vendor_ai_net_debug_dump_aiparm_ll(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_manager, FILE *fd, UINT32 head_va, UINT32 parm_va_ofs)
{
	UINT32 ai_head_addr = _cvt_user_va(proc_id, space, p_mem_manager, head_va);
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)ai_head_addr;
	UINT32 ai_parm_addr = _cvt_user_va(proc_id, space, p_mem_manager, p_head->parm_addr);
	UINT64 *p_ll_end_cmd;
	if (ai_parm_addr < parm_va_ofs) ai_parm_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it
	
	p_ll_end_cmd = (UINT64 *)(ai_parm_addr + p_head->parm_size - sizeof(UINT64));
	fprintf(fd, "  [ModeCtrl->addr]parm_addr = 0x%08x => [AI_LL_HEAD] mode = %u, parm = (0x%08x, %lu), eng = %u, ll_end = 0x%016llx\n", (UINT)head_va, p_head->mode, (UINT)p_head->parm_addr, p_head->parm_size, p_head->eng, *p_ll_end_cmd);

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		{
			CNN_LL_PARM *p_cnn_ll_parm = (CNN_LL_PARM*)ai_parm_addr;
			fprintf(fd, "      <CNN> input = 0x%08x, interm_in = 0x%08x, output[0] = 0x%08x, output[1] = 0x%08x, weight = 0x%08x, kmeans = 0x%08x, bias = 0x%08x\n", (UINT)p_cnn_ll_parm->input.bit.addr, (UINT)p_cnn_ll_parm->interm_in.bit.addr, (UINT)p_cnn_ll_parm->output[0].bit.addr, (UINT)p_cnn_ll_parm->output[1].bit.addr, (UINT)p_cnn_ll_parm->weight.bit.addr, (UINT)p_cnn_ll_parm->kmean.bit.addr, (UINT)p_cnn_ll_parm->bias_bnscal.bit.addr);
		}
		break;
	case AI_ENG_NUE:
		{
			NUE_LL_PARM *p_nue_ll_parm = (NUE_LL_PARM*)ai_parm_addr;
			fprintf(fd, "      <NUE> input = 0x%08x, elt_in = 0x%08x, roi_in = 0x%08x, output = 0x%08x, sv = 0x%08x, alpha = 0x%08x, rho = 0x%08x, kmean = 0x%08x\r\n", (UINT)p_nue_ll_parm->input.bit.addr, (UINT)p_nue_ll_parm->elt_in.bit.addr, (UINT)p_nue_ll_parm->roi_in.bit.addr, (UINT)p_nue_ll_parm->output.bit.addr, (UINT)p_nue_ll_parm->sv.bit.addr, (UINT)p_nue_ll_parm->alpha.bit.addr, (UINT)p_nue_ll_parm->rho.bit.addr, (UINT)p_nue_ll_parm->kmean.bit.addr);
		}
		break;
	case AI_ENG_NUE2:
		{
			NUE2_LL_PARM *p_nue2_ll_parm = (NUE2_LL_PARM*)ai_parm_addr;
			fprintf(fd, "      <NUE2> input[0] = 0x%08x, input[1] = 0x%08x, input[2] = 0x%08x, output[0] = 0x%08x, output[1] = 0x%08x, output[2] = 0x%08x\r\n", (UINT)p_nue2_ll_parm->input[0].bit.addr, (UINT)p_nue2_ll_parm->input[1].bit.addr, (UINT)p_nue2_ll_parm->input[2].bit.addr, (UINT)p_nue2_ll_parm->output[0].bit.addr, (UINT)p_nue2_ll_parm->output[1].bit.addr, (UINT)p_nue2_ll_parm->output[2].bit.addr);
		}
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}

}


VOID _vendor_ai_net_debug_dump_aiparm_comm(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_manager, FILE *fd, UINT32 head_va, NN_MODE mode)
{
	UINT32 ai_head_addr = _cvt_user_va(proc_id, space, p_mem_manager, head_va);
	fprintf(fd, "  [ModeCtrl->addr]parm_addr = 0x%08x => [COMM]\n", (UINT)head_va);
	
	switch (mode) {
	case NN_CONV:
	case NN_DECONV:
	case NN_MATMUL:
	case NN_BN:
	case NN_CORR:
		break;
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
        break;
	case NN_RESHAPE:
		{
			NN_PERMUTE_PARM *p_permute_parm = (NN_PERMUTE_PARM *)ai_head_addr;
			fprintf(fd, "      <RESHAPE> in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x\n", (UINT)p_permute_parm->in_addr, (UINT)p_permute_parm->out_addr, (UINT)p_permute_parm->tmp_addr);
		}
         break;
	case NN_PROPOSAL:
		{
			NN_CPU_PARM *p_cpu_parm = (NN_CPU_PARM *)ai_head_addr;
			fprintf(fd, "      <PROPOSAL> addr_in = 0x%08x, addr_out = 0x%08x\n", (UINT)p_cpu_parm->addr_in, (UINT)p_cpu_parm->addr_out);
		}
		break;

	case NN_POSTPROC:
		{
			NN_POSTPROC_PARM *p_postproc_parm = (NN_POSTPROC_PARM *)ai_head_addr;
#if AI_V4
			fprintf(fd, "      <POSTPROC> in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x, w(%lu), h(%lu), channel(%lu), batch(%lu), top_n(%lu)\n", (UINT)p_postproc_parm->in_addr, (UINT)p_postproc_parm->out_addr, (UINT)p_postproc_parm->tmp_addr, (UINT32)p_postproc_parm->shape.width, (UINT32)p_postproc_parm->shape.height, (UINT32)p_postproc_parm->shape.channel, (UINT32)p_postproc_parm->shape.batch_num, p_postproc_parm->top_n);
#else
			fprintf(fd, "      <POSTPROC> in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x, w(%lu), h(%lu), channel(%lu), batch(%lu), top_n(%lu)\n", (UINT)p_postproc_parm->in_addr, (UINT)p_postproc_parm->out_addr, (UINT)p_postproc_parm->tmp_addr, p_postproc_parm->width, p_postproc_parm->height, p_postproc_parm->channel, p_postproc_parm->batch_num, p_postproc_parm->top_n);
#endif
		}
		break;
	case NN_SOFTMAX:
		{
			NN_SOFTMAX_PARM *p_softmax_parm = (NN_SOFTMAX_PARM *)ai_head_addr;
#if (USE_NEON || (!CNN_25_MATLAB))
#if AI_V4
			fprintf(fd, "      <SOFTMAX> width = %lu, height = %lu, channel = %lu, batch_num = %lu, in_bit_fmt = (%d/%d/%d), out_bit_fmt = (%d/%d/%d), in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x\n", (UINT32)p_softmax_parm->shape.width, (UINT32)p_softmax_parm->shape.height, (UINT32)p_softmax_parm->shape.channel, (UINT32)p_softmax_parm->shape.batch_num, (int)p_softmax_parm->in_fmt.frac_bits, (int)p_softmax_parm->in_fmt.int_bits, (int)p_softmax_parm->in_fmt.sign_bits, (int)p_softmax_parm->out_fmt.frac_bits, (int)p_softmax_parm->out_fmt.int_bits, (int)p_softmax_parm->out_fmt.sign_bits, (UINT)p_softmax_parm->in_addr, (UINT)p_softmax_parm->out_addr, (UINT)p_softmax_parm->tmp_addr);
#else
			fprintf(fd, "      <SOFTMAX> width = %lu, height = %lu, channel = %lu, batch_num = %lu, in_bit_fmt = (%d/%d/%d), out_bit_fmt = (%d/%d/%d), in_addr = 0x%08x, out_addr = 0x%08x, in_trans_addr = 0x%08x, out_trans_addr = 0x%08x\n", (UINT32)p_softmax_parm->width, (UINT32)p_softmax_parm->height, (UINT32)p_softmax_parm->channel, (UINT32)p_softmax_parm->batch_num, (int)p_softmax_parm->in_bit_fmt.frac_bits, (int)p_softmax_parm->in_bit_fmt.int_bits, (int)p_softmax_parm->in_bit_fmt.sign_bits, (int)p_softmax_parm->out_bit_fmt.frac_bits, (int)p_softmax_parm->out_bit_fmt.int_bits, (int)p_softmax_parm->out_bit_fmt.sign_bits, (UINT)p_softmax_parm->in_addr, (UINT)p_softmax_parm->out_addr, (UINT)p_softmax_parm->in_trans_addr, (UINT)p_softmax_parm->out_trans_addr);
#endif
#else
			fprintf(fd, "      <SOFTMAX> width = %lu, height = %lu, channel = %lu, batch_num = %lu, in_bit_fmt = (%d/%d/%d), out_bit_fmt = (%d/%d/%d), in_addr = 0x%08x, out_addr = 0x%08x\n", (UINT32)p_softmax_parm->width, (UINT32)p_softmax_parm->height, (UINT32)p_softmax_parm->channel, (UINT32)p_softmax_parm->batch_num, (int)p_softmax_parm->in_bit_fmt.frac_bits, (int)p_softmax_parm->in_bit_fmt.int_bits, (int)p_softmax_parm->in_bit_fmt.sign_bits, (int)p_softmax_parm->out_bit_fmt.frac_bits, (int)p_softmax_parm->out_bit_fmt.int_bits, (int)p_softmax_parm->out_bit_fmt.sign_bits, (UINT)p_softmax_parm->in_addr, (UINT)p_softmax_parm->out_addr);
#endif
		}
		break;
/*
	case NN_PREPROC:
		{
			NN_PRE_PARM *p_pre_parm = (NN_PRE_PARM*)ai_head_addr;
			fprintf(fd, "      <PREPROC> in_addr = 0x%08x, out_addr = 0x%08x, interm_addr = 0x%08x, sub_img.plane_addr = 0x%08x\n", (UINT)p_pre_parm->in_addr, (UINT)p_pre_parm->out_addr, (UINT)p_pre_parm->interm_addr, (UINT)p_pre_parm->sub_img.plane_addr);
		}
		break;
*/		
	case NN_FC_POST:
		{
			NN_FC_POST_PARM *p_fc_post_parm = (NN_FC_POST_PARM*)ai_head_addr;
#if AI_V4
			fprintf(fd, "      <FC_POST> in_addr = 0x%08x, out_addr = 0x%08x, bias_addr = 0x%08x\n", (UINT)p_fc_post_parm->in_addr, (UINT)p_fc_post_parm->out_addr, (UINT)p_fc_post_parm->bias_addr);
#else
			fprintf(fd, "      <FC_POST> addr_in = 0x%08x, addr_out = 0x%08x, bias_addr = 0x%08x\n", (UINT)p_fc_post_parm->addr_in, (UINT)p_fc_post_parm->addr_out, (UINT)p_fc_post_parm->bias_addr);
#endif
		}
		break;
		
	case NN_POOL:
		{
			NN_POOL_PARM *p_pool_parm = (NN_POOL_PARM*)ai_head_addr;
			fprintf(fd, "      <POOL> in_addr = 0x%08x, out_addr = 0x%08x\n", (UINT)p_pool_parm->in_addr, (UINT)p_pool_parm->out_addr);
		}
		break;
		
	case NN_CUSTOMER:
		{
			NN_CUSTOM_PARM *p_cust_head = (NN_CUSTOM_PARM*)ai_head_addr;
#if CUST_SUPPORT_MULTI_IO
			fprintf(fd, "      <CUSTOMER> input_num = %d, output_num = %d, model_num = %d, temp_buf_addr = 0x%08x, temp_buf_size = %d, parm_size = %d\n", (int)p_cust_head->input_num, (int)p_cust_head->output_num, (int)p_cust_head->model_num, (UINT)p_cust_head->temp_buf_addr, (int)p_cust_head->temp_buf_size, (int)p_cust_head->parm_size);
			{
				UINT32 input_num     = p_cust_head->input_num;
				UINT32 output_num    = p_cust_head->output_num;
				UINT32 model_num     = p_cust_head->model_num;
				UINT32 i = 0;
				NN_DATA* input_info  = (NN_DATA*)(ai_head_addr + sizeof(NN_CUSTOM_PARM));
				NN_DATA* output_info = (NN_DATA*)(ai_head_addr + sizeof(NN_CUSTOM_PARM) + input_num*sizeof(NN_DATA));
				NN_DATA* model_info  = (NN_DATA*)(ai_head_addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num)*sizeof(NN_DATA));
				for (i = 0; i < input_num; i++) {
					fprintf(fd, "                 [   IN] = (0x%08x, 0x%08x, %8d, 0x%08x)\n", (UINT)input_info[i].pa, (UINT)input_info[i].va, (int)input_info[i].size, (UINT)(*(UINT32 *)&input_info[i].fmt));
				}
				for (i = 0; i < output_num; i++) {
					fprintf(fd, "                 [  OUT] = (0x%08x, 0x%08x, %8d, 0x%08x)\n", (UINT)output_info[i].pa, (UINT)output_info[i].va, (int)output_info[i].size, (UINT)(*(UINT32 *)&output_info[i].fmt));
				}
				for (i = 0; i < model_num; i++) {
					fprintf(fd, "                 [MODEL] = (0x%08x, 0x%08x, %8d, 0x%08x)\n", (UINT)model_info[i].pa, (UINT)model_info[i].va, (int)model_info[i].size, (UINT)(*(UINT32 *)&model_info[i].fmt));
				}
			}
#else
			fprintf(fd, "      <CUSTOMER> input.va = 0x%08x, output.va = 0x%08x, model.va = 0x%08x, input.pa = 0x%08x, output.pa = 0x%08x, param_size = %d\n", (UINT)p_cust_head->input.va, (UINT)p_cust_head->output.va, (UINT)p_cust_head->model.va, (UINT)p_cust_head->input.pa, (UINT)p_cust_head->output.pa, (int)p_cust_head->parm_size);
#endif
		}
		break;
		
	case NN_BNSCALE:
		{
			NN_BNSCALE_PARM *p_bn_scl_parm = (NN_BNSCALE_PARM*)ai_head_addr;
			fprintf(fd, "      <BNSCALE> in_addr = 0x%08x, out_addr = 0x%08x, mean_addr = 0x%08x, alpha_addr = 0x%08x, beta_addr = 0x%08x\n", (UINT)p_bn_scl_parm->in_addr, (UINT)p_bn_scl_parm->out_addr, (UINT)p_bn_scl_parm->mean_addr, (UINT)p_bn_scl_parm->alpha_addr, (UINT)p_bn_scl_parm->beta_addr);
		}
		break;
#if (USE_NEON || (!CNN_25_MATLAB))
	case NN_PRELU:
		break;
	case NN_SIGMOID:
		break;
	case NN_PRIORBOX:
		{
			#if CNN_CGEN_NEW_TMP_BUF
			NN_PRIORBOX_PARM *p_priorbox_parm = (NN_PRIORBOX_PARM *)ai_head_addr;
			#if AI_V4
			fprintf(fd, "      <PRIORBOX> in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x\n", (UINT)p_priorbox_parm->in_addr, (UINT)p_priorbox_parm->out_addr, (UINT)p_priorbox_parm->tmp_addr);
			#else
			fprintf(fd, "      <PRIORBOX> in_addr = 0x%08x, out_addr = 0x%08x, in_trans_addr = 0x%08x, out_trans_addr = 0x%08x\n", (UINT)p_priorbox_parm->in_addr, (UINT)p_priorbox_parm->out_addr, (UINT)p_priorbox_parm->in_trans_addr, (UINT)p_priorbox_parm->out_trans_addr);
			#endif
			#endif
		}
		break;
	case NN_DETOUT:
		{
			#if CNN_CGEN_NEW_TMP_BUF
			NN_DETOUT_PARM *p_detout_parm = (NN_DETOUT_PARM *)ai_head_addr;
			#if AI_V4
			fprintf(fd, "      <DETOUT> in_addr[0] = 0x%08x, in_addr[1] = 0x%08x, in_addr[2] = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x\n", (UINT)p_detout_parm->in_addr[0], (UINT)p_detout_parm->in_addr[1], (UINT)p_detout_parm->in_addr[2], (UINT)p_detout_parm->out_addr, (UINT)p_detout_parm->tmp_addr);
			#else
			fprintf(fd, "      <DETOUT> in_addr[0] = 0x%08x, in_addr[1] = 0x%08x, in_addr[2] = 0x%08x, out_addr = 0x%08x, in_trans_addr = 0x%08x, out_trans_addr = 0x%08x\n", (UINT)p_detout_parm->in_addr[0], (UINT)p_detout_parm->in_addr[1], (UINT)p_detout_parm->in_addr[2], (UINT)p_detout_parm->out_addr, (UINT)p_detout_parm->in_trans_addr, (UINT)p_detout_parm->out_trans_addr);
			#endif
			#endif
		}
		break;
    case NN_LSTM:
		{
			#if CNN_CGEN_NEW_TMP_BUF
			NN_LSTM_PARM *p_lstm_parm = (NN_LSTM_PARM *)ai_head_addr;
			fprintf(fd, "      <LSTM> in_addr0            = 0x%08x, in_addr1            = 0x%08x, out_addr            = 0x%08x, tmp_addr            = 0x%08x,  indicator_parm_addr = 0x%08x,\n" 
						"             feat_parm_addr[0]   = 0x%08x, feat_parm_addr[1]   = 0x%08x, feat_parm_addr[2]   = 0x%08x, feat_parm_addr[3]   = 0x%08x,\n"
						"             static_parm_addr[0] = 0x%08x, static_parm_addr[1] = 0x%08x, static_parm_addr[2] = 0x%08x, static_parm_addr[3] = 0x%08x,\n"
						"             hidden_parm_addr[0] = 0x%08x, hidden_parm_addr[1] = 0x%08x, hidden_parm_addr[2] = 0x%08x, hidden_parm_addr[3] = 0x%08x,\n"
						"             bias_parm_addr[0]   = 0x%08x, bias_parm_addr[1]   = 0x%08x, bias_parm_addr[2]   = 0x%08x, bias_parm_addr[3]   = 0x%08x\n", 
                (UINT)p_lstm_parm->in_addr0, (UINT)p_lstm_parm->in_addr1, (UINT)p_lstm_parm->out_addr, (UINT)p_lstm_parm->tmp_addr, (UINT)p_lstm_parm->indicator_parm_addr,
                (UINT)p_lstm_parm->feat_parm_addr[0], (UINT)p_lstm_parm->feat_parm_addr[1], (UINT)p_lstm_parm->feat_parm_addr[2], (UINT)p_lstm_parm->feat_parm_addr[3],
                (UINT)p_lstm_parm->static_parm_addr[0], (UINT)p_lstm_parm->static_parm_addr[1], (UINT)p_lstm_parm->static_parm_addr[2], (UINT)p_lstm_parm->static_parm_addr[3],
                (UINT)p_lstm_parm->hidden_parm_addr[0], (UINT)p_lstm_parm->hidden_parm_addr[1], (UINT)p_lstm_parm->hidden_parm_addr[2], (UINT)p_lstm_parm->hidden_parm_addr[3],
                (UINT)p_lstm_parm->bias_parm_addr[0], (UINT)p_lstm_parm->bias_parm_addr[1], (UINT)p_lstm_parm->bias_parm_addr[2], (UINT)p_lstm_parm->bias_parm_addr[3]);
			#endif
		}
		break;
    case NN_REVERSE:
		{
			NN_REVERSE_PARM *p_reverse_parm = (NN_REVERSE_PARM *)ai_head_addr;
			fprintf(fd, "      <REVERSE> in_addr = 0x%08x, out_addr = 0x%08x, tmp_addr = 0x%08x\n", (UINT)p_reverse_parm->in_addr, (UINT)p_reverse_parm->out_addr, (UINT)p_reverse_parm->tmp_addr);
		}
        break;
#endif
	default :
		break;
	}
}

HD_RESULT _vendor_ai_net_debug_dump_aiparm(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd) 
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 process_index = 0;
	UINT32 parm_va_ofs = 0;
	UINT32 ai_head_addr = 0;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	//dump mem_manager
	_vendor_ai_net_debug_dump_mem_manager(fd, p_mem_manager);

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		parm_va_ofs = ALIGN_CEIL_4(p_mem->va);
		ai_head_addr = p_mctrl[process_index].addr;
		if (ai_head_addr < parm_va_ofs) ai_head_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

		fprintf(fd, "  [%lu] ", process_index);
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			_vendor_ai_net_debug_dump_aiparm_app(proc_id, space, p_mem_manager, fd, ai_head_addr, parm_va_ofs);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			_vendor_ai_net_debug_dump_aiparm_ll(proc_id, space, p_mem_manager, fd, ai_head_addr, parm_va_ofs);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			_vendor_ai_net_debug_dump_aiparm_comm(proc_id, space, p_mem_manager, fd, ai_head_addr, (NN_MODE)p_mctrl[process_index].mode);
			break;
		}
	}

	return HD_OK;
}

VOID _vendor_ai_net_debug_dump_llcmd_detail(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_manager, FILE *fd, UINT32 head_va, UINT32 parm_va_ofs)
{
	UINT32 ai_head_addr = _cvt_user_va(proc_id, space, p_mem_manager, head_va);
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)ai_head_addr;
	UINT32 ai_parm_addr = _cvt_user_va(proc_id, space, p_mem_manager, p_head->parm_addr);
	UINT64 *p_ll_end_cmd, *p_ll_cmd_ptr;
	UINT32 i=0;
	if (ai_parm_addr < parm_va_ofs) ai_parm_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

	p_ll_end_cmd = (UINT64 *)(ai_parm_addr + p_head->parm_size - sizeof(UINT64));
	fprintf(fd, "  [ModeCtrl->addr]parm_addr = 0x%08x => [AI_LL_HEAD] mode = %u, parm = (0x%08x, %lu), eng = %u, ll_end = 0x%016llx\n", (UINT)head_va, p_head->mode, (UINT)p_head->parm_addr, p_head->parm_size, p_head->eng, *p_ll_end_cmd);

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		fprintf(fd, "      <CNN>\n");
		break;
	case AI_ENG_NUE:
		fprintf(fd, "      <NUE>\n");
		break;
	case AI_ENG_NUE2:
		fprintf(fd, "      <NUE2>\n");
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}

	for (i=0 ; i < p_head->parm_size ; i += sizeof(UINT64)) {
		p_ll_cmd_ptr = (UINT64 *)(ai_parm_addr + i);
		fprintf(fd, "      (%4d)  0x%016llx", (int)i, *p_ll_cmd_ptr);

		if ((*p_ll_cmd_ptr >> 61) == 4) {
			fprintf(fd, "   => cmd(UPD     ) , Reg(0x%03x) , Value(0x%08x), ByteEn(0x%1x), T(%d), O(%d)\n", (UINT)((*p_ll_cmd_ptr << 20) >> 52), (UINT)((*p_ll_cmd_ptr << 32) >> 32), (UINT)((*p_ll_cmd_ptr << 16) >> 60), (int)((*p_ll_cmd_ptr << 3) >> 63), (int)((*p_ll_cmd_ptr << 4) >> 63));
		} else if ((*p_ll_cmd_ptr >> 61) == 2) {
			fprintf(fd, "   => cmd(NEXT_UPD) , Addr(0x%08x)\n", (UINT)((*p_ll_cmd_ptr << 24) >> 32));
		} else if ((*p_ll_cmd_ptr >> 61) == 1) {
			fprintf(fd, "   => cmd(NEXT_LL ) , Addr(0x%08x), TableIndex(%d)\n", (UINT)((*p_ll_cmd_ptr << 24) >> 32), (int)((*p_ll_cmd_ptr << 56) >> 56));
		} else if ((*p_ll_cmd_ptr >> 61) == 0) {
			fprintf(fd, "   => cmd(NULL    ) , TableIndex(%d)\n", (int)((*p_ll_cmd_ptr << 56) >> 56));
		} else {
			fprintf(fd, "\n");
		}
	}
	fprintf(fd, "\n");
}

HD_RESULT _vendor_ai_net_debug_dump_llcmd(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 process_index = 0;
	UINT32 parm_va_ofs = 0;
	UINT32 ai_head_addr = 0;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	//dump mem_manager
	_vendor_ai_net_debug_dump_mem_manager(fd, p_mem_manager);

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		parm_va_ofs = ALIGN_CEIL_4(p_mem->va);
		ai_head_addr = p_mctrl[process_index].addr;
		if (ai_head_addr < parm_va_ofs) ai_head_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

		fprintf(fd, "  [%lu] ", process_index);
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			_vendor_ai_net_debug_dump_llcmd_detail(proc_id, space, p_mem_manager, fd, ai_head_addr, parm_va_ofs);
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			break;
		}
	}

	return HD_OK;
}

//force set w=0, h=0 in layer[1] llcmd, to test error handing of engine timeout
VOID _vendor_ai_net_debug_mod_llcmd_detail(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM * p_mem_manager, FILE *fd, UINT32 head_va, UINT32 parm_va_ofs)
{
	UINT32 ai_head_addr = _cvt_user_va(proc_id, space, p_mem_manager, head_va);
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)ai_head_addr;
	UINT32 ai_parm_addr = _cvt_user_va(proc_id, space, p_mem_manager, p_head->parm_addr);
	UINT64 *p_ll_end_cmd, *p_ll_cmd_ptr;
	UINT32 i=0;
	if (ai_parm_addr < parm_va_ofs) ai_parm_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

	p_ll_end_cmd = (UINT64 *)(ai_parm_addr + p_head->parm_size - sizeof(UINT64));
	fprintf(fd, "  [ModeCtrl->addr]parm_addr = 0x%08x => [AI_LL_HEAD] mode = %u, parm = (0x%08x, %lu), eng = %u, ll_end = 0x%016llx\n", (UINT)head_va, p_head->mode, (UINT)p_head->parm_addr, p_head->parm_size, p_head->eng, *p_ll_end_cmd);

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		fprintf(fd, "      <CNN>\n");
		break;
	case AI_ENG_NUE:
		//fprintf(fd, "      <NUE>\n");
		return;
		break;
	case AI_ENG_NUE2:
		//fprintf(fd, "      <NUE2>\n");
		return;
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}

	for (i=0 ; i < p_head->parm_size ; i += sizeof(UINT64)) {
		p_ll_cmd_ptr = (UINT64 *)(ai_parm_addr + i);
		//fprintf(fd, "      (%4d)  0x%016llx", (int)i, *p_ll_cmd_ptr);

		if ((*p_ll_cmd_ptr >> 61) == 4) {
			//fprintf(fd, "   => cmd(UPD     ) , Reg(0x%03x) , Value(0x%08x), ByteEn(0x%1x), T(%d), O(%d)\n", (UINT)((*p_ll_cmd_ptr << 20) >> 52), (UINT)((*p_ll_cmd_ptr << 32) >> 32), (UINT)((*p_ll_cmd_ptr << 16) >> 60), (int)((*p_ll_cmd_ptr << 3) >> 63), (int)((*p_ll_cmd_ptr << 4) >> 63));
			UINT32 reg = (UINT)((*p_ll_cmd_ptr << 20) >> 52);
			UINT32 val = (UINT)((*p_ll_cmd_ptr << 32) >> 32);
			UINT64 cmd = *(p_ll_cmd_ptr);
			//fprintf(fd, "cnn: reg=%03x\r\n", (int)reg);
			if (reg == 0x5c) {
				fprintf(fd, "cnn: w=%d, h=%d\r\n", (int)(val & 0x000003ff), (int)((val >> 12) & 0x03ff));
				cmd &= ~0x003ff3ff;
				*(UINT64 *)(p_ll_cmd_ptr) = cmd;
				val = (UINT)((*p_ll_cmd_ptr << 32) >> 32);
				fprintf(fd, "===> w=%d, h=%d\r\n", (int)(val & 0x000003ff), (int)((val >> 12) & 0x03ff));
			}
		} else if ((*p_ll_cmd_ptr >> 61) == 2) {
			//fprintf(fd, "   => cmd(NEXT_UPD) , Addr(0x%08x)\n", (UINT)((*p_ll_cmd_ptr << 24) >> 32));
		} else if ((*p_ll_cmd_ptr >> 61) == 1) {
			//fprintf(fd, "   => cmd(NEXT_LL ) , Addr(0x%08x), TableIndex(%d)\n", (UINT)((*p_ll_cmd_ptr << 24) >> 32), (int)((*p_ll_cmd_ptr << 56) >> 56));
		} else if ((*p_ll_cmd_ptr >> 61) == 0) {
			//fprintf(fd, "   => cmd(NULL    ) , TableIndex(%d)\n", (int)((*p_ll_cmd_ptr << 56) >> 56));
		} else {
			//fprintf(fd, "\n");
		}
	}
	//fprintf(fd, "\n");
	vendor_common_mem_cache_sync((VOID *)ai_parm_addr, p_head->parm_size - sizeof(UINT64), VENDOR_COMMON_MEM_DMA_TO_DEVICE); ///< cache clean - output to engine's input
}

//force set w=0, h=0 in layer[1] llcmd, to test error handing of engine timeout
HD_RESULT _vendor_ai_net_debug_mod_llcmd(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd, UINT32 i)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 process_index = 0;
	UINT32 parm_va_ofs = 0;
	UINT32 ai_head_addr = 0;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		parm_va_ofs = ALIGN_CEIL_4(p_mem->va);
		ai_head_addr = p_mctrl[process_index].addr;
		if (ai_head_addr < parm_va_ofs) ai_head_addr += parm_va_ofs; // if not fix yet(call this funciton before gen_init(), fix it

		//fprintf(fd, "  layer[%lu] ", process_index);
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (i == process_index) {
				fprintf(fd, "******* layer[%lu] -> modify w,h ", process_index);
				_vendor_ai_net_debug_mod_llcmd_detail(proc_id, space, p_mem_manager, fd, ai_head_addr, parm_va_ofs);
			}
			break;

		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			break;
		}
	}

	return HD_OK;
}


HD_RESULT _vendor_ai_net_debug_dump_mctrl_entry(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd)
{
	UINT32 i = 0;
	VENDOR_AI_NET_MCTRL_LIST *p_mctrl_list = {0};

	p_mctrl_list = vendor_ai_net_group_getmctrlresult(proc_id);
	if ((p_mctrl_list == NULL) || (p_mctrl_list->p_mctrl_entry == 0) || (p_mctrl_list->mctrl_num == 0)) {
		DBG_ERR("null p_mctrl_list\r\n");
		return HD_ERR_INV;
	}

	for (i = 0; i < p_mctrl_list->mctrl_num; i++) {
		fprintf(fd, "p_mctrl_entry idx(%lu) p_data(%p) p_group(%p) %c%c%c\n",
				p_mctrl_list->p_mctrl_entry[i].mc_idx,
				p_mctrl_list->p_mctrl_entry[i].p_data,
				p_mctrl_list->p_mctrl_entry[i].p_group,
				VENDOR_AI_NET_IS_BMP(p_mctrl_list->p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD) ? 'H' : ' ',
				VENDOR_AI_NET_IS_BMP(p_mctrl_list->p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_MIDDLE) ? 'M' : ' ',
				VENDOR_AI_NET_IS_BMP(p_mctrl_list->p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL) ? 'T' : ' ');
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_dump_group(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd)
{
	UINT32 i = 0;
	VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};

	p_group_list = vendor_ai_net_group_getgroupresult(proc_id);
	if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
		DBG_ERR("null p_group_list\r\n");
		return HD_ERR_INV;
	}
	for (i = 0; i < p_group_list->group_num; i++) {
		if (p_group_list->p_group[i].addr == 0) {
			continue;
		}
		fprintf(fd, "p_group_list idx(%lu) step(%lu) addr(%#lx) cnt(%lu) prev(%lu) next(%lu) p_head(%p) p_tail(%p)\n",
				i, p_group_list->p_group[i].step, p_group_list->p_group[i].addr, p_group_list->p_group[i].cnt,
				p_group_list->p_group[i].prev_num, p_group_list->p_group[i].next_num,
				p_group_list->p_group[i].p_head, p_group_list->p_group[i].p_tail);
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_dump_mem_entry(UINT32 proc_id, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd)
{
	UINT32 i = 0;
	VENDOR_AI_NET_MEM_LIST *p_mem_list = {0};

	p_mem_list = vendor_ai_net_group_getmemresult(proc_id);
	if ((p_mem_list == NULL) || (p_mem_list->p_mem == 0) || (p_mem_list->mem_num == 0)) {
		DBG_ERR("null p_mem_list\r\n");
		return HD_ERR_INV;
	}

	for (i = 0; i < p_mem_list->mem_num; i++) {
		fprintf(fd, "p_mem_list idx(%lu) p_group(%p, %lu) alloc(%lu) step(%lu) step_end(%lu)\n",
				i, p_mem_list->p_mem[i].p_group, p_mem_list->p_mem[i].p_group->g_idx,
				p_mem_list->p_mem[i].is_alloc, p_mem_list->p_mem[i].p_group->step,
				p_mem_list->p_mem[i].p_group->step_end);
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_dump_graph_dot(UINT32 proc_id, NET_DBG_ITEM item, NET_DBG_SPACE space, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, FILE *fd)
{
#if CNN_528_PSW
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 mc_idx = 0, in_idx = 0, out_idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 in_buf_num  = 0;
	UINT32 out_buf_num = 0;

	if (space == NET_DBG_SPACE_USER) {
		p_mem = &p_mem_manager->user_parm;
	} else if (space == NET_DBG_SPACE_KERL) {
		p_mem = &p_mem_manager->kerl_parm;
	}else {
		return HD_ERR_INV;
	}

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head  = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

if (item == NET_DBG_ITEM_DOT_BUF_NODE) {
	//--- Buffer as node ---
	{
		fprintf(fd, "digraph G {\r\n");
		fprintf(fd, "  node [style=filled, color=lightgrey, shape=rectangle];\r\n");
		fprintf(fd, "  edge [fontsize=28]\r\n");
		fprintf(fd, "  \r\n");
		fprintf(fd, "    edge_color_list [\r\n");
		fprintf(fd, "   label=<\r\n");
		fprintf(fd, "     <table border=\"10\" cellborder=\"1\" cellspacing=\"0\">\r\n");
		fprintf(fd, "       <tr><td>Edge Color</td></tr>\r\n");
		fprintf(fd, "       <tr><td> </td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffff00\">PREPROC</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffffff\">CONV</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"tomato\"><font color=\"#ffff00\">ELTWISE</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"green\">FC</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"yellow\">SOFTMAX</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"pink\">POSTPROC</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"darkgreen\">RESHAPE</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"red\">LSTM</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"salviablue\">REVERSE</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"grey\">OTHER</td></tr>\r\n");
		fprintf(fd, "     </table>>\r\n");
		fprintf(fd, "  ];\r\n");
		fprintf(fd, "  \r\n");
		fprintf(fd, "    text_color_list [\r\n");
		fprintf(fd, "    color = yellow\r\n");
		fprintf(fd, "   label=<\r\n");
		fprintf(fd, "     <table border=\"10\" cellborder=\"1\" cellspacing=\"0\">\r\n");
		fprintf(fd, "       <tr><td><font color=\"#000000\">Text Color</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td> </td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffffff\">VPE</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffff00\">NUE2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"green\">NUE</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"grey\">CPU</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"lightgrey\">DSP</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"tomato4\"><font color=\"#ffffff\">UNKNOWN</font></td></tr>\r\n");
		fprintf(fd, "     </table>>\r\n");
		fprintf(fd, "  ];\r\n");
	}
	{
		// buf_id/size mapping
		INT32 buf_id=0;

#if (NET_DBG_HEAP == 1)
		BOOL *b_bufsize_set = (BOOL*)vendor_ai_malloc(sizeof(BOOL)*p_head->mode_ctrl_num*2);   //only for debug cmd
#else
		BOOL *b_bufsize_set = (BOOL*)vendor_ai_malloc(sizeof(BOOL)*p_head->mode_ctrl_num*2);
#endif

		if (b_bufsize_set != NULL) {
			memset(b_bufsize_set, 0, sizeof(BOOL)*p_head->mode_ctrl_num*2);

			for (mc_idx = 0; mc_idx < p_head->mode_ctrl_num; mc_idx ++) {
				out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
				for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
					buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], out_idx);  // p_mctrl[mc_idx].out_buf_index[out_idx];
					if ((buf_id >= 0) && (b_bufsize_set[buf_id] == FALSE)){
						b_bufsize_set[buf_id] = TRUE;
						if (OUT_BUF_IS_TMP(&p_mctrl[mc_idx], out_idx) == TRUE) {
							//tmp buffer
							fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\", color=red]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
						} else {
							fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\"]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
						}
					}
				}
			}
#if (NET_DBG_HEAP == 1)
			vendor_ai_free(b_bufsize_set, sizeof(BOOL)*p_head->mode_ctrl_num*2);   //only for debug cmd
#else
			vendor_ai_free(b_bufsize_set, sizeof(BOOL)*p_head->mode_ctrl_num*2);
#endif
		}
	}
	{
		UINT32 src_buf_id = 0;
		UINT32 dst_buf_id = 0;
		char text_color[16]; // engine
		char edge_color[16]; // mode

		for (mc_idx = 0; mc_idx < p_head->mode_ctrl_num; mc_idx ++) {
			in_buf_num  = IN_BUF_NUM(&p_mctrl[mc_idx]);
			out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
			// find src_buf
			for (in_idx = 0; in_idx < in_buf_num; in_idx++) {
				if (mc_idx == 0) {
					// first mctrl
					in_idx = in_buf_num; // trick for first input layer ( bin file doesn't have any in_buf_index) // skip loop later;
				} else {
					// other normal mctrl
					if (IN_BUF_INDEX(&p_mctrl[mc_idx], in_idx) < 0) {  // p_mctrl[mc_idx].in_buf_index[in_idx]
						continue;
					}
					src_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], in_idx);  // p_mctrl[mc_idx].in_buf_index[in_idx];
				}

				// find dst_buf
				for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
					if (OUT_BUF_INDEX(&p_mctrl[mc_idx], out_idx) < 0) {  // p_mctrl[mc_idx].out_buf_index[out_idx]
						continue;
					}
					dst_buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], out_idx); // p_mctrl[mc_idx].out_buf_index[out_idx];

					// text color (engine)
					switch (p_mctrl[mc_idx].eng) {
						case NN_GEN_ENG_VPE:   snprintf(text_color, 16, "purple");    break;
						case NN_GEN_ENG_NUE2:  snprintf(text_color, 16, "purple");    break;
						case NN_GEN_ENG_CNN:   snprintf(text_color, 16, "blue");      break;
						case NN_GEN_ENG_CNN2:  snprintf(text_color, 16, "blue");      break;
						case NN_GEN_ENG_NUE:   snprintf(text_color, 16, "green");     break;
						case NN_GEN_ENG_CPU:   snprintf(text_color, 16, "grey");      break;
						case NN_GEN_ENG_DSP:   snprintf(text_color, 16, "lightgrey"); break;

						case NN_GEN_ENG_UNKNOWN:
						default:
							snprintf(text_color, 16, "tomato4");      break;
							break;
					}
					// edge color (mode)
					switch (p_mctrl[mc_idx].mode) {
						case NN_PREPROC:        snprintf(edge_color, 16, "purple");      break;
						case NN_CONV:           snprintf(edge_color, 16, "blue");        break;
						case NN_MATMUL:         snprintf(edge_color, 16, "blue");        break;
						case NN_CORR:           snprintf(edge_color, 16, "blue");        break;
						case NN_BN:             snprintf(edge_color, 16, "blue");        break;
						case NN_ELTWISE:        snprintf(edge_color, 16, "tomato");      break;
						case NN_FC:             snprintf(edge_color, 16, "green");       break;
						case NN_SOFTMAX:        snprintf(edge_color, 16, "yellow");      break;
						case NN_POSTPROC:       snprintf(edge_color, 16, "pink");        break;
						case NN_RESHAPE:        snprintf(edge_color, 16, "darkgreen");   break;
						case NN_LSTM:           snprintf(edge_color, 16, "red");         break;
						case NN_REVERSE:        snprintf(edge_color, 16, "salviablue");  break;

						case NN_DECONV:
						case NN_SVM:
						case NN_ROIPOOLING:
						case NN_REORGANIZATION:
						case NN_PROPOSAL:
						case NN_FC_POST:
						case NN_POOL:
						case NN_BNSCALE:
						case NN_CUSTOMER:
						case NN_ANCHOR:
						case NN_UPSAMPLE:
						case NN_SCALEUP:
						default:
							snprintf(edge_color, 16, "grey");      break;
							break;
					}

					if (mc_idx == 0) {
						fprintf(fd, "  \"Image\" -> \"(%lu)\" [label=\"%lu\", color=%s, fontcolor=%s];\r\n", dst_buf_id, mc_idx, edge_color, text_color);
					} else {
						fprintf(fd, "  \"(%lu)\" -> \"(%lu)\" [label=\"%lu\", color=%s, fontcolor=%s];\r\n", src_buf_id, dst_buf_id, mc_idx, edge_color, text_color);
					}
				} // for (out_idx = 0; out_idx < 2; out_idx++) {
			} // for (in_idx = 0; in_idx < 3; in_idx++) {
		}
	}

	{
		fprintf(fd, "}\r\n");
	}
} // if (item == NET_DBG_ITEM_DOT_BUF_NODE)


if (item == NET_DBG_ITEM_DOT_LAYER_NODE) {
	// mctrl as node
	{
		fprintf(fd, "digraph G {\r\n");
		fprintf(fd, "  node [style=filled, color=lightgrey, shape=rectangle, fontcolor=white];\r\n");
		fprintf(fd, "  \r\n");
		fprintf(fd, "  node_color [\r\n");
		fprintf(fd, "    color = yellow\r\n");
		fprintf(fd, "   label=<\r\n");
		fprintf(fd, "     <table border=\"10\" cellborder=\"1\" cellspacing=\"0\">\r\n");
		fprintf(fd, "       <tr><td><font color=\"#000000\">Node Color</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td> </td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffffff\">VPE</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffff00\">NUE2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"green\">NUE</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"grey\">CPU</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"lightgrey\">DSP</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"tomato4\"><font color=\"#ffffff\">UNKNOWN</font></td></tr>\r\n");
		fprintf(fd, "     </table>>\r\n");
		fprintf(fd, "  ];\r\n\r\n");
	}

	// define node(mctrl) name
	{
		CHAR node_color[16];
		CHAR mode_name[10];

		for (mc_idx = 0; mc_idx < p_head->mode_ctrl_num; mc_idx ++) {
			// mode name
			_cvt_trig_mode_name(p_mctrl[mc_idx].mode, mode_name, 10);
			// node color (engine)
			switch (p_mctrl[mc_idx].eng) {
				case NN_GEN_ENG_VPE:   snprintf(node_color, 16, "purple");    break;
				case NN_GEN_ENG_NUE2:  snprintf(node_color, 16, "purple");    break;
				case NN_GEN_ENG_CNN:   snprintf(node_color, 16, "blue");      break;
				case NN_GEN_ENG_CNN2:  snprintf(node_color, 16, "blue");      break;
				case NN_GEN_ENG_NUE:   snprintf(node_color, 16, "green");     break;
				case NN_GEN_ENG_CPU:   snprintf(node_color, 16, "grey");      break;
				case NN_GEN_ENG_DSP:   snprintf(node_color, 16, "lightgrey"); break;

				case NN_GEN_ENG_UNKNOWN:
				default:
					snprintf(node_color, 16, "tomato4");      break;
					break;
			}
			fprintf(fd, "  %lu[label=\"%lu\\n%s\", style=filled, color=%s]\r\n", mc_idx, mc_idx, mode_name, node_color);
		}
	}

	// node(mctrl) relationship
	{
		UINT32 id_list_start_addr = (UINT32)net_info.p_id_list;
		UINT32 start_ofs = 0, end_ofs = 0, list_ofs = 0, list_num = 0, idx2 = 0;
		UINT32 next_mctrl = 0;

		for (mc_idx = 0; mc_idx < p_head->mode_ctrl_num; mc_idx ++) {
			// next
			list_num = p_mctrl[mc_idx].next_num;
			list_ofs = p_mctrl[mc_idx].next_layer_idx_addr;

			start_ofs = list_ofs;
			end_ofs   = list_ofs + sizeof(UINT32)*list_num;
			for (idx2 = start_ofs; idx2 < end_ofs; idx2 += sizeof(UINT32)) {
				next_mctrl= *(UINT32 *)(id_list_start_addr + idx2);
				fprintf(fd, "  %lu->%lu\r\n", mc_idx, next_mctrl);
			}
		}
	}
	{
		fprintf(fd, "}\r\n");
	}
} // if (item == NET_DBG_ITEM_DOT_LAYER_NODE)


if (item == NET_DBG_ITEM_DOT_GROUP_NODE) {
	// mctrl as node
	{
		fprintf(fd, "digraph G {\r\n");
		fprintf(fd, "  node [style=filled, color=lightgrey, shape=rectangle, fontcolor=white];\r\n");
		fprintf(fd, "  \r\n");
		fprintf(fd, "  node_color [\r\n");
		fprintf(fd, "    color = yellow\r\n");
		fprintf(fd, "   label=<\r\n");
		fprintf(fd, "     <table border=\"10\" cellborder=\"1\" cellspacing=\"0\">\r\n");
		fprintf(fd, "       <tr><td><font color=\"#000000\">Node Color</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td> </td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffffff\">VPE</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"purple\"><font color=\"#ffff00\">NUE2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"blue\"><font color=\"#ffff00\">CNN2</font></td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"green\">NUE</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"grey\">CPU</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"lightgrey\">DSP</td></tr>\r\n");
		fprintf(fd, "       <tr><td bgcolor=\"tomato4\"><font color=\"#ffffff\">UNKNOWN</font></td></tr>\r\n");
		fprintf(fd, "     </table>>\r\n");
		fprintf(fd, "  ];\r\n\r\n");
	}

	// define node(group) name
	{
		UINT32 id_list_start_addr = (UINT32)net_info.p_id_list;
		UINT32 i, j, k, start_ofs = 0, end_ofs = 0;
		NN_GEN_MODE_CTRL *p_mctrl = NULL;
		VENDOR_AI_NET_LLGROUP_LIST *p_group_list = {0};
		VENDOR_AI_NET_LLGROUP *p_group_tmp = NULL;
		VENDOR_AI_NET_MCTRL_LIST *p_mctrl_list = {0};
		VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
		CHAR node_color[16];

		p_group_list = vendor_ai_net_group_getgroupresult(proc_id);
		if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
			DBG_ERR("null p_group_list\r\n");
			return HD_ERR_INV;
		}

		p_mctrl_list = vendor_ai_net_group_getmctrlresult(proc_id);
		if ((p_mctrl_list == NULL) || (p_mctrl_list->p_mctrl_entry == 0) || (p_mctrl_list->mctrl_num == 0)) {
			DBG_ERR("null p_mctrl_list\r\n");
			return HD_ERR_INV;
		}
		for (i = 0; i < p_group_list->group_num; i++) {
			if (p_group_list->p_group[i].addr == 0) {
				continue;
			}
			fprintf(fd, "  %ld[label=\"[%lu]\\n", p_group_list->p_group[i].g_idx, p_group_list->p_group[i].g_idx);
			list_for_each_entry(p_mctrl_entry, &p_group_list->p_group[i].mctrl_listhead, list) {
				fprintf(fd, "%lu ",	p_mctrl_entry->mc_idx);
			}
			switch (p_group_list->p_group[i].eng) {
				case NN_GEN_ENG_VPE:   snprintf(node_color, 16, "purple");    break;
				case NN_GEN_ENG_NUE2:  snprintf(node_color, 16, "purple");    break;
				case NN_GEN_ENG_CNN:   snprintf(node_color, 16, "blue");      break;
				case NN_GEN_ENG_CNN2:  snprintf(node_color, 16, "blue");      break;
				case NN_GEN_ENG_NUE:   snprintf(node_color, 16, "green");     break;
				case NN_GEN_ENG_CPU:   snprintf(node_color, 16, "grey");      break;
				case NN_GEN_ENG_DSP:   snprintf(node_color, 16, "lightgrey"); break;

				case NN_GEN_ENG_UNKNOWN:
				default:
					snprintf(node_color, 16, "tomato4");      break;
					break;
			}
			fprintf(fd, "\", style=filled, color=%s]\n", node_color);
		}

		// node(group) relationship
		for (i = 0; i < p_group_list->group_num; i++) {
			if (p_group_list->p_group[i].addr == 0) {
				continue;
			}
			if (p_group_list->p_group[i].p_tail == NULL) {
				continue;
			}
			p_mctrl = p_group_list->p_group[i].p_tail;
			start_ofs = p_mctrl->next_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl->next_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				p_group_tmp = (VENDOR_AI_NET_LLGROUP *)p_mctrl_list->p_mctrl_entry[k].p_group;
				if (p_group_tmp == NULL) {
					continue;
				}
				if (p_mctrl_list->p_mctrl_entry[k].p_data == p_group_tmp->p_head) {
					fprintf(fd, "%lu->%lu\n", p_group_list->p_group[i].g_idx, p_group_tmp->g_idx);
				}
			}
		}
	}
	{
		fprintf(fd, "}\r\n");
	}
} // if (item == NET_DBG_ITEM_DOT_GROUP_NODE)

#endif // #if CNN_528_PSW

	return HD_OK;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT _vendor_ai_net_debug_pars_addr(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 *fixed_offset)
{
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = &p_mem_manager->user_parm;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	UINT32 proc_layer_num=0;
	HD_RESULT rv = HD_OK;
	UINT32 idx=0;

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
	proc_layer_num = net_info.p_head->mode_ctrl_num;

	// check mctrl[0] to see if address is already fixed by gen_init()
	*fixed_offset = (p_mctrl[0].iomem.imem_addr < p_mem->va)? p_mem->va:0;

	if (*fixed_offset == 0) return HD_OK; // don't need to fix ... only before gen_init() need to fix offset -> real_addr

	// fix offset -> real_addr
	for (idx=0; idx< proc_layer_num; idx++) {
		p_mctrl[idx].iomem.imem_addr  += *fixed_offset;
		p_mctrl[idx].iomem.omem_addr  += *fixed_offset;
		#if CNN_FMT_V4
		p_mctrl[idx].in_bufinfo_addr  += *fixed_offset;
		p_mctrl[idx].out_bufinfo_addr += *fixed_offset;
		#endif
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_debug_unpars_addr(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 *fixed_offset)
{
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = &p_mem_manager->user_parm;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	UINT32 proc_layer_num=0;
	HD_RESULT rv = HD_OK;
	UINT32 idx=0;

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
	proc_layer_num = net_info.p_head->mode_ctrl_num;

	if (*fixed_offset == 0) return HD_OK; // don't need to fix ... only before gen_init() need to fix real_addr -> offset

	// fix real_addr -> offset
	for (idx=0; idx< proc_layer_num; idx++) {
		p_mctrl[idx].iomem.imem_addr  -= *fixed_offset;
		p_mctrl[idx].iomem.omem_addr  -= *fixed_offset;
		#if CNN_FMT_V4
		p_mctrl[idx].in_bufinfo_addr  -= *fixed_offset;
		p_mctrl[idx].out_bufinfo_addr -= *fixed_offset;
		#endif
	}
	return HD_OK;
}

HD_RESULT vendor_ai_net_debug_dump(UINT32 proc_id, NET_DBG_SPACE space, NET_DBG_ITEM item, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, CHAR *filepath) 
{
	HD_RESULT rv = HD_OK;
	UINT32 fixed_offset=0;
	FILE *fd;

	// open file
	if (filepath != NULL) {
		if ((fd = fopen(filepath, "wb")) == NULL) {
			DBG_ERR("open file(%s) fail\n", filepath);
			return HD_ERR_FAIL;
		}
	} else {
		fd = stdout; // dump to UART
	}

	// if called before gen_init() ... fix iomem_addr, bufinfo_addr (offset -> real_addr)
	_vendor_ai_net_debug_pars_addr(p_mem_manager, &fixed_offset);

	switch (item) {
		case NET_DBG_ITEM_IOMEM:
			rv = _vendor_ai_net_debug_dump_iomem(space, p_mem_manager, fd);
			break;
			
		case NET_DBG_ITEM_MCTRL:
			rv = _vendor_ai_net_debug_dump_mctrl(space, p_mem_manager, fd, fixed_offset);
			break;
			
		case NET_DBG_ITEM_AIPARM:
			rv = _vendor_ai_net_debug_dump_aiparm(proc_id, space, p_mem_manager, fd);
			break;

		case NET_DBG_ITEM_LLCMD:
			rv = _vendor_ai_net_debug_dump_llcmd(proc_id, space, p_mem_manager, fd);
			break;

		case NET_DBG_ITEM_CRASH: //force set w=0, h=0 in layer[1] llcmd, to test error handing of engine timeout
			rv = _vendor_ai_net_debug_mod_llcmd(proc_id, space, p_mem_manager, fd, 1);
			break;

		case NET_DBG_ITEM_MCTRL_ENTRY:
			rv = _vendor_ai_net_debug_dump_mctrl_entry(proc_id, space, p_mem_manager, fd);
			break;

		case NET_DBG_ITEM_GROUP:
			rv = _vendor_ai_net_debug_dump_group(proc_id, space, p_mem_manager, fd);
			break;

		case NET_DBG_ITEM_MEM_ENTRY:
			rv = _vendor_ai_net_debug_dump_mem_entry(proc_id, space, p_mem_manager, fd);
			break;

		case NET_DBG_ITEM_DOT_BUF_NODE:
		case NET_DBG_ITEM_DOT_LAYER_NODE:
		case NET_DBG_ITEM_DOT_GROUP_NODE:
			rv = _vendor_ai_net_debug_dump_graph_dot(proc_id, item, space, p_mem_manager, fd);
			break;

		default:
			rv =  HD_ERR_FAIL;
			break;
	}

	// if called before gen_init() ... fix iomem_addr, bufinfo_addr (real_addr -> offset)
	_vendor_ai_net_debug_unpars_addr(p_mem_manager, &fixed_offset);

	// close file
	if (filepath != NULL) {
		DBG_DUMP("dump to (%s) success !\n", filepath);
		fclose(fd);
	}	
	return rv;
}

static BOOL _vendor_ai_net_debug_layer_write_file(CHAR *filepath, UINT32 addr, UINT32 size)
{
	FILE *fsave = NULL;

	fsave = fopen(filepath, "wb");
	if (fsave == NULL) {
		DBG_ERR("fopen fail\n");
		return FALSE;
	}

	fwrite((UINT8 *)addr, size, 1, fsave);

	fclose(fsave);

	return TRUE;
}

static UINT32 _vendor_ai_net_debug_layer_update_mctrl_id(NN_GEN_MODE_CTRL *p_mctrl, UINT32 mctrl_num, UINT32 ori_mctrl_id)
{
	UINT32 new_mctrl_id = ori_mctrl_id;

	while (new_mctrl_id+1 < mctrl_num) {
		if (p_mctrl[new_mctrl_id+1].layer_index == p_mctrl[ori_mctrl_id].layer_index) {
			new_mctrl_id++;
		} else {
			break;
		}
	}
	return new_mctrl_id;
}

HD_RESULT _vendor_ai_net_debug_layer_dump_exe(UINT32 mctrl_id, NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, CHAR *img_folder_path)
{
	CHAR img_file_path[256]="";
	UINT32 idx = 0;
#if CNN_25_MATLAB
	UINT32 layer_id = p_mctrl[mctrl_id].layer_index;
#endif
	for (idx = 0; idx < p_mctrl[mctrl_id].iomem.omem_cnt; idx++) {
#if CNN_25_MATLAB
		UINT32 dump_size 	= p_io_mem[layer_id].SAO[idx].size;
		UINT32 dump_addr	= p_io_mem[layer_id].SAO[idx].va;
#else
		NN_DATA *p_sao = (NN_DATA*)p_mctrl[mctrl_id].iomem.omem_addr;
		UINT32 dump_size 	= p_sao[idx].size;
		UINT32 dump_addr	= p_sao[idx].va;

		if (dump_size >0) dump_size = OUT_BUF_SIZE(&p_mctrl[mctrl_id], idx); // p_mctrl[mctrl_id].output_buffsize[idx]; // maybe only 6 Bytes, but SAO = 8 (align_4 before)
#endif
		if (dump_size == 0) {
			continue;
		}

		// flush cache (maybe not required ?)
		vendor_common_mem_cache_sync((VOID *)dump_addr, dump_size, VENDOR_COMMON_MEM_DMA_FROM_DEVICE);

		switch (p_mctrl[mctrl_id].eng) {
		case NN_GEN_ENG_CNN:
			snprintf(img_file_path, 256, "%s/CNN_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		case NN_GEN_ENG_CNN2:
			snprintf(img_file_path, 256, "%s/CNN2_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		case NN_GEN_ENG_NUE:
			snprintf(img_file_path, 256, "%s/NUE_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		case NN_GEN_ENG_NUE2:
			snprintf(img_file_path, 256, "%s/NUE2_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		case NN_GEN_ENG_CPU:
			snprintf(img_file_path, 256, "%s/CPU_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		case NN_GEN_ENG_VPE:
			snprintf(img_file_path, 256, "%s/VPE_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		default:
			snprintf(img_file_path, 256, "%s/NAN_%d_OUT%d.bin", img_folder_path, (int)mctrl_id, (int)idx);
			break;
		}
		DBG_DUMP("img_file_path : %s; addr=0x%08x; size=%d\n", img_file_path, (unsigned int)dump_addr, (unsigned int)dump_size);
		_vendor_ai_net_debug_layer_write_file(img_file_path, dump_addr, dump_size);
	}

	return HD_OK;
}

static HD_RESULT _vendor_ai_net_debug_layer_dump_full(NN_GEN_MODE_CTRL *p_mctrl, UINT32 mctrl_num, NN_IOMEM *p_io_mem, CHAR *img_folder_path)
{
	UINT32 mctrl_id = 0;

	for (mctrl_id = 0; mctrl_id < mctrl_num; mctrl_id++) {
		// for "stripe" mctrl_id layers (many mctrl_id layer = one layer_id layer), only dump last mctrl_id layer for this stripe
		mctrl_id = _vendor_ai_net_debug_layer_update_mctrl_id(p_mctrl, mctrl_num, mctrl_id);

		// dump layer bin
		_vendor_ai_net_debug_layer_dump_exe(mctrl_id, p_mctrl, p_io_mem, img_folder_path);
	}
	return HD_OK;
}

static HD_RESULT _vendor_ai_net_debug_layer_dump_output(NN_GEN_MODE_CTRL *p_mctrl, UINT32 mctrl_num, NN_IOMEM *p_io_mem, CHAR *img_folder_path)
{
	UINT32 mctrl_id = 0;

	for (mctrl_id = 0; mctrl_id < mctrl_num; mctrl_id ++) {
#if CNN_25_MATLAB
		if (p_mctrl[mctrl_id].next_num > 0) {
#else
		if (OUT_BUF_ATTR_GET(&p_mctrl[mctrl_id], NN_OUT_BUF_ATTR_PRESERVE) == 0) {  // p_mctrl[mctrl_id].is_preserve
#endif
			continue; // this mctrl layer is NOT last node
		}

		// for "stripe" mctrl_id layers (many mctrl_id layer = one layer_id layer), only dump last mctrl_id layer for this stripe
		mctrl_id = _vendor_ai_net_debug_layer_update_mctrl_id(p_mctrl, mctrl_num, mctrl_id);

		// dump layer bin
		_vendor_ai_net_debug_layer_dump_exe(mctrl_id, p_mctrl, p_io_mem, img_folder_path);
	}

	return HD_OK;
}

HD_RESULT vendor_ai_net_debug_layer(UINT32 proc_id, NET_DBG_LAYER layer_opt, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, CHAR *filedir)
{
	HD_RESULT rv = HD_OK;
	NN_GEN_NET_INFO net_info = {0};
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 mctrl_num = 0;
	CHAR default_dir[32] = "";

	if (p_mem_manager == NULL) {
		DBG_ERR("null input(p_mem_manager)\r\n");
		return HD_ERR_INV;
	}

	//--- set/mkdir for filedir ---
	if (filedir == NULL) {
		snprintf(default_dir, 32, "/mnt/sd/output%lu", proc_id);
		filedir = (CHAR *)default_dir;
	}

	{
		CHAR t_cmd[256]= "";
		snprintf(t_cmd, 256, "mkdir --parents %s", filedir);
		system(t_cmd);
	}

	//--- get p_io_mem/p_mctrl ---
	rv = vendor_ais_get_net_info(&net_info, p_mem_manager->user_parm.va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}
#if CNN_25_MATLAB
	p_io_mem  = net_info.p_io_mem;
#endif
	p_mctrl   = net_info.p_mctrl;
	mctrl_num = net_info.p_head->mode_ctrl_num;

	//--- dump layer bin ---
	switch(layer_opt) {
		case NET_DBG_LAYER_FULL:
#if CNN_25_MATLAB
			rv = _vendor_ai_net_debug_layer_dump_full(p_mctrl, mctrl_num, p_io_mem, filedir);
#else
			rv = _vendor_ai_net_debug_layer_dump_full(p_mctrl, mctrl_num, NULL, filedir);
#endif
			break;

		case NET_DBG_LAYER_OUTPUT:
#if CNN_25_MATLAB
			rv = _vendor_ai_net_debug_layer_dump_output(p_mctrl, mctrl_num, p_io_mem, filedir);
#else
			rv = _vendor_ai_net_debug_layer_dump_output(p_mctrl, mctrl_num, NULL, filedir);
#endif
			break;

		default:
			rv =  HD_ERR_FAIL;
			break;
	}

	return rv;
}

HD_RESULT vendor_ai_net_debug_performance(UINT32 proc_id, CHAR *p_model_name, UINT64 *p_proc_time, DOUBLE p_cpu_loading)
{
	FILE *fd;
	// open file

	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT32 total_buf_size = 0;
	UINT64 average_proc_time = 0;
	UINT32 i = 1;
	CHAR dump_path[256] = {0};

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);

	snprintf(dump_path, 255,"network_performance_J%dW%dB%d.txt", (int)p_cfg->job_opt.method, (int)p_cfg->job_opt.wait_ms, (int)p_cfg->buf_opt.method);
	if ((fd = fopen(dump_path, "a")) == NULL) {
		printf("open file(%s) fail\n", dump_path);
		return -1;
	}

	total_buf_size = p_cfg->cfg_model.size + p_proc->priv.init_buf.size + p_proc->priv.work_buf.size;

	while(i<7 && p_proc_time[i] != 0) {
		average_proc_time += p_proc_time[i];
		i++;
	}
	average_proc_time = (i == 1) ? p_proc_time[0] : average_proc_time/(i-1);
	
	if(ftell(fd) < 10) {
		fprintf(fd, "===========================================================================================================================================\n");
		fprintf(fd, "model_name                                                                           model_bin_size  total_buf_size  proc_time  cpu_loading\n");
		fprintf(fd, "===========================================================================================================================================\n");
	}

	fprintf(fd, "%-85s%-16lu%-16lu%-11llu%-11f\n", p_model_name, p_cfg->cfg_model.size, total_buf_size, average_proc_time, p_cpu_loading);
	fclose(fd);
	return HD_OK;
}

HD_RESULT vendor_ai_net_debug_get_memsize(UINT32 proc_id, UINT32* p_model_size, UINT32* p_initbuf_size, UINT32* p_workbuf_size)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;

	p_proc = _vendor_ai_info(proc_id);
	p_cfg = _vendor_ai_cfg(proc_id);


	*p_model_size   = p_cfg->cfg_model.size;
	*p_initbuf_size = p_proc->priv.init_buf.size;
	*p_workbuf_size = p_proc->priv.work_buf.size;

	return HD_OK;
}

HD_RESULT vendor_ai_net_debug_init(VOID)
{
#if (NET_DBG_HEAP == 1)
	g_proc_trace = fix_proc_trace;
	memset(g_proc_trace, 0x0, sizeof(UINT32) * g_ai_support_net_max);
#else
	g_proc_trace = (UINT32 *)vendor_ai_malloc(sizeof(UINT32) * g_ai_support_net_max);
	if (g_proc_trace == NULL) {
		return HD_ERR_NOMEM;
	}
	memset(g_proc_trace, 0x0, sizeof(UINT32) * g_ai_support_net_max);
#endif

	return HD_OK;
}

HD_RESULT vendor_ai_net_debug_uninit(VOID)
{
#if (NET_DBG_HEAP == 1)
	g_proc_trace = 0;
#else
	if (g_proc_trace) {
		vendor_ai_free(g_proc_trace, sizeof(UINT32) * g_ai_support_net_max);
	}
#endif

	return HD_OK;
}

