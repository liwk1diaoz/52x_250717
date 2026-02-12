#if defined(__LINUX)
#include <linux/delay.h>
#endif
#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"
#include "vdodisp_api.h"
#include "kdrv_videoout/kdrv_vdoout.h"
#include "kdrv_videoout/kdrv_vdoout_lmt.h"
#include <gximage/gximage.h>
#include <kwrap/spinlock.h>
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_core.h"

#define VPRC_PULL_TIMEOUT 300//ms

struct SUB_WND_INFO {
	ISF_UNIT *p_thisunit;
	struct _ISF_UNIT *p_srcunit;
	int start;
} ;
struct SUB_WND_INFO test[ISF_VDOOUT_INPUT_MAX] = {0};

THREAD_HANDLE VDOWND_TSK_ID_0 = 0;
ID            VDOWND_FLG_ID_0 = 0;
SEM_HANDLE    VDOWND_SEM_ID_0 = {0};
static int    closing         = 0;
static VDOOUT_CONTEXT* p_ctx = NULL;

#define STKSIZE_VDOWND         2048
#define FLGVDOWND_STOPPED   FLGPTN_BIT(8)

static int get_block_size(void)
{
	if(!p_ctx){
		DBG_ERR("p_ctx is NULL\n");
		return 0;
	}

	if(p_ctx->info.bufFmt == VDO_PXLFMT_YUV420)
		return p_ctx->info.devSize.w * p_ctx->info.devSize.h * 3 / 2;
	else if(p_ctx->info.bufFmt == VDO_PXLFMT_YUV422)
		return p_ctx->info.devSize.w * p_ctx->info.devSize.h * 2;

	return 0;
}

