/**
	@brief Source file of vendor ai net memory.

	@file vendor_ai_net_mem.c

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
#include "../include/vendor_ai_net_util.h" //for debug

#include "vendor_ai_comm_flow.h"

#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_net_mem.h"
#include "vendor_ai_net_debug.h"
#include "vendor_ai_net_cmd.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net/nn_net.h"
#if NN_DLI
#include "kflow_ai_net/nn_dli.h"
#endif

#include "debug_util/graph_debug_buffer.h"

#include "vendor_ai_net_group.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][mem]"
#include "vendor_ai_debug.h"
//=============================================================


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define DUMMY_ADDR  0x40000000
#define INVALID_BUFID   (-100)    // id for life=0 block (DO NOT define -1, will same as bin normal invalid bufid)
	
#define nn_layer_idx_num 10 
#define nn_next_layer_num 2

#define DUMP_MEMORY_DEBUG    1    // 0: off, 1: on
#define CHK_MEMORY_OVERLAP   1    // 0: off, 1: on
#define DUMP_MEMORY_OVERLAP  0    // 0: off, 1: on

#define MEMORY_REORDER       1    // 0: off, 1: on
#define MEMORY_REORDER_DEBUG 1    // 0: off, 1: on

#define DEBUG_PRESERVE_BUF_NUM  16  // max support debug preserve buf number


#define GRAPH_FUNC_SELF         ((UINT32)0x00000001 << 0)   //0x00000001

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _NN_BUF   NN_BUF;
struct _NN_BUF
{
	INT32 id;
	UINT32 addr;
    UINT32 size;
	UINT32 life;
	UINT32 life_o;
	UINT32 start_t;
	UINT32 end_t;
	BOOL   is_group;
	BOOL   is_preserve;
	NN_BUF *next;
};

typedef struct _NN_MANAGER_BUF
{
	NN_BUF *p_buf;
	UINT32 cnt;
} NN_MANAGER_BUF;

typedef struct _NN_BUFID_LIST
{
	UINT32 *bufid;
	UINT32 cnt;
} NN_BUFID_LIST;

typedef enum
{
	ISIMG_FEATURE,
	ISIMG_IMAGE
} ISIMG;

typedef enum
{
	ELT_IN_SRC_DRAM,
	ELT_IN_SRC_GENTOOL,
} ELT_IN_SRC;

typedef enum {
	FMT_RGB = 0,
	FMT_BGR = 1,
	FMT_YUV420,
	FMT_YUV422,
	FMT_Y_ONLY,
	UV_PACKED,
} FMT;

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
#define mem_shrink_dbg(fmtstr, args...) do{if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG) printf(fmtstr, ##args);}while(0);
#define mem_shrink_dbg_prefix(level, fmtstr, args...) do{int i=0; if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG) { for(i=0;i<level;i++){printf("  ");} printf(fmtstr, ##args);}}while(0);
#define mem_shrink_dbg_manager(manager) do{if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG) { printf("   => "); nn_manage_debug_dump(manager);}}while(0);
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern BOOL _vendor_ai_net_is_linear_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL _vendor_ai_net_is_graph_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL _vendor_ai_net_is_shrink_buf(VENDOR_AI_NET_BUF_OPT buf_opt);
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
INT32    g_debug_preserve_bufid[DEBUG_PRESERVE_BUF_NUM] = {[0 ... DEBUG_PRESERVE_BUF_NUM-1]=-1};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if CNN_528_PSW  // _TODO : remove later !!
int _vendor_ai_net_mem_debug_set_preserve_buf(UINT32 buf_id)
{
	int i=0;
	for (i = 0; i < DEBUG_PRESERVE_BUF_NUM; i++) {
		if (g_debug_preserve_bufid[i] < 0) break;  // this slot with buf_id = (-1) can add new debug buf_id
	}
	if (i == DEBUG_PRESERVE_BUF_NUM) {
		printf("add debug preserve buf_id(%d) fail, debug max num(%d) exceed !!\n", (int)buf_id, (int)DEBUG_PRESERVE_BUF_NUM);
		return (-1);
	}
	g_debug_preserve_bufid[i] = (INT32)buf_id;
	printf("[MEM] debug : add buf_id(%d) to preserve list success !!\n", (int)buf_id);
	return 0;
}

int _vendor_ai_net_mem_debug_clr_preserve_buf(void)
{
	int i=0;
	for (i = 0; i < DEBUG_PRESERVE_BUF_NUM; i++) {
		g_debug_preserve_bufid[i] = (-1); // reset to init buf_id = (-1)
	}

	return 0;
}

BOOL _vendor_ai_net_mem_debug_is_bufid_debug_preserve(INT32 buf_id)
{
	int i=0;
	for (i = 0; i < DEBUG_PRESERVE_BUF_NUM; i++) {
		if (g_debug_preserve_bufid[i] == buf_id) return TRUE;
	}
	return FALSE;
}

int _vendor_ai_net_mem_debug_move_preserve_buf_to_unused_addr(NN_BUF *p_io_buff, UINT32 *end_addr)
{
	int i=0;
	for (i = 0; i < DEBUG_PRESERVE_BUF_NUM; i++) {
		if (g_debug_preserve_bufid[i] >= 0) {
			printf("[MEM] debug : move buf_id(%d)_addr = 0x%08x -> 0x%08x, and end_addr = 0x%08x -> 0x%08x\n", (int)g_debug_preserve_bufid[i], (UINT)p_io_buff[g_debug_preserve_bufid[i]].addr, (UINT)(*end_addr + 64), (UINT)(*end_addr), (UINT)(*end_addr + p_io_buff[g_debug_preserve_bufid[i]].size + 64));
			p_io_buff[g_debug_preserve_bufid[i]].addr = *end_addr + 64;
			*end_addr += p_io_buff[g_debug_preserve_bufid[i]].size + 64;
		}
	}
	return 0;
}

static INT32 _vendor_ai_net_mem_getmaxstep(UINT32 proc_id)
{
	UINT32 i = 0, max_step = 0;
	VENDOR_AI_NET_LLGROUP_LIST *p_group_list = vendor_ai_net_group_getgroupresult(proc_id);

	if ((p_group_list == NULL) || (p_group_list->p_group == 0) || (p_group_list->group_num == 0)) {
		DBG_ERR("null p_group_list\r\n");
		return (-1);
	}
	for (i = 0; i < p_group_list->group_num; i++) {
		max_step = MAX(max_step, p_group_list->p_group[i].step);
	}
	return max_step;
}

static UINT32 _vendor_ai_net_mem_calc_ideal_memory(NN_BUF *p_buff, UINT32 max_buf_num, UINT32 max_t)
{
	UINT32 idx=0, buf_id=0;
	UINT32 ideal_size_cur_t = 0;
	UINT32 ideal_size_total = 0;

	for (idx = 0; idx < max_t; idx++) {
		ideal_size_cur_t = 0;
		for (buf_id = 0; buf_id < max_buf_num; buf_id++) {
			if ((p_buff[buf_id].start_t <= idx) &&
				(p_buff[buf_id].end_t >= (idx+1))) {
				ideal_size_cur_t += p_buff[buf_id].size;
			}
		}
		ideal_size_total = MAX(ideal_size_total, ideal_size_cur_t);
	}
	return ideal_size_total;
}

static UINT32 nn_addr_insert_flag(UINT32 addr, UINT32 flag)
{
	if ((addr & 0xf0000000) != 0) {
		printf("flag is already in the address : 0x%08x\r\n", (UINT)addr);
		return addr;
	} if ((flag == NN_GEN_MODEL_ADDR_TYPE) || (flag == NN_GEN_BUF_ADDR_TYPE)) {
		return (addr + flag);
	} else {
		printf("unknown model/buffer address flag : 0x%08x\r\n", (UINT)flag);
		return addr;
	}
}

#if CHK_MEMORY_OVERLAP
static BOOL nn_manage_detect_overlap(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, NN_BUF *p_io_buff)
{
	BOOL rv = FALSE;
	UINT32 idx = 0, idx2 = 0, out_idx = 0;
	UINT32 out_buf_num	= 0;
	UINT32 max_buf_id = 0;
	UINT32 a_x1=0, a_x2=0, a_y1=0, a_y2=0, b_x1=0, b_x2=0, b_y1=0, b_y2=0;
	INT32 buf_id = 0;

	// find max buf_id
	for (idx = 0; idx < proc_layers_num; idx ++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);  // p_mctrl[idx].out_buf_index[out_idx];
			if (buf_id >=0) max_buf_id = (max_buf_id < (UINT32)buf_id)? (UINT32)buf_id:max_buf_id;
		}
	}

	// find if there're any two buffer overlapped
	for (idx = 0; idx < max_buf_id; idx++) {
		for (idx2 = idx+1; idx2 <= max_buf_id; idx2++) {
			a_x1 = p_io_buff[idx].start_t;
			a_x2 = p_io_buff[idx].end_t;
			a_y1 = p_io_buff[idx].addr;
			a_y2 = p_io_buff[idx].addr + p_io_buff[idx].size;
			b_x1 = p_io_buff[idx2].start_t;
			b_x2 = p_io_buff[idx2].end_t;
			b_y1 = p_io_buff[idx2].addr;
			b_y2 = p_io_buff[idx2].addr + p_io_buff[idx2].size;
			if (a_x2 > b_x1 && b_x2 > a_x1 &&
				a_y2 > b_y1 && b_y2 > a_y1)
			{
				DBG_WRN("proc_id(%lu) detect buffer overlap !! => buf_id(%lu)(%lu, %lu, %lu, %lu) & buf_id(%lu)(%lu, %lu, %lu, %lu)\r\n", proc_id, idx, a_x1, a_x2, a_y1, a_y2, idx2, b_x1, b_x2, b_y1, b_y2);
				rv = TRUE;
			}
		}
	}

	return rv;
}
#endif // #if CHK_MEMORY_OVERLAP

#if DUMP_MEMORY_OVERLAP
static BOOL nn_manage_dump_dot_buf_node(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 *buf_color, CHAR *filepath)
{
       UINT32 mc_idx = 0, in_idx = 0, out_idx = 0;
       UINT32 in_buf_num  = 0;
       UINT32 out_buf_num = 0;
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

       ////
       {
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
                       BOOL *b_bufsize_set = (BOOL*)vendor_ai_malloc(sizeof(BOOL)*proc_layers_num*2);   //only for debug cmd
#else
                       BOOL *b_bufsize_set = (BOOL*)vendor_ai_malloc(sizeof(BOOL)*proc_layers_num*2);
#endif

                       if (b_bufsize_set != NULL) {
                               memset(b_bufsize_set, 0, sizeof(BOOL)*proc_layers_num*2);

                               for (mc_idx = 0; mc_idx < proc_layers_num; mc_idx ++) {
                                       out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
                                       for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
                                               buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], out_idx);  // p_mctrl[mc_idx].out_buf_index[out_idx];
                                               if ((buf_id >= 0) && (b_bufsize_set[buf_id] == FALSE)){
                                                       b_bufsize_set[buf_id] = TRUE;
                                                       if (0){//(OUT_BUF_IS_TMP(&p_mctrl[mc_idx], out_idx) == TRUE) {
                                                               //tmp buffer
                                                               fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\", color=red]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
                                                       } else if (buf_color[buf_id] == 1) {
                                                               fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\", color=green]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
                                                       } else if (buf_color[buf_id] == 2) {
                                                               fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\", color=yellow]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
                                                       } else {
                                                               fprintf(fd, "   \"(%ld)\"[label=\"(%ld)\\n%lu\"]\r\n", buf_id, buf_id, OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx));  // p_mctrl[mc_idx].output_buffsize[out_idx]
                                                       }
                                               }
                                       }
                               }
#if (NET_DBG_HEAP == 1)
                               vendor_ai_free(b_bufsize_set, sizeof(BOOL)*proc_layers_num*2);   //only for debug cmd
#else
                               vendor_ai_free(b_bufsize_set, sizeof(BOOL)*proc_layers_num*2);
#endif
                       }
               }
               {
                       UINT32 src_buf_id = 0;
                       UINT32 dst_buf_id = 0;
                       char text_color[16]; // engine
                       char edge_color[16]; // mode

                       for (mc_idx = 0; mc_idx < proc_layers_num; mc_idx ++) {
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
       }
       ////
       // close file
       if (filepath != NULL) {
               DBG_DUMP("dump to (%s) success !\n", filepath);
               fclose(fd);
       }
       return TRUE;
}

static BOOL nn_manage_detect_overlap_to_bufdot(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, NN_BUF *p_io_buff)
{
       BOOL rv = FALSE;
       UINT32 idx = 0, idx2 = 0, out_idx = 0;
       UINT32 out_buf_num      = 0;
       UINT32 max_buf_id = 0;
       UINT32 a_y1=0, a_y2=0, b_y1=0, b_y2=0;
       INT32 buf_id = 0;
       char filepath[4096];

       UINT32 *buf_color = (UINT32*)vendor_ai_malloc(sizeof(UINT32)*proc_layers_num*2);   //only for debug cmd

       if (buf_color == NULL) {
               DBG_ERR("proc_id(%lu) fail to alloc b_buf_color UINT32 array\r\n", proc_id);
               return FALSE;
       }

       // find max buf_id
       for (idx = 0; idx < proc_layers_num; idx ++) {
               out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
               for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
                       buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);  // p_mctrl[idx].out_buf_index[out_idx];
                       if (buf_id >=0) max_buf_id = (max_buf_id < (UINT32)buf_id)? (UINT32)buf_id:max_buf_id;
               }
       }

       // find if there're any two buffer overlapped
       for (idx = 0; idx < max_buf_id; idx++) {
               memset(buf_color, 0, sizeof(BOOL)*proc_layers_num*2);
               buf_color[idx] = 1;
               for (idx2 = 0; idx2 <= max_buf_id; idx2++) {
                       if (idx == idx2) continue;
                       a_y1 = p_io_buff[idx].addr;
                       a_y2 = p_io_buff[idx].addr + p_io_buff[idx].size;
                       b_y1 = p_io_buff[idx2].addr;
                       b_y2 = p_io_buff[idx2].addr + p_io_buff[idx2].size;
                       if (a_y2 > b_y1 && b_y2 > a_y1)
                       {
                               buf_color[idx2] = 2;
                       }
               }
               // draw on dot_buf_graph
               snprintf(filepath, 4095, "/mnt/sd/overlap_debug/proc%d_buf%04d_overlap_dot_graph.txt", (int)proc_id, (int)idx);
               nn_manage_dump_dot_buf_node(proc_id, p_mctrl, proc_layers_num, buf_color, filepath);
       }

       vendor_ai_free(buf_color, sizeof(BOOL)*proc_layers_num*2);   //only for debug cmd
       return rv;
}
#endif // DUMP_MEMORY_OVERLAP

static BOOL nn_memory_graph_is_mctrl_end_with_out_buf_id(NN_GEN_MODE_CTRL *p_mctrl_main, INT32 end_out_buf_id)
{
	UINT32 buf_idx=0;
	UINT32 out_buf_num  = OUT_BUF_NUM(p_mctrl_main);
	INT32 out_buf_id=0;

	for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
		out_buf_id = OUT_BUF_INDEX(p_mctrl_main, buf_idx);
		if (out_buf_id < 0) {
			continue;
		}
		if (out_buf_id == end_out_buf_id) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL nn_manage_is_exactly_life_allpre(UINT32 mctrl_main, NN_GEN_MODE_CTRL *p_mctrl, UINT32 id_list_start_addr, int level, BOOL* p_buf_referenced)
{
	BOOL rv = FALSE;
	UINT32 mctrl_pre=0;
	UINT32 j=0, start_ofs = 0, end_ofs = 0;
	UINT32 in_idx = 0, in_buf_num = 0;
	INT32 buf_id = 0;
	BOOL b_first_time = FALSE;

	in_buf_num = IN_BUF_NUM(&p_mctrl[mctrl_main]);
	for (in_idx = 0; in_idx < in_buf_num; in_idx++) {
		buf_id = IN_BUF_INDEX(&p_mctrl[mctrl_main], in_idx);
		if (buf_id < 0) continue;
		b_first_time = (p_buf_referenced[buf_id] == 0)? TRUE:FALSE;
		p_buf_referenced[buf_id]++;

		if (b_first_time) {
			// loop all previous mctrl
			start_ofs = p_mctrl[mctrl_main].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[mctrl_main].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				mctrl_pre = *(UINT32 *)(id_list_start_addr + j);
				if (TRUE == nn_memory_graph_is_mctrl_end_with_out_buf_id(&p_mctrl[mctrl_pre], buf_id)) {
					rv = nn_manage_is_exactly_life_allpre(mctrl_pre, p_mctrl, id_list_start_addr, level+1, p_buf_referenced);
				}
			}
		}
	}

	return rv;
}

static BOOL nn_manage_is_exactly_life(UINT32 buf_id_main, UINT32 buf_id_check, UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 id_list_start_addr, NN_BUF *p_io_buff)
{
	BOOL total_rv = TRUE;
	UINT32 idx = 0, out_idx = 0;
	UINT32 out_buf_num      = 0;
	INT32 buf_id = 0;
	BOOL *p_buf_referenced = (BOOL *)vendor_ai_malloc(sizeof(BOOL) * proc_layers_num * 2);

	if (p_buf_referenced == NULL) {
		DBG_ERR("fail to alloc checked_mctrl_list ...\r\n");
		return FALSE;
	}
	// find which mctrl's OUT_BUF_INDEX = buf_id_main
	for (idx = 0; idx < proc_layers_num; idx ++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);
			if (buf_id == (INT32)buf_id_main) {
				// test every mctrl (who output buf_id_main), check if all previous path make referenced to buf_id_check equal to its life
				memset((void*)p_buf_referenced, 0, sizeof(BOOL)*proc_layers_num*2);
				nn_manage_is_exactly_life_allpre(idx, p_mctrl, id_list_start_addr, 0, p_buf_referenced);
				total_rv = total_rv && (p_buf_referenced[buf_id_check] == p_io_buff[buf_id_check].life);
			}
		}
	}
	if (p_buf_referenced) vendor_ai_free(p_buf_referenced, sizeof(BOOL) * proc_layers_num * 2);
	return total_rv;
}

static BOOL nn_manage_is_mctrl_pre_end_with_bufid(UINT32 mctrl_main, UINT32 buf_id_check, UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 id_list_start_addr, int level, BOOL* checked_mctrl_list)
{
	BOOL rv = FALSE, found = FALSE;
	UINT32 mctrl_pre=0;
	UINT32 j=0, start_ofs = 0, end_ofs = 0;
	UINT32 in_idx = 0, in_buf_num = 0;
	INT32 buf_id = 0;

	in_buf_num = IN_BUF_NUM(&p_mctrl[mctrl_main]);
	for (in_idx = 0; in_idx < in_buf_num; in_idx++) {
		buf_id = IN_BUF_INDEX(&p_mctrl[mctrl_main], in_idx);
		if (buf_id < 0) continue;
		if ((buf_id == (INT32)buf_id_check)) {
			return TRUE;
		}
	}

	// loop all previous mctrl
	start_ofs = p_mctrl[mctrl_main].prev_layer_idx_addr;
	end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[mctrl_main].prev_num;
	for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
		mctrl_pre = *(UINT32 *)(id_list_start_addr + j);
		if (checked_mctrl_list[mctrl_pre] == TRUE) {
			continue;
		} else {
			checked_mctrl_list[mctrl_pre] = TRUE;
		}
		// find if mctrl_pre's IN_BUF_INDEX = buf_id_check
		found = FALSE;
		in_buf_num = IN_BUF_NUM(&p_mctrl[mctrl_pre]);
		for (in_idx = 0; in_idx < in_buf_num; in_idx++) {
			buf_id = IN_BUF_INDEX(&p_mctrl[mctrl_pre], in_idx);
			if (buf_id < 0) continue;
			if ((buf_id == (INT32)buf_id_check)) {
				found = TRUE;
			}
		}
		if (found == TRUE) {
			return TRUE;
		} else {
			rv = nn_manage_is_mctrl_pre_end_with_bufid(mctrl_pre, buf_id_check, proc_id, p_mctrl, id_list_start_addr, level+1, checked_mctrl_list);
			if (rv == TRUE) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOL nn_manage_is_bufid_pre_of_bufid(UINT32 buf_id_main, UINT32 buf_id_check, UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 id_list_start_addr)
{
	BOOL rv = TRUE;
	BOOL total_rv = TRUE;
	UINT32 idx = 0, out_idx = 0;
	UINT32 out_buf_num      = 0;
	INT32 buf_id = 0;
	BOOL *checked_mctrl_list = (BOOL *)vendor_ai_malloc(sizeof(BOOL) * proc_layers_num);

	if (checked_mctrl_list == NULL) {
		DBG_ERR("fail to alloc checked_mctrl_list ...\r\n");
		return FALSE;
	}
	// find which mctrl's OUT_BUF_INDEX = buf_id_main
	for (idx = 0; idx < proc_layers_num; idx ++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);
			if (buf_id == (INT32)buf_id_main) {
				// start from mctrl[idx], find each previous mctrl whoes IN_BUF_INDEX = buf_id_check
				memset((void*)checked_mctrl_list, 0, sizeof(BOOL)*proc_layers_num);
				rv = nn_manage_is_mctrl_pre_end_with_bufid(idx, buf_id_check, proc_id, p_mctrl, id_list_start_addr, 0, checked_mctrl_list);
			}
		}
		total_rv = total_rv && rv;  // total_rv = TRUE , only if every mctrl (who output buf_id_main) can find buf_id_check for previous mctrls
		if (total_rv == FALSE) break;
	}
	if (checked_mctrl_list) vendor_ai_free(checked_mctrl_list, sizeof(BOOL) * proc_layers_num);
	return total_rv;
}

static BOOL nn_manage_is_mctrl_next_start_with_bufid(UINT32 mctrl_main, UINT32 buf_id_check, UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 id_list_start_addr, int level, BOOL* checked_mctrl_list)
{
	BOOL rv = FALSE, found = FALSE;
	UINT32 mctrl_next=0;
	UINT32 j=0, start_ofs = 0, end_ofs = 0;
	UINT32 out_idx = 0, out_buf_num = 0;
	INT32 buf_id = 0;

	out_buf_num = OUT_BUF_NUM(&p_mctrl[mctrl_main]);
	for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
		buf_id = OUT_BUF_INDEX(&p_mctrl[mctrl_main], out_idx);
		if (buf_id < 0) continue;
		if ((buf_id == (INT32)buf_id_check)) {
			return TRUE;
		}
	}

	// loop all next mctrl
	start_ofs = p_mctrl[mctrl_main].next_layer_idx_addr;
	end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[mctrl_main].next_num;
	for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
		mctrl_next = *(UINT32 *)(id_list_start_addr + j);
		if (checked_mctrl_list[mctrl_next] == TRUE) {
			continue;
		} else {
			checked_mctrl_list[mctrl_next] = TRUE;
		}
		// find if mctrl_pre's OUT_BUF_INDEX = buf_id_check
		found = FALSE;
		out_buf_num = OUT_BUF_NUM(&p_mctrl[mctrl_next]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[mctrl_next], out_idx);
			if (buf_id < 0) continue;
			if ((buf_id == (INT32)buf_id_check)) {
				found = TRUE;
			}
		}
		if (found == TRUE) {
			return TRUE;
		} else {
			rv = nn_manage_is_mctrl_next_start_with_bufid(mctrl_next, buf_id_check, proc_id, p_mctrl, id_list_start_addr, level+1, checked_mctrl_list);
			if (rv == TRUE) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOL nn_manage_is_bufid_next_of_bufid(UINT32 buf_id_main, UINT32 buf_id_check, UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 id_list_start_addr)
{
	BOOL rv = TRUE;
	BOOL total_rv = TRUE;
	UINT32 idx = 0, in_idx = 0;
	UINT32 in_buf_num      = 0;
	INT32 buf_id = 0;
	BOOL *checked_mctrl_list = (BOOL *)vendor_ai_malloc(sizeof(BOOL) * proc_layers_num);

	if (checked_mctrl_list == NULL) {
		DBG_ERR("fail to alloc checked_mctrl_list ...\r\n");
		return FALSE;
	}
	// find which mctrl's IN_BUF_INDEX = buf_id_main
	for (idx = 0; idx < proc_layers_num; idx ++) {
		in_buf_num = IN_BUF_NUM(&p_mctrl[idx]);
		for (in_idx = 0; in_idx < in_buf_num; in_idx++) {
			buf_id = IN_BUF_INDEX(&p_mctrl[idx], in_idx);
			if (buf_id == (INT32)buf_id_main) {
				// start from mctrl[idx], find each next mctrl whoes OUT_BUF_INDEX = buf_id_check
				memset((void*)checked_mctrl_list, 0, sizeof(BOOL)*proc_layers_num);
				rv = nn_manage_is_mctrl_next_start_with_bufid(idx, buf_id_check, proc_id, p_mctrl, id_list_start_addr, 0, checked_mctrl_list);
			}
		}
		total_rv = total_rv && rv;  // total_rv = TRUE , only if every mctrl (who input = buf_id_main) can find buf_id_check for next mctrls
		if (total_rv == FALSE) break;
	}
	if (checked_mctrl_list) vendor_ai_free(checked_mctrl_list, sizeof(BOOL) * proc_layers_num);
	return total_rv;
}

static BOOL nn_manage_postfix(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, NN_BUF *p_io_buff, UINT32 id_list_start_addr, UINT32 *end_addr)
{
	BOOL rv = TRUE;
	UINT32 idx = 0, idx2 = 0, out_idx = 0;
	UINT32 out_buf_num      = 0;
	UINT32 max_buf_id = 0;
	UINT32 a_y1=0, a_y2=0, b_y1=0, b_y2=0;
	INT32 buf_id = 0;
	UINT32 fix_idx = 0;

	// find max buf_id
	for (idx = 0; idx < proc_layers_num; idx ++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);  // p_mctrl[idx].out_buf_index[out_idx];
			if (buf_id >=0) max_buf_id = (max_buf_id < (UINT32)buf_id)? (UINT32)buf_id:max_buf_id;
		}
	}
try_again:
	// find if there're any two buffer overlapped
	for (idx = 0; idx < max_buf_id; idx++) {
		for (idx2 = idx+1; idx2 <= max_buf_id; idx2++) {
			if (idx == idx2) continue;
			a_y1 = p_io_buff[idx].addr;
			a_y2 = p_io_buff[idx].addr + p_io_buff[idx].size;
			b_y1 = p_io_buff[idx2].addr;
			b_y2 = p_io_buff[idx2].addr + p_io_buff[idx2].size;
			if (a_y2 > b_y1 && b_y2 > a_y1)
			{
				BOOL is_idx2_pre_of_idx = (nn_manage_is_bufid_pre_of_bufid(idx, idx2, proc_id, p_mctrl, proc_layers_num, id_list_start_addr) && nn_manage_is_bufid_next_of_bufid(idx2, idx, proc_id, p_mctrl, proc_layers_num, id_list_start_addr));
				BOOL is_idx_pre_of_idx2 = (nn_manage_is_bufid_pre_of_bufid(idx2, idx, proc_id, p_mctrl, proc_layers_num, id_list_start_addr) && nn_manage_is_bufid_next_of_bufid(idx, idx2, proc_id, p_mctrl, proc_layers_num, id_list_start_addr));
				BOOL is_exactly_life = (is_idx2_pre_of_idx)? nn_manage_is_exactly_life(idx, idx2, proc_id, p_mctrl, proc_layers_num, id_list_start_addr, p_io_buff):nn_manage_is_exactly_life(idx2, idx, proc_id, p_mctrl, proc_layers_num, id_list_start_addr, p_io_buff);
				if (FALSE == ((is_idx2_pre_of_idx || is_idx_pre_of_idx2) && is_exactly_life))
				{
					fix_idx = (p_io_buff[idx].size < p_io_buff[idx2].size)? idx:idx2;
					p_io_buff[fix_idx].addr = *end_addr;
					*end_addr += p_io_buff[fix_idx].size;
					vendor_ai_net_trace(proc_id, AI_BUF, "proc_id(%d) => [%d][%d] overlap, move [%d] to preserved\r\n", proc_id, idx, idx2, fix_idx);
					goto try_again;
				}
			}
		}
	}
	return rv;
}

static int nn_manage_debug_dump(NN_MANAGER_BUF *nn_manage_buf)
{
	UINT32 buf_idx=0;
	NN_BUF *sel_buff=NULL;

	if (nn_manage_buf->p_buf==NULL) {
		printf("buffer manager not initial\r\n");
		return -1;
	}

	printf("  [nn_manage_buf] (%lu) => ", nn_manage_buf->cnt);
	for (buf_idx=0; buf_idx<nn_manage_buf->cnt; buf_idx++) {
		sel_buff = &nn_manage_buf->p_buf[buf_idx];
		printf("<%d, 0x%08x, %lu, %lu> ", (int)sel_buff->id, (UINT)sel_buff->addr, sel_buff->size, sel_buff->life);
	}
	printf("\n");

	return 0;
}

static INT32 nn_manage_new_buf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF buf)
{
	NN_BUF *p_new_buff;
	UINT32 buf_idx = nn_manage_buf->cnt;
	if (nn_manage_buf->p_buf==NULL) {
		printf("buffer manager not initial\r\n");
		return -1;
	}
	
	if (nn_manage_buf->cnt>0) {
		NN_BUF *p_pre_buff = &nn_manage_buf->p_buf[buf_idx-1];
		NN_BUF *p_cur_buff = &nn_manage_buf->p_buf[buf_idx];
		memcpy(p_cur_buff, &buf, sizeof(NN_BUF));
		p_cur_buff->addr = p_pre_buff->addr + p_pre_buff->size;
	} else {
		p_new_buff = &nn_manage_buf->p_buf[0];
		memcpy(p_new_buff, &buf, sizeof(NN_BUF));
	}

	nn_manage_buf->cnt++;
	return buf_idx;
}

static void nn_manage_del_buf(NN_MANAGER_BUF *nn_manage_buf)
{
	UINT32 buf_idx=0;
	for (buf_idx=0; buf_idx<nn_manage_buf->cnt; buf_idx++) {
		UINT32 merge_cnt=0, merge_idx=0;
		if (nn_manage_buf->p_buf[buf_idx].life != 0) {
			continue;
		}
		
		for (merge_idx=(buf_idx+1); merge_idx<nn_manage_buf->cnt; merge_idx++) {
			if (nn_manage_buf->p_buf[merge_idx].life != 0) {
				break;
			}
		}

		merge_cnt = merge_idx - buf_idx - 1;
		/*if (merge_cnt == 0) {
			continue;
		}*/

		if (merge_idx<nn_manage_buf->cnt) {
			memmove(&nn_manage_buf->p_buf[buf_idx+1], &nn_manage_buf->p_buf[merge_idx], sizeof(NN_BUF)*(nn_manage_buf->cnt-merge_idx));
			if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG) {
				// original AI1 has BUG !!
				nn_manage_buf->p_buf[buf_idx].size =  nn_manage_buf->p_buf[merge_idx].addr - nn_manage_buf->p_buf[buf_idx].addr;
			} else {
				// correct way, fix AI1 BUG
				nn_manage_buf->p_buf[buf_idx].size =  nn_manage_buf->p_buf[buf_idx+1].addr - nn_manage_buf->p_buf[buf_idx].addr;
			}
			nn_manage_buf->p_buf[buf_idx].id = INVALID_BUFID;
			nn_manage_buf->cnt-= merge_cnt;
		} else {
			nn_manage_buf->p_buf[buf_idx].size = 0;
			nn_manage_buf->p_buf[buf_idx].id = INVALID_BUFID;
			nn_manage_buf->cnt-= (merge_cnt+1);
		}		
	}
}