static UINT32 argb_to_ycbcr(UINT32 argb)
{
#define cst_prom0   21
#define cst_prom1   79
#define cst_prom2   29
#define cst_prom3   43
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )

	UINT32 ycbcr = 0;

	ycbcr = RGB_GET_V((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_U((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_Y((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);

	return ycbcr;
}

static int clear_buffer(VDO_FRAME *in)
{
	int           ret, i;
	VDO_FRAME     dst_img;
	UINT32        va, pa;
	IRECT         dst_region;
	
	if(!in){
		DBG_ERR("in(%x) is NULL\n", (int)in);
		return -1;
	}

	memcpy(&dst_img, in, sizeof(VDO_FRAME));

	(&(((PVDO_COORD)(dst_img.reserved))->scale))->x = 0x00010000;
	(&(((PVDO_COORD)(dst_img.reserved))->scale))->y = 0x00010000;
	
	for(i = 0 ; i < 2 ; ++i){
		pa = in->phyaddr[i];
		if(!pa){
			DBG_ERR("in pa[%d] is NULL\n", i);
			return -1;
		}
		va = nvtmpp_sys_pa2va(pa);
		if(!va){
			DBG_ERR("nvtmpp_sys_pa2va() for dst pa[%d]=%x\n", i, (int)pa);
			return -1;
		}
		dst_img.phyaddr[i] = va;
		dst_img.addr[i] = va;
	}
	
	dst_region.x = 0;
	dst_region.y = 0;
	dst_region.w = in->size.w;
	dst_region.h = in->size.h;
	
	ret = gximg_fill_data_no_flush(&dst_img, &dst_region, argb_to_ycbcr(0), 0);
	if(ret)
		DBG_ERR("gximg_fill_data_no_flush() fail = %d\n", ret);
	
	return ret;
}

THREAD_DECLARE(vdownd_tsk_0, arglist)
{
	extern ISF_RV isf_vdoout_inputport_do_push(UINT32 id,ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms);
	ISF_UNIT *p_thisunit=&isf_vdoout0;
	UINT32 buf = 0, buf_size;
	static ISF_DATA_CLASS g_vdoout_data = {0};
	ISF_DATA data;
	int ret, i, should_sleep, pull_cnt, pull_ok_cnt, should_clear = 1;
	VDO_FRAME *frm_new, frm_old;
	BOOL allow_pull[ISF_VDOOUT_INPUT_MAX] = {0};
	THREAD_ENTRY();

	while(1){

		if(closing){
			DBG_ERR("vdownd_tsk_0 is closing\n");
			set_flg(VDOWND_FLG_ID_0, FLGVDOWND_STOPPED);
			break;
		}
		
		should_sleep = 1;
		pull_ok_cnt = 0;
		pull_cnt = 0;

		SEM_WAIT(VDOWND_SEM_ID_0);
		
		if(!(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN)){
			goto sleep;
		}

		//if no sub window is started, go to sleep
		for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
			if(p_thisunit->port_in[i] && 
				p_thisunit->port_in[i]->p_srcunit && 
				p_thisunit->port_in[i]->p_srcunit->port_outstate[0] && 
				(p_thisunit->port_in[i]->p_srcunit->port_outstate[0]->state == ISF_PORT_STATE_RUN))
				break;
		}
		if(i == ISF_VDOOUT_INPUT_MAX && !should_clear){
			goto sleep;
		}

		//allocate buffer for all windows
		buf_size = get_block_size();
		if(!buf_size){
			DBG_IND("fail to get block size\n");
			goto sleep;
		}
		memset(&data, 0, sizeof(data));
		p_thisunit->p_base->init_data(&data, &g_vdoout_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
		buf = p_thisunit->p_base->do_new(p_thisunit, ISF_IN(0), NVTMPP_VB_INVALID_POOL, 0, buf_size, &data, ISF_OUT_PROBE_EXT);
		if(!buf){
			DBG_ERR("fail to new buffer of %d bytes\n", buf_size);
			goto sleep;
		}

		frm_new = (VDO_FRAME*)&(data.desc);
		//DBG_ERR("buf(%x) size(%d) w(%d) h(%d) addr(%x,%x)\n", buf, buf_size, frm_new->size.w, frm_new->size.h, frm_new->addr[0], frm_new->addr[1]);
		frm_new->sign       = MAKEFOURCC('V','F','R','M');
    	frm_new->p_next = NULL;
		frm_new->pxlfmt     = p_ctx->info.bufFmt;
		frm_new->size.w     = p_ctx->info.devSize.w;
		frm_new->size.h     = p_ctx->info.devSize.h;
	    frm_new->count      = 1;
		frm_new->timestamp  = 0;
    	frm_new->pw[VDO_PINDEX_Y] = frm_new->size.w;
    	frm_new->pw[VDO_PINDEX_UV] = frm_new->size.w;
    	frm_new->ph[VDO_PINDEX_Y] = frm_new->size.h;
    	frm_new->ph[VDO_PINDEX_UV] = frm_new->size.h;
    	frm_new->loff[VDO_PINDEX_Y] = frm_new->size.w;
    	frm_new->loff[VDO_PINDEX_UV] = frm_new->size.w;
    	frm_new->phyaddr[VDO_PINDEX_Y] = nvtmpp_sys_va2pa(buf);
    	frm_new->phyaddr[VDO_PINDEX_UV] = nvtmpp_sys_va2pa(buf + frm_new->size.w*frm_new->size.h);
		frm_new->addr[VDO_PINDEX_Y]     = 0;  // kflow_videoenc will identify ((addr == 0) && (loff != 0)) to know that it's coming from HDAL user push_in
		frm_new->addr[VDO_PINDEX_UV]    = 0;  //   and kflow_videoenc will call nvtmpp_sys_pa2va() in kernel to convert phyaddr to viraddr
		memcpy(&frm_old, frm_new, sizeof(frm_old));
		
		if(clear_buffer(frm_new)){
			DBG_ERR("fail to clear buffer\n");
		}

		//push block for all windows
		for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
			if (closing)
				break;
			if(!p_thisunit->port_in[i] || 
				!p_thisunit->port_in[i]->p_srcunit ||
				!p_thisunit->port_in[i]->p_srcunit->port_outstate || 
				!(p_thisunit->port_in[i]->p_srcunit->port_outstate[0]->state == ISF_PORT_STATE_RUN)) {
				allow_pull[i] = FALSE;
				continue;
			} else {
				allow_pull[i] = TRUE;
			}
			ret = p_thisunit->port_in[i]->p_srcunit->do_setportparam(p_thisunit->port_in[i]->p_srcunit, ISF_OUT(0), 0x80001010+0x0101, data.h_data);//VDOPRC_PARAM_COMBINE_BLK
			if(ret) {
				//ISF_PORT *p_src = test[i].p_thisunit->port_in[i];
				//DBG_ERR("Set %s.out[%d] blk failed\n", test[i].p_srcunit->unit_name, p_src->oport);
			}
		}

		//pull block for all windows
		for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
			ISF_DATA data2 = {0};
			if (closing)
				break;
			if(!p_thisunit->port_in[i] ||
				!p_thisunit->port_in[i]->p_srcunit ||
				!p_thisunit->port_in[i]->p_srcunit->port_outstate[0] || 
				!(p_thisunit->port_in[i]->p_srcunit->port_outstate[0]->state == ISF_PORT_STATE_RUN) ||
				FALSE == allow_pull[i])
				continue;
			ret = p_thisunit->port_in[i]->do_pull(p_thisunit->port_in[i]->oport, &data2, NULL, VPRC_PULL_TIMEOUT);
			pull_cnt++;
			should_clear = 1;
			if(ret) {
				//DBG_ERR("%s do_pull failed\n", p_thisunit->port_in[i]->p_srcunit->unit_name);
			} else {
				pull_ok_cnt++;
				if (data.h_data != data2.h_data) {
					//DBG_ERR("hdata(0x%X) != %s h_data(0x%X)\n",hdata, p_thisunit->port_in[i]->p_srcunit->unit_name, data2.h_data);
					if(p_thisunit->port_in[i]->do_pull(p_thisunit->port_in[i]->oport, &data2, NULL, VPRC_PULL_TIMEOUT))
						pull_ok_cnt--;
				}
			}
		}
		//vproc push/pull overwrides frm_new, restore frm_new
		memcpy(frm_new, &frm_old, sizeof(frm_old));

		if(!p_thisunit->port_in[0]){
			DBG_ERR("isf_vdoout0.port_in[0] is NULL\n");
			p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), &data, ISF_IN_PROBE_PUSH);
			goto sleep;
		}
		//ever pull, but no pull is successful
		if(pull_cnt &&  !pull_ok_cnt && !should_clear){
			p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), &data, ISF_IN_PROBE_PUSH);
			goto sleep;
		}
		
		//1. at least one pull is successful, update LCD
		//2. no pull is tried, clear LCD
		ret = isf_vdoout_inputport_do_push(0, p_thisunit->port_in[0], &data, 0);
		if(ret)
			DBG_ERR("isf_vdoout_inputport_do_push() fail = %d\n", ret);
		else
			should_sleep = 0;

		if(!pull_ok_cnt){			
			should_clear = 0;
		}