static INT32 nn_manage_find_emptybuf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF buf)
{
	UINT32 buf_idx, i;
	if (nn_manage_buf->p_buf==NULL) {
		printf("buffer manager not initial\r\n");
		return -1;
	}
		
    for (buf_idx=0; buf_idx<nn_manage_buf->cnt; buf_idx++) {
		NN_BUF sel_buff = nn_manage_buf->p_buf[buf_idx];
        if ((sel_buff.life == 0) && (sel_buff.size > buf.size)) {
			memcpy(&nn_manage_buf->p_buf[buf_idx], &buf, sizeof(NN_BUF));
			//split buffer
			if ((buf_idx+1) < nn_manage_buf->cnt) {
				#if 0   // vsgen code will result in wrong answer, we can't memcpy for two overlap memory. This works for PC, but EVB failed!
				memcpy(&p_g_nn_manage_buf[buf_idx+1], &p_g_nn_manage_buf[buf_idx], sizeof(NN_BUF)*(g_nn_manage_buf_cnt-buf_idx));
				#else   // modify to one-by-one copy
				for (i = nn_manage_buf->cnt; i > buf_idx ; i--) {
					memcpy(&nn_manage_buf->p_buf[i], &nn_manage_buf->p_buf[i-1], sizeof(NN_BUF));
				}
				#endif
			}
			nn_manage_buf->p_buf[buf_idx].addr = sel_buff.addr;
			nn_manage_buf->p_buf[buf_idx+1].addr = sel_buff.addr + buf.size;
			nn_manage_buf->p_buf[buf_idx+1].size = sel_buff.size - buf.size;
			if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SIM_AI1_BUG) {
				// original AI1 BUG, didn't set life=0
			} else {
				nn_manage_buf->p_buf[buf_idx+1].life = 0;
			}
			nn_manage_buf->cnt++;
			return buf_idx;
		} else if ((sel_buff.life == 0) && (sel_buff.size == buf.size) ) {
			//buffer selected
			memcpy(&nn_manage_buf->p_buf[buf_idx], &buf, sizeof(NN_BUF));
			nn_manage_buf->p_buf[buf_idx].addr = sel_buff.addr;
			return buf_idx;
		}
	}
	buf_idx = nn_manage_new_buf(nn_manage_buf, buf);
    return buf_idx;
}

static INT32 nn_manage_add_pseudo_buf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF buf)
{
	UINT32 buf_idx, i=0;
	UINT32 buf_s, buf_e, emp_s, emp_e, overlap_s, overlap_e;

	if (nn_manage_buf->p_buf==NULL) {
		printf("buffer manager not initial\r\n");
		return -1;
	}

	for (buf_idx=0; buf_idx<nn_manage_buf->cnt; buf_idx++) {
		NN_BUF sel_buff = nn_manage_buf->p_buf[buf_idx];
		if (sel_buff.life == 0) {
			buf_s = buf.addr;
			buf_e = buf.addr + buf.size;
			emp_s = sel_buff.addr;
			emp_e = sel_buff.addr + sel_buff.size;
			overlap_s = MAX(buf_s, emp_s);
			overlap_e = MIN(buf_e, emp_e);

			if (overlap_e > overlap_s) {
				// add pseudo block
				if (overlap_s == emp_s && overlap_e == emp_e) {
					memcpy(&nn_manage_buf->p_buf[buf_idx], &buf, sizeof(NN_BUF));
					nn_manage_buf->p_buf[buf_idx].addr = sel_buff.addr;
					nn_manage_buf->p_buf[buf_idx].size = overlap_e - overlap_s;
					nn_manage_buf->p_buf[buf_idx].life = 1;
					nn_manage_buf->p_buf[buf_idx].id   = buf.id;
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", buf_idx, (UINT)nn_manage_buf->p_buf[buf_idx].addr, nn_manage_buf->p_buf[buf_idx].size, (int)nn_manage_buf->p_buf[buf_idx].id);
				}
				else if (overlap_s == emp_s && overlap_e < emp_e) {
					for (i = nn_manage_buf->cnt; i > buf_idx ; i--) {
						memcpy(&nn_manage_buf->p_buf[i], &nn_manage_buf->p_buf[i-1], sizeof(NN_BUF));
					}

					nn_manage_buf->p_buf[buf_idx].addr = overlap_s;
					nn_manage_buf->p_buf[buf_idx].size = overlap_e - overlap_s;
					nn_manage_buf->p_buf[buf_idx].life = 1;
					nn_manage_buf->p_buf[buf_idx].id   = buf.id;

					nn_manage_buf->p_buf[buf_idx+1].addr = overlap_e;
					nn_manage_buf->p_buf[buf_idx+1].size = emp_e - overlap_e;
					nn_manage_buf->p_buf[buf_idx+1].life = 0;
					nn_manage_buf->p_buf[buf_idx+1].id   = INVALID_BUFID;
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", buf_idx, (UINT)nn_manage_buf->p_buf[buf_idx].addr, nn_manage_buf->p_buf[buf_idx].size, (int)nn_manage_buf->p_buf[buf_idx].id);
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu)\n", buf_idx+1, (UINT)nn_manage_buf->p_buf[buf_idx+1].addr, nn_manage_buf->p_buf[buf_idx+1].size);
					nn_manage_buf->cnt++;
				}
				else if (overlap_s > emp_s && overlap_e == emp_e) {
					for (i = nn_manage_buf->cnt; i > buf_idx ; i--) {
						memcpy(&nn_manage_buf->p_buf[i], &nn_manage_buf->p_buf[i-1], sizeof(NN_BUF));
					}
					nn_manage_buf->p_buf[buf_idx].addr = emp_s;
					nn_manage_buf->p_buf[buf_idx].size = overlap_s - emp_s;
					nn_manage_buf->p_buf[buf_idx].life = 0;
					nn_manage_buf->p_buf[buf_idx].id   = INVALID_BUFID;

					nn_manage_buf->p_buf[buf_idx+1].addr = overlap_s;
					nn_manage_buf->p_buf[buf_idx+1].size = overlap_e - overlap_s;
					nn_manage_buf->p_buf[buf_idx+1].life = 1;
					nn_manage_buf->p_buf[buf_idx+1].id   = buf.id;
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu)\n", buf_idx, (UINT)nn_manage_buf->p_buf[buf_idx].addr, nn_manage_buf->p_buf[buf_idx].size);
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", buf_idx+1, (UINT)nn_manage_buf->p_buf[buf_idx+1].addr, nn_manage_buf->p_buf[buf_idx+1].size, (int)nn_manage_buf->p_buf[buf_idx+1].id);
					nn_manage_buf->cnt++;
				}
				else if (overlap_s > emp_s && overlap_e < emp_e) {
					for (i = nn_manage_buf->cnt; i > buf_idx ; i--) {
						memcpy(&nn_manage_buf->p_buf[i+1], &nn_manage_buf->p_buf[i-1], sizeof(NN_BUF));
					}
					nn_manage_buf->p_buf[buf_idx].addr = emp_s;
					nn_manage_buf->p_buf[buf_idx].size = overlap_s - emp_s;
					nn_manage_buf->p_buf[buf_idx].life = 0;
					nn_manage_buf->p_buf[buf_idx].id   = INVALID_BUFID;

					nn_manage_buf->p_buf[buf_idx+1].addr = overlap_s;
					nn_manage_buf->p_buf[buf_idx+1].size = overlap_e - overlap_s;
					nn_manage_buf->p_buf[buf_idx+1].life = 1;
					nn_manage_buf->p_buf[buf_idx+1].id   = buf.id;

					nn_manage_buf->p_buf[buf_idx+2].addr = overlap_e;
					nn_manage_buf->p_buf[buf_idx+2].size = emp_e - overlap_e;
					nn_manage_buf->p_buf[buf_idx+2].life = 0;
					nn_manage_buf->p_buf[buf_idx+2].id   = INVALID_BUFID;
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu)\n", buf_idx, (UINT)nn_manage_buf->p_buf[buf_idx].addr, nn_manage_buf->p_buf[buf_idx].size);
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", buf_idx+1, (UINT)nn_manage_buf->p_buf[buf_idx+1].addr, nn_manage_buf->p_buf[buf_idx+1].size, (int)nn_manage_buf->p_buf[buf_idx+1].id);
					mem_shrink_dbg("\t      => pseudo mamage(%lu) (0x%08x, %lu)\n", buf_idx+2, (UINT)nn_manage_buf->p_buf[buf_idx+2].addr, nn_manage_buf->p_buf[buf_idx+2].size);
					nn_manage_buf->cnt += 2;
				}
			} // if (overlap_e > overlap_s)
		} // if (sel_buff.life == 0)
	}

	// check tail
	{
		UINT32 las_e = 0;
		INT32 last_idx = 0;
		NN_BUF las_buff = {0};

		last_idx = nn_manage_buf->cnt-1;
		buf_s = buf.addr;
		buf_e = buf.addr + buf.size;

		if (nn_manage_buf->cnt) {
			las_buff = nn_manage_buf->p_buf[last_idx]; // last block
			las_e = las_buff.addr + las_buff.size;
		} else { // means it's empty, start from offset=0
			las_e = 0;
		}

		if (buf_s > las_e) {
			nn_manage_buf->p_buf[last_idx+1].addr = las_e;
			nn_manage_buf->p_buf[last_idx+1].size = buf_s - las_e;
			nn_manage_buf->p_buf[last_idx+1].life = 0;
			nn_manage_buf->p_buf[last_idx+1].id   = INVALID_BUFID;

			nn_manage_buf->p_buf[last_idx+2].addr = buf_s;
			nn_manage_buf->p_buf[last_idx+2].size = buf_e - buf_s;
			nn_manage_buf->p_buf[last_idx+2].life = 1;
			nn_manage_buf->p_buf[last_idx+2].id   = buf.id;
			mem_shrink_dbg("\t      => (last_1)pseudo mamage(%lu) (0x%08x, %lu)\n", last_idx+1, (UINT)nn_manage_buf->p_buf[last_idx+1].addr, nn_manage_buf->p_buf[last_idx+1].size);
			mem_shrink_dbg("\t      => (last_1)pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", last_idx+2, (UINT)nn_manage_buf->p_buf[last_idx+2].addr, nn_manage_buf->p_buf[last_idx+2].size, (int)nn_manage_buf->p_buf[last_idx+2].id);
			nn_manage_buf->cnt += 2;
		}
		else if (buf_s == las_e) {
			nn_manage_buf->p_buf[last_idx+1].addr = buf_s;
			nn_manage_buf->p_buf[last_idx+1].size = buf_e - buf_s;
			nn_manage_buf->p_buf[last_idx+1].life = 1;
			nn_manage_buf->p_buf[last_idx+1].id   = buf.id;
			mem_shrink_dbg("\t      => (last_2)pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", last_idx+1, (UINT)nn_manage_buf->p_buf[last_idx+1].addr, nn_manage_buf->p_buf[last_idx+1].size, (int)nn_manage_buf->p_buf[last_idx+1].id);
			nn_manage_buf->cnt ++;
		}
		else if (buf_s < las_e && buf_e > las_e) {
			nn_manage_buf->p_buf[last_idx + 1].addr = las_e;
			nn_manage_buf->p_buf[last_idx + 1].size = buf_e - las_e;
			nn_manage_buf->p_buf[last_idx + 1].life = 1;
			nn_manage_buf->p_buf[last_idx + 1].id = buf.id;
			mem_shrink_dbg("\t      => (last_3)pseudo mamage(%lu) (0x%08x, %lu) i_id(%d)\n", last_idx+1, (UINT)nn_manage_buf->p_buf[last_idx+1].addr, nn_manage_buf->p_buf[last_idx+1].size, (int)nn_manage_buf->p_buf[last_idx+1].id);
			nn_manage_buf->cnt ++;
		}
	}

    return buf_idx;
}

static INT32 nn_manage_find_buf(NN_MANAGER_BUF *nn_manage_buf, UINT32 id)
{
	NN_BUF *p_cur_buff;
	UINT32 buf_idx=0;
	if (nn_manage_buf->p_buf==NULL) {
		printf("buffer manager not initial\r\n");
		return -1;
	}
		
    for (buf_idx=0; buf_idx<nn_manage_buf->cnt; buf_idx++) {
		p_cur_buff = &nn_manage_buf->p_buf[buf_idx];
		if (p_cur_buff->id == (INT32)id) {
            return buf_idx;
		}
	}
    return -1;
}

#if MEMORY_REORDER
static INT32 nn_reorder_overlap_with_buf_id(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf)
{
	INT32 overlap_id = (-1);
	UINT32 idx = 0;
	UINT32 a_x1 = 0, a_x2 = 0, a_y1 = 0, a_y2 = 0, b_x1 = 0, b_x2 = 0, b_y1 = 0, b_y2 = 0;

	a_x1 = buf->start_t;
	a_x2 = buf->end_t;
	a_y1 = buf->addr;
	a_y2 = buf->addr + buf->size;

	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		if (nn_manage_buf->p_buf[idx].id == buf->id)
			continue;

		b_x1 = nn_manage_buf->p_buf[idx].start_t;
		b_x2 = nn_manage_buf->p_buf[idx].end_t;
		b_y1 = nn_manage_buf->p_buf[idx].addr;
		b_y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
		if (a_x2 > b_x1 && b_x2 > a_x1 &&
			a_y2 > b_y1 && b_y2 > a_y1)
		{
			overlap_id = idx;
		}
	}
	return overlap_id;
}

static NN_BUF* nn_reorder_top_touch_with_buf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf, BOOL is_leftmost)
{
	NN_BUF* top_touch_buf = NULL;
	UINT32 idx = 0;
	UINT32 a_x1 = 0, a_x2 = 0, a_y2 = 0, b_x1 = 0, b_x2 = 0, b_y1 = 0;

	a_x1 = buf->start_t;
	a_x2 = buf->end_t;
	a_y2 = buf->addr + buf->size;

	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		if (nn_manage_buf->p_buf[idx].id == buf->id)
			continue;

		b_x1 = nn_manage_buf->p_buf[idx].start_t;
		b_x2 = nn_manage_buf->p_buf[idx].end_t;
		b_y1 = nn_manage_buf->p_buf[idx].addr;
		if (a_x2 > b_x1 && b_x2 > a_x1 &&
			a_y2 == b_y1)
		{
			if (is_leftmost == FALSE) {
				top_touch_buf = &nn_manage_buf->p_buf[idx];
				break; // just return first found buf
			}
			// find leftmost buf
			if (top_touch_buf == NULL) {
				top_touch_buf = &nn_manage_buf->p_buf[idx];
			} else {
				if (b_x1 < top_touch_buf->start_t) {
					top_touch_buf = &nn_manage_buf->p_buf[idx];
				}
			}
		}
	}
	return top_touch_buf;
}
/*
static NN_BUF* nn_reorder_topmost_with_buf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf)
{
	NN_BUF* topmost_buf = NULL, *tmp = NULL;
	topmost_buf = buf;
	do {
		tmp = nn_reorder_top_touch_with_buf(nn_manage_buf, topmost_buf, TRUE);
		if (tmp) topmost_buf = tmp;
	} while (tmp != NULL);

	return topmost_buf;
}
*/
static NN_BUF* nn_reorder_below_touch_with_buf(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf)
{
	NN_BUF* below_touch_buf = NULL;
	UINT32 idx = 0;
	UINT32 a_x1 = 0, a_x2 = 0, a_y1 = 0, b_x1 = 0, b_x2 = 0, b_y2 = 0;

	a_x1 = buf->start_t;
	a_x2 = buf->end_t;
	a_y1 = buf->addr;

	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		if (nn_manage_buf->p_buf[idx].id == buf->id)
			continue;

		b_x1 = nn_manage_buf->p_buf[idx].start_t;
		b_x2 = nn_manage_buf->p_buf[idx].end_t;
		b_y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
		if (a_x2 > b_x1 && b_x2 > a_x1 &&
			a_y1 == b_y2)
		{
			below_touch_buf = &nn_manage_buf->p_buf[idx];
			break;
		}
	}
	return below_touch_buf;
}

static INT32 nn_reorder_find_lowest_fit(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf)
{
	NN_BUF tmp_buf = { 0 };
	UINT32 idx;
	INT32 overlap_id;

	memcpy(&tmp_buf, buf, sizeof(NN_BUF));

	tmp_buf.addr = 0; // start search from addr 0

TRY_AGAIN:
	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		if (nn_manage_buf->p_buf[idx].id == buf->id)
			continue;

		overlap_id = nn_reorder_overlap_with_buf_id(nn_manage_buf, &tmp_buf);
		if (overlap_id >= 0) {
			tmp_buf.addr = nn_manage_buf->p_buf[overlap_id].addr + nn_manage_buf->p_buf[overlap_id].size;
			goto TRY_AGAIN; // try again
		}
	}
	memcpy(buf, &tmp_buf, sizeof(NN_BUF));

	return 0;
}

static UINT32 nn_reorder_get_highest_free_addr(NN_MANAGER_BUF *nn_manage_buf, UINT32 time)
{
	UINT32 free_addr = 0, idx = 0;
	UINT32 b_x1 = 0, b_x2 = 0, b_y2;

	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		b_x1 = nn_manage_buf->p_buf[idx].start_t;
		b_x2 = nn_manage_buf->p_buf[idx].end_t;

		if (b_x1 <= time && time < b_x2 )
		{
			b_y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
			free_addr = MAX(free_addr, b_y2);
		}
	}

	return free_addr;
}

static UINT32 nn_reorder_get_lowest_free_addr(NN_MANAGER_BUF *nn_manage_buf, UINT32 time)
{
	UINT32 free_addr = 0, idx = 0;
	UINT32 b_x1 = 0, b_x2 = 0;

TRY_AGAIN:
	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		b_x1 = nn_manage_buf->p_buf[idx].start_t;
		b_x2 = nn_manage_buf->p_buf[idx].end_t;

		if (b_x1 <= time && time < b_x2 &&
		    free_addr == nn_manage_buf->p_buf[idx].addr)
		{
			free_addr = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
			goto TRY_AGAIN; // try again
		}
	}

	return free_addr;
}

static INT32 nn_reorder_drop_all(NN_MANAGER_BUF *nn_manage_buf)
{
	UINT32 idx = 0;
	NN_BUF *below_touch_buf = NULL;
	BOOL is_all_ground_stable = TRUE;
	UINT32 lowest_y1 = 0xFFFFFFFF;
	UINT32 lowest_idx = 0;

	do {
		is_all_ground_stable = TRUE;
		lowest_y1 = 0xFFFFFFFF;
		lowest_idx = 0;

		for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
			if (nn_manage_buf->p_buf[idx].addr == 0) {
				continue; // this buffer already on ground
			}

			// check if this buffer is onto some other buffer
			below_touch_buf = nn_reorder_below_touch_with_buf(nn_manage_buf, &nn_manage_buf->p_buf[idx]);
			if (below_touch_buf == NULL) {
				// this buffer is floating
				is_all_ground_stable = FALSE;

				if (lowest_y1 > nn_manage_buf->p_buf[idx].addr) {
					lowest_y1 = nn_manage_buf->p_buf[idx].addr;
					lowest_idx = idx;
				}
			}
		}
		if (is_all_ground_stable == FALSE) {
			nn_reorder_find_lowest_fit(nn_manage_buf, &nn_manage_buf->p_buf[lowest_idx]);
		}
	} while (is_all_ground_stable == FALSE);

	return 0;
}

static UINT32 nn_reorder_float_other_and_put_test(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf, UINT32 test_addr, BOOL do_recover)
{
	NN_BUF tmp_buf = { 0 };
	UINT32 idx;
	UINT32 a_y1 = 0, b_y2 = 0;
	UINT32 new_end_addr = 0, y2=0;
	UINT32 backup_idx = 0, backup_cnt = 0;

	memcpy(&tmp_buf, buf, sizeof(NN_BUF));

	// find sutiable addr for me
	{
		NN_BUF *below_buf = NULL;
		NN_BUF *top_buf = NULL;
		UINT32 top_buf_end_t_min = 0xFFFFFFFF;

		// assume me on lowest_free_addr first
		tmp_buf.addr = test_addr;

		// (1) find if there's any below_buf.end_t = my.end_t , move onto it
		below_buf = &tmp_buf;
		while (1) {
			below_buf = nn_reorder_below_touch_with_buf(nn_manage_buf, below_buf);
			if (below_buf == NULL)
				break;

			if (below_buf->end_t == tmp_buf.end_t) {
				tmp_buf.addr = below_buf->addr + below_buf->size;
				goto FLOAT_OTHER;
			}
		}

		// (2) no one equal to me, move my self down, sort by end_t to insert (if my.start_t < any top_buf.end_t , etc, my insertion will still be the critical path)
		while (1) {
			below_buf = nn_reorder_below_touch_with_buf(nn_manage_buf, &tmp_buf);
			if (below_buf == NULL)
				break;

			// search all top_buf,  find min end_t
			top_buf = below_buf;
			do {
				top_buf = nn_reorder_top_touch_with_buf(nn_manage_buf, top_buf, TRUE);
				if (top_buf) top_buf_end_t_min = MIN(top_buf_end_t_min, top_buf->end_t);
			} while (top_buf != NULL);

			// check if I can move down
			if (tmp_buf.end_t < below_buf->end_t &&
				tmp_buf.start_t < top_buf_end_t_min)
			{
				// move myself down
				tmp_buf.addr = below_buf->addr;
				continue;
			}
			break;
		}
	}

FLOAT_OTHER:

	if (do_recover) {
		// copy for backup , prepare to test run
		backup_idx = nn_manage_buf->cnt + 1; // skip 1 , because later will add one
		backup_cnt = nn_manage_buf->cnt;
		memcpy(&nn_manage_buf->p_buf[backup_idx], &nn_manage_buf->p_buf[0], sizeof(NN_BUF)*backup_cnt);
	}

	// float those who is higher than me
	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		a_y1 = tmp_buf.addr;
		b_y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
		if (b_y2 > a_y1)
		{
			nn_manage_buf->p_buf[idx].addr += tmp_buf.size + 1;  // float
		}
	}

	// add me to watch list now
	memcpy(&nn_manage_buf->p_buf[nn_manage_buf->cnt], &tmp_buf, sizeof(NN_BUF));
	nn_manage_buf->cnt++;

	// drop all
	nn_reorder_drop_all(nn_manage_buf);

	if (do_recover) {
		// find new_end
		for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
			y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
			new_end_addr = (new_end_addr < y2) ? y2 : new_end_addr;
		}

		// test done, roll back !
		memcpy(&nn_manage_buf->p_buf[0], &nn_manage_buf->p_buf[backup_idx], sizeof(NN_BUF)*backup_cnt);
		nn_manage_buf->cnt = backup_cnt;
	}

	return new_end_addr;
}

static INT32 nn_reorder_float_other_and_put(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *buf)
{
	UINT32 lowest_free_addr = 0;
	UINT32 highest_free_addr = 0;
	UINT32 new_end_addr_low = 0;
	UINT32 new_end_addr_high = 0;

	// get lowest free addr
	lowest_free_addr = nn_reorder_get_lowest_free_addr(nn_manage_buf, buf->start_t);
	new_end_addr_low = nn_reorder_float_other_and_put_test(nn_manage_buf, buf, lowest_free_addr, TRUE);

	// get highest free addr
	highest_free_addr = nn_reorder_get_highest_free_addr(nn_manage_buf, buf->start_t);
	new_end_addr_high = nn_reorder_float_other_and_put_test(nn_manage_buf, buf, highest_free_addr, TRUE);

	if (new_end_addr_low <= new_end_addr_high) {
		nn_reorder_float_other_and_put_test(nn_manage_buf, buf, lowest_free_addr, FALSE);
	} else {
		nn_reorder_float_other_and_put_test(nn_manage_buf, buf, highest_free_addr, FALSE);
	}
	return 0;
}

static INT32 nn_reorder_update_p_io_buff(NN_MANAGER_BUF *nn_manage_buf, NN_BUF *p_io_buff, UINT32 *end_addr)
{
	UINT32 idx = 0;
	UINT32 buf_id = 0;
	UINT32 y2 = 0;
	UINT32 new_end_addr = 0;
	UINT32 g_y1=0;
	NN_BUF *cur_buf = NULL;

	// find new_end_addr
	for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
		y2 = nn_manage_buf->p_buf[idx].addr + nn_manage_buf->p_buf[idx].size;
		new_end_addr = (new_end_addr < y2) ? y2 : new_end_addr;
	}

	// if reorder is better than original
	if (new_end_addr < *end_addr) {
		for (idx = 0; idx < nn_manage_buf->cnt; idx++) {
			g_y1    = nn_manage_buf->p_buf[idx].addr;
			cur_buf = nn_manage_buf->p_buf[idx].next;
			while (cur_buf != NULL) {
				buf_id = cur_buf->id;
				p_io_buff[buf_id].addr = g_y1 + cur_buf->addr;  // new addr + offset
				cur_buf = cur_buf->next;
			}
		}
		*end_addr = new_end_addr;
	} else {
		vendor_ai_net_trace(0, AI_BUF, "reorder(%d) >= original(%d)...skip reorder\n", (int)new_end_addr, (int)*end_addr);
	}
	return 0;
}

#if MEMORY_REORDER_DEBUG
static INT32 nn_reorder_debug_dump(NN_MANAGER_BUF *nn_manage_buf, UINT32 fn)
{
	GRAPH_DEBUG_BUFFER_HANDLER graph_debug_hdl = (-1);
	UINT32 buf_id = 0;
	UINT32 mc_idx = 0;
	CHAR blk_name[20];
	CHAR filepath[32];

	snprintf(filepath, 32, "/mnt/sd/nn_mem_%03lu.html", fn);

	graph_debug_buffer_open(filepath, &graph_debug_hdl);

	if (graph_debug_hdl == (-1)) {
		printf("open file (%s) fail ...\r\n", filepath);
		return -1;
	}

	for (mc_idx = 0; mc_idx < nn_manage_buf->cnt; mc_idx++) {

		buf_id = (UINT32)nn_manage_buf->p_buf[mc_idx].id;
		snprintf(blk_name, 20, "%lu", buf_id);

		graph_debug_buffer_add_block(graph_debug_hdl,
			nn_manage_buf->p_buf[mc_idx].start_t,
			nn_manage_buf->p_buf[mc_idx].end_t,
			nn_manage_buf->p_buf[mc_idx].addr,
			nn_manage_buf->p_buf[mc_idx].addr + nn_manage_buf->p_buf[mc_idx].size,
			blk_name,
			20);
	}

	graph_debug_buffer_close(graph_debug_hdl);
	return 0;
}
#endif  // MEMORY_REORDER_DEBUG

static INT32 nn_reorder_form_map_buff(NN_BUF *p_pre_buf, NN_BUF *p_map_buff, UINT32 max_buf_id, UINT32 proc_layers_num, UINT32 *max_map_num)
{
	UINT32 idx = 0, idx2 = 0;
	UINT32 m_buf_idx = 0; //merge buf idx
	UINT32 group_idx = proc_layers_num*2;
	UINT32 a_x1=0, a_x2=0, a_y1=0, a_y2=0, b_x1=0, b_x2=0, b_y1=0, b_y2=0;
	UINT32 g_x1=0, g_x2=0, g_y1=0, g_y2=0;
	NN_BUF *cur_buf = NULL, *last_buf = NULL;

	// reset is_group
	for (idx = 0; idx < max_buf_id; idx++) {
		p_pre_buf[idx].is_group = FALSE;
	}

	// p_io_buff -> p_map_buff
	for (idx = 0; idx <= max_buf_id; idx++) {
		if (p_pre_buf[idx].is_group) {
			continue; // this buf_id already form group with previous buf_id, skip this buf_id
		}

		//--- (1) find all overlap block first ---
		//-- init for first block --
		cur_buf = &p_map_buff[group_idx];
		last_buf = cur_buf;
		cur_buf->id = idx;
		cur_buf->addr = p_pre_buf[idx].addr;
		cur_buf->next = NULL;
		group_idx++;

		p_pre_buf[idx].is_group = TRUE;

		p_map_buff[m_buf_idx].id      = (INT32)m_buf_idx;
		p_map_buff[m_buf_idx].start_t = 0; // update later
		p_map_buff[m_buf_idx].end_t   = 0; // update later
		p_map_buff[m_buf_idx].addr    = 0; // update later
		p_map_buff[m_buf_idx].size    = 0; // update later
		p_map_buff[m_buf_idx].next    = cur_buf;

		//-- check block overlap --
		do {
			UINT32 cur_idx = cur_buf->id;

			a_x1 = p_pre_buf[cur_idx].start_t;
			a_x2 = p_pre_buf[cur_idx].end_t;
			a_y1 = p_pre_buf[cur_idx].addr;
			a_y2 = p_pre_buf[cur_idx].addr + p_pre_buf[cur_idx].size;

			for (idx2 = 0; idx2 <= max_buf_id; idx2++) {
				if (idx2 == cur_idx) continue; // skip self
				if (p_pre_buf[idx2].is_group) {
					continue; // this buf_id already form group with previous buf_id, skip this buf_id
				}

				b_x1 = p_pre_buf[idx2].start_t;
				b_x2 = p_pre_buf[idx2].end_t;
				b_y1 = p_pre_buf[idx2].addr;
				b_y2 = p_pre_buf[idx2].addr + p_pre_buf[idx2].size;
				if (a_x2 > b_x1 && b_x2 > a_x1 && a_y2 > b_y1 && b_y2 > a_y1)
				{
					// overlap, add group
					last_buf->next = &p_map_buff[group_idx];
					last_buf = last_buf->next;
					last_buf->id = idx2;
					last_buf->addr = b_y1;
					last_buf->next = NULL;
					group_idx++;

					p_pre_buf[idx2].is_group = TRUE;
				}
			}
			cur_buf = cur_buf->next;
		} while (cur_buf != NULL);

		//--- (2) find bounding box (g_x1, g_x2, g_y1, g_y2) ---
		cur_buf = p_map_buff[m_buf_idx].next;

		g_x1 = 0xFFFFFFFF;
		g_x2 = 0;
		g_y1 = 0xFFFFFFFF;
		g_y2 = 0;

		do {
			UINT32 cur_idx = cur_buf->id;

			a_x1 = p_pre_buf[cur_idx].start_t;
			a_x2 = p_pre_buf[cur_idx].end_t;
			a_y1 = p_pre_buf[cur_idx].addr;
			a_y2 = p_pre_buf[cur_idx].addr + p_pre_buf[cur_idx].size;

			g_x1 = MIN(g_x1, a_x1);
			g_x2 = MAX(g_x2, a_x2);
			g_y1 = MIN(g_y1, a_y1);
			g_y2 = MAX(g_y2, a_y2);

			cur_buf = cur_buf->next;
		} while (cur_buf != NULL);

		p_map_buff[m_buf_idx].start_t = g_x1;
		p_map_buff[m_buf_idx].end_t   = g_x2;
		p_map_buff[m_buf_idx].addr    = g_y1;
		p_map_buff[m_buf_idx].size    = g_y2-g_y1;

		//--- (3) update all block addr to offset(relate to g_y1)
		cur_buf = p_map_buff[m_buf_idx].next;

		do {
			UINT32 cur_idx = cur_buf->id;
			cur_buf->addr = p_pre_buf[cur_idx].addr - g_y1;

			cur_buf = cur_buf->next;
		} while (cur_buf != NULL);

		//--- (4) update m_buf_idx ---
		m_buf_idx++;
	}

	*max_map_num = m_buf_idx;
	return 0;
}

static INT32 nn_reorder_linear_overlap_shrink(NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_pre_buf, UINT32 proc_layers_num, UINT32 end_addr)
{
	UINT32 idx=0, buf_idx=0;
	INT32 out_buf_id=0, in_buf_id=0;
	UINT32 out_buf_num = 0;
	UINT32 in_buf_num = 0;
	INT32 in_buf_id_max = 0xFFFFFFFF;

	for (idx = 0; idx<proc_layers_num; idx++) {
		in_buf_num  = IN_BUF_NUM(&p_mctrl[idx]);
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);

		for (buf_idx = 0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].out_buf_index[buf_idx];
			if (out_buf_id >= 0) break;
		}

		in_buf_id_max = 0xFFFFFFFF;
		for (buf_idx = 0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].in_buf_index[buf_idx]
			if (in_buf_id >= 0) in_buf_id_max = MAX(in_buf_id_max, in_buf_id);
		}
		in_buf_id = in_buf_id_max;
		if (in_buf_id < 0) continue; // skip first mctrl ( all SAI = 0)

#if 1
		// CONV & ELTWISE
		if (p_mctrl[idx].mode == NN_ELTWISE || p_mctrl[idx].mode == NN_CONV) {
			if ((p_pre_buf[in_buf_id].life == 1) && (p_pre_buf[out_buf_id].life_o == 1) && (p_pre_buf[out_buf_id].size <= p_pre_buf[in_buf_id].size)) {
				p_pre_buf[in_buf_id].addr  = end_addr + 1;
				p_pre_buf[out_buf_id].addr = end_addr + 1;
				end_addr += 1 + p_pre_buf[in_buf_id].size;
				//DBG_IND("[overlap] mctrl(%lu) normal case , in(%ld) out(%ld)\r\n", idx, in_buf_id, out_buf_id);
			}
		}
#endif
		// update input life
		for (buf_idx = 0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].in_buf_index[buf_idx];
			if (in_buf_id >= 0) p_pre_buf[in_buf_id].life--;
		}
	}
	return 0;
}

static INT32 nn_reorder_graph_overlap_shrink(NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_pre_buf, UINT32 proc_layers_num, UINT32 end_addr)
{
	UINT32 idx=0, buf_idx=0;
	INT32 out_buf_id=0, in_buf_id=0;
	UINT32 out_buf_num = 0;
	UINT32 in_buf_num = 0;
	INT32 in_buf_id_max = 0xFFFFFFFF;

	for (idx = 0; idx<proc_layers_num; idx++) {
		in_buf_num  = IN_BUF_NUM(&p_mctrl[idx]);
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);

		for (buf_idx = 0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].out_buf_index[buf_idx];
			if (out_buf_id >= 0) break;
		}

		in_buf_id_max = 0xFFFFFFFF;
		for (buf_idx = 0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].in_buf_index[buf_idx];
			if (in_buf_id >= 0) in_buf_id_max = MAX(in_buf_id_max, in_buf_id);
		}
		in_buf_id = in_buf_id_max;
		if (in_buf_id < 0) continue;
#if 1
		// CONV & ELTWISE
		if (p_mctrl[idx].mode == NN_ELTWISE || p_mctrl[idx].mode == NN_CONV) {
			if ((p_pre_buf[in_buf_id].life == 1) && (p_pre_buf[out_buf_id].life_o == 1) && (p_pre_buf[out_buf_id].size <= p_pre_buf[in_buf_id].size)) {
				p_pre_buf[in_buf_id].addr  = end_addr + 1;
				p_pre_buf[out_buf_id].addr = end_addr + 1;
				end_addr += 1 + p_pre_buf[in_buf_id].size;
				//DBG_IND("[overlap] mctrl(%lu) normal case , in(%ld) out(%ld)\r\n", idx, in_buf_id, out_buf_id);
			}
		}
#endif
	}

	return 0;
}

static INT32 nn_reorder_exe(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 proc_layers_num, UINT32 job_method, UINT32 buf_method, UINT32 *end_addr, UINT32 *ideal_size)
{
	INT32 rv = 0;
	UINT32 out_idx = 0, max_buf_id = 0, max_map_num =0;
	UINT32 idx=0;
	UINT32 out_buf_num = 0;
	INT32  buf_id=0;
	NN_MANAGER_BUF nn_manage_buf = { 0 };
	NN_BUF *p_map_buff = NULL, *p_pre_buf = NULL;
	UINT32 max_t=0;
	#if MEMORY_REORDER_DEBUG
	UINT32 dbg_idx = 0;
	#endif

	// init
	nn_manage_buf.p_buf = (NN_BUF*)vendor_ai_malloc(proc_layers_num * 2 * 2 * sizeof(NN_BUF));// *2 input and output + *2 test_backup
	if (nn_manage_buf.p_buf == NULL) {
		DBG_ERR("nn_manage_buf alloc fail !!\n");
		rv = -1;
		goto exit;
	}
	memset(nn_manage_buf.p_buf, 0, sizeof(NN_BUF)*proc_layers_num * 2 * 2);
	nn_manage_buf.cnt = 0;

	p_map_buff = (NN_BUF*)vendor_ai_malloc(proc_layers_num*2*2 * sizeof(NN_BUF));		// *2  input and output + *2 group link-list
	if (p_map_buff == NULL) {
		DBG_ERR("p_map_buff alloc fail !!\n");
		rv = -1;
		goto exit;
	}
	memset(p_map_buff, 0, sizeof(NN_BUF)*proc_layers_num * 2*2);

	// find max buf_id
	for (idx = 0; idx < proc_layers_num; idx++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[idx], out_idx);  // p_mctrl[idx].out_buf_index[out_idx];
			if (buf_id >= 0) max_buf_id = (max_buf_id < (UINT32)buf_id) ? (UINT32)buf_id : max_buf_id;
		}
	}

	// p_io_buff -> p_pre_buf
	p_pre_buf = nn_manage_buf.p_buf;
	memcpy(p_pre_buf, p_io_buff, sizeof(NN_BUF)*proc_layers_num*2);

	// p_pre_buf -> p_pre_buf , [buf_O2] (find possible overlap)
	if (buf_method >= VENDOR_AI_NET_BUF_OPT_SHRINK_O2) {
		if (_vendor_ai_net_is_linear_job(job_method)) {
			nn_reorder_linear_overlap_shrink(p_mctrl, p_pre_buf, proc_layers_num, *end_addr); // pass-by-value, this will NOT update end_addr
		} else if (_vendor_ai_net_is_graph_job(job_method)) {
			nn_reorder_graph_overlap_shrink(p_mctrl, p_pre_buf, proc_layers_num, *end_addr);
		}
	}

	// p_pre_buff -> p_map_buf
	nn_reorder_form_map_buff(p_pre_buf, p_map_buff, max_buf_id, proc_layers_num, &max_map_num);

	// update max_t
	if (_vendor_ai_net_is_linear_job(job_method)) {
		max_t = proc_layers_num;
	} else if (_vendor_ai_net_is_graph_job(job_method)) {
		max_t = _vendor_ai_net_mem_getmaxstep(proc_id)+1;
	}

	// calculate ideal memory
	*ideal_size = _vendor_ai_net_mem_calc_ideal_memory(p_map_buff, max_map_num, max_t);

	// reset nn_manage_buf
	memset(nn_manage_buf.p_buf, 0, sizeof(NN_BUF)*proc_layers_num * 2 * 2);
	nn_manage_buf.cnt = 0;

	// start to reorder on (p_map_buf)
	for (idx = 0; idx < max_t; idx++) {
		for (buf_id = 0; (UINT32)buf_id < max_map_num; buf_id++) {
			if (p_map_buff[buf_id].start_t == idx) {
				// add new buf_id to watch list
				nn_reorder_float_other_and_put(&nn_manage_buf, &p_map_buff[buf_id]);
				#if MEMORY_REORDER_DEBUG
				if (vendor_ai_cmd_get_iomem_debug(proc_id) & VENDOR_AI_NET_CMD_IOMEM_REORDER_DEBUG) {
					nn_reorder_debug_dump(&nn_manage_buf, dbg_idx++);
				}
				#endif
			}
		}
	}

	// nn_manage_buf -> p_map_buf -> p_io_buff (update p_io_buffer & end_addr)
	nn_reorder_update_p_io_buff(&nn_manage_buf, p_io_buff, end_addr);

exit:
	if (nn_manage_buf.p_buf) {
		vendor_ai_free(nn_manage_buf.p_buf, proc_layers_num*2*2 *sizeof(NN_BUF));
	}
	if (p_map_buff) {
		vendor_ai_free(p_map_buff, proc_layers_num*2*2 *sizeof(NN_BUF));
	}
	return rv;
}

#endif  // MEMORY_REORDER

#if DUMP_MEMORY_DEBUG
static INT32 _nn_memory_debug_dump_bufid_mctrl(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, INT32 tar_buf_id, char* str_blk_name, UINT32 max_str_len)
{
	UINT32 j=0, k=0;
	INT32 cur_buf_id=0;
	BOOL b_found = FALSE;
	UINT32 cur_str_len=0;
	char tmp_mctrl_idx[32];

	// init str_blk_name
	snprintf(str_blk_name, (max_str_len-cur_str_len-1), "%lu ( ", tar_buf_id);
	cur_str_len = strlen(str_blk_name);

	// find which mctrl's output buf_id == this buf_id
	for (j = 0; j < proc_layers_num ; j++) {
		for (k = 0 ; k < OUT_BUF_NUM_REAL(&p_mctrl[j]) ; k++) {
			cur_buf_id = OUT_BUF_INDEX(&p_mctrl[j], k);
			if (cur_buf_id < 0 ) continue;
			if (cur_buf_id == tar_buf_id) {
				snprintf(tmp_mctrl_idx, 31, "%lu ", j);
				strncat(str_blk_name, tmp_mctrl_idx, (max_str_len-cur_str_len-1));
				cur_str_len = strlen(str_blk_name);

				b_found = TRUE;
				break;
			}
		}
		if (b_found == TRUE) {
			b_found = FALSE;
			continue;
		}
	}

	// tail of str_blk_name
	strncat(str_blk_name, ")", (max_str_len-cur_str_len-1));

	return 0;
}