sleep:
		SEM_SIGNAL(VDOWND_SEM_ID_0);
		if(should_sleep)
			vos_task_delay_ms(10);//msleep(10);
	}
	THREAD_RETURN(0);
}

int create_merge_window_thread(VDOOUT_CONTEXT* ctx)
{
	memset(test, 0, sizeof(test));
	closing = 0;
	p_ctx = ctx;

	THREAD_CREATE(VDOWND_TSK_ID_0, vdownd_tsk_0, NULL, "vdownd_tsk_0");
	if(VDOWND_TSK_ID_0 == 0){
		DBG_ERR("fail to create window merger\n");
		return -1;
	}

	SEM_CREATE(VDOWND_SEM_ID_0,1);
	OS_CONFIG_FLAG(VDOWND_FLG_ID_0);
	THREAD_RESUME(VDOWND_TSK_ID_0);

	return 0;
}

int destroy_merge_window_thread(void)
{
	FLGPTN flg_ptn;

	p_ctx = NULL;
	closing = 1;
	if (wai_flg(&flg_ptn, VDOWND_FLG_ID_0, FLGVDOWND_STOPPED, TWF_CLR) != E_OK) {
		DBG_ERR("fail to wait FLGVDOWND_STOPPED\n");
	}

	SEM_DESTROY(VDOWND_SEM_ID_0);
	rel_flg(VDOWND_FLG_ID_0);
	memset(test, 0, sizeof(test));

	return 0;
}

int set_src_unit(struct _ISF_UNIT *thisunit, struct _ISF_UNIT *srcunit)
{
#if 0
	int i;

	SEM_WAIT(VDOWND_SEM_ID_0);
	
	for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
		if(test[i].p_srcunit == srcunit){
			test[i].p_thisunit = thisunit;
			break;
		}
	}
	
	if(i == ISF_VDOOUT_INPUT_MAX){
		for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
			if(test[i].p_srcunit == NULL){
				test[i].p_thisunit = thisunit;
				test[i].p_srcunit = srcunit;
				break;
			}
		}
	}

	SEM_SIGNAL(VDOWND_SEM_ID_0);
	
	if(i == ISF_VDOOUT_INPUT_MAX)
		DBG_ERR("fail to allocate sub wnd info\n");
	
	return (i == ISF_VDOOUT_INPUT_MAX);
#else
	return 0;
#endif
}

int start_sub_wnd(ISF_UNIT *p_thisunit)
{
#if 0
	int i, found = 0;
	SEM_WAIT(VDOWND_SEM_ID_0);
	
	for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
		if(test[i].p_thisunit == p_thisunit){
			test[i].start = 1;
			found = 1;
		}
	}
	
	if(!found){
		for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
			if(test[i].p_thisunit == NULL){
				test[i].p_thisunit = p_thisunit;
				test[i].start = 1;
			}
		}
	}
	
	SEM_SIGNAL(VDOWND_SEM_ID_0);
	
	if(!found && i == ISF_VDOOUT_INPUT_MAX)
		DBG_ERR("fail to allocate sub wnd info\n");
	
	return (!found && i == ISF_VDOOUT_INPUT_MAX);
#else
	return 0;
#endif
}

int stop_sub_wnd(ISF_UNIT *p_thisunit)
{
#if 0
	int i, found = 0;
	

	SEM_WAIT(VDOWND_SEM_ID_0);

	for(i = 0 ; i < ISF_VDOOUT_INPUT_MAX ; ++i){
		if(test[i].p_thisunit == p_thisunit){
			memset(&(test[i]), 0, sizeof(struct SUB_WND_INFO));
			found = 1;
		}
	}

	SEM_SIGNAL(VDOWND_SEM_ID_0);

	if(found == 0)
		DBG_ERR("fail to stop sub wnd info\n");

	return (found == 0);
#else
	return 0;
#endif
}

#if 0
static int copy_image(VDO_FRAME *in, VDO_FRAME *out)
{
	int           ret = E_PAR, i;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa;
	IRECT         src_region;
	IPOINT        dst_location;
	
	if(!in || !out){
		DBG_ERR("in(%p) or out(%p) is NULL\n", in, out);
		return -1;
	}
	
	memcpy(&src_img, in, sizeof(VDO_FRAME));
	memcpy(&dst_img, out, sizeof(VDO_FRAME));
	
	(&(((PVDO_COORD)(src_img.reserved))->scale))->x = 0x00010000;
	(&(((PVDO_COORD)(src_img.reserved))->scale))->y = 0x00010000;
	(&(((PVDO_COORD)(dst_img.reserved))->scale))->x = 0x00010000;
	(&(((PVDO_COORD)(dst_img.reserved))->scale))->y = 0x00010000;
	
	for(i = 0 ; i < 2 ; ++i){
		pa = in->phyaddr[i];
		if(!pa){
			DBG_ERR("in pa[%d] is NULL\n", i);
			return -1;
		}
		va = nvtmpp_sys_pa2va(pa);
		if(!va){
			DBG_ERR("nvtmpp_sys_pa2va() for src pa[%d]=%x\n", i, (int)pa);
			return -1;
		}
		src_img.phyaddr[i] = va;
		src_img.addr[i] = va;

		pa = out->phyaddr[i];
		if(!pa){
			DBG_ERR("out pa[%d] is NULL\n", i);
			return -1;
		}
		va = nvtmpp_sys_pa2va(pa);
		if(!va){
			DBG_ERR("nvtmpp_sys_pa2va() for dst pa[%d]=%x\n", i, (int)pa);
			return -1;
		}
		dst_img.phyaddr[i] = va;
		dst_img.addr[i] = va;
	}
	
	src_region.x     = 0;
	src_region.y     = 0;
	src_region.w     = in->size.w;
	src_region.h     = in->size.h;
	dst_location.x   = 0;
	dst_location.y   = 0;
	
	ret = gximg_copy_data(&src_img, &src_region, &dst_img, &dst_location);
	if(ret)
		DBG_ERR("gximg_copy_data() fail = %d\n", ret);
	
	return ret;
}
#endif