static INT32 _nn_memory_debug_dump(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 proc_layers_num)
{
	GRAPH_DEBUG_BUFFER_HANDLER graph_debug_hdl = (-1);
	BOOL *b_bufsize_set = (BOOL*)vendor_ai_malloc(sizeof(BOOL)*proc_layers_num*2);  //only for debug cmd
	UINT32 mc_idx = 0, out_idx = 0;
	CHAR blk_name[512];
	CHAR filepath[64] = "";
	UINT32 out_buf_num = 0;
	INT32 buf_id = 0, buf_size = 0;

	if (b_bufsize_set == NULL) {
		DBG_ERR("alloc b_bufsize_set fail ...\r\n");
		return -1;
	}

	snprintf(filepath, 64, "/mnt/sd/proc%lu_nn_mem.html", proc_id);

	memset(b_bufsize_set, 0, sizeof(BOOL)*proc_layers_num*2);
	graph_debug_buffer_open(filepath, &graph_debug_hdl);

	if (graph_debug_hdl == (-1)) {
		DBG_ERR("open file (%s) fail ...\r\n", filepath);
		vendor_ai_free(b_bufsize_set, sizeof(BOOL)*proc_layers_num*2); //only for debug cmd
		return -1;
	}

	for (mc_idx = 0; mc_idx < proc_layers_num; mc_idx ++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
		for (out_idx = 0; out_idx < out_buf_num; out_idx++) {
			buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], out_idx);  // p_mctrl[mc_idx].out_buf_index[out_idx];
			buf_size = OUT_BUF_SIZE(&p_mctrl[mc_idx], out_idx);
			if ((buf_id >= 0) && (buf_size > 0) && (b_bufsize_set[buf_id] == FALSE)){
				b_bufsize_set[buf_id] = TRUE;
				if (0) {
					snprintf(blk_name, 20, "%lu", buf_id); // only buf_id
				} else {
					_nn_memory_debug_dump_bufid_mctrl(proc_id, p_mctrl, proc_layers_num, buf_id, blk_name, 511);  // buf_id ( mctrl mctrl mctrl )
				}

				graph_debug_buffer_add_block(graph_debug_hdl,
					p_io_buff[buf_id].start_t,
					p_io_buff[buf_id].end_t,
					p_io_buff[buf_id].addr,
					p_io_buff[buf_id].addr + p_io_buff[buf_id].size,
					blk_name,
					511);
			}
		}
	}
	graph_debug_buffer_close(graph_debug_hdl);
	vendor_ai_free(b_bufsize_set, sizeof(BOOL)*proc_layers_num*2); //only for debug cmd

	printf("proc_id(%lu) dump (%s) ... done!!\r\n", proc_id, filepath);
	return 0;
}
#endif


static INT32 nn_memory_linear_shrink(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 proc_layers_num, UINT32 used_start_addr, UINT32 *end_addr)
{
	UINT32 idx=0, buf_idx=0, buf_idx2=0;
	UINT32 in_buf_num  = 0;
	UINT32 out_buf_num = 0;
	UINT32 out_buf_size = 0;
	INT32 in_buf_id=0, out_buf_id=0, buf_id=0;
	NN_MANAGER_BUF nn_manage_buf = { 0 };

	// init
	nn_manage_buf.p_buf = (NN_BUF*)vendor_ai_malloc(proc_layers_num * 2 * sizeof(NN_BUF));
	if (nn_manage_buf.p_buf == NULL) {
		DBG_ERR("nn_manage_buf alloc fail !!\n");
		return -1;
	}
	memset(nn_manage_buf.p_buf, 0, sizeof(NN_BUF)*proc_layers_num * 2);
	nn_manage_buf.cnt = 0;

	// start to shrink
	*end_addr = used_start_addr;

	for (idx=0; idx<proc_layers_num; idx++) {
		in_buf_num  = IN_BUF_NUM(&p_mctrl[idx]);
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);

		for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].out_buf_index[buf_idx];
			buf_id = nn_manage_find_buf(&nn_manage_buf, out_buf_id);
			if (out_buf_id<0) {
				continue;
			}
			if (buf_id < 0) {
				buf_id = nn_manage_find_emptybuf(&nn_manage_buf, p_io_buff[out_buf_id]);
				p_io_buff[out_buf_id].start_t = idx;
				if ((nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size) > *end_addr) {
					*end_addr = (nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size);
				}
			}
			p_io_buff[out_buf_id].addr = nn_manage_buf.p_buf[buf_id].addr;
#if 1
			// release tmp buffer life
			if (TRUE == OUT_BUF_IS_TMP(&p_mctrl[idx], buf_idx)) {
				nn_manage_buf.p_buf[buf_id].life = 0;
				p_io_buff[out_buf_id].end_t = idx + 1;
				nn_manage_del_buf(&nn_manage_buf);
			}
#endif
		}

		for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].in_buf_index[buf_idx];
			buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
			if (buf_id >= 0) {
if (0){
				nn_manage_buf.p_buf[buf_id].life--;
}else{
				for (buf_idx2=0; buf_idx2<out_buf_num; buf_idx2++) {
					out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx2);
					out_buf_size = OUT_BUF_SIZE(&p_mctrl[idx], buf_idx2);
					if ((out_buf_id >= 0) && (out_buf_size > 0)) nn_manage_buf.p_buf[buf_id].life--;
				}
}
				if (nn_manage_buf.p_buf[buf_id].life == 0) {
					p_io_buff[in_buf_id].end_t = idx + 1;
					nn_manage_del_buf(&nn_manage_buf);
				}
			}
		}
		#if 0
		tmp_buf_id = p_reg[idx].layer_idx_reg.tmp_buf_index;
		if (tmp_buf_id >= 0) {
			buf_id = nn_manage_find_buf(tmp_buf_id);
			if (buf_id < 0) {
				buf_id = nn_manage_find_emptybuf(p_io_buff[tmp_buf_id]);
				if ((p_g_nn_manage_buf[buf_id].addr + p_g_nn_manage_buf[buf_id].size) > end_addr) {
					end_addr = (p_g_nn_manage_buf[buf_id].addr + p_g_nn_manage_buf[buf_id].size);
				}
			}
			p_io_buff[tmp_buf_id].addr = p_g_nn_manage_buf[buf_id].addr;

			p_g_nn_manage_buf[buf_id].life--;
			if (p_g_nn_manage_buf[buf_id].life == 0) {
				nn_manage_del_buf();
			}
		}
		#endif
	}

	if (nn_manage_buf.p_buf) {
		vendor_ai_free(nn_manage_buf.p_buf, proc_layers_num * 2 * sizeof(NN_BUF));
	}
	return 0;
}

static BOOL _nn_memory_is_inbuf_equal_outbuf_with_large_life_o(NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, INT32 in_buf_id, VENDOR_AI_NET_LLGROUP *p_group)
{
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
	UINT32 mc_idx = 0, buf_idx =0;
	UINT32 out_buf_num = 0;
	INT32 out_buf_id=0;

	list_for_each_entry(p_mctrl_entry, &p_group->mctrl_listhead, list) {
		mc_idx = p_mctrl_entry->mc_idx;
		out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);

		for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].out_buf_index[buf_idx];
			if (out_buf_id<0) {
				continue;
			}
			if (in_buf_id == out_buf_id && p_io_buff[out_buf_id].life_o > 1) {
				mem_shrink_dbg("\t    => i_id(%ld) equal mc_idx(%lu) o_id(%ld), skip pseudo\n", in_buf_id, mc_idx, out_buf_id);
				return TRUE;
			}
		}
	}
	return FALSE;
}

#if 1  // NEW_GRAPH_SHRINK
static BOOL nn_memory_graph_is_group_end_with_out_buf_id(VENDOR_AI_NET_LLGROUP *p_group, INT32 end_out_buf_id)
{
	UINT32 buf_idx=0;
	NN_GEN_MODE_CTRL *p_mctrl_tmp = p_group->p_tail;
	UINT32 out_buf_num  = OUT_BUF_NUM(p_mctrl_tmp);
	INT32 out_buf_id=0;

	for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
		out_buf_id = OUT_BUF_INDEX(p_mctrl_tmp, buf_idx);  // p_mctrl_tmp->out_buf_index[buf_idx];
		if (out_buf_id < 0) {
			continue;
		}
		if (out_buf_id == end_out_buf_id) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL nn_memory_graph_is_group_begin_with_in_buf_id(VENDOR_AI_NET_LLGROUP *p_group, INT32 begin_in_buf_id)
{
	UINT32 buf_idx=0;
	NN_GEN_MODE_CTRL *p_mctrl_tmp = p_group->p_head;
	UINT32 in_buf_num  = IN_BUF_NUM(p_mctrl_tmp);
	INT32 in_buf_id=0;

	for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
		in_buf_id = IN_BUF_INDEX(p_mctrl_tmp, buf_idx);  // p_mctrl_tmp->in_buf_index[buf_idx];
		if (in_buf_id < 0) {
			continue;
		}
		if (in_buf_id == begin_in_buf_id) {
			return TRUE;
		}
	}
	return FALSE;
}

static void nn_memory_graph_bufid_list_add(NN_BUFID_LIST *buf_id_list, INT32 buf_id)
{
	UINT32 i=0;
	for (i=0 ; i < buf_id_list->cnt ; i++) {
		if (buf_id_list->bufid[i] == (UINT32)buf_id) return;
	}

	buf_id_list->bufid[buf_id_list->cnt] = buf_id;
	buf_id_list->cnt++;
}

static void nn_memory_graph_bufid_list_sort_by_start_t(NN_BUFID_LIST *buf_id_list, NN_BUF *p_io_buff)
{
	INT32 buf_id_i = 0, buf_id_j = 0 ;
	UINT32 tmp=0;
	UINT32 i=0, j=0;

	if (buf_id_list->cnt <= 1) return;

	for (i=0 ; i < buf_id_list->cnt-1 ; i++) {
		for (j=i+1 ; j < buf_id_list->cnt ; j++) {
			buf_id_i = buf_id_list->bufid[i];
			buf_id_j = buf_id_list->bufid[j];

			if (p_io_buff[buf_id_j].start_t > p_io_buff[buf_id_i].start_t) {
				// swap i,j
				tmp = buf_id_list->bufid[i];
				buf_id_list->bufid[i] = buf_id_list->bufid[j];
				buf_id_list->bufid[j] = tmp;
			}
		}
	}
}

static UINT32 nn_memory_graph_group_release(NN_MANAGER_BUF *nn_manage_buf, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, VENDOR_AI_NET_LLGROUP *p_group, int level)
{
	UINT32 mc_idx = 0;
	//UINT32 step_end = 0;
	UINT32 buf_idx=0;
	UINT32 in_buf_num  = 0;
	INT32 in_buf_id=0, pre_buf_id = -99, buf_id=0;
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;

	list_for_each_entry(p_mctrl_entry, &p_group->mctrl_listhead, list) {
		mc_idx   = p_mctrl_entry->mc_idx;
		in_buf_num  = IN_BUF_NUM(&p_mctrl[mc_idx]);
		//step_end = p_group->step_end;
		mem_shrink_dbg_prefix(level, "  => mc_idx(%lu)\n", mc_idx);
		for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].in_buf_index[buf_idx];
			if (in_buf_id < 0) {
				continue;
			}
			//p_io_buff[in_buf_id].end_t = step_end; // BUG : set here is wrong !!
			pre_buf_id = -99;
			do {
				buf_id = nn_manage_find_buf(nn_manage_buf, in_buf_id);
				mem_shrink_dbg_prefix(level, "   => i_id(%ld) (%ld, %ld)\n", in_buf_id, pre_buf_id, buf_id);
				if (buf_id >= 0) {
					if (pre_buf_id == buf_id) {
						break;
					}
					else {
						pre_buf_id = buf_id;
					}

					mem_shrink_dbg_prefix(level, "   => manage(%lu).life(%lu -> %lu)\n", buf_id, nn_manage_buf->p_buf[buf_id].life, nn_manage_buf->p_buf[buf_id].life-1);
					nn_manage_buf->p_buf[buf_id].life--;

					if (nn_manage_buf->p_buf[buf_id].life == 0) {
						//p_io_buff[in_buf_id].end_t = step_end;
						mem_shrink_dbg_manager(nn_manage_buf);
						nn_manage_del_buf(nn_manage_buf);
						mem_shrink_dbg_manager(nn_manage_buf);
					}
				}
			} while (buf_id >= 0);
		}
	}
	//if (0) printf("%d\n", (int)step_end); // dummy, remove
	return 0;
}

static int nn_memory_graph_group_release_allpre(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 id_list_start_addr, NN_MANAGER_BUF *nn_manage_buf, VENDOR_AI_NET_LLGROUP *p_group, UINT32 graph_func, int level, UINT32* p_buf_referenced)
{
	VENDOR_AI_NET_MCTRL_LIST *p_mctrl_list = {0};
	UINT32 k=0;
	UINT32 j=0, start_ofs = 0, end_ofs = 0;
	VENDOR_AI_NET_LLGROUP *p_group_pre = NULL;
	NN_GEN_MODE_CTRL *p_mctrl_head = NULL;

	UINT32 buf_idx=0;
	UINT32 in_buf_num  = 0;
	INT32 in_buf_id=0;
	BOOL b_first_time = FALSE;

	p_mctrl_list = vendor_ai_net_group_getmctrlresult(proc_id);
	if ((p_mctrl_list == NULL) || (p_mctrl_list->p_mctrl_entry == 0) || (p_mctrl_list->mctrl_num == 0)) {
		DBG_ERR("null p_mctrl_list\r\n");
		return (-1);
	}

	if (graph_func & GRAPH_FUNC_SELF) {
		mem_shrink_dbg_prefix(level,"  [%d]group(%lu) release\n", level, p_group->g_idx);
		nn_memory_graph_group_release(nn_manage_buf, p_mctrl, p_io_buff, p_group, level);
	} else {
		mem_shrink_dbg_prefix(level,"  [%d]group(%lu) self-skip\n", level, p_group->g_idx);
	}

	p_mctrl_head = p_group->p_head;
	in_buf_num = IN_BUF_NUM(p_mctrl_head);
	mem_shrink_dbg_prefix(level, "  [%d]group(%lu) prev_num(%lu)\n", level, p_group->g_idx, p_mctrl_head->prev_num);

	for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
		in_buf_id = IN_BUF_INDEX(p_mctrl_head, buf_idx);  // p_mctrl_head->in_buf_index[buf_idx];
		if (in_buf_id < 0) {
			continue;
		}

		if (p_buf_referenced[in_buf_id] == (UINT32)(-100)) {
			mem_shrink_dbg_prefix(level, "  => group(%lu) in_buf_id(%d) (-100) done.\n", p_group->g_idx, (int)in_buf_id);
			continue;
		}
		b_first_time = (p_buf_referenced[in_buf_id] == 0)? TRUE:FALSE;
		p_buf_referenced[in_buf_id]++;

		if (b_first_time) {
			mem_shrink_dbg_prefix(level, "  => group(%lu) in_buf_id(%d) call prev release.\n", p_group->g_idx, (int)in_buf_id);

			// loop all previous group
			start_ofs = p_mctrl_head->prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl_head->prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				p_group_pre = (VENDOR_AI_NET_LLGROUP *)p_mctrl_list->p_mctrl_entry[k].p_group;
				if (p_group_pre == NULL) {
					DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", k); // this should normally never happen!!
					continue;
				}

				if (TRUE == nn_memory_graph_is_group_end_with_out_buf_id(p_group_pre, in_buf_id)) {
					mem_shrink_dbg_prefix(level, "   => <B>prev group(%lu) all release\n", p_group_pre->g_idx);
					nn_memory_graph_group_release_allpre(proc_id, p_mctrl, p_io_buff, id_list_start_addr, nn_manage_buf, p_group_pre, GRAPH_FUNC_SELF, level+1, p_buf_referenced);
					mem_shrink_dbg_prefix(level, "   => <E>prev group(%lu) all release\n", p_group_pre->g_idx);
				}
			}
		} else {
			mem_shrink_dbg_prefix(level, "  => prev group(%lu) buf_id(%d) done.\n", p_group->g_idx, (int)in_buf_id);
		}
	}

	for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
		in_buf_id = IN_BUF_INDEX(p_mctrl_head, buf_idx);  // p_mctrl_head->in_buf_index[buf_idx];
		if (in_buf_id < 0) {
			continue;
		}
		if (((graph_func & GRAPH_FUNC_SELF) == 0) &&
			(p_buf_referenced[in_buf_id] == p_io_buff[in_buf_id].life)) {
			p_buf_referenced[in_buf_id]--; // tricky solution to minus 1 => let this buf_id delay to reach (-100) otherwise end_t will be wrong (see 00018_yolo_v3_pdvd_416, layer 54) (see00017_yolo_v3_pdvd_320, layer 99)
		}
	}
	return 0;
}

static int nn_memory_graph_group_add_pseudo(NN_MANAGER_BUF *nn_manage_buf, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, VENDOR_AI_NET_LLGROUP *p_group, int level, NN_BUFID_LIST *buf_id_list)
{
	UINT32 mc_idx = 0;
	UINT32 buf_idx=0;
	INT32 in_buf_id=0, buf_id=0;
	UINT32 in_buf_num  = 0;
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;

	list_for_each_entry(p_mctrl_entry, &p_group->mctrl_listhead, list) {
		mc_idx = p_mctrl_entry->mc_idx;
		in_buf_num = IN_BUF_NUM(&p_mctrl[mc_idx]);
		mem_shrink_dbg_prefix(level, "  => mc_idx(%lu)\n", mc_idx);
		for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].in_buf_index[buf_idx];
			buf_id = nn_manage_find_buf(nn_manage_buf, in_buf_id);
			if (in_buf_id < 0) {
				continue;
			}
			if (buf_id >= 0) {
				// (1) add extra life
				mem_shrink_dbg_prefix(level, "   => i_id(%ld) manage(%lu).life(%lu -> %lu)\n", in_buf_id, buf_id, nn_manage_buf->p_buf[buf_id].life, nn_manage_buf->p_buf[buf_id].life+1);
				nn_manage_buf->p_buf[buf_id].life++;
			} else {
				// check if (in_buf_id) = (any mctrl's out_buf_id with life_o > 1) => don't need add pseudo, later will add real one
				//if (FALSE == _nn_memory_is_inbuf_equal_outbuf_with_large_life_o(p_mctrl, p_io_buff, in_buf_id, p_group)) {
					// (2) if input be delete, add pseudo one(s) with life = 1
					mem_shrink_dbg_prefix(level, "   => i_id(%ld) need add pseudo", in_buf_id);
					if (vendor_ai_cmd_get_iomem_debug(0) & VENDOR_AI_NET_CMD_IOMEM_SHRINK_DEBUG) {
						printf("\t  tar =><%d, 0x%08x, %lu, %lu>  , ", (int)p_io_buff[in_buf_id].id, (UINT)p_io_buff[in_buf_id].addr, p_io_buff[in_buf_id].size, p_io_buff[in_buf_id].life);
						nn_manage_debug_dump(nn_manage_buf);
					}
					#if 0
					//nn_manage_add_pseudo_buf(nn_manage_buf, p_io_buff[in_buf_id]);  // new way : record buf_id(s) who need add pseudo. Later will sort them by start_t then do actually add pseudo.
					#else
					nn_memory_graph_bufid_list_add(buf_id_list, in_buf_id);
					#endif
				//}
			}
		}
	}
	return 0;
}

static int nn_memory_graph_group_add_pseudo_allpre(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 id_list_start_addr, NN_MANAGER_BUF *nn_manage_buf, VENDOR_AI_NET_LLGROUP *p_group, UINT32 graph_func, int level, UINT32 *p_buf_referenced, NN_BUFID_LIST *buf_id_list)
{
	VENDOR_AI_NET_MCTRL_LIST *p_mctrl_list = {0};
	UINT32 j, k, start_ofs = 0, end_ofs = 0;
	VENDOR_AI_NET_LLGROUP *p_group_pre = NULL;
	NN_GEN_MODE_CTRL *p_mctrl_head = NULL;

	UINT32 buf_idx=0;
	UINT32 in_buf_num  = 0;
	INT32 in_buf_id=0;
	BOOL b_first_time = FALSE;

	p_mctrl_list = vendor_ai_net_group_getmctrlresult(proc_id);
	if ((p_mctrl_list == NULL) || (p_mctrl_list->p_mctrl_entry == 0) || (p_mctrl_list->mctrl_num == 0)) {
		DBG_ERR("null p_mctrl_list\r\n");
		return (-1);
	}

	if (graph_func & GRAPH_FUNC_SELF) {
		mem_shrink_dbg_prefix(level,"  [%d]group(%lu) add pseudo\n", level, p_group->g_idx);
		nn_memory_graph_group_add_pseudo(nn_manage_buf, p_mctrl, p_io_buff, p_group, level, buf_id_list);
	} else {
		mem_shrink_dbg_prefix(level,"  [%d]group(%lu) self-skip\n", level, p_group->g_idx);
	}

	p_mctrl_head = p_group->p_head;
	in_buf_num = IN_BUF_NUM(p_mctrl_head);
	mem_shrink_dbg_prefix(level, "  [%d]group(%lu) prev_num(%lu)\n", level, p_group->g_idx, p_mctrl_head->prev_num);

	for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
		in_buf_id = IN_BUF_INDEX(p_mctrl_head, buf_idx);  // p_mctrl_head->in_buf_index[buf_idx];
		if (in_buf_id < 0) {
			continue;
		}

		if (p_buf_referenced[in_buf_id] == (UINT32)(-100)) {
			mem_shrink_dbg_prefix(level, "  => group(%lu) in_buf_id(%d) (-100) done.\n", p_group->g_idx, (int)in_buf_id);
			continue;
		}
		b_first_time = (p_buf_referenced[in_buf_id] == 0)? TRUE:FALSE;
		p_buf_referenced[in_buf_id]++;

		if (b_first_time) {
			mem_shrink_dbg_prefix(level, "  => group(%lu) in_buf_id(%d) call prev add.\n", p_group->g_idx, (int)in_buf_id);

			// loop all previous group
			start_ofs = p_mctrl_head->prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl_head->prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				p_group_pre = (VENDOR_AI_NET_LLGROUP *)p_mctrl_list->p_mctrl_entry[k].p_group;
				if (p_group_pre == NULL) {
					DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", k); // this should normally never happen!!
					continue;
				}

				if (TRUE == nn_memory_graph_is_group_end_with_out_buf_id(p_group_pre, in_buf_id)) {
					//mem_shrink_dbg_prefix(level, "  => group(%lu) call prev release.\n", p_group->g_idx);
					mem_shrink_dbg_prefix(level, "   => <B>prev group(%lu) all add life/pseudo\n", p_group_pre->g_idx);
					nn_memory_graph_group_add_pseudo_allpre(proc_id, p_mctrl, p_io_buff, id_list_start_addr, nn_manage_buf, p_group_pre, GRAPH_FUNC_SELF, level+1, p_buf_referenced, buf_id_list);
					mem_shrink_dbg_prefix(level, "   => <E>prev group(%lu) all add life/pseudo\n", p_group_pre->g_idx);
				}
			}
		} else {
			mem_shrink_dbg_prefix(level, "  => prev group(%lu) buf_id(%d) done.\n", p_group->g_idx, (int)in_buf_id);
		}
	}

	for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
		in_buf_id = IN_BUF_INDEX(p_mctrl_head, buf_idx);  // p_mctrl_head->in_buf_index[buf_idx];
		if (in_buf_id < 0) {
			continue;
		}
		if (((graph_func & GRAPH_FUNC_SELF) == 0) &&
			(p_buf_referenced[in_buf_id] == p_io_buff[in_buf_id].life)) {
			p_buf_referenced[in_buf_id]--; // tricky solution to minus 1 => let this buf_id delay to reach (-100) otherwise end_t will be wrong (see 00018_yolo_v3_pdvd_416, layer 54) (see00017_yolo_v3_pdvd_320, layer 99)
		}
	}
	return 0;
}

static int nn_memory_graph_update_buf_referenced_for_group(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 id_list_start_addr, VENDOR_AI_NET_LLGROUP *p_group, int level, UINT32* p_buf_referenced, UINT32 *p_buf_updated, UINT32 top_step)
{
	VENDOR_AI_NET_MCTRL_LIST *p_mctrl_list = {0};
	UINT32 j, k, start_ofs = 0, end_ofs = 0;
	VENDOR_AI_NET_LLGROUP *p_group_next = NULL;
	NN_GEN_MODE_CTRL *p_mctrl_tail = NULL;

	UINT32 buf_idx=0;
	UINT32 out_buf_num  = 0, out_buf_num_2 = 0;
	INT32 out_buf_id=0;

	p_mctrl_list = vendor_ai_net_group_getmctrlresult(proc_id);
	if ((p_mctrl_list == NULL) || (p_mctrl_list->p_mctrl_entry == 0) || (p_mctrl_list->mctrl_num == 0)) {
		DBG_ERR("null p_mctrl_list\r\n");
		return (-1);
	}

	p_mctrl_tail = p_group->p_tail;
	out_buf_num = OUT_BUF_NUM(p_mctrl_tail);

	for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
		out_buf_id = OUT_BUF_INDEX(p_mctrl_tail, buf_idx);  // p_mctrl_tail->out_buf_index[buf_idx];
		if (out_buf_id < 0) {
			continue;
		}

		if (p_buf_updated[out_buf_id] == 1) {
			VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
			UINT32 b_idx=0;
			UINT32 mc_idx = 0;
			INT32 b_id=0;

			list_for_each_entry(p_mctrl_entry, &p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				out_buf_num_2 = OUT_BUF_NUM(&p_mctrl[mc_idx]);

				for (b_idx=0; b_idx<out_buf_num_2; b_idx++) {
					b_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], b_idx);  // p_mctrl[mc_idx].out_buf_index[b_idx];
					if (b_id<0) {
						continue;
					}
					if (p_io_buff[b_id].is_preserve == FALSE) {
						p_io_buff[b_id].end_t = p_io_buff[out_buf_id].end_t;
						mem_shrink_dbg("[END_T_2] p_io_buff[%d].end_t = %d(from p_io_buff[%d])\n", (int)b_id, (int)p_io_buff[b_id].end_t, (int)out_buf_id);
					}
				}
			}
			continue;
		}

		if ((p_buf_referenced[out_buf_id] == p_io_buff[out_buf_id].life) || (p_buf_referenced[out_buf_id] == (UINT32)(-100))){

			// first time to set p_buf_updated to (-100) => can set  step_end now !!
			if (p_buf_referenced[out_buf_id] == p_io_buff[out_buf_id].life) {
				VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
				UINT32 b_idx=0;
				UINT32 mc_idx = 0;
				INT32 b_id=0;

				list_for_each_entry(p_mctrl_entry, &p_group->mctrl_listhead, list) {
					mc_idx = p_mctrl_entry->mc_idx;
					out_buf_num_2 = OUT_BUF_NUM(&p_mctrl[mc_idx]);

					for (b_idx=0; b_idx<out_buf_num_2; b_idx++) {
						b_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], b_idx);  // p_mctrl[mc_idx].out_buf_index[b_idx];
						if (b_id<0) {
							continue;
						}
						if (p_io_buff[b_id].is_preserve == FALSE) {
							p_io_buff[b_id].end_t = top_step;
							mem_shrink_dbg("[END_T_1] p_io_buff[%d].end_t = %d(from top_step=%d)\n", (int)b_id, (int)p_io_buff[b_id].end_t, (int)top_step);
						}
					}
				}
				mem_shrink_dbg_prefix(level, "  [%d]group(%lu) out_buf_id(%d) set (-100)\n", level, p_group->g_idx, (int)out_buf_id);
			} else {
				mem_shrink_dbg_prefix(level, "  [%d]group(%lu) out_buf_id(%d) keep (-100)\n", level, p_group->g_idx, (int)out_buf_id);
			}

			//set this buf_id as (-100), never need to recursive pierce through this buffer later
			p_buf_referenced[out_buf_id] = (UINT32)(-100);
			p_buf_updated[out_buf_id] = 1;

			// recursive call next group to check more buffer
			start_ofs = p_mctrl_tail->next_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl_tail->next_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				p_group_next = (VENDOR_AI_NET_LLGROUP *)p_mctrl_list->p_mctrl_entry[k].p_group;
				if (p_group_next == NULL) {
					DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", k); // this should normally never happen!!
					continue;
				}
				if (TRUE == nn_memory_graph_is_group_begin_with_in_buf_id(p_group_next, out_buf_id)) {
					nn_memory_graph_update_buf_referenced_for_group(proc_id, p_mctrl, p_io_buff, id_list_start_addr, p_group_next, level+1, p_buf_referenced, p_buf_updated, top_step);
				}
			}
		} else {
			mem_shrink_dbg_prefix(level, "  [%d]group(%lu) out_buf_id(%d) (%d/%d) done.\n", level, p_group->g_idx, (int)out_buf_id, (int)p_buf_referenced[out_buf_id], (int)p_io_buff[out_buf_id].life);
		}
	}

	return 0;
}

static INT32 nn_memory_graph_shrink(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 proc_layers_num, UINT32 used_start_addr, UINT32 *end_addr, UINT32 id_list_start_addr)
{
	VENDOR_AI_NET_MEM_LIST *p_mem_list = {0};
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
	UINT32 i = 0, is_alloc = 0;
	UINT32 j = 0;
	UINT32 mc_idx = 0;
	UINT32 step = 0, max_step = 0;
	//UINT32 step_end = 0;
	//INT32 pre_buf_id = -99;

	UINT32 buf_idx=0;
	UINT32 in_buf_num  = 0;
	UINT32 out_buf_num = 0;
	INT32 in_buf_id=0, out_buf_id=0, buf_id=0;
	NN_MANAGER_BUF nn_manage_buf = { 0 };
	UINT32 *p_buf_referenced = NULL;
	UINT32 *p_buf_updated = NULL;
	NN_BUFID_LIST buf_add_list = {0};

	// init
	nn_manage_buf.p_buf = (NN_BUF*)vendor_ai_malloc(proc_layers_num * 2 * sizeof(NN_BUF));
	if (nn_manage_buf.p_buf == NULL) {
		DBG_ERR("nn_manage_buf alloc fail !!\n");
		goto exit;
	}
	memset(nn_manage_buf.p_buf, 0, sizeof(NN_BUF)*proc_layers_num * 2);
	nn_manage_buf.cnt = 0;

	p_buf_referenced = (UINT32*)vendor_ai_malloc(proc_layers_num * 6 * sizeof(UINT32));
	if (p_buf_referenced == NULL) {
		DBG_ERR("p_buf_referenced alloc fail !!\n");
		goto exit;
	}
	memset(p_buf_referenced, 0, sizeof(UINT32)*proc_layers_num*6);
	p_buf_updated      = (UINT32*)(((UINT32)p_buf_referenced) + (sizeof(UINT32)*proc_layers_num*2));
	buf_add_list.bufid = (UINT32*)(((UINT32)p_buf_updated) + (sizeof(UINT32)*proc_layers_num*2));

	// get max_step
	max_step = _vendor_ai_net_mem_getmaxstep(proc_id);

	// start to shrink
	*end_addr = used_start_addr;

	p_mem_list = vendor_ai_net_group_getmemresult(proc_id);

	if ((p_mem_list == NULL) || (p_mem_list->p_mem == 0) || (p_mem_list->mem_num == 0)) {
		DBG_ERR("null p_mem_list\r\n");
		goto exit;
	}

	for (i = 0; i < p_mem_list->mem_num; i++) {
		is_alloc = p_mem_list->p_mem[i].is_alloc;

		if (is_alloc == 1) {
			//[1] release all previous group buffer
			mem_shrink_dbg("group(%lu) release all previous\n", p_mem_list->p_mem[i].p_group->g_idx);
			///memset(p_buf_referenced, 0, sizeof(UINT32)*proc_layers_num*2);
			for (j=0; j< proc_layers_num*2; j++) {
				if (p_buf_referenced[j] != (UINT32)(-100)) {
					p_buf_referenced[j] = 0;
				}
			}
			nn_memory_graph_group_release_allpre(proc_id, p_mctrl, p_io_buff, id_list_start_addr, &nn_manage_buf, p_mem_list->p_mem[i].p_group, 0, 0, p_buf_referenced);

			//[2] update buffer referenced ... set as (-100)
			mem_shrink_dbg("group(%lu) update buffer referenced\n", p_mem_list->p_mem[i].p_group->g_idx);
			memset(p_buf_updated, 0, sizeof(UINT32)*proc_layers_num*2);
			nn_memory_graph_update_buf_referenced_for_group(proc_id, p_mctrl, p_io_buff, id_list_start_addr, p_mem_list->p_mem[0].p_group, 0, p_buf_referenced, p_buf_updated, p_mem_list->p_mem[i].p_group->step);

			mem_shrink_dbg("group(%lu) alloc\n", p_mem_list->p_mem[i].p_group->g_idx);
			//[3] for each mctrl => (1) find output(life_o <=1) (2) input life-- & del
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				in_buf_num  = IN_BUF_NUM(&p_mctrl[mc_idx]);
				out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
				step   = p_mem_list->p_mem[i].p_group->step;
				mem_shrink_dbg("\t[1] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
					out_buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].out_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, out_buf_id);
					if (out_buf_id<0) {
						continue;
					}
					if (buf_id < 0 && p_io_buff[out_buf_id].life_o <= 1) {
						buf_id = nn_manage_find_emptybuf(&nn_manage_buf, p_io_buff[out_buf_id]);
						mem_shrink_dbg("\t    => o_id(%ld) find manage(%ld) (0x%08x, %lu), end_addr = 0x%08x -> 0x%08x\n", out_buf_id, buf_id, (UINT)nn_manage_buf.p_buf[buf_id].addr, nn_manage_buf.p_buf[buf_id].size, (UINT)*end_addr, (UINT)MAX(*end_addr,(nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size)));
						p_io_buff[out_buf_id].start_t = step;
						p_io_buff[out_buf_id].end_t   = max_step+1; // init first, end_t will update later
						if ((nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size) > *end_addr) {
							*end_addr = (nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size);
						}
						p_io_buff[out_buf_id].addr = nn_manage_buf.p_buf[buf_id].addr;
					}
				}

				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].in_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
					if (in_buf_id < 0) {
						continue;
					}
					if (buf_id >= 0) {
						mem_shrink_dbg("\t    => i_id(%ld) manage(%lu).life(%lu -> %lu)\n", in_buf_id, buf_id, nn_manage_buf.p_buf[buf_id].life, nn_manage_buf.p_buf[buf_id].life-1);
						nn_manage_buf.p_buf[buf_id].life--;
						if (nn_manage_buf.p_buf[buf_id].life == 0) {
							nn_manage_del_buf(&nn_manage_buf);
						}
					}
				}
			}

			//[4] for each mctrl => (1) input life++ (prevent other branch use) (2) if input be delete, add pseudo one(s) 
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				in_buf_num = IN_BUF_NUM(&p_mctrl[mc_idx]);
				mem_shrink_dbg("\t[2] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].in_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
					if (in_buf_id < 0) {
						continue;
					}
					if (buf_id >= 0) {
						// (1) add extra life
						mem_shrink_dbg("\t    => i_id(%ld) manage(%lu).life(%lu -> %lu)\n", in_buf_id, buf_id, nn_manage_buf.p_buf[buf_id].life, nn_manage_buf.p_buf[buf_id].life+1);
						nn_manage_buf.p_buf[buf_id].life++;
					} else {
						// check if (in_buf_id) = (any mctrl's out_buf_id with life_o > 1) => don't need add pseudo, later will add real one
						if (FALSE == _nn_memory_is_inbuf_equal_outbuf_with_large_life_o(p_mctrl, p_io_buff, in_buf_id, p_mem_list->p_mem[i].p_group)) {
							// (2) if input be delete, add pseudo one(s) with life = 1
							mem_shrink_dbg("\t    => i_id(%ld) need add pseudo\n", in_buf_id);
							nn_manage_add_pseudo_buf(&nn_manage_buf, p_io_buff[in_buf_id]);
						}
					}
				}

			}


			//[6] recover all previous group by add life / add pseudo
			mem_shrink_dbg("group(%lu) add life/pseudo all previous\n", p_mem_list->p_mem[i].p_group->g_idx);
			////memset(p_buf_referenced, 0, sizeof(UINT32)*proc_layers_num*2);
			for (j=0; j< proc_layers_num*2; j++) {
				if (p_buf_referenced[j] != (UINT32)(-100)) {
					p_buf_referenced[j] = 0;
				}
			}
			// reset buf_add_list
			buf_add_list.cnt = 0;
			memset(buf_add_list.bufid, 0, sizeof(UINT32)*proc_layers_num*2);

			// get buf_add_list
			nn_memory_graph_group_add_pseudo_allpre(proc_id, p_mctrl, p_io_buff, id_list_start_addr, &nn_manage_buf, p_mem_list->p_mem[i].p_group, 0, 0, p_buf_referenced, &buf_add_list);

			// execute add pseudo by buf_add_list
			nn_memory_graph_bufid_list_sort_by_start_t(&buf_add_list, p_io_buff);
			for (j = 0; j < buf_add_list.cnt; j++) {
				if (nn_manage_find_buf(&nn_manage_buf, buf_add_list.bufid[j]) < 0) {
					nn_manage_add_pseudo_buf(&nn_manage_buf, p_io_buff[buf_add_list.bufid[j]]);
				}
			}

			//[5] for each mctrl => (1) find output(life_o >1) (this output buffer can't reuse any branch internal buffer, so find output buffer here... with life already recovered)
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				out_buf_num = OUT_BUF_NUM(&p_mctrl[mc_idx]);
				step   = p_mem_list->p_mem[i].p_group->step;
				mem_shrink_dbg("\t[3] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
					out_buf_id = OUT_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].out_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, out_buf_id);
					if (out_buf_id<0) {
						continue;
					}
					if (buf_id < 0 && p_io_buff[out_buf_id].life_o > 1) {
						buf_id = nn_manage_find_emptybuf(&nn_manage_buf, p_io_buff[out_buf_id]);
						mem_shrink_dbg("\t    => o_id(%ld) life_o(%lu) find manage(%ld) (0x%08x, %lu), end_addr = 0x%08x -> 0x%08x\n", out_buf_id, p_io_buff[out_buf_id].life_o, buf_id, (UINT)nn_manage_buf.p_buf[buf_id].addr, nn_manage_buf.p_buf[buf_id].size, (UINT)*end_addr, (UINT)MAX(*end_addr,(nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size)));
						p_io_buff[out_buf_id].start_t = step;
						p_io_buff[out_buf_id].end_t   = max_step+1; // init first, end_t will update later
						if ((nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size) > *end_addr) {
							*end_addr = (nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size);
						}
						p_io_buff[out_buf_id].addr = nn_manage_buf.p_buf[buf_id].addr;
					}
				}
			}


		} // is_alloc = 1
		else if (is_alloc == 0) {
			///nn_memory_graph_group_release(&nn_manage_buf, p_mctrl, p_io_buff, p_mem_list->p_mem[i].p_group);
			/****mem_shrink_dbg("group(%lu) release\n", p_mem_list->p_mem[i].p_group->g_idx);
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx   = p_mctrl_entry->mc_idx;
				in_buf_num = IN_BUF_NUM(&p_mctrl[mc_idx]);
				step_end = p_mem_list->p_mem[i].p_group->step_end;

				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = IN_BUF_INDEX(&p_mctrl[mc_idx], buf_idx);  // p_mctrl[mc_idx].in_buf_index[buf_idx];
					if (in_buf_id < 0) {
						continue;
					}
					p_io_buff[in_buf_id].end_t = step_end;
					mem_shrink_dbg("\t  mc_idx(%lu), i_id(%d) step_end = %d\n", mc_idx, (int)in_buf_id, (int)step_end);
				}
			}****/
		} // is_alloc = 0
	} // for (i = 0; i < p_mem_list->mem_num; i++)
exit:
	if (nn_manage_buf.p_buf) {
		vendor_ai_free(nn_manage_buf.p_buf, proc_layers_num * 2 * sizeof(NN_BUF));
	}
	if (p_buf_referenced) {
		vendor_ai_free(p_buf_referenced, proc_layers_num * 6 * sizeof(UINT32));
	}
	return 0;
}
#else
static INT32 nn_memory_graph_shrink(UINT32 proc_id, NN_GEN_MODE_CTRL *p_mctrl, NN_BUF *p_io_buff, UINT32 proc_layers_num, UINT32 used_start_addr, UINT32 *end_addr)
{
	VENDOR_AI_NET_MEM_LIST *p_mem_list = {0};
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
	UINT32 i = 0, is_alloc = 0;
	UINT32 mc_idx = 0;
	UINT32 step = 0, step_end = 0, max_step = 0;
	INT32 pre_buf_id = -99;

	UINT32 buf_idx=0;
	UINT32 in_buf_num  = sizeof(p_mctrl->in_buf_index)  / sizeof(p_mctrl->in_buf_index[0]);
	UINT32 out_buf_num = sizeof(p_mctrl->out_buf_index) / sizeof(p_mctrl->out_buf_index[0]);
	INT32 in_buf_id=0, out_buf_id=0, buf_id=0;
	NN_MANAGER_BUF nn_manage_buf = { 0 };

	// init
	nn_manage_buf.p_buf = (NN_BUF*)calloc(proc_layers_num * 2, sizeof(NN_BUF));
	if (nn_manage_buf.p_buf == NULL) {
		DBG_ERR("nn_manage_buf alloc fail !!\n");
		goto exit;
	}
	memset(nn_manage_buf.p_buf, 0, sizeof(NN_BUF)*proc_layers_num * 2);
	nn_manage_buf.cnt = 0;

	// get max_step
	max_step = _vendor_ai_net_mem_getmaxstep(proc_id);

	// start to shrink
	*end_addr = used_start_addr;

	p_mem_list = vendor_ai_net_group_getmemresult(proc_id);

	if ((p_mem_list == NULL) || (p_mem_list->p_mem == 0) || (p_mem_list->mem_num == 0)) {
		DBG_ERR("null p_mem_list\r\n");
		goto exit;
	}

	for (i = 0; i < p_mem_list->mem_num; i++) {
		is_alloc = p_mem_list->p_mem[i].is_alloc;

		if (is_alloc == 1) {
			mem_shrink_dbg("group(%lu) alloc\n", p_mem_list->p_mem[i].p_group->g_idx);
			// for each mctrl => (1) find output(life_o <=1) (2) input life-- & del
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				step   = p_mem_list->p_mem[i].p_group->step;
				mem_shrink_dbg("\t[1] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
					out_buf_id = p_mctrl[mc_idx].out_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, out_buf_id);
					if (out_buf_id<0) {
						continue;
					}
					if (buf_id < 0 && p_io_buff[out_buf_id].life_o <= 1) {
						buf_id = nn_manage_find_emptybuf(&nn_manage_buf, p_io_buff[out_buf_id]);
						mem_shrink_dbg("\t    => o_id(%ld) find manage(%ld) (0x%08x, %lu), end_addr = 0x%08x -> 0x%08x\n", out_buf_id, buf_id, (UINT)nn_manage_buf.p_buf[buf_id].addr, nn_manage_buf.p_buf[buf_id].size, (UINT)*end_addr, (UINT)MAX(*end_addr,(nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size)));
						p_io_buff[out_buf_id].start_t = step;
						p_io_buff[out_buf_id].end_t   = max_step+1; // init first, end_t will update later
						if ((nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size) > *end_addr) {
							*end_addr = (nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size);
						}
						p_io_buff[out_buf_id].addr = nn_manage_buf.p_buf[buf_id].addr;
					}
				}

				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = p_mctrl[mc_idx].in_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
					if (in_buf_id < 0) {
						continue;
					}
					if (buf_id >= 0) {
						mem_shrink_dbg("\t    => i_id(%ld) manage(%lu).life(%lu -> %lu)\n", in_buf_id, buf_id, nn_manage_buf.p_buf[buf_id].life, nn_manage_buf.p_buf[buf_id].life-1);
						nn_manage_buf.p_buf[buf_id].life--;
						if (nn_manage_buf.p_buf[buf_id].life == 0) {
							nn_manage_del_buf(&nn_manage_buf);
						}
					}
				}
			}

			// for each mctrl => (1) input life++ (prevent other branch use) (2) if input be delete, add pseudo one(s) 
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				mem_shrink_dbg("\t[2] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = p_mctrl[mc_idx].in_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
					if (in_buf_id < 0) {
						continue;
					}
					if (buf_id >= 0) {
						// (1) add extra life
						mem_shrink_dbg("\t    => i_id(%ld) manage(%lu).life(%lu -> %lu)\n", in_buf_id, buf_id, nn_manage_buf.p_buf[buf_id].life, nn_manage_buf.p_buf[buf_id].life+1);
						nn_manage_buf.p_buf[buf_id].life++;
					} else {
						// check if (in_buf_id) = (any mctrl's out_buf_id with life_o > 1) => don't need add pseudo, later will add real one
						if (FALSE == _nn_memory_is_inbuf_equal_outbuf_with_large_life_o(p_mctrl, p_io_buff, in_buf_id, p_mem_list->p_mem[i].p_group)) {
							// (2) if input be delete, add pseudo one(s) with life = 1
							mem_shrink_dbg("\t    => i_id(%ld) need add pseudo\n", in_buf_id);
							nn_manage_add_pseudo_buf(&nn_manage_buf, p_io_buff[in_buf_id]);
						}
					}
				}

			}
			// for each mctrl => (1) find output(life_o >1) (this output buffer can't reuse any branch internal buffer, so find output buffer here... with life already recovered)
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx = p_mctrl_entry->mc_idx;
				step   = p_mem_list->p_mem[i].p_group->step;
				mem_shrink_dbg("\t[3] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
					out_buf_id = p_mctrl[mc_idx].out_buf_index[buf_idx];
					buf_id = nn_manage_find_buf(&nn_manage_buf, out_buf_id);
					if (out_buf_id<0) {
						continue;
					}
					if (buf_id < 0 && p_io_buff[out_buf_id].life_o > 1) {
						buf_id = nn_manage_find_emptybuf(&nn_manage_buf, p_io_buff[out_buf_id]);
						mem_shrink_dbg("\t    => o_id(%ld) life_o(%lu) find manage(%ld) (0x%08x, %lu), end_addr = 0x%08x -> 0x%08x\n", out_buf_id, p_io_buff[out_buf_id].life_o, buf_id, (UINT)nn_manage_buf.p_buf[buf_id].addr, nn_manage_buf.p_buf[buf_id].size, (UINT)*end_addr, (UINT)MAX(*end_addr,(nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size)));
						p_io_buff[out_buf_id].start_t = step;
						p_io_buff[out_buf_id].end_t   = max_step+1; // init first, end_t will update later
						if ((nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size) > *end_addr) {
							*end_addr = (nn_manage_buf.p_buf[buf_id].addr + nn_manage_buf.p_buf[buf_id].size);
						}
						p_io_buff[out_buf_id].addr = nn_manage_buf.p_buf[buf_id].addr;
					}
				}
			}
		} // is_alloc = 1
		else if (is_alloc == 0) {
			mem_shrink_dbg("group(%lu) release\n", p_mem_list->p_mem[i].p_group->g_idx);
			list_for_each_entry(p_mctrl_entry, &p_mem_list->p_mem[i].p_group->mctrl_listhead, list) {
				mc_idx   = p_mctrl_entry->mc_idx;
				step_end = p_mem_list->p_mem[i].p_group->step_end;
				mem_shrink_dbg("\t[1] mc_idx(%lu)\n", mc_idx);
				for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
					in_buf_id = p_mctrl[mc_idx].in_buf_index[buf_idx];
					if (in_buf_id < 0) {
						continue;
					}
					//p_io_buff[in_buf_id].end_t = step_end; // BUG : set here is wrong !!
					pre_buf_id = -99;
					do {
						buf_id = nn_manage_find_buf(&nn_manage_buf, in_buf_id);
						mem_shrink_dbg("\t    => i_id(%ld) (%ld, %ld)\n", in_buf_id, pre_buf_id, buf_id);
						if (buf_id >= 0) {
							if (pre_buf_id == buf_id) {
								break;
							}
							else {
								pre_buf_id = buf_id;
							}
							mem_shrink_dbg("\t      => manage(%lu).life(%lu -> %lu)\n", buf_id, nn_manage_buf.p_buf[buf_id].life, nn_manage_buf.p_buf[buf_id].life-1);
							nn_manage_buf.p_buf[buf_id].life--;
							if (nn_manage_buf.p_buf[buf_id].life == 0) {
								p_io_buff[in_buf_id].end_t = step_end;
								nn_manage_del_buf(&nn_manage_buf);
							}
						}
					} while (buf_id >= 0);
				}
			}
		} // is_alloc = 0
	} // for (i = 0; i < p_mem_list->mem_num; i++)