ISF_RV isf_vdoout_inputport_do_push_win(UINT32 id,ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms)
{
#if 0
    ISF_UNIT *p_thisunit=&isf_vdoout0;
	VDO_FRAME *frm_old = NULL, *frm_new = NULL;
	UINT32 buf = 0;
	static ISF_DATA_CLASS g_vdoout_data = {0};
	int ret;

	SEM_WAIT(VDOWND_SEM_ID_0);
	if(test.valid == 0){
		SEM_SIGNAL(VDOWND_SEM_ID_0);
		frm_old = (VDO_FRAME*)p_data->desc;
		memset(&test.data, 0, sizeof(test.data));
		p_thisunit->p_base->init_data(&test.data, &g_vdoout_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
		buf = p_thisunit->p_base->do_new(p_thisunit, ISF_IN(0), NVTMPP_VB_INVALID_POOL, 0, frm_old->size.w*frm_old->size.h*2, &test.data, ISF_OUT_PROBE_EXT);
		frm_new = (VDO_FRAME*)&(test.data.desc);
		frm_new->sign       = MAKEFOURCC('V','F','R','M');
    	frm_new->p_next = NULL;
		frm_new->pxlfmt     = frm_old->pxlfmt;
		frm_new->size.w     = frm_old->size.w;
		frm_new->size.h     = frm_old->size.h;
	    frm_new->count      = 1;
		frm_new->timestamp  = 0;
    	frm_new->pw[VDO_PINDEX_Y] = frm_new->size.w;
    	frm_new->pw[VDO_PINDEX_UV] = frm_new->size.w;
    	frm_new->ph[VDO_PINDEX_Y] = frm_new->size.h;
    	frm_new->ph[VDO_PINDEX_UV] = frm_new->size.h;
    	frm_new->loff[VDO_PINDEX_Y] = frm_new->size.w;
    	frm_new->loff[VDO_PINDEX_UV] = frm_new->size.w;
    	frm_new->phyaddr[VDO_PINDEX_Y] = nvtmpp_sys_va2pa(buf);
    	frm_new->phyaddr[VDO_PINDEX_UV] = nvtmpp_sys_va2pa(buf + frm_new->size.w*frm_new->size.h);
		frm_new->addr[VDO_PINDEX_Y]     = 0;  // kflow_videoenc will identify ((addr == 0) && (loff != 0)) to know that it's coming from HDAL user push_in
		frm_new->addr[VDO_PINDEX_UV]    = 0;  //   and kflow_videoenc will call nvtmpp_sys_pa2va() in kernel to convert phyaddr to viraddr

		ret = copy_image(frm_old, frm_new);		
		if(ret){
			DBG_ERR("fail to copy src image to dst image\n");
			p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), &test.data, ISF_IN_PROBE_REL);
			p_thisunit->p_base->do_release(p_thisunit, p_port->iport, p_data, ISF_IN_PROBE_REL);
			return -1;
		}
		p_thisunit->p_base->do_release(p_thisunit, p_port->iport, p_data, ISF_IN_PROBE_REL);
	
		SEM_WAIT(VDOWND_SEM_ID_0);
		test.id = id;
		test.p_thisunit = p_thisunit;
		test.p_port = p_port;
		test.port = ISF_IN(0);
		test.valid = 1;
		SEM_SIGNAL(VDOWND_SEM_ID_0);
	}else{
		SEM_SIGNAL(VDOWND_SEM_ID_0);
		//p_thisunit->p_base->do_probe(p_thisunit, p_port->iport, ISF_IN_PROBE_PUSH, ISF_ENTER);
		//DBG_ERR("iport(%x) sign(%x,%x) size(%d)\r\n",p_port->iport, p_data->sign, ISF_SIGN_DATA, p_data->size);
		//frm = (VDO_FRAME*)p_data->desc;
		//DBG_ERR("fmt(%x) w(%d) h(%d) loff(%d,%d) pw(%d,%d) ph(%d,%d)\n", frm->pxlfmt, frm->size.w, frm->size.h, frm->loff[0], frm->loff[1], (int)frm->pw[0], (int)frm->pw[1], (int)frm->ph[0], (int)frm->ph[1]);
		//p_thisunit->p_base->do_probe(p_thisunit, p_port->iport, ISF_IN_PROBE_PROC, ISF_OK);
		p_thisunit->p_base->do_release(p_thisunit, p_port->iport, p_data, ISF_IN_PROBE_REL);
	}
#endif

    return ISF_OK;
}