exit:
	if (nn_manage_buf.p_buf) 
		free(nn_manage_buf.p_buf);
	return 0;
}
#endif // NEW_GRAPH_SHRINK

#if CNN_25_MATLAB
UINT32 nn_net_alloc_mem(UINT32 proc_id, NN_IOMEM *p_io_mem, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 buf_method, UINT32 job_method, UINT32 id_list_start_addr)
#else
HD_RESULT nn_net_alloc_mem(UINT32 proc_id, UINT32 parm_va_ofs, NN_GEN_MODE_CTRL *p_mctrl, UINT32 proc_layers_num, UINT32 buf_method, UINT32 job_method, UINT32 id_list_start_addr, UINT32 *io_buf_size)
#endif
{
	const UINT32 used_start_addr = 0x0;
	UINT32 idx=0, buf_idx=0, buf_idx2=0;
	UINT32 buf_addr=0, end_addr=0, ideal_size=0;
	INT32 in_buf_id, out_buf_id, out_buf_size;
	UINT32 in_buf_num=0, out_buf_num=0;
	NN_BUF *p_io_buff;
#if CNN_25_MATLAB
	if ((p_io_mem == NULL) || (p_mctrl == NULL) || (proc_layers_num == 0)) {
		printf("[%s] wrong intput: p_io_mem=%08x, p_mctrl=%08x, proc_layers_num=%08x\r\n"
				, __FUNCTION__, (int)p_io_mem, (int)p_mctrl, (int)proc_layers_num);
		return 0;
	}
#else
	if ((p_mctrl == NULL) || (proc_layers_num == 0)) {
		printf("[%s] wrong intput: p_mctrl=%08x, proc_layers_num=%08x\r\n"
				, __FUNCTION__, (int)p_mctrl, (int)proc_layers_num);
		return 0;
	}
#endif
	p_io_buff			= (NN_BUF*)vendor_ai_malloc(proc_layers_num *2 * sizeof(NN_BUF));		// *2  input and output
	if (p_io_buff == NULL) {
		DBG_ERR("p_io_buff alloc fail\n");
		goto exit;
	}

	memset(p_io_buff, 0, sizeof(NN_BUF)*proc_layers_num*2);

	buf_addr = used_start_addr;
	for (idx=0; idx<proc_layers_num; idx++) {
		in_buf_num  = IN_BUF_NUM(&p_mctrl[idx]);
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (buf_idx=0; buf_idx<in_buf_num; buf_idx++) {
			in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].in_buf_index[buf_idx];
			if (in_buf_id >= 0) {
if (0){
				p_io_buff[in_buf_id].life++;
}else{
				for (buf_idx2=0; buf_idx2<out_buf_num; buf_idx2++) {
					out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx2);
					out_buf_size = OUT_BUF_SIZE(&p_mctrl[idx], buf_idx2);
					if ((out_buf_id >= 0) && (out_buf_size > 0)) p_io_buff[in_buf_id].life++;
				}
}
			}
		}
		
		for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].out_buf_index[buf_idx];
			out_buf_size = OUT_BUF_SIZE(&p_mctrl[idx], buf_idx); // p_mctrl[idx].output_buffsize[buf_idx];
						
			if ((out_buf_id<0) || (out_buf_size<=0)) {
				continue;
			}

			if (out_buf_id >= 0) {
				p_io_buff[out_buf_id].life_o++;
			}

			if (p_io_buff[out_buf_id].addr!=0) {
				continue;
			}
#if CNN_25_MATLAB
			if (p_mctrl[idx].next_num == 0) {    // matlab-gentool decide "last node" to keep buffer
#else
			if (OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PRESERVE) == 1) { // p_mctrl[idx].is_preserve // c-gentool will decide which layer to keep buffer
#endif
				p_io_buff[out_buf_id].life++; // path end layer, cant reuse buffer, add extra life
				p_io_buff[out_buf_id].is_preserve = TRUE;
			}

			if (TRUE == OUT_BUF_IS_TMP(&p_mctrl[idx], buf_idx)) {	//if (buf_idx >= 4) {
				// tmp buffer, add life to prevent reuse by other branch
				p_io_buff[out_buf_id].life++;
				p_io_buff[out_buf_id].is_preserve = TRUE;
			}


			p_io_buff[out_buf_id].id	= out_buf_id;
			#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
			p_io_buff[out_buf_id].size = ALIGN_CEIL_64(out_buf_size);
			p_io_buff[out_buf_id].addr = ALIGN_CEIL_64(buf_addr);
			#else
			p_io_buff[out_buf_id].size = ALIGN_CEIL_32(out_buf_size);
			p_io_buff[out_buf_id].addr = ALIGN_CEIL_32(buf_addr);
			#endif

			buf_addr += p_io_buff[out_buf_id].size;
			p_io_buff[out_buf_id].start_t = idx;
			p_io_buff[out_buf_id].end_t   = proc_layers_num;
		}
	}
	
	#if 0
	//mainly for 313 VPE
	for (idx=0; idx<proc_layers_num; idx++) {
		tmp_buf_id = p_reg[idx].layer_idx_reg.tmp_buf_index;
		if (tmp_buf_id >= 0) {
			p_io_buff[tmp_buf_id].life++;
			p_io_buff[tmp_buf_id].id	= tmp_buf_id;
			if ((p_reg[idx].mode == NN_PREPROC) && (p_reg[idx].platform == ENGINE)) {
				///< VPE condition: ensure address is 16 alignment
				p_io_buff[tmp_buf_id].size	= ALIGN_CEIL_4(p_reg[idx].size_reg.tmp_buffsize) + vpe_aligned;
				p_io_buff[tmp_buf_id].addr	= ALIGN_CEIL_16(buf_addr);
			} else {
				p_io_buff[tmp_buf_id].size	= ALIGN_CEIL_4(p_reg[idx].size_reg.tmp_buffsize);
				p_io_buff[tmp_buf_id].addr	= ALIGN_CEIL_4(buf_addr);
			}
			buf_addr += p_io_buff[tmp_buf_id].size;
		}
	}
	#endif
	
	end_addr = buf_addr;

	//--- SHRINK & REORDER ---
	if (_vendor_ai_net_is_linear_job(job_method) && _vendor_ai_net_is_shrink_buf(buf_method)) {
		// linear shrink
		nn_memory_linear_shrink(proc_id, p_mctrl, p_io_buff, proc_layers_num, used_start_addr, &end_addr);

#if MEMORY_REORDER
		if (buf_method >= VENDOR_AI_NET_BUF_OPT_SHRINK_O1) {
			nn_reorder_exe(proc_id, p_mctrl, p_io_buff, proc_layers_num, job_method, buf_method, &end_addr, &ideal_size);
			vendor_ai_net_trace(proc_id, AI_BUF, "proc_id(%lu) user_buffer size = %lu (ideal = %lu)\r\n", proc_id, (end_addr - used_start_addr), ideal_size);
		}
#endif
	}

	if (_vendor_ai_net_is_graph_job(job_method) && _vendor_ai_net_is_shrink_buf(buf_method)) {
		// graph shrink
		#if 1 // NEW_GRAPH_SHRINK
		nn_memory_graph_shrink(proc_id, p_mctrl, p_io_buff, proc_layers_num, used_start_addr, &end_addr, id_list_start_addr);
		#else
		nn_memory_graph_shrink(proc_id, p_mctrl, p_io_buff, proc_layers_num, used_start_addr, &end_addr);
		#endif

#if MEMORY_REORDER
		if (buf_method >= VENDOR_AI_NET_BUF_OPT_SHRINK_O1) {
			nn_reorder_exe(proc_id, p_mctrl, p_io_buff, proc_layers_num, job_method, buf_method, &end_addr, &ideal_size);
			vendor_ai_net_trace(proc_id, AI_BUF, "proc_id(%lu) user_buffer size = %lu (ideal = %lu)\r\n", proc_id, (end_addr - used_start_addr), ideal_size);
		}
#endif
		nn_manage_postfix(proc_id, p_mctrl, proc_layers_num, p_io_buff, id_list_start_addr, &end_addr);
	}

#if !CNN_25_MATLAB
{
	//-------------------- [ DEBUG ] --------------------
	for (idx=0; idx<proc_layers_num; idx++) {
		out_buf_num = OUT_BUF_NUM(&p_mctrl[idx]);
		for (buf_idx=0; buf_idx<out_buf_num; buf_idx++) {
			out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);  // p_mctrl[idx].out_buf_index[buf_idx];
			out_buf_size = OUT_BUF_SIZE(&p_mctrl[idx], buf_idx); // p_mctrl[idx].output_buffsize[buf_idx];

			if ((out_buf_id<0) || (out_buf_size<=0)) {
				continue;
			}

			// debug mode : let some buf become preserve, set by proc command => echo iomem set_preserve_bufid 0 42 > /proc/kflow_ai/cmd
			if (TRUE == _vendor_ai_net_mem_debug_is_bufid_debug_preserve(out_buf_id)) {
				printf("[MEM] debug : mctrl(%d) buf_id(%d) will preserve !!\n", (int)idx, (int)out_buf_id);
				OUT_BUF_ATTR_SET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PRESERVE, 1); //p_mctrl[idx].is_preserve = 1;
			}
		}
	}
	// debug mode : let some buf move to "pure address"
	_vendor_ai_net_mem_debug_move_preserve_buf_to_unused_addr(p_io_buff, &end_addr);
	_vendor_ai_net_mem_debug_clr_preserve_buf();
}
#endif

#if CHK_MEMORY_OVERLAP
if (vendor_ai_cmd_get_iomem_debug(proc_id) & VENDOR_AI_NET_CMD_IOMEM_CHK_OVERLAP) {
	// check buffer overlap !!
	if (TRUE == nn_manage_detect_overlap(proc_id, p_mctrl, proc_layers_num, p_io_buff)) {
		DBG_WRN("proc_id(%lu) buffer overlap detected !!\r\n", proc_id);
	}
}
#endif

#if DUMP_MEMORY_OVERLAP
	nn_manage_detect_overlap_to_bufdot(proc_id, p_mctrl, proc_layers_num, p_io_buff);
#endif

#if DUMP_MEMORY_DEBUG
if (vendor_ai_cmd_get_iomem_debug(proc_id) & VENDOR_AI_NET_CMD_IOMEM_DUMP_DEBUG) {
	_nn_memory_debug_dump(proc_id, p_mctrl, p_io_buff, proc_layers_num);
}
#endif

	//---- check memroy exceed 256MB limit ----
	if (end_addr >= 0x10000000) {
		DBG_ERR("proc_id(%lu) iobuf alloc size = %u > iobuf limit = %u (256MB), iobuf alloc failed !!\r\n", proc_id, (unsigned int)end_addr, (unsigned int)0x10000000);
		return HD_ERR_LIMIT;
	}

	//---- start to assign iomem ----
	for (idx=0; idx<proc_layers_num; idx++) {
#if CNN_25_MATLAB
		const UINT32 layer_idx = p_mctrl[idx].layer_index;
#endif
		const INT32 in_buf_id0 = IN_BUF_INDEX(&p_mctrl[idx], 0) < 0 ? -1 : IN_BUF_INDEX(&p_mctrl[idx], 0); // p_mctrl[idx].in_buf_index[0];
		const INT32 in_buf_id1 = IN_BUF_INDEX(&p_mctrl[idx], 1) < 0 ? -1 : IN_BUF_INDEX(&p_mctrl[idx], 1); // p_mctrl[idx].in_buf_index[1];
#if (USE_NEON || (!CNN_25_MATLAB))
		const INT32 in_buf_id2 = IN_BUF_INDEX(&p_mctrl[idx], 2) < 0 ? -1 : IN_BUF_INDEX(&p_mctrl[idx], 2); // p_mctrl[idx].in_buf_index[2];
#endif

		const INT32 out_buf_id0 = OUT_BUF_INDEX(&p_mctrl[idx], 0) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], 0); // p_mctrl[idx].out_buf_index[0];
		const INT32 out_buf_id1 = OUT_BUF_INDEX(&p_mctrl[idx], 1) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], 1); // p_mctrl[idx].out_buf_index[1];
		//const INT32 out_buf_id2 = OUT_BUF_INDEX(&p_mctrl[idx], 2) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], 2); // p_mctrl[idx].out_buf_index[2];
		#if CNN_CGEN_NEW_TMP_BUF
		INT32 tmp_buf_id0 = -1;
		#endif
#if !CNN_25_MATLAB
		NN_DATA *p_sai = (NN_DATA *)(p_mctrl[idx].iomem.imem_addr);
		NN_DATA *p_sao = (NN_DATA *)(p_mctrl[idx].iomem.omem_addr);
#endif

#if CNN_CGEN_NEW_TMP_BUF
#if CNN_FMT_V4
		tmp_buf_id0 = -1;
		for (buf_idx = 0; buf_idx < OUT_BUF_NUM(&p_mctrl[idx]); buf_idx++) {
			if (TRUE == OUT_BUF_IS_TMP(&p_mctrl[idx], buf_idx)) {
				tmp_buf_id0 = OUT_BUF_INDEX(&p_mctrl[idx], buf_idx);
				break;
			}
		}
#else
		tmp_buf_id0 = OUT_BUF_INDEX(&p_mctrl[idx], 4) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], 4); // p_mctrl[idx].out_buf_index[4];
#endif
#endif

#if CNN_25_MATLAB
		// buffer address
		if ((p_mctrl[idx].mode==NN_CONV) && (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_CONV_IN_ISIMG) == ISIMG_IMAGE)) { //if ((p_mctrl[idx].mode==NN_CONV) && (p_reg[idx].in_reg.in_isimg==IMAGE)) { //image mode IN_ISIMAGE
			p_io_mem[layer_idx].SAI[0].va  = (in_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_io_mem[layer_idx].SAI[1].va  = (in_buf_id0<0) ? 0 : (p_io_mem[layer_idx].SAI[0].va + p_io_mem[layer_idx].SAI[0].size);
			p_io_mem[layer_idx].SAI[2].va  = (in_buf_id0<0) ? 0 : (p_io_mem[layer_idx].SAI[1].va + p_io_mem[layer_idx].SAI[1].size);
			p_io_mem[layer_idx].SAO[0].va	= (out_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_io_mem[layer_idx].SAO[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
			//intermediate
			p_io_mem[layer_idx].SAI[6].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
		} 
				
		
		else {  //non-image
			p_io_mem[layer_idx].SAI[0].va	= (in_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_io_mem[layer_idx].SAO[0].va	= (out_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			switch (p_mctrl[idx].mode) {
			case NN_CONV:
				p_io_mem[layer_idx].SAI[6].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_io_mem[layer_idx].SAO[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				break;
			case NN_ROIPOOLING:
				p_io_mem[layer_idx].SAI[5].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);			
				break;
			case NN_ELTWISE:
				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_ELTWISE_IN_SRC) == ELT_IN_SRC_DRAM) { //if (p_mctrl[idx].eltwise_in1_src == ELT_IN_SRC_DRAM) {  //if (p_reg[idx].eltwise_reg.eltwise_in1_src == DRAM) {
					p_io_mem[layer_idx].SAI[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				}

				// ELTWISE  SAI[6] maybe  model_addr / buf_addr  => original vsgen will check (p_reg[idx].eltwise_reg.eltwise_in1_src == GENTOOL) to update&overwrite later
				//  so on the other hand, now check only if (eltwise_in1_src != GENTOOL) to update buf_addr
				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_ELTWISE_IN_SRC) != ELT_IN_SRC_GENTOOL) {//if (p_mctrl[idx].eltwise_in1_src != ELT_IN_SRC_GENTOOL) {
					p_io_mem[layer_idx].SAI[6].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				}

				p_io_mem[layer_idx].SAO[1].va = (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				break;
			case NN_PREPROC:			
				if (idx != 0) { //for MTCNN case
					if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_RGB || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_BGR) {
						p_io_mem[layer_idx].SAI[1].va = p_io_mem[layer_idx-1].SAO[1].va;
						p_io_mem[layer_idx].SAI[2].va = p_io_mem[layer_idx-1].SAO[2].va;
					}

					if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_YUV420) {
						p_io_mem[layer_idx].SAI[1].va = p_io_mem[layer_idx].SAO[0].va + p_io_mem[layer_idx].SAI[1].size *2; //UV size*2 = Y size
					}		
				}

				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_RGB || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_BGR || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_YUV420) {
					// merge pad_en + rot_en + scale  into one case ...
					if (OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PREPROC_OUT_FMT) == FMT_RGB) { //if (p_reg[idx].preproc_reg.out_fmt == RGB) {
							p_io_mem[layer_idx].SAO[1].va = p_io_mem[layer_idx].SAO[0].va + p_io_mem[layer_idx].SAO[0].size;
							p_io_mem[layer_idx].SAO[2].va = p_io_mem[layer_idx].SAO[1].va + p_io_mem[layer_idx].SAO[1].size;
					}
					else { //out_fmt == BGR
							p_io_mem[layer_idx].SAO[2].va = p_io_mem[layer_idx].SAO[0].va;
							p_io_mem[layer_idx].SAO[1].va = p_io_mem[layer_idx].SAO[2].va + p_io_mem[layer_idx].SAO[2].size;
							p_io_mem[layer_idx].SAO[0].va = p_io_mem[layer_idx].SAO[1].va + p_io_mem[layer_idx].SAO[1].size;
					}
				}			

				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == UV_PACKED) {
					p_io_mem[layer_idx].SAO[0].size = p_io_mem[layer_idx].SAO[0].size * 2;						
					p_io_mem[layer_idx].SAI[0].size = p_io_mem[layer_idx].SAI[0].size * 2;						
				}				
				
				break;
			case NN_CUSTOMER:
				p_io_mem[layer_idx].SAI[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_io_mem[layer_idx].SAO[1].va = (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				break;
			case NN_POSTPROC:
			case NN_ANCHOR:
			case NN_DECONV:
			case NN_SVM:			
			case NN_REORGANIZATION:
			case NN_RESHAPE:
			case NN_PROPOSAL:
			case NN_FC:
			case NN_SOFTMAX:
			case NN_FC_POST:
			case NN_POOL:			
			case NN_BNSCALE:
			default:
				break;
			}
		}
#else
		// buffer address
		if ((p_mctrl[idx].mode==NN_CONV) && (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_CONV_IN_ISIMG) == ISIMG_IMAGE)) { //if ((p_mctrl[idx].mode==NN_CONV) && (p_reg[idx].in_reg.in_isimg==IMAGE)) { //image mode IN_ISIMAGE
			p_sai[0].va  = (in_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_sai[1].va  = (in_buf_id0<0) ? 0 : (p_sai[0].va + p_sai[0].size);
			p_sai[2].va  = (in_buf_id0<0) ? 0 : (p_sai[1].va + p_sai[1].size);
			p_sao[0].va	= (out_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_sao[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
			//intermediate
			p_sai[6].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
			p_sai[6].size = (p_sai[6].va == 0)? 0:p_io_buff[in_buf_id1].size;
		} 
				
		
		else {  //non-image
			p_sai[0].va	= (in_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
			p_sai[0].size = (p_sai[0].va == 0)? 0:p_io_buff[in_buf_id0].size;
			p_sao[0].va	= (out_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);

			if (p_mctrl[idx].mode != NN_PREPROC) {
				p_sao[0].size = (p_sao[0].va == 0)? 0:p_io_buff[out_buf_id0].size;
			}

			switch (p_mctrl[idx].mode) {
			case NN_CONV:
#if AI_V4
			case NN_DEPTHWISE:
#endif
				p_sai[6].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[6].size = (p_sai[6].va == 0)? 0:p_io_buff[in_buf_id1].size;
				p_sao[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				break;
			case NN_MATMUL:
				p_sai[4].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[4].size = (p_sai[4].va == 0)? 0:p_io_buff[in_buf_id1].size;
				if (OUT_BUF_NUM(&p_mctrl[idx]) >= 2) {
					p_sao[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				}
				break;
			case NN_CORR:
				p_sai[4].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[4].size = (p_sai[4].va == 0)? 0:p_io_buff[in_buf_id1].size;
				p_sao[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				break;
			case NN_BN:
				p_sai[5].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[5].size = (p_sai[5].va == 0)? 0:p_io_buff[in_buf_id1].size;
				if (OUT_BUF_NUM(&p_mctrl[idx]) >= 2) {
					p_sao[1].va	= (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				}
				break;
			case NN_ROIPOOLING:
				p_sai[5].va	= (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);			
				p_sai[5].size = (p_sai[5].va == 0)? 0:p_io_buff[in_buf_id1].size;
				break;
			case NN_ELTWISE:
				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_ELTWISE_IN_SRC) == ELT_IN_SRC_DRAM) {  //if (p_reg[idx].eltwise_reg.eltwise_in1_src == DRAM) {
					p_sai[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sai[1].size = (p_sai[1].va == 0)? 0:p_io_buff[in_buf_id1].size;
				}

				// ELTWISE  SAI[6] maybe  model_addr / buf_addr  => original vsgen will check (p_reg[idx].eltwise_reg.eltwise_in1_src == GENTOOL) to update&overwrite later
				//  so on the other hand, now check only if (eltwise_in1_src != GENTOOL) to update buf_addr
				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_ELTWISE_IN_SRC) != ELT_IN_SRC_GENTOOL) {
					p_sai[6].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sai[6].size = (p_sai[6].va == 0)? 0:p_io_buff[in_buf_id1].size;
				}

				p_sao[1].va = (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				break;
			case NN_PREPROC:			
				if (idx != 0) { //for MTCNN case
					if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_RGB || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_BGR) {
						NN_DATA *p_sao_pre = (NN_DATA*)p_mctrl[idx-1].iomem.omem_addr; // this maybe wrong, maybe stripe cause (mctrl_idx - 1) , but layer_idx still the same
						p_sai[1].va = p_sao_pre[1].va;
						p_sai[2].va = p_sao_pre[2].va;
					}
					if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_YUV420) {
						p_sai[1].va = p_sao[0].va + p_sai[1].size *2; //UV size*2 = Y size
					}		
				}

				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_RGB || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_BGR || IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == FMT_YUV420) {
					// merge pad_en + rot_en + scale  into one case ...
					if (OUT_BUF_ATTR_GET(&p_mctrl[idx], NN_OUT_BUF_ATTR_PREPROC_OUT_FMT) == FMT_RGB) { //if (p_reg[idx].preproc_reg.out_fmt == RGB) {
							p_sao[1].va = p_sao[0].va + p_sao[0].size;
							p_sao[2].va = p_sao[1].va + p_sao[1].size;
					}
					else { //out_fmt == BGR
							p_sao[2].va = p_sao[0].va;
							p_sao[1].va = p_sao[2].va + p_sao[2].size;
							p_sao[0].va = p_sao[1].va + p_sao[1].size;
					}
				}			

				if (IN_BUF_ATTR_GET(&p_mctrl[idx], NN_IN_BUF_ATTR_PREPROC_IN_FMT) == UV_PACKED) {
					p_sao[0].size = p_sao[0].size * 2;						
					p_sai[0].size = p_sai[0].size * 2;						
				}				
				
				break;
			case NN_CUSTOMER:
#if CUST_SUPPORT_MULTI_IO
				{
					int i=0;
					UINT32 last_sai=0;
					INT32 in_buf_id = -1, out_buf_id = -1;

					// use IN_NUM as loop  (for example, maybe buf_id => 4 in/ 2 out , but sai/sao => 5 in / 1 out ) , we only assign 4-to-4
					for (i=0 ; i< (int)IN_BUF_NUM(&p_mctrl[idx]) ; i++) {
						in_buf_id = IN_BUF_INDEX(&p_mctrl[idx], i) < 0 ? -1 : IN_BUF_INDEX(&p_mctrl[idx], i);
						p_sai[i].va   = (in_buf_id<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id].addr, NN_GEN_BUF_ADDR_TYPE);
						p_sai[i].size = (p_sai[i].va == 0)? 0:p_io_buff[in_buf_id].size;
					}
					// use sao_num as loop (for example, maybe buf_id => 4 in/ 2 out , but sai/sao => 5 in / 1 out ) , we only assign 1-to-1
					for (i=0 ; i< (int)p_mctrl[idx].iomem.omem_cnt; i++) {
						out_buf_id = OUT_BUF_INDEX(&p_mctrl[idx], i) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], i);
						p_sao[i].va   = (out_buf_id<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id].addr, NN_GEN_BUF_ADDR_TYPE);
						p_sao[i].size = (p_sao[i].va == 0)? 0:p_io_buff[out_buf_id].size;
					}

					#if CNN_CGEN_NEW_TMP_BUF
					// if last out buf_id is tmp
					i = OUT_BUF_NUM(&p_mctrl[idx]) - 1; // last out buf
					if (OUT_BUF_IS_TMP(&p_mctrl[idx], i) == TRUE) {
						tmp_buf_id0 = OUT_BUF_INDEX(&p_mctrl[idx], i) < 0 ? -1 : OUT_BUF_INDEX(&p_mctrl[idx], i);
						// assign tmp to last sai
						last_sai = p_mctrl[idx].iomem.imem_cnt - 1;
						p_sai[last_sai].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
						p_sai[last_sai].size = (p_sai[last_sai].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
					}
					#endif
				}
#else
				p_sai[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[1].size = (p_sai[1].va == 0)? 0:p_io_buff[in_buf_id1].size;
				p_sao[1].va = (out_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[out_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sao[1].size = (p_sao[1].va == 0)? 0:p_io_buff[out_buf_id1].size;
				#if CNN_CGEN_NEW_TMP_BUF
				p_sao[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sao[2].size = (p_sao[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
#endif
				break;
			case NN_POSTPROC:
			case NN_ANCHOR:
			case NN_DECONV:
			case NN_SVM:			
			case NN_REORGANIZATION:
			case NN_RESHAPE:
			case NN_PROPOSAL:
			case NN_FC:
				break;
			case NN_SOFTMAX:
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
#if NN_DLI
			case NN_DLI_SQRT:
			case NN_DLI_EXP:
			case NN_DLI_RESIZE:
			case NN_DLI_LOG:
			case NN_DLI_SIN:
			case NN_DLI_FLOOR:
			case NN_DLI_ROUND:
			case NN_DLI_SOFTMAX:
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
			case NN_DLI_DIV:
			case NN_DLI_POW:
			case NN_DLI_EQUAL:
			case NN_DLI_GREATER:
			case NN_DLI_LESS:
				// input2 can be from iomem(buffer) or weight(model)
				if((p_sai[1].va & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
					p_sai[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sai[1].size = (p_sai[1].va == 0)? 0:p_io_buff[in_buf_id1].size;
				}
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
#endif
			case NN_FC_POST:
			case NN_POOL:			
			case NN_BNSCALE:
				break;
#if (USE_NEON || (!CNN_25_MATLAB))
			case NN_PRELU:
				break;
			case NN_SIGMOID:
				break;
			case NN_PRIORBOX:
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
			case NN_NORM:
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
			case NN_LAYER_NORMALIZATION:
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
			case NN_DETOUT:
				p_sai[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[1].size = (p_sai[1].va == 0)? 0:p_io_buff[in_buf_id1].size;
				p_sai[3].va = (in_buf_id2<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id2].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[3].size = (p_sai[3].va == 0)? 0:p_io_buff[in_buf_id2].size;
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
				break;
            case NN_FP2FIX:
	        case NN_LSTM:
				if (IN_BUF_NUM_REAL(&p_mctrl[idx]) > 1) {
					p_sai[1].va = (in_buf_id1<0) ? 0 : nn_addr_insert_flag(p_io_buff[in_buf_id1].addr, NN_GEN_BUF_ADDR_TYPE);
					p_sai[1].size = (p_sai[1].va == 0)? 0:p_io_buff[in_buf_id1].size;
				}
				#if CNN_CGEN_NEW_TMP_BUF
				p_sai[2].va = (tmp_buf_id0<0) ? 0 : nn_addr_insert_flag(p_io_buff[tmp_buf_id0].addr, NN_GEN_BUF_ADDR_TYPE);
				p_sai[2].size = (p_sai[2].va == 0)? 0:p_io_buff[tmp_buf_id0].size;
				#endif
                break;
	            case NN_REVERSE:
#endif
			default:
				break;
			}
		}

		for (buf_idx = 0; buf_idx < p_mctrl[idx].iomem.imem_cnt ; buf_idx++) {
			p_sai[buf_idx].pa = p_sai[buf_idx].va;
		}
		for (buf_idx = 0; buf_idx < p_mctrl[idx].iomem.omem_cnt ; buf_idx++) {
			p_sao[buf_idx].pa = p_sao[buf_idx].va;
		}
#endif
	}
	
exit:
	if (p_io_buff) {
		vendor_ai_free(p_io_buff, proc_layers_num *2 * sizeof(NN_BUF));
	}
	
	vendor_ai_net_trace(proc_id, AI_BUF, "proc_id(%lu) user_buffer size = %lu\r\n", proc_id, (end_addr - used_start_addr));
	*io_buf_size = (end_addr - used_start_addr);
	return HD_OK;
}

HD_RESULT _vendor_ai_net_mem_alloc_mem_iomem_fix(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 buf_method, UINT32 job_method)
{
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = &p_mem_manager->user_parm;
	NN_GEN_NET_INFO net_info = {0};
	
	NN_GEN_MODEL_HEAD *p_head;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 id_list_start_addr = 0;
	UINT32 proc_layer_num;
	UINT32 io_buf_size = 0;
	
	HD_RESULT rv = HD_OK;

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
	id_list_start_addr = (UINT32)net_info.p_id_list;
	proc_layer_num = p_head->mode_ctrl_num;

	// calculate io_buf_size
#if CNN_25_MATLAB
	io_buf_size = nn_net_alloc_mem(proc_id, p_io_mem, p_mctrl, proc_layer_num, buf_method, job_method, id_list_start_addr);
#else
	rv = nn_net_alloc_mem(proc_id, p_mem->va, p_mctrl, proc_layer_num, buf_method, job_method, id_list_start_addr, &io_buf_size);
	if (rv != HD_OK) {
		DBG_ERR("_vendor_ai_net_mem_alloc_mem_iomem fail...\r\n");
		return rv;
	}
#endif

	// update io_buf_size
	p_mem_manager->user_buff.size = io_buf_size;
	p_head->io_buff_size = io_buf_size;

	return HD_OK;
}

HD_RESULT _vendor_ai_net_mem_alloc_mem_iomem_fix_test_zero(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
#if CNN_25_MATLAB
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = &p_mem_manager->user_parm;
	NN_GEN_NET_INFO net_info = {0};
	
	NN_GEN_MODEL_HEAD *p_head;
	NN_IOMEM *p_io_mem;
	
	HD_RESULT rv = HD_OK;
	UINT32 idx = 0, idx2 = 0;

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
	p_io_mem = net_info.p_io_mem;

	// fix IOMEM
	for (idx = 0; idx < p_head->layer_num; idx++) {
		for (idx2 = 0; idx2 < NN_IMEM_NUM; idx2++) {
			if (p_io_mem[idx].SAI[idx2].va == 0x0ccccccc) p_io_mem[idx].SAI[idx2].va = DUMMY_ADDR;

		}
		for (idx2 = 0; idx2 < NN_OMEM_NUM; idx2++) {
			if (p_io_mem[idx].SAO[idx2].va == 0x0ccccccc) p_io_mem[idx].SAO[idx2].va = DUMMY_ADDR;
		}
	}
#endif
	return HD_OK;
}

//--------------------------------------------------------------------------------
#if CNN_25_MATLAB
VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_app(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode)
{
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)(p_mctrl->addr + va_ofs);
	UINT32 ai_parm_addr = p_head->parm_addr + va_ofs;
	
	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)ai_parm_addr;
			p_parm->in_addr           = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out0_addr         = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0]; // new bin handle order in gentool
			p_parm->out1_addr         = p_io_mem->SAO[1].va + OUT_BUF_OFFSET(p_mctrl, 1); // p_mctrl->stripe_outaddr[1]; // new bin handle order in gentool
			p_parm->in_interm_addr    = p_io_mem->SAI[6].va + IN_BUF_OFFSET(p_mctrl, 1);  // p_mctrl->stripe_inaddr[1];;
			p_parm->out_interm_addr   = p_parm->out0_addr;
			p_parm->tmp_buf_addr      = 0;
			p_parm->elt.addr                  = p_io_mem->SAI[6].va;  //model parameters(SAI[6])
			#if 0 // alread in bin file, don't have to modify
			p_parm->conv.weight_addr  = p_io_mem->SAI[3].va;          //model parameters(SAI[3])
			p_parm->conv.bias_addr    = p_io_mem->SAI[5].va;          //model parameters(SAI[5])
			p_parm->conv.fcd.quanti_kmeans_addr = p_io_mem->SAI[4].va; //model parameters(SAI[4])
			p_parm->conv.fcd.p_vlc_code         //null addr type
			p_parm->conv.fcd.p_vlc_valid        //null addr type
			p_parm->conv.fcd.p_vlc_ofs          //null addr type
			p_parm->norm.bn_scl.bn_scale_addr = p_io_mem->SAI[5].va;  //model parameters(SAI[5])			
			#endif
		}
		break;
	case AI_MODE_ROIPOOL: {
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)ai_parm_addr;
			p_parm->in_addr           = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr          = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->roi_ker.roi_addr  = p_io_mem->SAI[5].va;
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->svm_ker.sv_addr        = p_io_mem->SAI[2].va;
			p_parm->svm_ker.alpha_addr     = p_io_mem->SAI[3].va;
			p_parm->fcd.quanti_kmeans_addr = p_io_mem->SAI[7].va;
			#if 0
			p_parm->fcd.p_vlc_code
			p_parm->fcd.p_vlc_valid
			p_parm->fcd.p_vlc_ofs
			#endif
		}
		break;
	case AI_MODE_FC: {
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->in_interm_addr         = 0;  //not use
			p_parm->out_interm_addr        = (p_parm->out_interm_addr==0)? 0:p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			#if 0
			p_parm->fc_ker.weight_addr     = p_io_mem->SAI[2].va;
			p_parm->fc_ker.bias_addr       = p_io_mem->SAI[4].va;
			p_parm->fcd.quanti_kmeans_addr = p_io_mem->SAI[7].va;
			p_parm->fcd.p_vlc_code
			p_parm->fcd.p_vlc_valid
			p_parm->fcd.p_vlc_ofs
			#endif
		}
		break;
	case AI_MODE_PERMUTE: {
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)ai_parm_addr;
			p_parm->in_addr[0]   = p_io_mem->SAI[0].va;
            p_parm->in_addr[1]   = p_io_mem->SAI[1].va;
            p_parm->in_addr[2]   = p_io_mem->SAI[2].va;
			p_parm->out_addr[0]  = p_io_mem->SAO[0].va;
            p_parm->out_addr[1]  = p_io_mem->SAO[1].va;
            p_parm->out_addr[2]  = p_io_mem->SAO[2].va;
		}
		break;
	default:
		break;
	}
}

VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_ll(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode)
{
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)(p_mctrl->addr + va_ofs);
	UINT32 ai_parm_addr = p_head->parm_addr + va_ofs;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		p_cnn_ll_parm = (CNN_LL_PARM*)ai_parm_addr;

		p_cnn_ll_parm->input.bit.addr     = (p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0));  // p_mctrl->stripe_inaddr[0]);
		p_cnn_ll_parm->interm_in.bit.addr = (p_io_mem->SAI[6].va + IN_BUF_OFFSET(p_mctrl, 1));  // p_mctrl->stripe_inaddr[1]); // need to add stripe offset for SAI[6] (c-gentool maybe ELTWISE + stripe)
		p_cnn_ll_parm->output[0].bit.addr = (p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0)); // p_mctrl->stripe_outaddr[0]); // new bin handle order in gentool
		p_cnn_ll_parm->output[1].bit.addr = (p_io_mem->SAO[1].va + OUT_BUF_OFFSET(p_mctrl, 1)); // p_mctrl->stripe_outaddr[1]); // new bin handle order in gentool
		#if 0
		p_cnn_ll_parm->weight.bit.addr
		p_cnn_ll_parm->kmean.bit.addr
		p_cnn_ll_parm->bias_bnscal.bit.addr
		#endif
		break;
	case AI_ENG_NUE:
		p_nue_ll_parm = (NUE_LL_PARM*)ai_parm_addr;

		p_nue_ll_parm->input.bit.addr  = (p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0));  // p_mctrl->stripe_inaddr[0]);
		p_nue_ll_parm->elt_in.bit.addr = 0; // _TODO
		p_nue_ll_parm->roi_in.bit.addr = (p_io_mem->SAI[5].va + IN_BUF_OFFSET(p_mctrl, 1));
		p_nue_ll_parm->output.bit.addr = (p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0)); // p_mctrl->stripe_outaddr[0]);
		#if 0
		p_nue_ll_parm->sv.bit.addr
		p_nue_ll_parm->alpha.bit.addr
		p_nue_ll_parm->rho.bit.addr
		p_nue_ll_parm->kmean.bit.addr
		#endif
		break;
	case AI_ENG_NUE2:
		p_nue2_ll_parm = (NUE2_LL_PARM*)ai_parm_addr;

		p_nue2_ll_parm->input[0].bit.addr  = (p_io_mem->SAI[0].va);   // _TODO : some tricky fix?
		p_nue2_ll_parm->input[1].bit.addr  = (p_io_mem->SAI[1].va);
		p_nue2_ll_parm->input[2].bit.addr  = (p_io_mem->SAI[2].va);
		p_nue2_ll_parm->output[0].bit.addr = (p_io_mem->SAO[0].va);
		p_nue2_ll_parm->output[1].bit.addr = (p_io_mem->SAO[1].va);
		p_nue2_ll_parm->output[2].bit.addr = (p_io_mem->SAO[2].va);
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
}

VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_comm(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode, UINT32 proc_id)
{
	UINT32 ai_head_addr = (p_mctrl->addr + va_ofs);

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
	case NN_RESHAPE:
		break;

	case NN_PROPOSAL:
		{
			NN_CPU_PARM *p_cpu_parm = (NN_CPU_PARM *)ai_head_addr;
			p_cpu_parm->addr_in  = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_cpu_parm->addr_out = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;

	case NN_POSTPROC:
		{
			NN_POSTPROC_PARM *p_postproc_parm = (NN_POSTPROC_PARM *)ai_head_addr;
			p_postproc_parm->in_addr  = p_io_mem->SAI[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_postproc_parm->out_addr = p_io_mem->SAO[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			#if 0
			p_postproc_parm->tmp_addr  // _TODO : ???
			#endif
		}
		break;
	case NN_SOFTMAX:
		{
			NN_SOFTMAX_PARM *p_softmax_parm = (NN_SOFTMAX_PARM *)ai_head_addr;
			p_softmax_parm->in_addr  = p_io_mem->SAI[0].va;
			p_softmax_parm->out_addr = p_io_mem->SAO[0].va;
		}
		break;
#if 0
	case NN_PREPROC:
		{
			NN_PRE_PARM *p_pre_parm = (NN_PRE_PARM*)ai_head_addr;
			p_pre_parm->in_addr
			p_pre_parm->out_addr
			p_pre_parm->interm_addr
			p_pre_parm->sub_img.plane_addr
		}
		break;
#endif	
	case NN_FC_POST:
		{
			NN_FC_POST_PARM *p_fc_post_parm = (NN_FC_POST_PARM*)ai_head_addr;
			p_fc_post_parm->addr_in   = p_io_mem->SAI[0].va;
			p_fc_post_parm->addr_out  = p_io_mem->SAO[0].va;
			p_fc_post_parm->bias_addr = p_io_mem->SAI[1].va;
		}
		break;
		
	case NN_POOL:
		{
			NN_POOL_PARM *p_pool_parm = (NN_POOL_PARM*)ai_head_addr;
			p_pool_parm->in_addr  = p_io_mem->SAI[0].va;
			p_pool_parm->out_addr = p_io_mem->SAO[0].va;
		}
		break;
		
	case NN_CUSTOMER:
		{
			NN_CUSTOM_PARM *p_cust_head = (NN_CUSTOM_PARM*)ai_head_addr;
			p_cust_head->input.va  = p_io_mem->SAI[0].va;
			p_cust_head->output.va = p_io_mem->SAO[0].va;
			p_cust_head->model.va  = p_io_mem->SAI[1].va;

			p_cust_head->input.pa   = vendor_ais_user_buff_va2pa(p_cust_head->input.va, proc_id);
			p_cust_head->output.pa  = vendor_ais_user_buff_va2pa(p_cust_head->output.va, proc_id);
			if (p_cust_head->model.va > 0)
				p_cust_head->model.pa   = vendor_ais_user_model_va2pa(p_cust_head->model.va, proc_id);
		}
		break;
		
	case NN_BNSCALE:
		{
			NN_BNSCALE_PARM *p_bn_scl_parm = (NN_BNSCALE_PARM*)ai_head_addr;
			p_bn_scl_parm->in_addr    = p_io_mem->SAI[0].va;
			p_bn_scl_parm->out_addr   = p_io_mem->SAO[0].va;
			p_bn_scl_parm->mean_addr  = p_io_mem->SAI[1].va;
			p_bn_scl_parm->alpha_addr = p_io_mem->SAI[2].va;
			p_bn_scl_parm->beta_addr  = p_io_mem->SAI[3].va;
		}
		break;

	default :
		break;
	}
}
#else
VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_app(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode)
{
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)(p_mctrl->addr + va_ofs);
	UINT32 ai_parm_addr = p_head->parm_addr + va_ofs;
	NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
	NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);
	
	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)ai_parm_addr;
			p_parm->in_addr           = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
#if 1
			p_parm->out0_addr         = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0]; // new bin handle order in gentool
			p_parm->out1_addr         = p_sao[1].va + OUT_BUF_OFFSET(p_mctrl, 1); // p_mctrl->stripe_outaddr[1]; // new bin handle order in gentool
#else
			if (mode == NN_ELTWISE) {  // _TODO :  remove this test code later when v3 bin is ready
				p_parm->out0_addr         = p_sao[1].va + p_mctrl->stripe_outaddr[1];
				p_parm->out1_addr         = p_sao[0].va + p_mctrl->stripe_outaddr[0];
			} else {
				p_parm->out0_addr         = p_sao[0].va + p_mctrl->stripe_outaddr[0];
				p_parm->out1_addr         = p_sao[1].va + p_mctrl->stripe_outaddr[1];
			}
#endif
			p_parm->in_interm_addr    = p_sai[6].va + IN_BUF_OFFSET(p_mctrl, 1);  // p_mctrl->stripe_inaddr[1];
			p_parm->out_interm_addr   = p_parm->out0_addr;
			p_parm->tmp_buf_addr      = 0;
			p_parm->elt.addr                  = p_sai[6].va;  //model parameters(SAI[6])
			#if 0 // alread in bin file, don't have to modify
			p_parm->conv.weight_addr  = p_io_mem->SAI[3].va;          //model parameters(SAI[3])
			p_parm->conv.bias_addr    = p_io_mem->SAI[5].va;          //model parameters(SAI[5])
			p_parm->conv.fcd.quanti_kmeans_addr = p_io_mem->SAI[4].va; //model parameters(SAI[4])
			p_parm->conv.fcd.p_vlc_code         //null addr type
			p_parm->conv.fcd.p_vlc_valid        //null addr type
			p_parm->conv.fcd.p_vlc_ofs          //null addr type
			p_parm->norm.bn_scl.bn_scale_addr = p_io_mem->SAI[5].va;  //model parameters(SAI[5])			
			#endif
		}
		break;
	case AI_MODE_ROIPOOL: {
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)ai_parm_addr;
			p_parm->in_addr           = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr          = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->roi_ker.roi_addr  = p_sai[5].va;
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->svm_ker.sv_addr        = p_sai[2].va;
			p_parm->svm_ker.alpha_addr     = p_sai[3].va;
			p_parm->fcd.quanti_kmeans_addr = p_sai[7].va;
			#if 0
			p_parm->fcd.p_vlc_code
			p_parm->fcd.p_vlc_valid
			p_parm->fcd.p_vlc_ofs
			#endif
		}
		break;
	case AI_MODE_FC: {
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			p_parm->in_interm_addr         = 0;  //not use
			p_parm->out_interm_addr        = (p_parm->out_interm_addr==0)? 0:p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			#if 0
			p_parm->fc_ker.weight_addr     = p_io_mem->SAI[2].va;
			p_parm->fc_ker.bias_addr       = p_io_mem->SAI[4].va;
			p_parm->fcd.quanti_kmeans_addr = p_io_mem->SAI[7].va;
			p_parm->fcd.p_vlc_code
			p_parm->fcd.p_vlc_valid
			p_parm->fcd.p_vlc_ofs
			#endif
		}
		break;
	case AI_MODE_PERMUTE: {
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)ai_parm_addr;
			p_parm->in_addr                = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_parm->out_addr               = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)ai_parm_addr;
			p_parm->in_addr[0]   = p_sai[0].va;
            p_parm->in_addr[1]   = p_sai[1].va;
            p_parm->in_addr[2]   = p_sai[2].va;
			p_parm->out_addr[0]  = p_sao[0].va;
            p_parm->out_addr[1]  = p_sao[1].va;
            p_parm->out_addr[2]  = p_sao[2].va;
		}
		break;
	default:
		break;
	}
}
#if AI_V4
VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_multi_ll(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode)
{
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)(p_mctrl->addr + va_ofs);
	UINT32 ai_parm_addr = p_head->parm_addr + va_ofs;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
	NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
	NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);
	UINT32 parm_start_addr = 0;
	UINT32 end_flg = 0;
	UINT32 depwise_cnt = 0;
	UINT32 depwise_one_layer_i_num = IN_BUF_NUM(p_mctrl);
	UINT32 depwise_one_layer_o_num = OUT_BUF_NUM(p_mctrl);
	parm_start_addr = ai_parm_addr;

	while (end_flg == 0) {
		UINT32 ll_cmd_tail = ai_parm_addr + p_head->parm_size - sizeof(UINT64);

		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
			p_cnn_ll_parm = (CNN_LL_PARM*)parm_start_addr;

			p_cnn_ll_parm->input.bit.addr     = (p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0 + depwise_one_layer_i_num*depwise_cnt));  // p_mctrl->stripe_inaddr[0]);
			if(p_mctrl->mode == NN_CORR){
				p_cnn_ll_parm->weight.bit.addr = (p_sai[4].va + IN_BUF_OFFSET(p_mctrl, 1 + depwise_one_layer_i_num*depwise_cnt));  // Corr SAI[4] is a weight from local ativation
			}else{
				p_cnn_ll_parm->interm_in.bit.addr = (p_sai[6].va + IN_BUF_OFFSET(p_mctrl, 1 + depwise_one_layer_i_num*depwise_cnt));  // p_mctrl->stripe_inaddr[1]);  // need to add stripe offset for sai[6] (c-gentool maybe ELTWISE + stripe)
			}

#if 1
			p_cnn_ll_parm->output[0].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0 + depwise_one_layer_o_num*depwise_cnt)); //p_mctrl->stripe_outaddr[0]); // new bin handle order in gentool
			p_cnn_ll_parm->output[1].bit.addr = (p_sao[1].va + OUT_BUF_OFFSET(p_mctrl, 1 + depwise_one_layer_o_num*depwise_cnt)); // p_mctrl->stripe_outaddr[1]); // new bin handle order in gentool
#else
			if (mode == NN_ELTWISE) {  // _TODO :  remove this test code later when v3 bin is ready
				p_cnn_ll_parm->output[0].bit.addr = (p_sao[1].va + p_mctrl->stripe_outaddr[1]);
				p_cnn_ll_parm->output[1].bit.addr = (p_sao[0].va + p_mctrl->stripe_outaddr[0]);
			} else {
				p_cnn_ll_parm->output[0].bit.addr = (p_sao[0].va + p_mctrl->stripe_outaddr[0]);
				p_cnn_ll_parm->output[1].bit.addr = (p_sao[1].va + p_mctrl->stripe_outaddr[1]);
			}
#endif
			#if 0
			p_cnn_ll_parm->weight.bit.addr
			p_cnn_ll_parm->kmean.bit.addr
			p_cnn_ll_parm->bias_bnscal.bit.addr
			#endif
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(CNN_LL_PARM);

			break;
		case AI_ENG_NUE:
			p_nue_ll_parm = (NUE_LL_PARM*)parm_start_addr;

			p_nue_ll_parm->input.bit.addr  = (p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0));  // p_mctrl->stripe_inaddr[0]);
			p_nue_ll_parm->elt_in.bit.addr = 0; // _TODO
			p_nue_ll_parm->roi_in.bit.addr = (p_sai[5].va + IN_BUF_OFFSET(p_mctrl, 1));
			p_nue_ll_parm->output.bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0)); // p_mctrl->stripe_outaddr[0]);
			#if 0
			p_nue_ll_parm->sv.bit.addr
			p_nue_ll_parm->alpha.bit.addr
			p_nue_ll_parm->rho.bit.addr
			p_nue_ll_parm->kmean.bit.addr
			#endif
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE_LL_PARM);
			break;
		case AI_ENG_NUE2:
			p_nue2_ll_parm = (NUE2_LL_PARM*)parm_start_addr;

			p_nue2_ll_parm->input[0].bit.addr  = (p_sai[0].va);   // _TODO : some tricky fix?
			p_nue2_ll_parm->input[1].bit.addr  = (p_sai[1].va);
			p_nue2_ll_parm->input[2].bit.addr  = (p_sai[2].va);
			p_nue2_ll_parm->output[0].bit.addr = (p_sao[0].va);
			p_nue2_ll_parm->output[1].bit.addr = (p_sao[1].va);
			p_nue2_ll_parm->output[2].bit.addr = (p_sao[2].va);
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE2_LL_PARM);
			break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}

		while (1) {
			UINT64* p_tmp_cmd = (UINT64*)parm_start_addr;
			UINT32 cmd_type = (UINT32)(p_tmp_cmd[0] >> 60);

			parm_start_addr = parm_start_addr + sizeof(UINT64);

			// if temp addr is over the end of the parm addr, then set end flg and break the loop
			if (parm_start_addr > ll_cmd_tail) {
				end_flg = 1;
				break;
			}
			// if command type is next ll, then parse the next ll addr
			if (cmd_type == 2) {
				break;
			}
		}
		// update depwise_cnt for next time fix address
		depwise_cnt++;
	}
}
#endif
VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_ll(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode)
{
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)(p_mctrl->addr + va_ofs);
	UINT32 ai_parm_addr = p_head->parm_addr + va_ofs;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
	NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
	NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		p_cnn_ll_parm = (CNN_LL_PARM*)ai_parm_addr;

		p_cnn_ll_parm->input.bit.addr     = (p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0));  // p_mctrl->stripe_inaddr[0]);

		if (IN_BUF_NUM_REAL(p_mctrl) > 1) {
			if (p_mctrl->mode == NN_BN) {
				p_cnn_ll_parm->bias_bnscal.bit.addr = (p_sai[5].va + IN_BUF_OFFSET(p_mctrl, 1));
			} else if (p_mctrl->mode == NN_MATMUL || p_mctrl->mode == NN_CORR) {
				p_cnn_ll_parm->weight.bit.addr = (p_sai[4].va + IN_BUF_OFFSET(p_mctrl, 1));  // Matmul SAI[4] is a weight from local ativation
			} else {
				p_cnn_ll_parm->interm_in.bit.addr = (p_sai[6].va + IN_BUF_OFFSET(p_mctrl, 1));  // p_mctrl->stripe_inaddr[1]);  // need to add stripe offset for sai[6] (c-gentool maybe ELTWISE + stripe)
			}
		}
#if 1
		p_cnn_ll_parm->output[0].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0)); //p_mctrl->stripe_outaddr[0]); // new bin handle order in gentool

		if (OUT_BUF_NUM_REAL(p_mctrl) > 1) {
			if (p_mctrl->mode != NN_BN) {
				p_cnn_ll_parm->output[1].bit.addr = (p_sao[1].va + OUT_BUF_OFFSET(p_mctrl, 1)); // p_mctrl->stripe_outaddr[1]); // new bin handle order in gentool
			}
		}
#else
		if (mode == NN_ELTWISE) {  // _TODO :  remove this test code later when v3 bin is ready
			p_cnn_ll_parm->output[0].bit.addr = (p_sao[1].va + p_mctrl->stripe_outaddr[1]);
			p_cnn_ll_parm->output[1].bit.addr = (p_sao[0].va + p_mctrl->stripe_outaddr[0]);
		} else {
			p_cnn_ll_parm->output[0].bit.addr = (p_sao[0].va + p_mctrl->stripe_outaddr[0]);
			p_cnn_ll_parm->output[1].bit.addr = (p_sao[1].va + p_mctrl->stripe_outaddr[1]);
		}
#endif
		#if 0
		p_cnn_ll_parm->weight.bit.addr
		p_cnn_ll_parm->kmean.bit.addr
		p_cnn_ll_parm->bias_bnscal.bit.addr
		#endif
		break;
	case AI_ENG_NUE:
		p_nue_ll_parm = (NUE_LL_PARM*)ai_parm_addr;

		p_nue_ll_parm->input.bit.addr  = (p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0));  // p_mctrl->stripe_inaddr[0]);
		p_nue_ll_parm->elt_in.bit.addr = 0; // _TODO
		if (IN_BUF_NUM_REAL(p_mctrl) > 1) {
			p_nue_ll_parm->roi_in.bit.addr = (p_sai[5].va + IN_BUF_OFFSET(p_mctrl, 1));
		}
		p_nue_ll_parm->output.bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0)); // p_mctrl->stripe_outaddr[0]);
		#if 0
		p_nue_ll_parm->sv.bit.addr
		p_nue_ll_parm->alpha.bit.addr
		p_nue_ll_parm->rho.bit.addr
		p_nue_ll_parm->kmean.bit.addr
		#endif
		break;
	case AI_ENG_NUE2:
		p_nue2_ll_parm = (NUE2_LL_PARM*)ai_parm_addr;
#if 0
		p_nue2_ll_parm->input[0].bit.addr  = (p_sai[0].va);   // _TODO : some tricky fix?
		p_nue2_ll_parm->input[1].bit.addr  = (p_sai[1].va);
		p_nue2_ll_parm->input[2].bit.addr  = (p_sai[2].va);
		p_nue2_ll_parm->output[0].bit.addr = (p_sao[0].va);
		p_nue2_ll_parm->output[1].bit.addr = (p_sao[1].va);
		p_nue2_ll_parm->output[2].bit.addr = (p_sao[2].va);
#else
		if ((p_nue2_ll_parm->output[0].bit.addr > 0) && (p_nue2_ll_parm->output[1].bit.addr == 0) && (p_nue2_ll_parm->output[2].bit.addr == 0)) {
			// Y
			p_nue2_ll_parm->output[0].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0));
			p_nue2_ll_parm->output[1].bit.addr = 0;
			p_nue2_ll_parm->output[2].bit.addr = 0;
		} else {
			// RGB/BGR
			UINT32 diff = 0;
			if (p_nue2_ll_parm->output[0].bit.addr > p_nue2_ll_parm->output[1].bit.addr) {
				diff = p_nue2_ll_parm->output[0].bit.addr - p_nue2_ll_parm->output[1].bit.addr;

				p_nue2_ll_parm->output[2].bit.addr = (p_sao[2].va + OUT_BUF_OFFSET(p_mctrl, 0));
				p_nue2_ll_parm->output[1].bit.addr = (p_sao[2].va + OUT_BUF_OFFSET(p_mctrl, 0) + diff);
				p_nue2_ll_parm->output[0].bit.addr = (p_sao[2].va + OUT_BUF_OFFSET(p_mctrl, 0) + diff + diff);
			} else {
				diff = p_nue2_ll_parm->output[1].bit.addr - p_nue2_ll_parm->output[0].bit.addr;

				p_nue2_ll_parm->output[0].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0));
				p_nue2_ll_parm->output[1].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0) + diff);
				p_nue2_ll_parm->output[2].bit.addr = (p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0) + diff + diff);
			}
			//p_nue2_ll_parm->input[0].bit.addr  = (p_sai[0].va);   // _TODO : some tricky fix?
			//p_nue2_ll_parm->input[1].bit.addr  = (p_sai[1].va);
			//p_nue2_ll_parm->input[2].bit.addr  = (p_sai[2].va);
		}
#endif
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
}

VOID _vendor_ai_net_mem_alloc_mem_aiparm_fix_comm(NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_io_mem, UINT32 va_ofs, NN_MODE mode, UINT32 proc_id)
{
	UINT32 ai_head_addr = (p_mctrl->addr + va_ofs);
	NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
	NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);

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
			p_permute_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_permute_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
		}	
		break;

	case NN_PROPOSAL:
		{
			NN_CPU_PARM *p_cpu_parm = (NN_CPU_PARM *)ai_head_addr;
			p_cpu_parm->addr_in  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_cpu_parm->addr_out = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
		}
		break;

	case NN_POSTPROC:
		{
			NN_POSTPROC_PARM *p_postproc_parm = (NN_POSTPROC_PARM *)ai_head_addr;
			p_postproc_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_postproc_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			#if 0
			p_postproc_parm->tmp_addr  // _TODO : ???
			#endif
		}
		break;
	case NN_SOFTMAX:
		{
			NN_SOFTMAX_PARM *p_softmax_parm = (NN_SOFTMAX_PARM *)ai_head_addr;
			p_softmax_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_softmax_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			#if AI_V4
			p_softmax_parm->tmp_addr       = p_sai[2].va;
			#else
			p_softmax_parm->in_trans_addr  = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 4); // p_mctrl->stripe_outaddr[4];
			p_softmax_parm->out_trans_addr = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 5); // p_mctrl->stripe_outaddr[5];
			#endif
			#endif
		}
		break;
	case NN_LAYER_NORMALIZATION:
		{
			NN_LAYER_NORMALIZATION_PARM *p_layer_norm_parm = (NN_LAYER_NORMALIZATION_PARM *)ai_head_addr;
			p_layer_norm_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_layer_norm_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			#if AI_V4
			p_layer_norm_parm->tmp_addr       = p_sai[2].va;
			#else
			p_layer_norm_parm->in_trans_addr  = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 4); // p_mctrl->stripe_outaddr[4];
			p_layer_norm_parm->out_trans_addr = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 5); // p_mctrl->stripe_outaddr[5];
			#endif
			#endif
		}
		break;
#if NN_DLI
	case NN_DLI_SQRT:
	case NN_DLI_EXP:
	case NN_DLI_LOG:
	case NN_DLI_SIN:
	case NN_DLI_FLOOR:
	case NN_DLI_ROUND:
		{
			NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm = (NN_DLI_ElEMENTWISE_PARM *)ai_head_addr;
			NN_DLI_TENSOR_INFO *p_input1_info = (NN_DLI_TENSOR_INFO *)(p_dli_elementwise_parm->input1_info_va);
			NN_DLI_TENSOR_INFO *p_output_info = (NN_DLI_TENSOR_INFO *)(p_dli_elementwise_parm->output_info_va);
			p_input1_info->data_va  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_output_info->data_va  = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			p_dli_elementwise_parm->temp_buf_va = p_sai[2].va;
			#endif
		}
		break;
	case NN_DLI_DIV:
	case NN_DLI_POW:
	case NN_DLI_EQUAL:
	case NN_DLI_GREATER:
	case NN_DLI_LESS:
		{
			NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm = (NN_DLI_ElEMENTWISE_PARM *)ai_head_addr;
			NN_DLI_TENSOR_INFO *p_input1_info = (NN_DLI_TENSOR_INFO *)(p_dli_elementwise_parm->input1_info_va);
			NN_DLI_TENSOR_INFO *p_input2_info = (NN_DLI_TENSOR_INFO *)(p_dli_elementwise_parm->input2_info_va);
			NN_DLI_TENSOR_INFO *p_output_info = (NN_DLI_TENSOR_INFO *)(p_dli_elementwise_parm->output_info_va);
			// input1 can be from iomem(buffer) or weight(model)
			if((p_input1_info->data_va & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
				p_input1_info->data_va  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			}
			// input2 can be from iomem(buffer) or weight(model)
			if((p_input2_info->data_va & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
				p_input2_info->data_va  = p_sai[1].va + IN_BUF_OFFSET(p_mctrl, 0);
			}
			p_output_info->data_va  = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			p_dli_elementwise_parm->temp_buf_va = p_sai[2].va;
			#endif
		}
		break;
	case NN_DLI_RESIZE:
		{
			NN_DLI_RESIZE_PARM *p_dli_resize_parm = (NN_DLI_RESIZE_PARM *)ai_head_addr;
			NN_DLI_TENSOR_INFO *p_input_info = (NN_DLI_TENSOR_INFO *)(p_dli_resize_parm->input_info_va);
			NN_DLI_TENSOR_INFO *p_output_info = (NN_DLI_TENSOR_INFO *)(p_dli_resize_parm->output_info_va);
			p_input_info->data_va  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_output_info->data_va  = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			p_dli_resize_parm->temp_buf_va = p_sai[2].va;
			#endif
		}
		break;
	case NN_DLI_SOFTMAX:
		{
			NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm = (NN_DLI_SOFTMAX_PARM *)ai_head_addr;
			NN_DLI_TENSOR_INFO *p_input_info = (NN_DLI_TENSOR_INFO *)(p_dli_softmax_parm->input_info_va);
			NN_DLI_TENSOR_INFO *p_output_info = (NN_DLI_TENSOR_INFO *)(p_dli_softmax_parm->output_info_va);
			p_input_info->data_va  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_output_info->data_va  = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			p_dli_softmax_parm->temp_buf_va = p_sai[2].va;
			#endif
		}
		break;
#endif
#if 0
	case NN_PREPROC:
		{
			NN_PRE_PARM *p_pre_parm = (NN_PRE_PARM*)ai_head_addr;
			p_pre_parm->in_addr
			p_pre_parm->out_addr
			p_pre_parm->interm_addr
			p_pre_parm->sub_img.plane_addr
		}
		break;
#endif	
	case NN_FC_POST:
		{
			NN_FC_POST_PARM *p_fc_post_parm = (NN_FC_POST_PARM*)ai_head_addr;
			#if AI_V4
			p_fc_post_parm->in_addr   = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_fc_post_parm->out_addr  = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#else
			p_fc_post_parm->addr_in   = p_sai[0].va;
			p_fc_post_parm->addr_out  = p_sao[0].va;
			#endif
			p_fc_post_parm->bias_addr = p_sai[1].va;  // unuse?
		}
		break;
		
	case NN_POOL:
		{
			NN_POOL_PARM *p_pool_parm = (NN_POOL_PARM*)ai_head_addr;
			p_pool_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_pool_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
		}
		break;
		
	case NN_CUSTOMER:
		{
			NN_CUSTOM_PARM *p_cust_head = (NN_CUSTOM_PARM*)ai_head_addr;
#if CUST_SUPPORT_MULTI_IO
			{
				UINT32 input_num     = p_cust_head->input_num;
				UINT32 output_num    = p_cust_head->output_num;
				UINT32 i = 0;
				NN_DATA* input_info  = (NN_DATA*)(ai_head_addr + sizeof(NN_CUSTOM_PARM));
				NN_DATA* output_info = (NN_DATA*)(ai_head_addr + sizeof(NN_CUSTOM_PARM) + input_num*sizeof(NN_DATA));
				for (i = 0; i < input_num; i++) {
					input_info[i].va  = p_sai[i].va + IN_BUF_OFFSET(p_mctrl, i);
				}
				for (i = 0; i < output_num; i++) {
					output_info[i].va = p_sao[i].va + OUT_BUF_OFFSET(p_mctrl, i);
				}
				// check tmp buffer (for example, maybe buf_id => 4 in/ 2 out , but sai/sao => 5 in / 1 out ) , and custom num => 4 in / 1 out
				if (p_mctrl->iomem.imem_cnt > input_num) {
					p_cust_head->temp_buf_addr = p_sai[input_num].va; // assign last sai to tmp_buffer_addr
				}
			}
#else
			p_cust_head->input.va  = p_sai[0].va;
			p_cust_head->output.va = p_sao[0].va;
			p_cust_head->model.va  = p_sai[1].va;
			#if 0
			p_cust_head->input.pa   = vendor_ais_user_buff_va2pa(p_cust_head->input.va, proc_id);
			p_cust_head->output.pa  = vendor_ais_user_buff_va2pa(p_cust_head->output.va, proc_id);
			if (p_cust_head->model.va > 0)
				p_cust_head->model.pa   = vendor_ais_user_model_va2pa(p_cust_head->model.va, proc_id);
			#endif
			#if CNN_CGEN_NEW_TMP_BUF
			//NOTE : how to use p_sao[2]? In fact, NN_CUSTOM_PARM is NOT changed. The source_pub cust_nn will directly access p_sao[2] to access the tmp buffer
			#endif
#endif
		}
		break;
		
	case NN_BNSCALE:
		{
			NN_BNSCALE_PARM *p_bn_scl_parm = (NN_BNSCALE_PARM*)ai_head_addr;
			p_bn_scl_parm->in_addr    = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_bn_scl_parm->out_addr   = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			p_bn_scl_parm->mean_addr  = p_sai[1].va; // unuse?
			p_bn_scl_parm->alpha_addr = p_sai[2].va; // unuse?
			p_bn_scl_parm->beta_addr  = p_sai[3].va; // unuse?
		}
		break;

#if (USE_NEON || (!CNN_25_MATLAB))
	case NN_PRELU:
		break;
	case NN_SIGMOID:
		break;
	case NN_PRIORBOX:
		{
			NN_PRIORBOX_PARM *p_priorbox_parm = (NN_PRIORBOX_PARM *)ai_head_addr;
			p_priorbox_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);  // p_mctrl->stripe_inaddr[0];
			p_priorbox_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0); // p_mctrl->stripe_outaddr[0];
			#if CNN_CGEN_NEW_TMP_BUF
			#if AI_V4
			p_priorbox_parm->tmp_addr       = p_sai[2].va;
			#else
			p_priorbox_parm->in_trans_addr  = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 4); // p_mctrl->stripe_outaddr[4];
			p_priorbox_parm->out_trans_addr = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 5); // p_mctrl->stripe_outaddr[5];
			#endif
			#endif
		}
		break;
	case NN_DETOUT:
		{
			NN_DETOUT_PARM *p_detout_parm = (NN_DETOUT_PARM *)ai_head_addr;
			p_detout_parm->in_addr[0]     = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_detout_parm->in_addr[1]     = p_sai[1].va + IN_BUF_OFFSET(p_mctrl, 1);
			p_detout_parm->in_addr[2]     = p_sai[3].va + IN_BUF_OFFSET(p_mctrl, 2);
			p_detout_parm->out_addr       = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			#if CNN_CGEN_NEW_TMP_BUF
			#if AI_V4
			p_detout_parm->tmp_addr       = p_sai[2].va;
			#else
			p_detout_parm->in_trans_addr  = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 4); // p_mctrl->stripe_outaddr[4];
			p_detout_parm->out_trans_addr = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 5); // p_mctrl->stripe_outaddr[5];
			p_detout_parm->tmp_addr       = p_sai[2].va + OUT_BUF_OFFSET(p_mctrl, 6); // p_mctrl->stripe_outaddr[6];
			#endif
			#endif
		}
		break;
    case NN_FP2FIX:
        break;
	case NN_LSTM:
        {
            NN_LSTM_PARM *p_lstm_parm = (NN_LSTM_PARM *)ai_head_addr;
			p_lstm_parm->in_addr0     = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			if (IN_BUF_NUM_REAL(p_mctrl) > 1)
				p_lstm_parm->in_addr1     = p_sai[1].va + IN_BUF_OFFSET(p_mctrl, 1);
			p_lstm_parm->out_addr     = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
            #if CNN_CGEN_NEW_TMP_BUF
		    p_lstm_parm->tmp_addr     = p_sai[2].va;
		    #endif
	    }
        break;
	case NN_REVERSE:
		{
			NN_REVERSE_PARM *p_reverse_parm = (NN_REVERSE_PARM *)ai_head_addr;
			p_reverse_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_reverse_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
		}
        break;
	case NN_NORM:
		{
			NN_NORM_PARM *p_norm_parm = (NN_NORM_PARM *)ai_head_addr;
			p_norm_parm->in_addr  = p_sai[0].va + IN_BUF_OFFSET(p_mctrl, 0);
			p_norm_parm->out_addr = p_sao[0].va + OUT_BUF_OFFSET(p_mctrl, 0);
			p_norm_parm->tmp_addr = p_sai[2].va;
		}
        break;
#endif

	default :
		break;
	}
}
#endif

HD_RESULT _vendor_ai_net_mem_alloc_mem_aiparm_fix(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	VENDOR_AIS_FLOW_MEM_PARM *p_mem = &p_mem_manager->user_parm;
	NN_GEN_NET_INFO net_info = {0};
	
	NN_GEN_MODEL_HEAD *p_head;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	UINT32 process_index = 0;
	UINT32 va_ofs = 0;
#if CNN_25_MATLAB
	UINT32 layer_index = 0;
#endif
	NN_MODE mode;
	
	HD_RESULT rv = HD_OK;

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
	proc_layer_num = p_head->mode_ctrl_num;

	// fix AIPARM
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		
		va_ofs = ALIGN_CEIL_4(p_mem->va);
		mode = (NN_MODE)p_mctrl[process_index].mode;
#if CNN_25_MATLAB
		layer_index = p_mctrl[process_index].layer_index;

		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			_vendor_ai_net_mem_alloc_mem_aiparm_fix_app(&p_mctrl[process_index], &p_io_mem[layer_index], va_ofs, mode);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			_vendor_ai_net_mem_alloc_mem_aiparm_fix_ll(&p_mctrl[process_index], &p_io_mem[layer_index], va_ofs, mode);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			_vendor_ai_net_mem_alloc_mem_aiparm_fix_comm(&p_mctrl[process_index], &p_io_mem[layer_index], va_ofs, mode, proc_id);
			break;
		}
#else
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			_vendor_ai_net_mem_alloc_mem_aiparm_fix_app(&p_mctrl[process_index], NULL, va_ofs, mode);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
#if AI_V4
			if (p_mctrl[process_index].mode == NN_DEPTHWISE || p_mctrl[process_index].mode == NN_CORR) {
				_vendor_ai_net_mem_alloc_mem_aiparm_fix_multi_ll(&p_mctrl[process_index], NULL, va_ofs, mode);
			} else
#endif
			{
				_vendor_ai_net_mem_alloc_mem_aiparm_fix_ll(&p_mctrl[process_index], NULL, va_ofs, mode);
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			_vendor_ai_net_mem_alloc_mem_aiparm_fix_comm(&p_mctrl[process_index], NULL, va_ofs, mode, proc_id);
			break;
		}
#endif
	}
	return HD_OK;
}
#endif // CNN_528_PSW , _TODO : remove later !!
//---------------------------------------------------------------------------------------------------

HD_RESULT _vendor_ai_net_mem_pars_addr(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
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

	for (idx=0; idx< proc_layer_num; idx++) {
		p_mctrl[idx].iomem.imem_addr  += p_mem->va;
		p_mctrl[idx].iomem.omem_addr  += p_mem->va;
		#if CNN_FMT_V4
		p_mctrl[idx].in_bufinfo_addr  += p_mem->va;
		p_mctrl[idx].out_bufinfo_addr += p_mem->va;
		#endif
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_mem_unpars_addr(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
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

	for (idx=0; idx< proc_layer_num; idx++) {
		p_mctrl[idx].iomem.imem_addr  -= p_mem->va;
		p_mctrl[idx].iomem.omem_addr  -= p_mem->va;
		#if CNN_FMT_V4
		p_mctrl[idx].in_bufinfo_addr  -= p_mem->va;
		p_mctrl[idx].out_bufinfo_addr -= p_mem->va;
		#endif
	}
	return HD_OK;
}

HD_RESULT _vendor_ai_net_mem_alloc_mem(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 buf_method, UINT32 job_method)
{	
	HD_RESULT rv = HD_OK;
#if CNN_528_PSW  // _TODO : remove later !!
	// iomem_addr, bufinfo_addr (offset -> real_addr)
	_vendor_ai_net_mem_pars_addr(p_mem_manager);

	// IOMEM
	switch(buf_method) {
		case VENDOR_AI_NET_BUF_OPT_FULL:
		case VENDOR_AI_NET_BUF_OPT_SHRINK:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O1:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O2:
		case VENDOR_AI_NET_BUF_OPT_SHRINK_O3:
			rv = _vendor_ai_net_mem_alloc_mem_iomem_fix(proc_id, p_mem_manager, buf_method, job_method);
			break;

		case VENDOR_AI_NET_BUF_OPT_TEST_ZERO:
		default:
			rv = _vendor_ai_net_mem_alloc_mem_iomem_fix_test_zero(proc_id, p_mem_manager);
			break;
	}

	// AIPARM
	if (HD_OK == rv) rv = _vendor_ai_net_mem_alloc_mem_aiparm_fix(proc_id, p_mem_manager);

	// iomem_addr, bufinfo_addr (real_addr -> offset) (because later gen_init() will do normal remap)
	_vendor_ai_net_mem_unpars_addr(p_mem_manager);
#endif
	return rv;
}


