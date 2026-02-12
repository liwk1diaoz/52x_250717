/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"
#define _TOFIX_ 0

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _BSMUX_QUEUE_INFO {
    UINT32 queue_head;
    UINT32 queue_tail;
	UINT32 queue_max;
	UINT32 queue_full;
	UINT32 queue_active;
	UINT32 start_addr;
	UINT32 end_addr;
	UINT32 gop_cnt;
	UINT32 gop_sec;
	UINT32 gop_max;
#if BSMUX_TEST_USE_HEAP
	UINT32 *gop_num;
#else
	UINT32 gop_num[BSMUXER_BSQ_ROLLBACK_SECNEW];
#endif
} BSMUX_QUEUE_INFO;

#define QUE_NUM(p) ((p->queue_head >= p->queue_tail ? 0 : p->queue_max) + p->queue_head - p->queue_tail)

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
//>> debug
static UINT32        g_debug_mem_buf = BSMUX_DEBUG_MEMBUF;
static UINT32        g_debug_bsq     = BSMUX_DEBUG_BSQ;
static UINT32        g_debug_bitrate = BSMUX_DEBUG_BITRATE;
#define BSMUX_MEM_DUMP(fmtstr, args...) do { if(g_debug_mem_buf) DBG_DUMP(fmtstr, ##args); } while(0)
#define BSMUX_BSQ_DUMP(fmtstr, args...) do { if(g_debug_bsq) DBG_DUMP(fmtstr, ##args); } while(0)
#define BSMUX_MEM_IND(fmtstr, args...) do { if(g_debug_mem_buf) DBG_IND(fmtstr, ##args); } while(0)
#define BSMUX_BSQ_IND(fmtstr, args...) do { if(g_debug_bsq) DBG_IND(fmtstr, ##args); } while(0)
extern BOOL          g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

//>> memory control
static UINT32        g_bsmux_mem_phy_addr[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32        g_bsmux_mem_vrt_addr[BSMUX_MAX_CTRL_NUM] = {0};
static BSMUX_MEM_BUF g_bsmux_mem_buf[BSMUX_MAX_CTRL_NUM] = {0};

//>> bs saveq control
static BSMUX_SAVEQ_FILE_INFO g_bsmux_saveq_file_info[BSMUX_MAX_CTRL_NUM] = {0};
static BSMUX_QUEUE_INFO      g_bsmux_queue_info[BSMUX_MAX_CTRL_NUM] = {0};
#if 1
static BSMUX_SAVEQ_BS_INFO   g_bsmux_temp_bs_info[BSMUX_MAX_CTRL_NUM] = {0};
#endif

//>> others
static INT32         g_bsmux_vid_keyframe_count[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32        g_bsmux_bs_tbr_count[BSMUX_MAX_CTRL_NUM] = {0};
static INT32         g_bsmux_sub_keyframe_count[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32        g_bsmux_sub_bs_tbr_count[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32        g_bsmux_open_count = 0;
static UINT32        g_bsmux_start_count = 0;
static UINT32        g_bsmux_msg_count = 0;

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
//>> memory control
static BSMUX_MEM_BUF *bsmux_get_mem_buf(UINT32 id)
{
	return &g_bsmux_mem_buf[id];
}
//>> bs saveq control
static BSMUX_QUEUE_INFO *bsmux_get_queue_info(UINT32 id)
{
	return &g_bsmux_queue_info[id];
}
//>> bs saveq control
static BSMUX_SAVEQ_FILE_INFO *bsmux_get_file_info(UINT32 id)
{
	return &g_bsmux_saveq_file_info[id];
}
//>> bs saveq control
static VOID bsmux_dump_queue_info(UINT32 id, UINT32 cnt)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT64 t_stamp;

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get queue null\r\n", id);
		return;
	}

	bsmux_saveq_lock();
	{
		//copy data to new buf
		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + cnt;
		t_stamp = bsmux_util_calc_timestamp(id, p_new_buf->uiTimeStamp);
		DBG_DUMP("Q[%d][%d]-[%d][%d] pa=0x%x va=0x%x size=0x%X\r\n", id, cnt,
			p_new_buf->uiType, p_new_buf->uiIsKey, p_new_buf->uiBSMemAddr, p_new_buf->uiBSVirAddr, p_new_buf->uiBSSize);
		DBG_DUMP("Q[%d][%d]-[0x%x][%d] timestamp=%llu dur=%llu\r\n", id, cnt,
			p_new_buf->uibufid, p_new_buf->uiCountSametype, p_new_buf->uiTimeStamp, t_stamp);
	}
	bsmux_saveq_unlock();
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//>> memory control
ER bsmux_ctrl_mem_init(UINT32 id)
{
	BSMUX_MEM_BUF *pMembuf = NULL;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	pMembuf = bsmux_get_mem_buf(id);
	if (bsmux_util_is_null_obj((void *)pMembuf)) {
		DBG_ERR("[%d] get mem buf null\r\n", id);
		return E_NOEXS;
	} else {
		pMembuf->addr         = 0;
		pMembuf->size         = 0;
		pMembuf->end          = 0;
		pMembuf->max          = 0;
		pMembuf->nowaddr      = 0;
		pMembuf->last2card    = 0;
		pMembuf->real2card    = 0;
		pMembuf->end2card     = 0;
		pMembuf->total2card   = 0;
		pMembuf->allowmaxsize = 0;
		pMembuf->blksize      = 0;
		pMembuf->blkoffset    = 0;
		pMembuf->blkusednum   = 0;
		pMembuf->free_size    = 0;
		pMembuf->used_size    = 0;
	}
	return E_OK;
}

//>> memory control
ER bsmux_ctrl_mem_alloc(UINT32 id)
{
	BSMUX_REC_MEMINFO Mem = {0};
	UINT32            uiBufSize = 0;
	//HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32            ddr_id = DDR_ID0;
	UINT32            phy_addr;
	void              *vrt_addr;
	UINT32            venc_pa = 0, venc_size = 0, venc_va = 0;
	UINT32            aenc_pa = 0, aenc_size = 0, aenc_va = 0;
	ER                ret;
	HD_RESULT         rv;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	ret = bsmux_ctrl_mem_init(id);
	if (ret != E_OK) {
		DBG_ERR("[%d] mem init fail\r\n", id);
		return E_SYS;
	}

	ret = bsmux_ctrl_mem_cal_range(id, &uiBufSize);
	if (ret != E_OK) {
		DBG_ERR("[%d] get need memsize fail\r\n", id);
		return E_SYS;
	}
	if (uiBufSize == 0) {
		DBG_ERR("[%d] get need memsize zero\r\n", id);
		return E_SYS;
	}

	// create mempool
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_DDR_ID, (UINT32 *)&ddr_id);
	rv = hd_common_mem_alloc("BsMux", &phy_addr, (void **)&vrt_addr, uiBufSize, ddr_id);
	if (rv != HD_OK) {
		DBG_ERR("BsMux alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(uiBufSize), (int)ddr_id);
		return E_SYS;
	}
	BSMUX_MEM_DUMP("BSM:[%d] BSMX pa = 0x%X mmap va = 0x%X size = 0x%X\r\n",
		id, (unsigned int)(phy_addr), (unsigned int)(vrt_addr), (unsigned int)(uiBufSize));

	// step: set mem buf
	Mem.addr = (UINT32)vrt_addr;
	Mem.size = uiBufSize;

	g_bsmux_mem_phy_addr[id] = (UINT32)phy_addr;
	g_bsmux_mem_vrt_addr[id] = (UINT32)vrt_addr;

	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_ADDR, phy_addr);
	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_VIRT, (UINT32)vrt_addr);
	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_SIZE, uiBufSize);

	// step: set videnc buf
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_ADDR, &venc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_SIZE, &venc_size);
	if (venc_size) {
		venc_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, venc_pa, venc_size);
		if (venc_va) {
			bsmux_util_set_param(id, BSMUXER_PARAM_BUF_VIDENC_VIRT, venc_va);
		} else {
			DBG_WRN("BSM:[%d] VENC pa 0x%X mmap failed size 0x%X\r\n",
				id, venc_pa, venc_size);
		}
	}
	BSMUX_MEM_DUMP("BSM:[%d] VENC pa = 0x%X mmap va = 0x%X size = 0x%X\r\n",
		id, (unsigned int)(venc_pa), (unsigned int)(venc_va), (unsigned int)(venc_size));

	// step: set audenc buf
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_ADDR, &aenc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_SIZE, &aenc_size);
	if (aenc_size) {
		aenc_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aenc_pa, aenc_size);
		if (aenc_va) {
			bsmux_util_set_param(id, BSMUXER_PARAM_BUF_AUDENC_VIRT, aenc_va);
		} else {
			DBG_WRN("BSM:[%d] AENC pa 0x%X mmap failed size 0x%X\r\n",
				id, aenc_pa, aenc_size);
		}
	}
	BSMUX_MEM_DUMP("BSM:[%d] AENC pa = 0x%X mmap va = 0x%X size = 0x%X\r\n",
		id, (unsigned int)(aenc_pa), (unsigned int)(aenc_va), (unsigned int)(aenc_size));

	// step: set sub videnc buf
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_ADDR, &venc_pa);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, &venc_size);
	if (venc_size) {
		venc_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, venc_pa, venc_size);
		if (venc_va) {
			bsmux_util_set_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT, venc_va);
		} else {
			DBG_WRN("BSM:[%d] SUB VENC pa 0x%X mmap failed size 0x%X\r\n",
				id, venc_pa, venc_size);
		}
	}
	BSMUX_MEM_DUMP("BSM:[%d] SUB VENC pa = 0x%X mmap va = 0x%X size = 0x%X\r\n",
		id, (unsigned int)(venc_pa), (unsigned int)(venc_va), (unsigned int)(venc_size));

	ret = bsmux_ctrl_mem_set_range(id, (UINT32) &Mem);
	if (ret != E_OK) {
		DBG_ERR("[%d] set mem range fail\r\n", id);
		return E_SYS;
	}

	return E_OK;
}

//>> memory control
ER bsmux_ctrl_mem_free(UINT32 id)
{
	UINT32    venc_va = 0, venc_size = 0;
	UINT32    aenc_va = 0, aenc_size = 0;
	HD_RESULT rv;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	rv = hd_common_mem_free((UINT32)g_bsmux_mem_phy_addr[id], (void *)g_bsmux_mem_vrt_addr[id]);
	if (rv != HD_OK) {
		DBG_ERR("BsMux release blk failed! (%d)\r\n", rv);
		return E_SYS;
	}

	g_bsmux_mem_phy_addr[id] = 0;
	g_bsmux_mem_vrt_addr[id] = 0;

	// step: set mem buf
	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_ADDR, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_VIRT, 0);
	bsmux_util_set_param(id, BSMUXER_PARAM_MEM_SIZE, 0);

#if BSMUX_TEST_USE_HEAP
	bsmux_ctrl_bs_initqueue(id, 0, 0); //reset flow
#endif

	// step: set videnc buf
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_VIRT, &venc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_VIDENC_SIZE, &venc_size);
	if ((venc_size) && (venc_va)) {
		hd_common_mem_munmap((void *)venc_va, venc_size);
	}

	// step: set audenc buf
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_VIRT, &aenc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_AUDENC_SIZE, &aenc_size);
	if ((aenc_size) && (aenc_va)) {
		hd_common_mem_munmap((void *)aenc_va, aenc_size);
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_VIRT, &venc_va);
	bsmux_util_get_param(id, BSMUXER_PARAM_BUF_SUB_VIDENC_SIZE, &venc_size);
	if ((venc_size) && (venc_va)) {
		hd_common_mem_munmap((void *)venc_va, venc_size);
	}

	BSMUX_MEM_DUMP("BSM:[%d] free mem done\r\n", id);

	return E_OK;
}

//>> memory control
ER bsmux_ctrl_mem_cal_range(UINT32 id, UINT32 *p_size)
{
	UINT32 mins = 0, need_size = 0;
	ER ret;

	// ==============================
	// =     Plugin Buf             =
	// ==============================
	ret = bsmux_util_plugin_get_size(id, &need_size);
	if (ret != E_OK) {
		DBG_ERR("[%d] get plugin need size fail\r\n", id);
		return E_SYS;
	}
	mins += need_size;
	BSMUX_MEM_IND("BSM:[%d] plugin buf 0x%X\r\n", id, need_size);

	// ==============================
	// =     BSQ Buf                =
	// ==============================
	bsmux_ctrl_bs_calcqueue(id, &need_size);
	mins += need_size;
	BSMUX_MEM_IND("BSM:[%d] bsq buf 0x%X\r\n", id, need_size);

	// ==============================
	// =     Data  Buffer           =
	// ==============================
	bsmux_ctrl_mem_cal_buffer(id, &need_size);
	mins += need_size;
	BSMUX_MEM_IND("BSM:[%d] copy buf 0x%X\r\n", id, need_size);

	mins = ALIGN_CEIL_64(mins);
	BSMUX_MEM_IND("Getmin%d = %ld KB\r\n", id,  mins / 1024);
	*p_size = mins;

	return E_OK;
}

//>> memory control
ER bsmux_ctrl_mem_set_range(UINT32 id, UINT32 value)
{
	BSMUX_REC_MEMINFO *pMem = (BSMUX_REC_MEMINFO *)value;
	BSMUX_MEM_BUF mem_buf = {0};
	UINT32 addr = pMem->addr;
	UINT32 size = pMem->size;
	UINT32 mins = 0;
	UINT32 alloc_addr = 0, need_size = 0;
	ER ret;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range\r\n", id);
		return E_ID;
	}

	if (addr == 0 || size == 0) {
		DBG_ERR("[%d] invalid mem range\r\n", id);
		return E_PAR;
	}

	// check size enough
	bsmux_ctrl_mem_cal_range(id, &mins);
	if (size < mins) {
		DBG_ERR("MINS=0x%lx, size=0x%lx TOO SMALL\r\n", mins, size);
		return E_SYS;
	}

	// ==============================
	// =     Plugin Buf             =
	// ==============================
	bsmux_util_plugin_set_size(id, addr, &need_size);
	alloc_addr = addr;
	addr += need_size;
	size -= need_size;
	// debug membuf
	BSMUX_MEM_DUMP("BSM:[%d] PluginBuf addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);

	// ==============================
	// =     Data  Buffer (copy)    =
	// ==============================
	bsmux_ctrl_mem_cal_buffer(id, &need_size);
	if (size < need_size) {
		DBG_ERR("[%d] set CopyBuf range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	mem_buf.addr = alloc_addr;
	mem_buf.size = need_size;
	ret = bsmux_ctrl_mem_set_buffer(id, &mem_buf);
	if (ret != E_OK) {
		DBG_ERR("bsmux[%d] set mem buffer fail\r\n", id);
		return E_SYS;
	}
	addr += need_size;
	size -= need_size;
	// debug membuf
	BSMUX_MEM_DUMP("BSM:[%d] CopyBuf addr 0x%X size 0x%X\r\n", id, mem_buf.addr, mem_buf.size);

	// ==============================
	// =     BSQ Buf                =
	// ==============================
	bsmux_ctrl_bs_calcqueue(id, &need_size);
	if (size < need_size) {
		DBG_ERR("[%d] set BSQBuf range error (%d < %d)\r\n", id, size, need_size);
		return E_NOMEM;
	}
	alloc_addr = addr;
	addr += need_size;
	size -= need_size;
	BSMUX_MEM_DUMP("BSM:[%d] BSQBuf addr 0x%X size 0x%X\r\n", id, alloc_addr, need_size);
	//init q
	ret = bsmux_ctrl_bs_initqueue(id, alloc_addr, need_size);
	if (ret != E_OK) {
		DBG_ERR("bsmux[%d] initqueue fail\r\n", id);
		return E_SYS;
	}
	//init file
	ret = bsmux_ctrl_bs_init_fileinfo(id);
	if (ret != E_OK) {
		DBG_ERR("bsmux[%d] init fileinfo fail\r\n", id);
		return E_SYS;
	}

	return E_OK;
}

//>> memory control
ER bsmux_ctrl_mem_det_range(UINT32 id)
{
	UINT32 bufaddr = 0, bufend = 0, bufsize = 0, useable = 0;
	UINT32 nowaddr = 0, last2card = 0, real2card = 0, writeoffset = 0;
	UINT64 total = 0, free = 0, used = 0, wait_for_write = 0, wait_for_push = 0;
	UINT64 bufuse_pct = 0, buffree_pct = 0;
	UINT64 alarm_pct = 50;

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &bufaddr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &bufend);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFSIZE, &bufsize);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_USEABLE, &useable);

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &nowaddr);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_LAST2CARD, &last2card);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &real2card);
	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEOFFSET, &writeoffset);

	if ((real2card < bufaddr) || (real2card > (bufend))) {
		real2card = writeoffset;
	}

#if BSMUX_TEST_RESV_BUFF
	DBG_IND("[%d]total: range(0x%x-0x%x) (size:0x%x)\r\n", id, bufaddr, bufend, bufsize);
	DBG_IND("[%d]useable: range(0x%x-0x%x) (size:0x%x)\r\n", id, bufaddr, (bufaddr + useable), useable);
	if (last2card >= real2card) {
		wait_for_write = (UINT64)(last2card - real2card);
		DBG_IND("[%d]wait_for_write: range(0x%x-0x%x) (size:0x%x)\r\n",
			id, real2card, last2card, wait_for_write);
	} else {
		wait_for_write = (UINT64)(useable - (real2card - last2card));
		DBG_IND("[%d]wait_for_write: range(0x%x-0x%x),(0x%x-0x%x) (size:0x%x)\r\n",
			id, bufaddr, last2card, real2card, (bufaddr + useable), wait_for_write);
	}
	if (nowaddr >= last2card) {
		wait_for_push = (UINT64)(nowaddr - last2card);
		DBG_IND("[%d]wait_for_push: range(0x%x-0x%x) (size:0x%x)\r\n",
			id, last2card, nowaddr, wait_for_push);
	} else {
		wait_for_push = (UINT64)(useable - (last2card - nowaddr));
		DBG_IND("[%d]wait_for_push: range(0x%x-0x%x),(0x%x-0x%x) (size:0x%x)\r\n",
			id, bufaddr, nowaddr, last2card, (bufaddr + useable), wait_for_push);
	}
	if (real2card >= nowaddr) {
		free = (UINT64)(real2card - nowaddr);
		DBG_IND("[%d]free: range(0x%x-0x%x) (size:0x%x)\r\n",
			id, nowaddr, nowaddr, free);
	} else {
		free = (UINT64)(useable - (nowaddr - real2card));
		DBG_IND("[%d]free: range(0x%x-0x%x),(0x%x-0x%x) (size:0x%x)\r\n",
			id, bufaddr, real2card, nowaddr, (bufaddr + useable), free);
	}

	total = (UINT64)useable;
	used = wait_for_push + wait_for_write;
	if (real2card == last2card && last2card == nowaddr) {
		if (bsmux_util_is_not_normal()) {
			used = (UINT64)useable;
			free = 0;
		} else {
			used = 0;
			free = (UINT64)useable;
		}
	}
	if (used > total) {
		used = total;
		free = 0;
	}
	if (((((used * 1000) / total) % 10) == 5) && ((((free * 1000) / total) % 10) == 5)) {
		bufuse_pct  = ((((used * 1000) / total) + 5) / 10);
		buffree_pct = ((((free * 1000) / total) + 5) / 10) - 1;
	} else {
		bufuse_pct  = ((((used * 1000) / total) + 5) / 10);
		buffree_pct = ((((free * 1000) / total) + 5) / 10);
	}

	if ((bufuse_pct + buffree_pct) != 100) {
		DBG_WRN("[%d] bufuse_pct=(0x%lx, %d) buffree_pct=(0x%lx, %d)\r\n", id, (unsigned long)bufuse_pct ,(int)bufuse_pct, (unsigned long)buffree_pct ,(int)buffree_pct);
		DBG_WRN("[%d] used=0x%lx free=0x%lx total=0x%lx\r\n", id, (unsigned long)used, (unsigned long)(total-used), (unsigned long)total);
		DBG_WRN("[%d] used=0x%lx free=0x%lx total=0x%lx\r\n", id, (unsigned long)(total-free), (unsigned long)free, (unsigned long)total);
		if ((used + free) > total) {
			used = (used > total) ? (total-free) : used;
			free = (free > total) ? (total-used) : free;
		}
		DBG_WRN("[%d] used=0x%lx free=0x%lx total=0x%lx\r\n", id, (unsigned long)used, (unsigned long)free, (unsigned long)total);
		DBG_WRN("[%d] real2card=0x%lx last2card=0x%lx nowaddr=0x%lx\r\n", id,
			(unsigned long)real2card, (unsigned long)last2card, (unsigned long)nowaddr);
	} else {
		if (bufuse_pct >= alarm_pct)
			BSMUX_MSG("[%d] use:%d free:%d total:%d\r\n", id, (int)bufuse_pct, (int)buffree_pct, (int)(bufuse_pct + buffree_pct));
	}
#else
	DBG_WRN("unsupported\r\n");
#endif

	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_FREESIZE, free);
	bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_USEDSIZE, used);

	return E_OK;
}

//>> memory control (copybuf)
ER bsmux_ctrl_mem_cal_buffer(UINT32 id, UINT32 *p_size)
{
	UINT32 resvsec = 0, tbr = 0, sub_tbr = 0, step;
	UINT32 size = 0;
	UINT32 num = 0;
#ifdef BSMUX_LOCKNUMMIN
	UINT32 resvblksec = BSMUX_LOCKNUMMIN;
#else
	UINT32 resvblksec = BSMUX_NORMALNUMMIN;
#endif
	UINT32 lock_ms = 0;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range\r\n", id);
		return E_ID;
	}

	// get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_BUFRESSEC, &resvsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &tbr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_TBR, &sub_tbr);
	step = bsmux_util_set_writeblock_size(id);

	bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_LOCKSEC, &lock_ms);
	if (lock_ms) {
		resvblksec = BSMUX_LOCKNUMMIN; //2: one for our own, one for output
	} else {
		resvblksec = BSMUX_NORMALNUMMIN; //3: one for our own, one for padding reserved, one for output
	}

	if (sub_tbr) {
		size = ALIGN_CEIL(((tbr + sub_tbr) * resvsec), step);
	} else {
		size = ALIGN_CEIL((tbr * resvsec), step);
	}
#if BSMUX_TEST_RESV_BUFF
	num = resvblksec + 1; //reserve blksize
#else
	num = resvblksec;
#endif
	if ((size / step) < num) {
		size += (num - (size / step)) * step;
	}
	BSMUX_MSG("BSM:[%d] step=0x%x num=%d size=0x%x\r\n", id, step, num, size);
	BSMUX_MEM_IND("BSM:[%d] BUF size=0x%X sec=%d\r\n", id, size, resvsec);
	*p_size = size;
	return E_OK;
}

//>> memory control (copybuf)
ER bsmux_ctrl_mem_set_buffer(UINT32 id, void *p_buf)
{
	BSMUX_MEM_BUF *pBuf0 = NULL;
	BSMUX_MEM_BUF *pGiveBuf = (BSMUX_MEM_BUF *)p_buf;
	UINT32 temp = 0;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	if (bsmux_util_is_null_obj((void *)pGiveBuf)) {
		DBG_ERR("[%d] input buffer null\r\n", id);
		return E_PAR;
	}

	pBuf0 = bsmux_get_mem_buf(id);
	if (bsmux_util_is_null_obj((void *)pBuf0)) {
		DBG_ERR("[%d] get mem buf null\r\n", id);
		return E_NOEXS;
	}

	pBuf0->addr = pGiveBuf->addr;
	pBuf0->size = pGiveBuf->size;
	pBuf0->end  = pGiveBuf->addr + pGiveBuf->size; //minus 1M

	temp = bsmux_util_set_writeblock_size(id);
	if ((temp) && (pBuf0->size % temp)) {
		pBuf0->max  = pBuf0->end - temp; //minus 500k
		DBG_WRN("[%d] size 0x%X not align writeblk 0x%X\r\n", id, pBuf0->size, temp);
	} else {
		pBuf0->max  = pBuf0->end;
#if 0
		temp = 0;
#endif
	}

	pBuf0->nowaddr  = pBuf0->addr;
	pBuf0->last2card = pBuf0->addr;
	pBuf0->blksize = temp;
	pBuf0->total2card = 0;
	pBuf0->real2card = pBuf0->addr;
	pBuf0->end2card = pBuf0->addr;
	pBuf0->blkoffset = pBuf0->addr;
	pBuf0->blkusednum = 0;
	pBuf0->allowmaxsize = pGiveBuf->size - temp;
	if (pBuf0->allowmaxsize > 0xF0000000) { //might error!
		DBG_DUMP("ERR gNMR_MemBuf[%d] start=0x%lx, end=0x%lx\r\n", id, pGiveBuf->addr, pBuf0->end);
		DBG_DUMP("ERR gNMR_MemBuf[%d] allowmaxsize=0x%lx.\r\n", id, pBuf0->allowmaxsize);
	}
	BSMUX_MEM_DUMP("BSM:[%d] nowaddr 0x%X blksize 0x%X max 0x%X allowmaxsize 0x%X\r\n",
		id, pBuf0->nowaddr, pBuf0->blksize, pBuf0->max, pBuf0->allowmaxsize);

	return E_OK;
}

//>> memory control (copybuf)
ER bsmux_ctrl_mem_get_bufinfo(UINT32 id, UINT32 type, UINT32 *value)
{
	BSMUX_MEM_BUF *p_info = NULL;

	p_info = bsmux_get_mem_buf(id);
	if (p_info == NULL) {
		DBG_ERR("[%d] get mem buf null\r\n", id);
		return E_NOEXS;
	}

	switch (type) {
	case BSMUX_CTRL_NOWADDR:
		*value = p_info->nowaddr;
		break;
	case BSMUX_CTRL_LAST2CARD:
		*value = p_info->last2card;
		break;
	case BSMUX_CTRL_REAL2CARD:
		*value = p_info->real2card;
		break;
	case BSMUX_CTRL_END2CARD:
		*value = p_info->end2card;
		break;
	case BSMUX_CTRL_TOTAL2CARD:
		*value = p_info->total2card;
		break;
	case BSMUX_CTRL_WRITEBLOCK:
		*value = p_info->blksize;
		break;
	case BSMUX_CTRL_BUFADDR:
		*value = p_info->addr;
		break;
	case BSMUX_CTRL_BUFSIZE:
		*value = p_info->size;
		break;
	case BSMUX_CTRL_BUFEND:
		*value = p_info->max;
		break;
	case BSMUX_CTRL_WRITEOFFSET:
		*value = p_info->blkoffset;
		break;
	case BSMUX_CTRL_USEDBLOCK:
		*value = p_info->blkusednum;
		break;
	case BSMUX_CTRL_USEABLE:
		*value = p_info->allowmaxsize;
		break;
	case BSMUX_CTRL_FREESIZE:
		*value = p_info->free_size;
		break;
	case BSMUX_CTRL_USEDSIZE:
		*value = p_info->used_size;
		break;
	default:
		break;
	}

	return E_OK;
}

//>> memory control (copybuf)
ER bsmux_ctrl_mem_set_bufinfo(UINT32 id, UINT32 type, UINT32 value)
{
	BSMUX_MEM_BUF *p_info = NULL;

	p_info = bsmux_get_mem_buf(id);
	if (p_info == NULL) {
		DBG_ERR("[%d] get mem buf null\r\n", id);
		return E_NOEXS;
	}

	switch (type) {
	case BSMUX_CTRL_NOWADDR:
		p_info->nowaddr = value;
		break;
	case BSMUX_CTRL_LAST2CARD:
		p_info->last2card = value;
		break;
	case BSMUX_CTRL_REAL2CARD:
		p_info->real2card = value;
		break;
	case BSMUX_CTRL_END2CARD:
		p_info->end2card = value;
		break;
	case BSMUX_CTRL_TOTAL2CARD:
		p_info->total2card = value;
		break;
	case BSMUX_CTRL_WRITEBLOCK:
		p_info->blksize = value;
		break;
	case BSMUX_CTRL_BUFADDR:
		p_info->addr = value;
		break;
	case BSMUX_CTRL_BUFSIZE:
		p_info->size = value;
		break;
	case BSMUX_CTRL_BUFEND:
		p_info->max = value;
		break;
	case BSMUX_CTRL_WRITEOFFSET:
		p_info->blkoffset = value;
		break;
	case BSMUX_CTRL_USEDBLOCK:
		p_info->blkusednum = value;
		break;
	case BSMUX_CTRL_USEABLE:
		p_info->allowmaxsize = value;
		break;
	case BSMUX_CTRL_FREESIZE:
		p_info->free_size = value;
		break;
	case BSMUX_CTRL_USEDSIZE:
		p_info->used_size = value;
		break;
	default:
		break;
	}

	return E_OK;
}

//>> memory control (dbg)
ER bsmux_ctrl_mem_dbg(UINT32 value)
{
	g_debug_mem_buf = value;
	return E_OK;
}

//>> bs saveq control
UINT32 bsmux_ctrl_bs_getnum(UINT32 id, UINT32 lock)
{
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 num;

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get qinfo null\r\n", id);
		return 0;
	}

	if (lock) bsmux_saveq_lock();
	{
		num = QUE_NUM(p_qinfo);
	}
	if (lock) bsmux_saveq_unlock();

	return num;
}

//>> bs saveq control
UINT32 bsmux_ctrl_bs_active(UINT32 id, UINT32 lock)
{
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 active;

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get qinfo null\r\n", id);
		return 0;
	}

	if (lock) bsmux_saveq_lock();
	{
		active = p_qinfo->queue_active;
	}
	if (lock) bsmux_saveq_unlock();

	return active;
}

//>> bs saveq control (GOPs)
ER bsmux_ctrl_bs_calcgopnum(UINT32 id, UINT32 *p_num, UINT32 *p_sec)
{
	UINT32 vfr = 0, gop = 0, emron = 0, resvsec = 0, rollsec = 0;
	UINT32 emrloop = 0, strgbuf = 0;
	UINT32 sec = 0, num = 0;

	// get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_BUFRESSEC, &resvsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);
	if (emrloop && strgbuf) {
		rollsec = 2;//temp suggestion
	}

	sec = resvsec;
	if (emron) {
		sec += rollsec;
	}

	if (vfr < gop) {
		DBG_WRN("[%d] vfr=%d less than gop=%d\r\n", id, vfr, gop);
	} else {
		num = (sec * (vfr / gop));
	}

#if BSMUX_TEST_USE_HEAP
#else
	if (num > BSMUXER_BSQ_ROLLBACK_SECNEW) { //array size
		DBG_WRN("[%d] iframe num exceed max(%d)\r\n", id, BSMUXER_BSQ_ROLLBACK_SECNEW);
	}
#endif
	BSMUX_BSQ_DUMP("BSM:[%d] sec=%d gopnum=%d\r\n", id, sec, num);
	*p_num = num;
	*p_sec = sec;
	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_calcqueue(UINT32 id, UINT32 *p_size)
{
	UINT32 resvsec = 0, vfr = 0, gop = 0;
	UINT32 emron = 0, rollsec = 0;
	UINT32 emrloop = 0, strgbuf = 0;
	UINT32 sec = 0, size = 0, secentry;

	// get prj setting
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_BUFRESSEC, &resvsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);
	if (emrloop && strgbuf) {
		rollsec = 2;//temp suggestion
	}
	secentry = bsmux_util_calc_entry_per_sec(id);
	sec = resvsec;
	if (emron) {
		sec += rollsec;
	}
	size = (sizeof(BSMUX_SAVEQ_BS_INFO)) * (sec * (vfr / gop)) * secentry;
	BSMUX_MEM_IND("BSM:[%d] BSQ size=0x%X sec=%d\r\n", id, size, sec);
	*p_size = size;
	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_initqueue(UINT32 id, UINT32 addr, UINT32 size)
{
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 gop_max = 0, gop_sec = 0;
#if BSMUX_TEST_USE_HEAP
#else
	UINT32 i;
#endif

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get qinfo null\r\n", id);
		return E_NOEXS;
	}

#if BSMUX_TEST_USE_HEAP
	if ((addr == 0) && (size == 0)) {
		//reset flow
		if (p_qinfo->gop_num) {
			free(p_qinfo->gop_num);
			p_qinfo->gop_num = NULL;
		}
		p_qinfo->queue_head = 0;
		p_qinfo->queue_tail = 0;
		p_qinfo->queue_active = 0;
		return E_OK;
	}
#endif

	bsmux_ctrl_bs_calcgopnum(id, &gop_max, &gop_sec);
	if (gop_sec == 0) {
		DBG_WRN("[%d] calc gop error\r\n", id);
	}

	bsmux_saveq_lock();
	{
		p_qinfo->start_addr = addr;
		p_qinfo->end_addr   = addr + size;
		p_qinfo->queue_max  = (size / (sizeof(BSMUX_SAVEQ_BS_INFO)));
		p_qinfo->queue_head = 0;
		p_qinfo->queue_tail = 0;
		p_qinfo->gop_cnt    = 0;
		p_qinfo->gop_sec    = gop_sec;
		p_qinfo->gop_max    = gop_max;
#if BSMUX_TEST_USE_HEAP
		p_qinfo->gop_num = calloc(p_qinfo->gop_max, sizeof(UINT32));
#else
		for (i = 0; i < BSMUXER_BSQ_ROLLBACK_SECNEW; i++) {
			p_qinfo->gop_num[i] = 0;
		}
#endif
		p_qinfo->queue_active = 1;
	}
	bsmux_saveq_unlock();

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_enqueue(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_SAVEQ_BS_INFO *p_in_buf = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 new_head;
	UINT32 queue_max;
	UINT64 t_stamp;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	if (bsmux_util_is_null_obj((void *)p_in_buf)) {
		DBG_ERR("[%d] input data null\r\n", id);
		return E_NOEXS;
	}

	// debug bsq
	BSMUX_BSQ_IND("BSM:[%d] enqueue 0x%X 0x%X 0x%X\r\n",
		id, p_in_buf->uiBSMemAddr, p_in_buf->uiBSSize, p_in_buf->uibufid);

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get queue null\r\n", id);
		return E_NOEXS;
	}

	t_stamp = bsmux_util_calc_timestamp(id, p_in_buf->uiTimeStamp);
	if (bsmux_util_check_duration(id, p_bsq) == FALSE) {
		DBG_DUMP("bsmux_enqueue:[%d][0x%x][0x%x][0x%x]dur=%llu invalid\r\n",
			id, p_in_buf->uiType, p_in_buf->uiBSMemAddr, p_in_buf->uiBSSize, t_stamp);
		bsmux_cb_result(BSMUXER_RESULT_PROCDATAFAIL, id);
		return E_OBJ;
	}

	bsmux_saveq_lock();
	{
		//calc new head idx
		new_head = p_qinfo->queue_head + 1;
		queue_max = p_qinfo->queue_max;
		if (new_head == queue_max) {
			BSMUX_BSQ_IND("new_head %d queue_max %d\r\n", new_head, queue_max);
			new_head = 0;
		}

		//check the queue is full
		if (new_head == p_qinfo->queue_tail) {
			DBG_ERR("%d queue full\r\n", id);
			bsmux_saveq_unlock();
			bsmux_cb_result(BSMUXER_RESULT_PROCDATAFAIL, id);
			return E_SYS;
		}

		//copy data to head buf
		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + new_head;
		#if 1
		memset(p_new_buf, 0x00, sizeof(BSMUX_SAVEQ_BS_INFO));
		#endif
		#if 0
		memcpy(p_new_buf, p_in_buf, sizeof(BSMUX_SAVEQ_BS_INFO));
		#else
		p_new_buf->uiType = p_in_buf->uiType;
		p_new_buf->uiBSMemAddr = p_in_buf->uiBSMemAddr;
		p_new_buf->uiBSSize = p_in_buf->uiBSSize;
		p_new_buf->uiIsKey = p_in_buf->uiIsKey;
		p_new_buf->uiBSVirAddr = p_in_buf->uiBSVirAddr;
		p_new_buf->uibufid = p_in_buf->uibufid;
		p_new_buf->uiPortID = p_in_buf->uiPortID;
		p_new_buf->uiCountSametype = p_in_buf->uiCountSametype;
		p_new_buf->uiTimeStamp = p_in_buf->uiTimeStamp;
		p_new_buf->uiTOftMemAddr = p_in_buf->uiTOftMemAddr;
		p_new_buf->uiTOftVirAddr = p_in_buf->uiTOftVirAddr;
		#endif
		p_new_buf->uiCountSametype = 0;

		#if 1
		g_bsmux_temp_bs_info[id].uiType = p_in_buf->uiType;
		g_bsmux_temp_bs_info[id].uiBSMemAddr = p_in_buf->uiBSMemAddr;
		g_bsmux_temp_bs_info[id].uiBSSize = p_in_buf->uiBSSize;
		g_bsmux_temp_bs_info[id].uiIsKey = p_in_buf->uiIsKey;
		g_bsmux_temp_bs_info[id].uiBSVirAddr = p_in_buf->uiBSVirAddr;
		g_bsmux_temp_bs_info[id].uibufid = p_in_buf->uibufid;
		g_bsmux_temp_bs_info[id].uiPortID = p_in_buf->uiPortID;
		g_bsmux_temp_bs_info[id].uiCountSametype = new_head;
		g_bsmux_temp_bs_info[id].uiTimeStamp = p_in_buf->uiTimeStamp;
		g_bsmux_temp_bs_info[id].uiTOftMemAddr = p_in_buf->uiTOftMemAddr;
		g_bsmux_temp_bs_info[id].uiTOftVirAddr = p_in_buf->uiTOftVirAddr;
		#endif

		//new head
		p_qinfo->queue_head = new_head;
	}
	bsmux_saveq_unlock();

	// store vid iframe
	if (p_in_buf->uiIsKey) {
		bsmux_ctrl_bs_add_lastI(id, new_head);
		BSMUX_BSQ_DUMP("[%d] AddI[%d] addr=0x%lx size=0x%lx\r\n",
			id, new_head, p_in_buf->uiBSMemAddr, p_in_buf->uiBSSize);
		#if BSMUX_DEBUG_BSQ
		{
			#if 1
			UINT8 *p2nd = (UINT8 *)(p_in_buf->uiBSVirAddr);
			#else
			UINT8 *p2nd = (UINT8 *)(bsmux_util_get_buf_va(id, p_in_buf->uiBSMemAddr));
			#endif
			BSMUX_BSQ_IND("0x%lx 0x%lx 0x%lx 0x%lx\r\n", *(p2nd+2), *(p2nd+3), *(p2nd+4), *(p2nd+5));
		}
		#endif
	}

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_dequeue(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_QUEUE_INFO *p_qinfo;
	BSMUX_SAVEQ_BS_INFO *p_ret_buf = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	UINT32 new_tail;
	UINT32 queue_max;
	UINT64 t_stamp;
	UINT32 t_dur_us = 0;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("get queue NULL\r\n");
		return E_NOEXS;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_DUR_US_MAX, &t_dur_us);

	// handle saveq
	bsmux_saveq_lock();
	{
		//check the queue is empty
		if (p_qinfo->queue_tail == p_qinfo->queue_head) {
			DBG_DUMP("bsmux_dequeue:[%d] queue empty\r\n", id);
			bsmux_saveq_unlock();
			return E_OK;
		}

		new_tail = p_qinfo->queue_tail + 1;
		queue_max = p_qinfo->queue_max;
		if (new_tail == queue_max) {
			BSMUX_BSQ_IND("new_tail %d queue_max %d\r\n", new_tail, queue_max);
			new_tail = 0;
		}
		//copy data
		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + new_tail;
		#if 1
		t_stamp = bsmux_util_calc_timestamp(id, p_new_buf->uiTimeStamp);
		if (t_stamp > (UINT64)t_dur_us)
		{
			DBG_DUMP("bsmux_dequeue:[%d][0x%x][0x%x][0x%x]dur=%llu\r\n",
				id, p_new_buf->uiType, p_new_buf->uiBSMemAddr, p_new_buf->uiBSSize, t_stamp);
			DBG_DUMP("bsmux_dequeue:[%d][%d][%d][%d]\r\n",
				id, p_qinfo->queue_head, new_tail, p_qinfo->queue_max);
			//--- data from dequeue ---
			DBG_DUMP("bsmux_dequeue[%d]:[%d][type=0x%x][pa=0x%x][va=0x%x][size=0x%x][dur=%llu]\r\n",
				new_tail, id,
				p_new_buf->uiType, p_new_buf->uiBSMemAddr, p_new_buf->uiBSVirAddr, p_new_buf->uiBSSize, t_stamp);
			DBG_DUMP("bsmux_dequeue[%d]:[%d][bufid=0x%x][oft_pa=0x%x][oft_va=0x%x]\r\n",
				new_tail, id, p_new_buf->uibufid, p_new_buf->uiTOftMemAddr, p_new_buf->uiTOftVirAddr);
			//--- data from dequeue ---
			//--- data from enqueue ---
			DBG_DUMP("bsmux_enqueue[%d]:[%d][type=0x%x][pa=0x%x][va=0x%x][size=0x%x][dur=%llu]\r\n",
				g_bsmux_temp_bs_info[id].uiCountSametype, id,
				g_bsmux_temp_bs_info[id].uiType,
				g_bsmux_temp_bs_info[id].uiBSMemAddr,
				g_bsmux_temp_bs_info[id].uiBSVirAddr,
				g_bsmux_temp_bs_info[id].uiBSSize,
				bsmux_util_calc_timestamp(id, g_bsmux_temp_bs_info[id].uiTimeStamp));
			DBG_DUMP("bsmux_enqueue[%d]:[%d][bufid=0x%x][oft_pa=0x%x][oft_va=0x%x]\r\n",
				g_bsmux_temp_bs_info[id].uiCountSametype, id,
				g_bsmux_temp_bs_info[id].uibufid,
				g_bsmux_temp_bs_info[id].uiTOftMemAddr,
				g_bsmux_temp_bs_info[id].uiTOftVirAddr);
			//--- data from enqueue ---
			DBG_DUMP("bsmux_dequeue:[%d] return\r\n", id);
			p_qinfo->queue_tail = new_tail;
			bsmux_saveq_unlock();
			bsmux_cb_result(BSMUXER_RESULT_PROCDATAFAIL, id);
			return E_OK;
		}
		#endif
		#if 0
		memcpy(p_ret_buf, p_new_buf, sizeof(BSMUX_SAVEQ_BS_INFO));
		#else
		p_ret_buf->uiType = p_new_buf->uiType;
		p_ret_buf->uiBSMemAddr = p_new_buf->uiBSMemAddr;
		p_ret_buf->uiBSSize = p_new_buf->uiBSSize;
		p_ret_buf->uiIsKey = p_new_buf->uiIsKey;
		p_ret_buf->uiBSVirAddr = p_new_buf->uiBSVirAddr;
		p_ret_buf->uibufid = p_new_buf->uibufid;
		p_ret_buf->uiPortID = p_new_buf->uiPortID;
		p_ret_buf->uiCountSametype = p_new_buf->uiCountSametype;
		p_ret_buf->uiTimeStamp = p_new_buf->uiTimeStamp;
		p_ret_buf->uiTOftMemAddr = p_new_buf->uiTOftMemAddr;
		p_ret_buf->uiTOftVirAddr = p_new_buf->uiTOftVirAddr;
		#endif
		if (p_new_buf->uiCountSametype == 0) {
			p_new_buf->uiCountSametype = 1;
		} else if (p_new_buf->uiCountSametype == 1) {
			DBG_IND("bsmux_dequeue:[%d] index[%d] used\r\n", id, new_tail);
		} else {
			DBG_DUMP("bsmux_dequeue:[%d] index[%d](=0x%x) invalid\r\n", id, new_tail, p_new_buf->uiCountSametype);
		}
		p_qinfo->queue_tail = new_tail;
	}
	bsmux_saveq_unlock();

	t_stamp = bsmux_util_calc_timestamp(id, p_ret_buf->uiTimeStamp);
	if (t_stamp > (UINT64)t_dur_us) {
		BSMUX_BSQ_DUMP("bsmux_dequeue:[%d][0x%x][0x%x][0x%x]dur=%llu\r\n",
			id, p_ret_buf->uiType, p_ret_buf->uiBSMemAddr, p_ret_buf->uiBSSize, t_stamp);
		BSMUX_BSQ_DUMP("bsmux_dequeue:[%d][%d][%d][%d]\r\n",
			id, p_qinfo->queue_head, p_qinfo->queue_tail, p_qinfo->queue_max);
		bsmux_dump_queue_info(id, p_qinfo->queue_tail);
		#if 0
		// check tag
		if (bsmux_util_check_tag(id, (void *)p_ret_buf) == FALSE) {
			p_ret_buf->uiCountSametype = BSMUX_INVALID_IDX;
		}
		#else
		p_ret_buf->uiCountSametype = BSMUX_INVALID_IDX;
		bsmux_dump_queue_info(id, (new_tail<2)?(new_tail+queue_max-2):(new_tail-2));
		bsmux_dump_queue_info(id, (new_tail<1)?(new_tail+queue_max-1):(new_tail-1));
		bsmux_dump_queue_info(id, new_tail);
		bsmux_dump_queue_info(id, ((new_tail+1)>queue_max)?((new_tail+1)-queue_max):(new_tail+1));
		bsmux_dump_queue_info(id, ((new_tail+2)>queue_max)?((new_tail+2)-queue_max):(new_tail+2));
		#endif
	}

	// log vid iframe
	if (p_ret_buf->uiIsKey) {
		BSMUX_BSQ_DUMP("[%d] GetI[%d] addr=0x%lx size=0x%lx\r\n",
			id, new_tail, p_ret_buf->uiBSMemAddr, p_ret_buf->uiBSSize);
	}

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_predequeue(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_QUEUE_INFO *p_qinfo;
	BSMUX_SAVEQ_BS_INFO *p_ret_buf = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	UINT32 new_tail;
	UINT32 queue_max;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("get queue NULL\r\n");
		return E_NOEXS;
	}

	// handle saveq
	bsmux_saveq_lock();
	{
		//check the queue is empty
		if (p_qinfo->queue_tail == p_qinfo->queue_head) {
			DBG_DUMP("%s:[%d] queue empty\r\n", __func__, id);
			bsmux_saveq_unlock();
			return E_OK;
		}

		new_tail = p_qinfo->queue_tail + 1;
		queue_max = p_qinfo->queue_max;
		if (new_tail == queue_max) {
			BSMUX_BSQ_IND("new_tail %d queue_max %d\r\n", new_tail, queue_max);
			new_tail = 0;
		}
		//copy data
		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + new_tail;
		memcpy(p_ret_buf, p_new_buf, sizeof(BSMUX_SAVEQ_BS_INFO));
	}
	bsmux_saveq_unlock();

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_rollback(UINT32 id, UINT32 sec, UINT32 check)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	BSMUX_QUEUE_INFO *p_qinfo;
	UINT32 new_tail;
	UINT32 queue_max;
	UINT32 vfn = 0;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	if (vfn != 0) {
		DBG_ERR("[%d] still running\r\n", id);
		return E_SYS;
	}

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("get queue NULL\r\n");
		return E_NOEXS;
	}

	// handle saveq
	bsmux_saveq_lock();
	{
		queue_max = p_qinfo->queue_max;
		new_tail = bsmux_ctrl_bs_get_lastI(id, sec);// use get last iframe api
		if (new_tail > queue_max) {
			DBG_ERR("[%d] rollback %d new_tail %d exceed queue_max %d\r\n", id, sec, new_tail, queue_max);
			bsmux_saveq_unlock();
			return E_SYS;
		}
		DBG_IND("[%d] get_lastI %d\r\n", id, new_tail);

		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + new_tail;

		BSMUX_BSQ_IND("addr=0x%lx size=0x%lx\r\n", p_new_buf->uiBSMemAddr, p_new_buf->uiBSSize);
		#if BSMUX_DEBUG_BSQ
		{
			#if 1
			UINT8 *p2nd = (UINT8 *)(p_new_buf->uiBSVirAddr);
			#else
			UINT8 *p2nd = (UINT8 *)(bsmux_util_get_buf_va(id, p_new_buf->uiBSMemAddr));
			#endif
			BSMUX_BSQ_IND("0x%lx 0x%lx 0x%lx 0x%lx\r\n", *(p2nd+2), *(p2nd+3), *(p2nd+4), *(p2nd+5));
		}
		#endif

		if (!p_new_buf->uiIsKey) {
			DBG_ERR("[%d] rollback %d error\r\n", id, sec);
			bsmux_saveq_unlock();
			return E_SYS;
		}

		if (bsmux_util_check_tag(id, p_new_buf) == FALSE) {
			DBG_ERR("RollWrong[%d][0x%lx][0x%lx]\r\n", id, p_new_buf->uiBSMemAddr, p_new_buf->uiBSSize);
			bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
			bsmux_saveq_unlock();
			return E_SYS;
		}

		if (check == 0) {
			if (new_tail) {
				p_qinfo->queue_tail = new_tail - 1;
			} else {
				p_qinfo->queue_tail = queue_max - 1;
			}

			BSMUX_BSQ_IND("rollback new_tail %d queue_max %d\r\n", p_qinfo->queue_tail, queue_max);
		}
	}
	bsmux_saveq_unlock();

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_reorder(UINT32 id, UINT32 sec)
{
	BSMUX_SAVEQ_BS_INFO *p_saveq_start = NULL;
	BSMUX_SAVEQ_BS_INFO *p_new_buf = NULL;
	BSMUX_QUEUE_INFO *p_qinfo;
	UINT32 new_tail, org_tail;
	UINT32 queue_max;
	UINT32 flag_type;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("get queue NULL\r\n");
		return E_NOEXS;
	}

	// handle saveq
	bsmux_saveq_lock();
	{
		queue_max = p_qinfo->queue_max;
		org_tail = p_qinfo->queue_tail + 1;// next bs dequeue
		if (org_tail == queue_max) {
			org_tail = 0;
		}
		BSMUX_BSQ_DUMP("reorder:[%d] org=%d max=%d\r\n", id, org_tail, queue_max);

		new_tail = p_qinfo->queue_tail;// last bs to be check
		p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
		p_new_buf = p_saveq_start + new_tail;
		flag_type = p_new_buf->uiType;
		BSMUX_BSQ_IND("BSM:[%d] flag_type=%d\r\n", id, flag_type);

		// If SUB type is found, it means that the frame of MAIN is larger than SUB.
		// Set the frame of SUB as invalid until the last key frame of the MAIN is found.
		while (new_tail != org_tail) {

			p_saveq_start = (BSMUX_SAVEQ_BS_INFO *)p_qinfo->start_addr;
			p_new_buf = p_saveq_start + new_tail;

			if ((p_new_buf->uiIsKey) && (p_new_buf->uiType != flag_type)) {
				DBG_DUMP("BSM:[%d]-[%d] found (%d)key\r\n", id, new_tail, p_new_buf->uiType);
				p_new_buf->uiCountSametype = 0;
				break;
			}

			if (p_new_buf->uiType == flag_type) {
				DBG_IND("BSM:[%d]-[%d][D] type=%d idx=%d key=%d\r\n", id, new_tail,
					p_new_buf->uiType, p_new_buf->uiCountSametype, p_new_buf->uiIsKey);
				p_new_buf->uiCountSametype = BSMUX_INVALID_IDX;
			} else {
				DBG_IND("BSM:[%d]-[%d][M] type=%d idx=%d key=%d\r\n", id, new_tail,
					p_new_buf->uiType, p_new_buf->uiCountSametype, p_new_buf->uiIsKey);
				p_new_buf->uiCountSametype = 0;
			}

			if (new_tail == 0) {
				new_tail = queue_max - 1; //fix queue_max not available
			} else {
				new_tail --;
			}
		}

		if (new_tail) {
			p_qinfo->queue_tail = new_tail - 1;
		} else {
			p_qinfo->queue_tail = queue_max - 1;
		}

		BSMUX_BSQ_DUMP("reorder:[%d] new=%d max=%d\r\n", id, p_qinfo->queue_tail, queue_max);
	}
	bsmux_saveq_unlock();

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_copy2strgbuf(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_MEM_BUF *pBuf = NULL;
	UINT32 bsAlign, step1 = 0, step2 = 0, newBS = 0, bs2card_step = 0, bs2write;
	UINT32 datalen = 0;
	UINT32 lastWrite = 0;
	UINT32 pa;
	UINT32 max_addr = 0;
	UINT32 filetype = 0;
	UINT32 vid_ok = 0;
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	// check input
	if (pBsq->uiBSSize == 0) {
		DBG_DUMP("BSM:[over=%d]copy no\r\n", id);
		return E_PAR;
	}

	pBuf = bsmux_get_mem_buf(id);
	if (bsmux_util_is_null_obj((void *)pBuf)) {
		DBG_ERR("get mem buf null\r\n");
		return E_NOEXS;
	}
	if (pBuf->blksize == 0) {
		DBG_ERR("[%d] blksize zero, assign to 1M\r\n", id);
		pBuf->blksize = BSMUX_BS2CARD_1M;
	}
	bs2card_step = pBuf->blksize;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);
#if BSMUX_TEST_RESV_BUFF
	{
		max_addr = pBuf->addr + pBuf->allowmaxsize; //already reserve blksize
	}
#else
	if (filetype == MEDIA_FILEFORMAT_TS) {
		max_addr = pBuf->addr + pBuf->allowmaxsize; //already reserve blksize
	} else {
		max_addr = pBuf->max;
	}
#endif

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);

	bsAlign = bsmux_util_muxalign(id, pBsq->uiBSSize);
	if ((pBuf->nowaddr + bsAlign) > pBuf->max) {
		step1 = pBuf->max - pBuf->nowaddr;
		step2 = bsAlign - step1;
		BSMUX_MSG("padding=0x%X (now=0x%X, max=0x%x, bs=0x%X)\r\n", step2, pBuf->nowaddr, pBuf->max, bsAlign);
	} else {
		step1 = bsAlign;
		step2 = 0;
	}

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &lastWrite);
	if ((pBuf->nowaddr < lastWrite) && (pBuf->nowaddr + step1 > lastWrite)) {
		DBG_ERR("[%d][%d] slow! now=0x%X + step=0x%X > lastWrite=0x%X!\r\n", id, pBsq->uiType, pBuf->nowaddr, step1, lastWrite);
		bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
		#if _TOFIX_
		return E_NOMEM; ///< Insufficient memory
		#endif
	}

	//data copy
	pa = bsmux_util_get_buf_pa(id, pBuf->nowaddr);
	if (!pa) {
		DBG_ERR("[%d] get now addr zero\r\n", id);
		return E_NOEXS;
	}
	#if _TOFIX_
	if (bsmux_util_is_not_normal()) {
		DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
		return E_NOMEM; ///< Insufficient memory
	}
	#endif
	dst.phy_addr = pa;
	dst.vrt_addr = pBuf->nowaddr;
	datalen = bsmux_util_engcpy((VOID *)&dst, (VOID *)pBsq, step1, mux_method);

	if (pBsq->uiType == BSMUX_TYPE_VIDEO) {
		if (E_OK != bsmux_util_update_vidinfo(id, pBuf->nowaddr, p_bsq)) {
			DBG_ERR("[%d] update vidinfo (0x%x/0x%x) failed\r\n", id, pBuf->nowaddr, pBsq->uiBSSize);
		}
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] copy=0x%X size=0x%X\r\n", id, pBuf->nowaddr, datalen);
		}
	}
	else if (pBsq->uiType == BSMUX_TYPE_SUBVD) {
		if (E_OK != bsmux_util_update_vidinfo(id, pBuf->nowaddr, p_bsq)) {
			DBG_ERR("[%d] sub update vidinfo (0x%x/0x%x) failed\r\n", id, pBuf->nowaddr, pBsq->uiBSSize);
		}
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] sub copy=0x%X size=0x%X\r\n", id, pBuf->nowaddr, datalen);
		}
	}

	pBuf->nowaddr += datalen;
	if (pBuf->nowaddr > pBuf->max) {
		DBG_WRN("nowaddr 0x%X exceed max 0x%X\r\n", pBuf->nowaddr, pBuf->max);
	}

	if (step2) {
		//newBS = pBuf->max - pBuf->last2card;
		bs2write = pBuf->max - pBuf->last2card;
		BSMUX_MEM_DUMP("bs2write=0x%X\r\n", bs2write);
	} else {
		#if 0
		newBS = pBuf->nowaddr - pBuf->last2card;
		#else
		if (pBuf->last2card > pBuf->nowaddr) {
			DBG_ERR("last2card 0x%X > nowaddr 0x%X\r\n", pBuf->last2card, pBuf->nowaddr);
			//--- dump info ---
			DBG_DUMP("copy2buffer:[%d][type=0x%x][pa=0x%x][va=0x%x][size=0x%x]\r\n",
				id, pBsq->uiType, pBsq->uiBSMemAddr, pBsq->uiBSVirAddr, pBsq->uiBSSize);
			DBG_DUMP("copy2buffer:[%d][step1=0x%x][step2=0x%x][pa=0x%x][datalen=0x%x]\r\n",
				id, step1, step2, pa, datalen);
			bsmux_util_dump_info(id);
			//--- dump info ---
			newBS = 0;
		} else {
			newBS = pBuf->nowaddr - pBuf->last2card;
		}
		#endif

		if (newBS < bs2card_step) {
			//newBS = 0;
			bs2write = 0;
		} else {
			//newBS = bsmux_util_calc_align(newBS, bs2card_step, NMEDIAREC_ALIGN_ROUND);
			bs2write = bsmux_util_calc_align(newBS, bs2card_step, NMEDIAREC_ALIGN_ROUND);
		}
	}

#if 1
	bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_VID, &vid_ok);
	if (vid_ok) {
		UINT32 total_size = 0, bs_to_write2, bs_to_write3;
		DBG_IND("[%d][STRGBUF][VIDOK]bs_to_write..%d..\r\n", id, bs2write);
		bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_TOTAL_SIZE, &total_size);
		bs_to_write2 = pBuf->nowaddr - pBuf->last2card;
		bs_to_write3 = total_size + bs_to_write2;
		DBG_IND("[%d][STRGBUF][VIDOK]total_size:%d bs_to_write2:%d bs_to_write3:%d\r\n",
			id, total_size, bs_to_write2, bs_to_write3);
//#NT#2020/01/14# FIX BS2CARD LAST -begin
		UINT32 mod = bs_to_write3 % pBuf->blksize;
		DBG_IND("[%d][STRGBUF][VIDOK]mod:%d\r\n", id, mod);
		if (mod) {
			bs2write = bs_to_write2 - mod + pBuf->blksize;
		} else {
			bs2write = bs_to_write2;
		}
		DBG_IND("[%d][STRGBUF][VIDOK]BS2CARD LAST %d\r\n",id, bs2write);
//#NT#2020/01/14# FIX BS2CARD LAST -end
		pBuf->nowaddr += (bs2write - bs_to_write2);
		DBG_IND("[%d][STRGBUF][VIDOK]nowaddr=0x%x last2card=0x%x\r\n",
			id, pBuf->nowaddr, pBuf->last2card);
	}
#endif

	if (bs2write) {
		BSMUXER_FILE_BUF add_info = {0};
		DBG_IND("[%d][STRGBUF] AddCard %d (%d)\r\n", id, pBuf->last2card, bs2write);
		// take care normal add_info
		add_info.fileop |= BSMUXER_FOP_CONT_WRITE;
		add_info.addr = pBuf->last2card;
		add_info.size = bs2write;
		bsmux_util_strgbuf_handle_space(id, (void *)&add_info);
		//DBG_DUMP("[%d][STRGBUF] PUSH %d (%d)\r\n", id, pBuf->last2card, bs2write);

		pBuf->last2card += bs2write;
		//pBuf->total2card += bs2write;

		if (pBuf->nowaddr == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] now(0x%X) achieve max(0x%X)\r\n", id, pBuf->nowaddr, pBuf->max);
			pBuf->nowaddr = pBuf->addr;
		}
		if (pBuf->last2card == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] last(0x%X) achieve max(0x%X)\r\n", id, pBuf->last2card, pBuf->max);
			pBuf->last2card = pBuf->addr;
		}
		//DBG_DUMP("now %d last %d\r\n", p_buf->nowaddr, p_buf->last2card);
		{
			UINT32 end2card, buf_addr = 0, buf_end = 0;
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFADDR, &buf_addr);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_BUFEND, &buf_end);
			end2card = add_info.addr + add_info.size;
			if (end2card == buf_end) {
				DBG_IND("BSM:[%d] addr(0x%X) achieve max(0x%X)\r\n", id, end2card, buf_end);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, buf_addr);
			} else {
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_END2CARD, end2card);
			}
		}
	} else {

	}

	if (vid_ok) {
		if (pBuf->nowaddr >= pBuf->max) {
			DBG_ERR("[%d] nowaddr=0x%x max=0x%x overwriter\n", id, pBuf->nowaddr, pBuf->max);
		}
		if (pBuf->nowaddr >= max_addr) {
			pBuf->nowaddr = pBuf->addr + (pBuf->nowaddr - max_addr);
		}
		pBuf->nowaddr = pBuf->addr + bsmux_util_calc_align((pBuf->nowaddr - pBuf->addr), pBuf->blksize, NMEDIAREC_ALIGN_CEIL);
		if (pBuf->nowaddr == max_addr) {
			pBuf->nowaddr = pBuf->addr;
		}
		pBuf->last2card = pBuf->nowaddr;
		DBG_IND("[%d][STRGBUF][VIDOK]nowaddr=0x%x last2card=0x%x\r\n",
			id, pBuf->nowaddr, pBuf->last2card);
	}

#if BSMUX_TEST_RESV_BUFF
#else
	if (filetype == MEDIA_FILEFORMAT_TS)
#endif
	{
		if (pBuf->nowaddr >= max_addr) {
			UINT32 remaining_size = 0, dst_pa, src_pa;
			remaining_size = newBS - bs2write; //not add to file Q
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &lastWrite);

			#if 1
			if ((lastWrite >= pBuf->addr) && (lastWrite <= pBuf->end)
				&& ((pBuf->addr + remaining_size) > lastWrite)) {
			#else
			if ((pBuf->addr < lastWrite) && ((pBuf->addr + remaining_size) > lastWrite)) {
			#endif
				/**
					Rollback: if buffer start addr plus remaining_size large than lastWrite,
					it might cause buffer data overwrite.
				*/
				DBG_ERR("[%d] rollback pending\r\n", id);
				bsmux_util_dump_info(id);
				#if 1
				DBG_ERR("[%d] addr=0x%x remain=0x%x lastWrite=0x%x\r\n", id, pBuf->addr, remaining_size, lastWrite);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				#if _TOFIX_
				return E_NOMEM; ///< Insufficient memory
				#endif
				#endif
				//bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
			} else {
				{
					UINT8 *p1nd = (UINT8 *)(pBuf->nowaddr);
					UINT8 *p2nd = (UINT8 *)(pBuf->last2card);
					BSMUX_MEM_DUMP("0x%X => 0x%lx 0x%lx 0x%lx 0x%lx | 0x%X => 0x%lx 0x%lx 0x%lx 0x%lx\r\n",
						pBuf->last2card, *(p2nd), *(p2nd+1), *(p2nd+2), *(p2nd+3),
						pBuf->nowaddr, *(p1nd), *(p1nd+1), *(p1nd+2), *(p1nd+3));
				}
				if (remaining_size) {
					dst_pa = bsmux_util_get_buf_pa(id, pBuf->addr);
					src_pa = bsmux_util_get_buf_pa(id, pBuf->last2card);
					if (!dst_pa || !src_pa) {
						DBG_ERR("[%d] get dst/src addr zero\r\n", id);
						return E_NOEXS;
					}
					#if _TOFIX_
					if (bsmux_util_is_not_normal()) {
						DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
						return E_NOMEM; ///< Insufficient memory
					}
					#endif
					dst.phy_addr = dst_pa;
					dst.vrt_addr = pBuf->addr;
					src.phy_addr = src_pa;
					src.vrt_addr = pBuf->last2card;
					bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, remaining_size, mux_method); //rollback
				}
				{
					UINT8 *p1nd = (UINT8 *)(pBuf->nowaddr);
					UINT8 *p2nd = (UINT8 *)(pBuf->last2card);
					BSMUX_MEM_DUMP("0x%X => 0x%lx 0x%lx 0x%lx 0x%lx | 0x%X => 0x%lx 0x%lx 0x%lx 0x%lx\r\n",
						pBuf->last2card, *(p2nd), *(p2nd+1), *(p2nd+2), *(p2nd+3),
						pBuf->nowaddr, *(p1nd), *(p1nd+1), *(p1nd+2), *(p1nd+3));
				}
				pBuf->nowaddr = pBuf->addr;//rollback
				pBuf->nowaddr += remaining_size;
				pBuf->last2card = pBuf->addr; //rollback
				DBG_IND(">>[%d]  rollback start_addr 0x%X max_addr 0x%X nowaddr 0x%X size=0x%x\r\n", id, pBuf->addr, max_addr, pBuf->nowaddr, remaining_size);
			}
		}
	}

	if (step2) {
		BSMUX_MEM_DUMP("step2 data copy=0x%X\r\n", step2);
		DBG_IND("[%d] copy-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
			pBuf->addr, pBuf->nowaddr, pBuf->last2card, pBuf->real2card, pBuf->end);

		if ((pBuf->real2card >= pBuf->addr) && (pBuf->real2card <= pBuf->end)) {
			// mdat buffer area
			if ((pBuf->addr + step2) > pBuf->real2card) {
				DBG_ERR("[%d][%d] slow2! buf=0x%X + step=0x%X > lastWrite=0x%X!\r\n", id, pBsq->uiType, pBuf->addr, step2, lastWrite);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				#if _TOFIX_
				return E_NOMEM; ///< Insufficient memory
				#endif
			}
		}

		// data copy
		pa = bsmux_util_get_buf_pa(id, pBuf->addr);
		if (!pa) {
			DBG_ERR("[%d] get buf addr zero\r\n", id);
			return E_NOEXS;
		}
		#if _TOFIX_
		if (bsmux_util_is_not_normal()) {
			DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
			return E_NOMEM; ///< Insufficient memory
		}
		#endif
		dst.phy_addr = pa;
		dst.vrt_addr = pBuf->addr;
		src.phy_addr = pBsq->uiBSMemAddr + step1;
		src.vrt_addr = pBsq->uiBSVirAddr + step1;
		datalen = bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, step2, mux_method);
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] copy=0x%X size2=0x%X\r\n", id, pBuf->addr, datalen);
		}
		// update
		pBuf->nowaddr = pBuf->addr + datalen;

		DBG_IND("[%d] copy-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
			pBuf->addr, pBuf->nowaddr, pBuf->last2card, pBuf->real2card, pBuf->end);
	}

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_copy2buffer(UINT32 id, void *p_bsq)
{
	BSMUX_SAVEQ_BS_INFO *pBsq = (BSMUX_SAVEQ_BS_INFO *)p_bsq;
	BSMUX_MEM_BUF *pBuf = NULL;
	UINT32 bsAlign, step1 = 0, step2 = 0, newBS = 0, bs2card_step = 0, bs2write;
	UINT32 datalen = 0;
	UINT32 lastWrite = 0;
	UINT32 frontPut = 0;
	UINT32 pa;
	UINT32 max_addr = 0;
	UINT32 filetype = 0;
	UINT32 front_moov = 0;
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_FILETYPE, &filetype);

	if (pBsq->uiType == BSMUX_TYPE_THUMB) {
		if ((filetype == MEDIA_FILEFORMAT_MOV) || (filetype == MEDIA_FILEFORMAT_MP4)) {
			// no need to update offset since the data is placed on the container
			return E_OK;
		}
	}

	// check input
	if (pBsq->uiBSSize == 0) {
		DBG_DUMP("BSM:[over=%d]copy no\r\n", id);
		return E_PAR;
	}

	pBuf = bsmux_get_mem_buf(id);
	if (bsmux_util_is_null_obj((void *)pBuf)) {
		DBG_ERR("get mem buf null\r\n");
		return E_NOEXS;
	}
	if (pBuf->blksize == 0) {
		DBG_ERR("[%d] blksize zero, assign to 1M\r\n", id);
		pBuf->blksize = BSMUX_BS2CARD_1M;
	}
	bs2card_step = pBuf->blksize;

#if BSMUX_TEST_RESV_BUFF
	{
		max_addr = pBuf->addr + pBuf->allowmaxsize; //already reserve blksize
	}
#else
	if (filetype == MEDIA_FILEFORMAT_TS) {
		max_addr = pBuf->addr + pBuf->allowmaxsize; //already reserve blksize
	} else {
		max_addr = pBuf->max;
	}
#endif

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);

	bsAlign = bsmux_util_muxalign(id, pBsq->uiBSSize);
	if ((pBuf->nowaddr + bsAlign) > pBuf->max) {
		step1 = pBuf->max - pBuf->nowaddr;
		step2 = bsAlign - step1;
		BSMUX_MSG("padding=0x%X (now=0x%X, max=0x%x, bs=0x%X)\r\n", step2, pBuf->nowaddr, pBuf->max, bsAlign);
	} else {
		step1 = bsAlign;
		step2 = 0;
	}

	bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &lastWrite);
	if ((pBuf->nowaddr < lastWrite) && (pBuf->nowaddr + step1 > lastWrite)) {
		DBG_ERR("[%d][%d] slow! now=0x%X + step=0x%X > lastWrite=0x%X!\r\n", id, pBsq->uiType, pBuf->nowaddr, step1, lastWrite);
		bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
		#if _TOFIX_
		return E_NOMEM; ///< Insufficient memory
		#endif
	}

	//data copy
	pa = bsmux_util_get_buf_pa(id, pBuf->nowaddr);
	if (!pa) {
		DBG_ERR("[%d] get now addr zero\r\n", id);
		return E_NOEXS;
	}
	#if _TOFIX_
	if (bsmux_util_is_not_normal()) {
		DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
		return E_NOMEM; ///< Insufficient memory
	}
	#endif
	dst.phy_addr = pa;
	dst.vrt_addr = pBuf->nowaddr;
	datalen = bsmux_util_engcpy((VOID *)&dst, (VOID *)pBsq, step1, mux_method);

	if (pBsq->uiType == BSMUX_TYPE_VIDEO) {
		if (E_OK != bsmux_util_update_vidinfo(id, pBuf->nowaddr, p_bsq)) {
			DBG_ERR("[%d] update vidinfo (0x%x/0x%x) failed\r\n", id, pBuf->nowaddr, pBsq->uiBSSize);
		}
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] copy=0x%X size=0x%X\r\n", id, pBuf->nowaddr, datalen);
		}
	}
	else if (pBsq->uiType == BSMUX_TYPE_SUBVD) {
		if (E_OK != bsmux_util_update_vidinfo(id, pBuf->nowaddr, p_bsq)) {
			DBG_ERR("[%d] sub update vidinfo (0x%x/0x%x) failed\r\n", id, pBuf->nowaddr, pBsq->uiBSSize);
		}
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] sub copy=0x%X size=0x%X\r\n", id, pBuf->nowaddr, datalen);
		}
	}

	pBuf->nowaddr += datalen;
	if (pBuf->nowaddr > pBuf->max) {
		DBG_WRN("nowaddr 0x%X exceed max 0x%X\r\n", pBuf->nowaddr, pBuf->max);
	}

	if (step2) {
		//newBS = pBuf->max - pBuf->last2card;
		bs2write = pBuf->max - pBuf->last2card;
		BSMUX_MEM_DUMP("bs2write=0x%X\r\n", bs2write);
	} else {
		#if 0
		newBS = pBuf->nowaddr - pBuf->last2card;
		#else
		if (pBuf->last2card > pBuf->nowaddr) {
			DBG_ERR("last2card 0x%X > nowaddr 0x%X\r\n", pBuf->last2card, pBuf->nowaddr);
			//--- dump info ---
			DBG_DUMP("copy2buffer:[%d][type=0x%x][pa=0x%x][va=0x%x][size=0x%x]\r\n",
				id, pBsq->uiType, pBsq->uiBSMemAddr, pBsq->uiBSVirAddr, pBsq->uiBSSize);
			DBG_DUMP("copy2buffer:[%d][step1=0x%x][step2=0x%x][pa=0x%x][datalen=0x%x]\r\n",
				id, step1, step2, pa, datalen);
			bsmux_util_dump_info(id);
			//--- dump info ---
			newBS = 0;
		} else {
			newBS = pBuf->nowaddr - pBuf->last2card;
		}
		#endif

		if (newBS < bs2card_step) {
			//newBS = 0;
			bs2write = 0;
		} else {
			//newBS = bsmux_util_calc_align(newBS, bs2card_step, NMEDIAREC_ALIGN_ROUND);
			bs2write = bsmux_util_calc_align(newBS, bs2card_step, NMEDIAREC_ALIGN_ROUND);
		}
	}

	if (bs2write) {

		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			bsmux_util_get_param(id, BSMUXER_PARAM_FRONT_MOOV, &front_moov);
			if (front_moov) {
				BSMUX_MSG("[%d] front_moov flow before prepare 1st wrblk\r\n", id);
				bsmux_util_make_moov(id, 1);
			}
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, (UINT32)pBuf->last2card, (UINT64)bs2write, 0);
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, TRUE);
		} else {
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_NORMAL, (UINT32)pBuf->last2card, (UINT64)bs2write, 0);
		}
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, TRUE);

		pBuf->last2card += bs2write;
		pBuf->total2card += bs2write;

		if (pBuf->nowaddr == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] now(0x%X) achieve max(0x%X)\r\n", id, pBuf->nowaddr, pBuf->max);
			pBuf->nowaddr = pBuf->addr;
		}
		if (pBuf->last2card == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] last(0x%X) achieve max(0x%X)\r\n", id, pBuf->last2card, pBuf->max);
			pBuf->last2card = pBuf->addr;
		}
	} else {
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, FALSE);
	}

#if BSMUX_TEST_RESV_BUFF
#else
	if (filetype == MEDIA_FILEFORMAT_TS)
#endif
	{
		if (pBuf->nowaddr >= max_addr) {
			UINT32 remaining_size = 0, dst_pa, src_pa;
			remaining_size = newBS - bs2write; //not add to file Q
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_REAL2CARD, &lastWrite);

			#if 1
			if ((lastWrite >= pBuf->addr) && (lastWrite <= pBuf->end)
				&& ((pBuf->addr + remaining_size) > lastWrite)) {
			#else
			if ((pBuf->addr < lastWrite) && ((pBuf->addr + remaining_size) > lastWrite)) {
			#endif
				/**
					Rollback: if buffer start addr plus remaining_size large than lastWrite,
					it might cause buffer data overwrite.
				*/
				DBG_ERR("[%d] rollback pending\r\n", id);
				bsmux_util_dump_info(id);
				#if 1
				DBG_ERR("[%d] addr=0x%x remain=0x%x lastWrite=0x%x\r\n", id, pBuf->addr, remaining_size, lastWrite);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				#if _TOFIX_
				return E_NOMEM; ///< Insufficient memory
				#endif
				#endif
				//bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
			} else {
				{
					UINT8 *p1nd = (UINT8 *)(pBuf->nowaddr);
					UINT8 *p2nd = (UINT8 *)(pBuf->last2card);
					BSMUX_MEM_DUMP("0x%X => 0x%lx 0x%lx 0x%lx 0x%lx | 0x%X => 0x%lx 0x%lx 0x%lx 0x%lx\r\n",
						pBuf->last2card, *(p2nd), *(p2nd+1), *(p2nd+2), *(p2nd+3),
						pBuf->nowaddr, *(p1nd), *(p1nd+1), *(p1nd+2), *(p1nd+3));
				}
				if (remaining_size) {
					dst_pa = bsmux_util_get_buf_pa(id, pBuf->addr);
					src_pa = bsmux_util_get_buf_pa(id, pBuf->last2card);
					if (!dst_pa || !src_pa) {
						DBG_ERR("[%d] get dst/src addr zero\r\n", id);
						return E_NOEXS;
					}
					#if _TOFIX_
					if (bsmux_util_is_not_normal()) {
						DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
						return E_NOMEM; ///< Insufficient memory
					}
					#endif
					dst.phy_addr = dst_pa;
					dst.vrt_addr = pBuf->addr;
					src.phy_addr = src_pa;
					src.vrt_addr = pBuf->last2card;
					bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, remaining_size, mux_method); //rollback
				}
				{
					UINT8 *p1nd = (UINT8 *)(pBuf->nowaddr);
					UINT8 *p2nd = (UINT8 *)(pBuf->last2card);
					BSMUX_MEM_DUMP("0x%X => 0x%lx 0x%lx 0x%lx 0x%lx | 0x%X => 0x%lx 0x%lx 0x%lx 0x%lx\r\n",
						pBuf->last2card, *(p2nd), *(p2nd+1), *(p2nd+2), *(p2nd+3),
						pBuf->nowaddr, *(p1nd), *(p1nd+1), *(p1nd+2), *(p1nd+3));
				}
				pBuf->nowaddr = pBuf->addr;//rollback
				pBuf->nowaddr += remaining_size;
				pBuf->last2card = pBuf->addr; //rollback
				BSMUX_MEM_DUMP(">> rollback start_addr 0x%X max_addr 0x%X nowaddr 0x%X size=0x%x\r\n", pBuf->addr, max_addr, pBuf->nowaddr, remaining_size);
			}
		}
	}

	if (step2) {
		BSMUX_MEM_DUMP("step2 data copy=0x%X\r\n", step2);
		DBG_IND("[%d] copy-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
			pBuf->addr, pBuf->nowaddr, pBuf->last2card, pBuf->real2card, pBuf->end);

		if ((pBuf->real2card >= pBuf->addr) && (pBuf->real2card <= pBuf->end)) {
			// mdat buffer area
			if ((pBuf->addr + step2) > pBuf->real2card) {
				DBG_ERR("[%d][%d] slow2! buf=0x%X + step=0x%X > lastWrite=0x%X!\r\n", id, pBsq->uiType, pBuf->addr, step2, lastWrite);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				#if _TOFIX_
				return E_NOMEM; ///< Insufficient memory
				#endif
			}
		}

		// data copy
		pa = bsmux_util_get_buf_pa(id, pBuf->addr);
		if (!pa) {
			DBG_ERR("[%d] get buf addr zero\r\n", id);
			return E_NOEXS;
		}
		#if _TOFIX_
		if (bsmux_util_is_not_normal()) {
			DBG_DUMP("[%s-%d][%d][NOT_NORMAL]\r\n", __func__, __LINE__, id);
			return E_NOMEM; ///< Insufficient memory
		}
		#endif
		dst.phy_addr = pa;
		dst.vrt_addr = pBuf->addr;
		src.phy_addr = pBsq->uiBSMemAddr + step1;
		src.vrt_addr = pBsq->uiBSVirAddr + step1;
		datalen = bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, step2, mux_method);
		if (pBsq->uiIsKey) {
			BSMUX_MEM_DUMP("BSM:[%d] copy=0x%X size2=0x%X\r\n", id, pBuf->addr, datalen);
		}
		// update
		pBuf->nowaddr = pBuf->addr + datalen;

		DBG_IND("[%d] copy-(buf_addr=0x%x nowaddr=0x%x last2card=0x%x real2card=0x%x buf_end=0x%x)\r\n", id,
			pBuf->addr, pBuf->nowaddr, pBuf->last2card, pBuf->real2card, pBuf->end);
	}

	return E_OK;
}

//>> bs saveq control
ER bsmux_ctrl_bs_putall2card(UINT32 id)
{
	BSMUX_MEM_BUF *pBuf = NULL;
	UINT32 vfn = 0, sub_vfn = 0;
	UINT32 blk = 0, newBS = 0, newBSalign = 0;
	UINT32 frontPut = 0, bs2card_step;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}

	pBuf = bsmux_get_mem_buf(id);
	if (bsmux_util_is_null_obj((void *)pBuf)) {
		DBG_ERR("[%d] get mem buf null\r\n", id);
		return E_NOEXS;
	}
	bs2card_step = pBuf->blksize;

	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	if (vfn == 0 && sub_vfn == 0) {
		DBG_DUMP("NO updateHDR\r\n");
		DBG_DUMP("_upd[%d]-E22\r\n", id);
		bsmux_util_dump_info(id);
		return E_OK;
	}

	if (pBuf->nowaddr > pBuf->last2card) {
		blk = pBuf->nowaddr - pBuf->last2card;
	} else if (pBuf->nowaddr < pBuf->last2card) {
		DBG_DUMP("BSM:[%d] now(0x%X) < last(0x%X)\r\n", id, pBuf->nowaddr, pBuf->last2card);
		blk = (pBuf->max - pBuf->last2card) + (pBuf->nowaddr - pBuf->addr);
	} else {
		DBG_DUMP("NO putall2card\r\n");
		return E_OK;
	}

#if 0
	UINT32 bs2card_step = pBuf->blksize;
	newBS = bsmux_util_calc_align(blk, bs2card_step, NMEDIAREC_ALIGN_CEIL);
	if (newBS > 0) {

		if (pBuf->last2card + newBS > pBuf->max) {
			pBuf->nowaddr = pBuf->addr + (newBS - (pBuf->max - pBuf->last2card));
		} else {
			pBuf->nowaddr = (pBuf->last2card + newBS);
		}

#if 0
		bsmux_util_prepare_buf(id, BSMUX_BS2CARD_NORMAL, (UINT32)pBuf->last2card, (UINT64)newBS, 0);
#else
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			//bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, (UINT32)pBuf->last2card, (UINT64)newBS, 0);
			//bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, TRUE);
			DBG_DUMP("NO updateHDR\r\n");
			DBG_DUMP("_upd[%d]-E33\r\n", id);
			return E_OK;
		} else {
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_NORMAL, (UINT32)pBuf->last2card, (UINT64)newBS, 0);
		}
#endif

		pBuf->total2card += newBS;
		pBuf->last2card = pBuf->nowaddr;

		if (pBuf->nowaddr == pBuf->max) {
			pBuf->nowaddr = pBuf->addr;
		}
		if (pBuf->last2card == pBuf->max) {
			pBuf->last2card = pBuf->addr;
		}
	}
#else
	newBS = blk;
	if (newBS) {

		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			DBG_DUMP("[%d] putall2card frontPut\r\n", id);
			if (bsmux_util_is_not_normal()) {
				DBG_DUMP("NO putall2card because not_normal\r\n");
				return E_OK;
			}
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_FRONT, (UINT32)pBuf->last2card, (UINT64)newBS, 0);
			bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, TRUE);
		} else {
			bsmux_util_prepare_buf(id, BSMUX_BS2CARD_NORMAL, (UINT32)pBuf->last2card, (UINT64)newBS, 0);
		}
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, TRUE);

#if 1 //for buffer blk align
		newBSalign = bsmux_util_calc_align(blk, bs2card_step, NMEDIAREC_ALIGN_CEIL);
#endif

		if (pBuf->last2card + newBSalign > pBuf->max) {
			pBuf->nowaddr = pBuf->addr + (newBSalign - (pBuf->max - pBuf->last2card));
		} else {
			pBuf->nowaddr = (pBuf->last2card + newBSalign);
		}

		pBuf->total2card += newBS; //total2card realsize
		pBuf->last2card = pBuf->nowaddr;

		if (pBuf->nowaddr == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] now(0x%X) achieve max(0x%X)\r\n", id, pBuf->nowaddr, pBuf->max);
			pBuf->nowaddr = pBuf->addr;
		}
		if (pBuf->last2card == pBuf->max) {
			BSMUX_MEM_DUMP("BSM:[%d] last(0x%X) achieve max(0x%X)\r\n", id, pBuf->last2card, pBuf->max);
			pBuf->last2card = pBuf->addr;
		}
	} else {
		//blkPut
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_BLKPUT, FALSE);
	}
#endif

	return E_OK;
}

//>> bs saveq control (fileinfo)
ER bsmux_ctrl_bs_init_fileinfo(UINT32 id)
{
	BSMUX_SAVEQ_FILE_INFO *pFileinfo = NULL;

	pFileinfo = bsmux_get_file_info(id);
	if (bsmux_util_is_null_obj((void *)pFileinfo)) {
		DBG_ERR("[%d] get file info null\r\n", id);
		return E_NOEXS;
	}

	pFileinfo->vidCount = 0;
	pFileinfo->copyTotalCount = 0;
	pFileinfo->vidCopyCount = 0;
	pFileinfo->audCopyCount = 0;
	pFileinfo->nowFileOft = 0;
	pFileinfo->copyPos = 0;
	pFileinfo->gpsCopyCount = 0;
	pFileinfo->vidSec = 0;
	pFileinfo->headerbyte = 0;
	pFileinfo->vidCopySec = 0;
	pFileinfo->headerok = 0;
	pFileinfo->thumbok = 0;
	pFileinfo->extNum = 0;
	pFileinfo->extSec = 0;
	pFileinfo->subvidCopyCount = 0;
	pFileinfo->extNextSec = 0;
	pFileinfo->vidDropCount = 0;
	pFileinfo->audDropCount = 0;
	pFileinfo->subvidDropCount = 0;
	//pFileinfo->cut_blk_count = 0;
	pFileinfo->vid_put_count = 0;
	pFileinfo->aud_put_count = 0;
	pFileinfo->sub_put_count = 0;
	pFileinfo->rollack_sec = 0;
	pFileinfo->variable_rate = 0;
	{
		UINT32 i, num = BSMUX_MAX_META_NUM;
		for (i = 0; i < num; i++)
			pFileinfo->meta_copy_count[i] = 0;
	}
	return E_OK;
}

//>> bs saveq control (fileinfo)
ER bsmux_ctrl_bs_get_fileinfo(UINT32 id, UINT32 type, UINT32 *value)
{
	BSMUX_SAVEQ_FILE_INFO *pFileinfo = NULL;

	pFileinfo = bsmux_get_file_info(id);
	if (bsmux_util_is_null_obj((void *)pFileinfo)) {
		DBG_ERR("[%d] get file info null\r\n", id);
		return E_NOEXS;
	}

	switch (type) {
	case BSMUX_FILEINFO_TOTALVF:
		*value = pFileinfo->vidCopyCount;
		break;
	case BSMUX_FILEINFO_TOTALAF:
		*value = pFileinfo->audCopyCount;
		break;
	case BSMUX_FILEINFO_NOWSEC:
		*value = pFileinfo->vidCopySec;
		break;
	case BSMUX_FILEINFO_ADDVF:
		*value = pFileinfo->vidCount;
		break;
	case BSMUX_FILEINFO_COPYPOS:
		*value = pFileinfo->copyPos;
		break;
	case BSMUX_FILEINFO_TOTALGPS:
		*value = pFileinfo->gpsCopyCount;
		break;
	case BSMUX_FILEINFO_TOTAL:
		*value = pFileinfo->copyTotalCount;
		break;
	case BSMUX_FILEINFO_FRONTPUT:
		*value = pFileinfo->headerok;
		break;
	case BSMUX_FILEINFO_EXTNUM:
		*value = pFileinfo->extNum;
		break;
	case BSMUX_FILEINFO_EXTSEC:
		*value = pFileinfo->extSec;
		break;
	case BSMUX_FILEINFO_BLKPUT:
		*value = pFileinfo->blkPut;
		break;
	case BSMUX_FILEINFO_SUBVF:
		*value = pFileinfo->subvidCopyCount;
		break;
	case BSMUX_FILEINFO_THUMBPUT:
		*value = pFileinfo->thumbok;
		break;
	case BSMUX_FILEINFO_EXTNSEC:
		*value = pFileinfo->extNextSec;
		break;
	case BSMUX_FILEINFO_TOTALVF_DROP:
		*value = pFileinfo->vidDropCount;
		break;
	case BSMUX_FILEINFO_TOTALAF_DROP:
		*value = pFileinfo->audDropCount;
		break;
	case BSMUX_FILEINFO_SUBVF_DROP:
		*value = pFileinfo->subvidDropCount;
		break;
	case BSMUX_FILEINFO_CUTBLK:
		*value = pFileinfo->cut_blk_count;
		break;
	case BSMUX_FILEINFO_PUT_VF:
		*value = pFileinfo->vid_put_count;
		break;
	case BSMUX_FILEINFO_PUT_AF:
		*value = pFileinfo->aud_put_count;
		break;
	case BSMUX_FILEINFO_PUT_SUBVF:
		*value = pFileinfo->sub_put_count;
		break;
	case BSMUX_FILEINFO_ROLLSEC:
		*value = pFileinfo->rollack_sec;
		break;
	case BSMUX_FILEINFO_VAR_RATE:
		*value = pFileinfo->variable_rate;
		break;
	case BSMUX_FILEINFO_TOTALMETA:
	case BSMUX_FILEINFO_TOTALMETA2:
		{
			UINT32 index = type-BSMUX_FILEINFO_TOTALMETA;
			*value = pFileinfo->meta_copy_count[index];
		}
		break;
	default:
		break;
	}

	return E_OK;
}

//>> bs saveq control (fileinfo)
ER bsmux_ctrl_bs_set_fileinfo(UINT32 id, UINT32 type, UINT32 value)
{
	BSMUX_SAVEQ_FILE_INFO *pFileinfo = NULL;

	pFileinfo = bsmux_get_file_info(id);
	if (bsmux_util_is_null_obj((void *)pFileinfo)) {
		DBG_ERR("[%d] get file info null\r\n", id);
		return E_NOEXS;
	}

	switch (type) {
	case BSMUX_FILEINFO_TOTALVF:
		pFileinfo->vidCopyCount = value;
		break;
	case BSMUX_FILEINFO_TOTALAF:
		pFileinfo->audCopyCount = value;
		break;
	case BSMUX_FILEINFO_NOWSEC:
		pFileinfo->vidCopySec = value;
		break;
	case BSMUX_FILEINFO_ADDVF:
		pFileinfo->vidCount = value;
		break;
	case BSMUX_FILEINFO_COPYPOS:
		pFileinfo->copyPos = value;
		break;
	case BSMUX_FILEINFO_TOTALGPS:
		pFileinfo->gpsCopyCount = value;
		break;
	case BSMUX_FILEINFO_TOTAL:
		pFileinfo->copyTotalCount = value;
		break;
	case BSMUX_FILEINFO_FRONTPUT:
		pFileinfo->headerok = value;
		break;
	case BSMUX_FILEINFO_EXTNUM:
		pFileinfo->extNum = value;
		break;
	case BSMUX_FILEINFO_EXTSEC:
		pFileinfo->extSec = value;
		break;
	case BSMUX_FILEINFO_BLKPUT:
		pFileinfo->blkPut = value;
		break;
	case BSMUX_FILEINFO_SUBVF:
		pFileinfo->subvidCopyCount = value;
		break;
	case BSMUX_FILEINFO_THUMBPUT:
		pFileinfo->thumbok = value;
		break;
	case BSMUX_FILEINFO_EXTNSEC:
		pFileinfo->extNextSec = value;
		break;
	case BSMUX_FILEINFO_TOTALVF_DROP:
		pFileinfo->vidDropCount = value;
		break;
	case BSMUX_FILEINFO_TOTALAF_DROP:
		pFileinfo->audDropCount = value;
		break;
	case BSMUX_FILEINFO_SUBVF_DROP:
		pFileinfo->subvidDropCount = value;
		break;
	case BSMUX_FILEINFO_CUTBLK:
		pFileinfo->cut_blk_count = value;
		break;
	case BSMUX_FILEINFO_PUT_VF:
		pFileinfo->vid_put_count = value;
		break;
	case BSMUX_FILEINFO_PUT_AF:
		pFileinfo->aud_put_count = value;
		break;
	case BSMUX_FILEINFO_PUT_SUBVF:
		pFileinfo->sub_put_count = value;
		break;
	case BSMUX_FILEINFO_ROLLSEC:
		pFileinfo->rollack_sec = value;
		break;
	case BSMUX_FILEINFO_VAR_RATE:
		pFileinfo->variable_rate = value;
		break;
	case BSMUX_FILEINFO_TOTALMETA:
	case BSMUX_FILEINFO_TOTALMETA2:
		{
			UINT32 index = type-BSMUX_FILEINFO_TOTALMETA;
			pFileinfo->meta_copy_count[index] = value;
		}
		break;
	default:
		break;
	}

	return E_OK;
}

//>> bs saveq control (GOPs)
ER bsmux_ctrl_bs_add_lastI(UINT32 id, UINT32 bsqnum)
{
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 cnt, max;

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get queue null\r\n", id);
		return E_NOEXS;
	}

	bsmux_saveq_lock();
	{
		max = p_qinfo->gop_max;
		cnt = p_qinfo->gop_cnt;
		p_qinfo->gop_num[(cnt%max)] = bsqnum;
		p_qinfo->gop_cnt = cnt + 1;
	}
	bsmux_saveq_unlock();

	BSMUX_BSQ_IND("AddLast[%d][%d]=%d! cnt=%d!\r\n", id, cnt%max, bsqnum, cnt);
	return E_OK;
}

//>> bs saveq control (GOPs)
UINT32 bsmux_ctrl_bs_get_lastI(UINT32 id, UINT32 backSec)
{
	BSMUX_QUEUE_INFO *p_qinfo = NULL;
	UINT32 cnt, max, sec;
	UINT32 backcnt = 0, newindex = 0, wantI = 0;
	UINT32 rollback_duration = 0;

	p_qinfo = bsmux_get_queue_info(id);
	if (bsmux_util_is_null_obj((void *)p_qinfo)) {
		DBG_ERR("[%d] get queue null\r\n", id);
		return E_NOEXS;
	}
#if 0
	if (backSec) {
		backcnt = backSec -1; //rollback 0 and 1 would be the same
	}
#else
	if (backSec == 1) {
		backcnt = backSec -1; //rollback 0 and 1 would be the same
	} else {
		backcnt = backSec;
	}
#endif
	max = p_qinfo->gop_max;
	sec = p_qinfo->gop_sec;
	if (sec < backcnt) {
		DBG_WRN("gop_sec=%d backSec=%d\r\n", sec, backcnt);
		return E_SYS;
	}
	backcnt *= (max / sec); //gop per sec
	cnt = p_qinfo->gop_cnt;
	if (cnt <= backcnt) {
		newindex = 0; //from start
		rollback_duration = cnt / (max / sec);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_ROLLSEC, rollback_duration);
	} else {
		newindex = (cnt - backcnt) -1;  //-1 ,count and index difference is 1
		rollback_duration = backcnt / (max / sec);
		bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_ROLLSEC, rollback_duration);
	}
	wantI = p_qinfo->gop_num[(newindex%max)];

	DBG_DUMP("BSM:[%d] rollback %dsec\r\n", id, rollback_duration);
	BSMUX_BSQ_DUMP("GetLast[%d][%d]=%d! cnt=%d! org=%d!\r\n", id, newindex%max, wantI, newindex, cnt);
	return wantI;
}

//>> bs saveq control (dbg)
ER bsmux_ctrl_bs_dbg(UINT32 value)
{
	g_debug_bsq = value;
	return E_OK;
}

/* =========================================================================== */
/* =                          Main Ctrl Interface                            = */
/* =========================================================================== */

ER bsmux_ctrl_init(void)
{
	UINT32 num = BSMUX_MAX_CTRL_NUM;
	UINT32 id;

	// step: install id
	bsmux_install_id();

	// step: set status idle, action none
	bsmux_tsk_init();

	// step: init ctrl count
	if (g_bsmux_start_count) {
		DBG_WRN("g_bsmux_start_count not zero\r\n");
		for (id = 0; id < num; id++) {
			if (g_bsmux_mem_phy_addr[id] || g_bsmux_mem_vrt_addr[id]) {
				if (bsmux_ctrl_mem_free(id) != E_OK) {
					DBG_WRN("[%d] free mem might wrong\r\n", id);
					g_bsmux_mem_phy_addr[id] = 0;
					g_bsmux_mem_vrt_addr[id] = 0;
				}
			}
			g_bsmux_vid_keyframe_count[id] = 0;
			g_bsmux_bs_tbr_count[id] = 0;
			g_bsmux_sub_keyframe_count[id] = 0;
			g_bsmux_sub_bs_tbr_count[id] = 0;
		}
		bsmux_util_engine_close();
		g_bsmux_start_count = 0;
	}
	if (g_bsmux_open_count) {
		DBG_WRN("g_bsmux_open_count not zero\r\n");
		g_bsmux_open_count = 0;
	}

	return E_OK;
}

ER bsmux_ctrl_open(UINT32 id)
{
#if 0
	if (avl_check_available(__xstring(__section_name__)) != TRUE) {
		DBG_FATAL("Not Support.\r\n");
		vos_util_delay_ms(100);         // add delay to show dbg_fatal message in linux
		vos_debug_halt();
	}
#endif

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out of range\r\n", id);
		return E_ID;
	}

	// step: start tsk
	if (g_bsmux_open_count == 0) {
		bsmux_tsk_start();
	}
	g_bsmux_open_count++;

	return E_OK;
}

ER bsmux_ctrl_start(UINT32 id)
{
	ER ret;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out of range\r\n", id);
		return E_ID;
	}

	// step: plugin util/makerobj & set fstype
	ret = bsmux_util_plugin(id);
	if (ret != E_OK) {
		DBG_ERR("[%d] plugin util fail\r\n", id);
		return ret;
	}
	bsmux_util_plugin_clean(id);

	// step: alloc/check mem
	ret = bsmux_ctrl_mem_alloc(id);
	if (ret != E_OK) {
		DBG_ERR("[%d] alloc mem fail\r\n", id);
		return ret;
	}

	// step: set got first I (-1)
	g_bsmux_vid_keyframe_count[id] = -1;
	g_bsmux_sub_keyframe_count[id] = -1;

	// step: wait ready
	bsmux_util_wait_ready(id);

	// step: show setting & put version
	bsmux_util_show_setting(id);

	// step: set status init and assign action
	bsmux_tsk_status_init(id);
	// step: set status resume
	ret = bsmux_tsk_status_resume(id);
	if (ret != E_OK) {
		DBG_ERR("set status idle\r\n");
		return ret;
	}
	if (g_bsmux_start_count == 0) {
		bsmux_util_set_result(BSMUXER_RESULT_NORMAL);
		bsmux_util_engine_open();
		g_bsmux_msg_count = 0;
	}
	g_bsmux_start_count++;

	return E_OK;
}

ER bsmux_ctrl_stop(UINT32 id)
{
	ER ret;

	DBG_DUMP("BSM:[%d] stop -B\r\n", id);

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out of range\r\n", id);
		return E_ID;
	}

	// step: set need to stop
	bsmux_tsk_trig_getall(id);

	// step: wait file end
	//bsmux_tsk_wait_getall(id);
	bsmux_util_wait_ready(id);

	// step: free mem
	ret = bsmux_ctrl_mem_free(id);
	if (ret != E_OK) {
		DBG_ERR("[%d] free mem fail\r\n", id);
		return ret;
	}

	// step: reset something...
	g_bsmux_vid_keyframe_count[id] = 0;
	g_bsmux_bs_tbr_count[id] = 0;
	g_bsmux_sub_keyframe_count[id] = 0;
	g_bsmux_sub_bs_tbr_count[id] = 0;

	g_bsmux_start_count--;
	if (g_bsmux_start_count == 0) {
		bsmux_util_set_result(BSMUXER_RESULT_NORMAL);
		bsmux_util_engine_close();
		g_bsmux_msg_count = 0;
	}

	DBG_DUMP("BSM:[%d] stop -E\r\n", id);

	return E_OK;
}

ER bsmux_ctrl_close(UINT32 id)
{
	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		return E_ID;
	}
	#if 0
	// step: set status suspend
	bsmux_tsk_status_idle(id);
	#endif
	// step: stop tsk
	g_bsmux_open_count --;
	if (g_bsmux_open_count == 0) {
		bsmux_tsk_stop();
	}

	return E_OK;
}

ER bsmux_ctrl_uninit(void)
{
	UINT32 num = BSMUX_MAX_CTRL_NUM;
	UINT32 id;

	// step: uninit ctrl count
	if (g_bsmux_start_count) {
		DBG_WRN("g_bsmux_start_count not zero\r\n");
		for (id = 0; id < num; id++) {
			if (g_bsmux_mem_phy_addr[id] || g_bsmux_mem_vrt_addr[id]) {
				if (bsmux_ctrl_mem_free(id) != E_OK) {
					DBG_WRN("[%d] free mem might wrong\r\n", id);
					g_bsmux_mem_phy_addr[id] = 0;
					g_bsmux_mem_vrt_addr[id] = 0;
				}
			}
			g_bsmux_vid_keyframe_count[id] = 0;
			g_bsmux_bs_tbr_count[id] = 0;
			g_bsmux_sub_keyframe_count[id] = 0;
			g_bsmux_sub_bs_tbr_count[id] = 0;
		}
		bsmux_util_engine_close();
		g_bsmux_start_count = 0;
	}
	if (g_bsmux_open_count) {
		DBG_WRN("g_bsmux_open_count not zero\r\n");
		g_bsmux_open_count = 0;
	}

	bsmux_tsk_destroy();
	bsmux_uninstall_id();
	return E_OK;
}

ER bsmux_ctrl_in(UINT32 id, void *p_data)
{
	BSMUXER_DATA *pBSBuf = (BSMUXER_DATA *)p_data;
	BSMUX_SAVEQ_BS_INFO block = {0};
	UINT32 tbr = 0, vfr = 0, gop = 0;
	UINT32 aud_en = 0, adts_bytes = 0;
	UINT32 wanted = 0, ratio = 0;
	UINT32 nalu_va;
	#if BSMUX_TEST_NALU
	UINT32 nalu_pa;
	#endif
	UINT32 mux_method = 0;
	BSMUX_MUX_INFO dst = {0};
	BSMUX_MUX_INFO src = {0};
	ER status = E_OK;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id %d out range \r\n", id);
		status = E_ID;
		goto label_ctrl_in_exit;
	}

	if (bsmux_util_is_null_obj((void *)pBSBuf)) {
		DBG_ERR("[%d] input data null\r\n", id);
		status = E_NOEXS;
		goto label_ctrl_in_exit;
	}

	// bs info
	block.uiBSMemAddr = pBSBuf->bSMemAddr;
	block.uiBSSize    = pBSBuf->bSSize;
	block.uiType      = pBSBuf->type;
	block.uiIsKey     = pBSBuf->isKey;
	block.uiBSVirAddr = pBSBuf->bSVirAddr;
	block.uibufid     = pBSBuf->bufid;
	block.uiPortID    = id;
	block.uiTimeStamp = pBSBuf->bSTimeStamp;

#if BSMUX_TEST_MULTI_TILE
	// set TileOft
	if (pBSBuf->naluOftMemAddr && pBSBuf->naluOftVirAddr) {
		block.uiTOftMemAddr = pBSBuf->naluOftMemAddr;
		block.uiTOftVirAddr = pBSBuf->naluOftVirAddr;
	}
#endif

	bsmux_util_get_param(id, BSMUXER_PARAM_MUXMETHOD, &mux_method);

	// enqueue & trigger tsk
	switch (block.uiType) {
	case BSMUX_TYPE_VIDEO:
		{
			if (block.uiIsKey) {
				if (g_bsmux_vid_keyframe_count[id] == (-1)) {
					g_bsmux_vid_keyframe_count[id] = 0;
				}
				// set NALU
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &nalu_va);
				if (pBSBuf->nalusize && nalu_va) {
					#if BSMUX_TEST_NALU
#if BSMUX_TEST_MULTI_TILE
					UINT32 type = 0;
					bsmux_util_get_param(id, BSMUXER_PARAM_VID_CODECTYPE, &type);
					if (type == MEDIAVIDENC_H265) { // tile
						UINT32 cnt;
						UINT32 nalunum = pBSBuf->naluNum;
						UINT32 cntsize = 0, offset = 0;
						bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &nalu_va);
						nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
						//vps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluVPSSize, mux_method);
							cntsize += pBSBuf->naluVPSSize;
						}
						offset += pBSBuf->naluVPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_VPSSIZE, pBSBuf->naluVPSSize);
						//sps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluSPSSize, mux_method);
							cntsize += pBSBuf->naluSPSSize;
						}
						offset += pBSBuf->naluSPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SPSSIZE, pBSBuf->naluSPSSize);
						//pps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluPPSSize, mux_method);
							cntsize += pBSBuf->naluPPSSize;
						}
						offset += pBSBuf->naluPPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_PPSSIZE, pBSBuf->naluPPSSize);
						//check
						if (cntsize > BSMUX_VIDEO_DESC_SIZE) {
							DBG_ERR("[%d] descsize(0x%x)(%d) too big\r\n", id, cntsize, nalunum);
						} else {
							DBG_IND("[%d] descsize(0x%x)(%d)\r\n", id, cntsize, nalunum);
							bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, cntsize);
							bsmux_util_set_param(id, BSMUXER_PARAM_VID_NALUNUM, nalunum);
						}
						//bsmux_util_dump_buffer((char *)nalu_va, cntsize);
						//update
						for (cnt = 0; cnt < (nalunum - 1); cnt++) {
							block.uiBSSize += offset;
						}
					} else {
						bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &nalu_va);
						nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
						dst.phy_addr = nalu_pa;
						dst.vrt_addr = nalu_va;
						src.phy_addr = pBSBuf->naluaddr;
						src.vrt_addr = pBSBuf->naluVirAddr;
						bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->nalusize, mux_method);
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, pBSBuf->nalusize);
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_NALUNUM, 1); //reset 1
					}
#else
					bsmux_util_get_param(id, BSMUXER_PARAM_VID_DESCADDR, &nalu_va);
					nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
					dst.phy_addr = nalu_pa;
					dst.vrt_addr = nalu_va;
					src.phy_addr = pBSBuf->naluaddr;
					src.vrt_addr = pBSBuf->naluVirAddr;
					bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->nalusize, mux_method);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, pBSBuf->nalusize);
#endif
					#else
					nalu_va = bsmux_util_get_buf_va(id, pBSBuf->naluaddr);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCADDR, nalu_va);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_DESCSIZE, pBSBuf->nalusize);
					#endif
				} else {
					DBG_ERR("[%d] key frame nalu null\r\n", id);
				}
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &tbr);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
				if (vfr == 80) {
					gop = 20;
				} else if (vfr == 50) {
					gop = 25;
				}
				if (vfr < gop) {
					gop = vfr;
				}
				// check tbr
				wanted = tbr / (vfr / gop);
				ratio = g_bsmux_bs_tbr_count[id] * 10 / wanted;
				if (ratio > 15) {
					DBG_DUMP("BSM:[%d] key TBR=%ld KB, wanted=%ld KB ra=%d\r\n", id, g_bsmux_bs_tbr_count[id] / 1024, wanted / 1024, ratio);
				}
				if (g_debug_bitrate) {
					DBG_DUMP("tbr %d vfr %d gop %d totalbit %d\r\n", tbr, vfr, gop, g_bsmux_bs_tbr_count[id]);
				}
				g_bsmux_bs_tbr_count[id] = 0;
				// check vfr
				if ((g_bsmux_vid_keyframe_count[id] != (int)gop) && (g_bsmux_vid_keyframe_count[id] != 0)) {
					DBG_DUMP("vfr %d gop %d keycount %d\r\n", vfr, gop, g_bsmux_vid_keyframe_count[id]);
					bsmux_cb_result(BSMUXER_RESULT_GOPMISMATCH, id);
				}
				g_bsmux_vid_keyframe_count[id] = 0;
			}
			if (g_bsmux_vid_keyframe_count[id] == (-1)) {
				BSMUX_BSQ_IND("[%d] Drop since not got first I\r\n", id);
				goto label_ctrl_in_exit;
			}
			// check pBsq (header or tag)
			if (bsmux_util_check_tag(id, &block) == FALSE) {
				DBG_ERR("InWrong[%d][0x%lx][0x%lx]\r\n", id, block.uiBSMemAddr, block.uiBSSize);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				goto label_ctrl_in_exit;
			}
			g_bsmux_bs_tbr_count[id] += block.uiBSSize;
			g_bsmux_vid_keyframe_count[id] ++;
			bsmux_ctrl_bs_enqueue(id, &block);
			bsmux_tsk_bs_in();
			bsmux_util_chk_frm_sync(id, p_data);
		}
		break;
	case BSMUX_TYPE_SUBVD:
		{
			if (block.uiIsKey) {
				if (g_bsmux_sub_keyframe_count[id] == (-1)) {
					g_bsmux_sub_keyframe_count[id] = 0;
				}
				// set NALU
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &nalu_va);
				if (pBSBuf->nalusize && nalu_va) {
					#if BSMUX_TEST_NALU
#if BSMUX_TEST_MULTI_TILE
					UINT32 type = 0;
					bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_CODECTYPE, &type);
					if (type == MEDIAVIDENC_H265) { // tile
						UINT32 cnt;
						UINT32 nalunum = pBSBuf->naluNum;
						UINT32 cntsize = 0, offset = 0;
						bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &nalu_va);
						nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
						//vps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluVPSSize, mux_method);
							cntsize += pBSBuf->naluVPSSize;
						}
						offset += pBSBuf->naluVPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_VPSSIZE, pBSBuf->naluVPSSize);
						//sps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluSPSSize, mux_method);
							cntsize += pBSBuf->naluSPSSize;
						}
						offset += pBSBuf->naluSPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_SPSSIZE, pBSBuf->naluSPSSize);
						//pps
						for (cnt = 0; cnt < nalunum; cnt++)
						{
							dst.phy_addr = (nalu_pa + cntsize);
							dst.vrt_addr = (nalu_va + cntsize);
							src.phy_addr = (pBSBuf->naluaddr + offset);
							src.vrt_addr = (pBSBuf->naluVirAddr + offset);
							bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->naluPPSSize, mux_method);
							cntsize += pBSBuf->naluPPSSize;
						}
						offset += pBSBuf->naluPPSSize;
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_PPSSIZE, pBSBuf->naluPPSSize);
						//check
						if (cntsize > BSMUX_VIDEO_DESC_SIZE) {
							DBG_ERR("[%d] sub descsize(0x%x)(%d) too big\r\n", id, cntsize, nalunum);
						} else {
							DBG_IND("[%d] sub descsize(0x%x)(%d)\r\n", id, cntsize, nalunum);
							bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, cntsize);
							bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_NALUNUM, nalunum);
						}

						//bsmux_util_dump_buffer((char *)nalu_va, cntsize);

						for (cnt = 0; cnt < (nalunum - 1); cnt++) {
							block.uiBSSize += offset;
						}

					} else {
						bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &nalu_va);
						nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
						dst.phy_addr = nalu_pa;
						dst.vrt_addr = nalu_va;
						src.phy_addr = pBSBuf->naluaddr;
						src.vrt_addr = pBSBuf->naluVirAddr;
						bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->nalusize, mux_method);
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, pBSBuf->nalusize);
						bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_NALUNUM, 1); //reset 1
					}
#else
					bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, &nalu_va);
					nalu_pa = bsmux_util_get_buf_pa(id, nalu_va);
					dst.phy_addr = nalu_pa;
					dst.vrt_addr = nalu_va;
					src.phy_addr = pBSBuf->naluaddr;
					src.vrt_addr = pBSBuf->naluVirAddr;
					bsmux_util_memcpy((VOID *)&dst, (VOID *)&src, pBSBuf->nalusize, mux_method);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, pBSBuf->nalusize);
#endif
					#else
					nalu_va = bsmux_util_get_buf_va(id, pBSBuf->naluaddr);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCADDR, nalu_va);
					bsmux_util_set_param(id, BSMUXER_PARAM_VID_SUB_DESCSIZE, pBSBuf->nalusize);
					#endif
				} else {
					DBG_ERR("[%d] sub key frame nalu null\r\n", id);
				}
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_TBR, &tbr);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &vfr);
				bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_GOP, &gop);
				if (vfr == 80) {
					gop = 20;
				} else if (vfr == 50) {
					gop = 25;
				}
				if (vfr < gop) {
					gop = vfr;
				}
				// check tbr
				wanted = tbr / (vfr / gop);
				ratio = g_bsmux_sub_bs_tbr_count[id] * 10 / wanted;
				if (ratio > 15) {
					DBG_DUMP("BSM:[%d] sub key TBR=%ld KB, wanted=%ld KB ra=%d\r\n", id, g_bsmux_sub_bs_tbr_count[id] / 1024, wanted / 1024, ratio);
				}
				if (g_debug_bitrate) {
					DBG_DUMP("sub tbr %d vfr %d gop %d totalbit %d\r\n", tbr, vfr, gop, g_bsmux_sub_bs_tbr_count[id]);
				}
				g_bsmux_sub_bs_tbr_count[id] = 0;
				// check vfr
				if ((g_bsmux_sub_keyframe_count[id] != (int)gop) && (g_bsmux_sub_keyframe_count[id] != 0)) {
					DBG_DUMP("sub vfr %d gop %d keycount %d\r\n", vfr, gop, g_bsmux_sub_keyframe_count[id]);
					bsmux_cb_result(BSMUXER_RESULT_GOPMISMATCH, id);
				}
				g_bsmux_sub_keyframe_count[id] = 0;
			}
			if (g_bsmux_sub_keyframe_count[id] == (-1)) {
				BSMUX_BSQ_IND("[%d] Drop since not got first sub I\r\n", id);
				goto label_ctrl_in_exit;
			}
			// check pBsq (header or tag)
			if (bsmux_util_check_tag(id, &block) == FALSE) {
				DBG_ERR("SUB-InWrong[%d][0x%lx][0x%lx]\r\n", id, block.uiBSMemAddr, block.uiBSSize);
				bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				goto label_ctrl_in_exit;
			}
			g_bsmux_sub_bs_tbr_count[id] += block.uiBSSize;
			g_bsmux_sub_keyframe_count[id] ++;
			bsmux_ctrl_bs_enqueue(id, &block);
			bsmux_tsk_bs_in();
			#if 0
			bsmux_util_chk_frm_sync(id, p_data);
			#endif
		}
		break;
	case BSMUX_TYPE_AUDIO:
		{
			if (g_bsmux_vid_keyframe_count[id] == (-1)) {
				BSMUX_BSQ_IND("[%d] Drop since not got first I\r\n", id);
				goto label_ctrl_in_exit;
			}
			// check audio
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_ON, &aud_en);
			bsmux_util_get_param(id, BSMUXER_PARAM_AUD_EN_ADTS, &adts_bytes);
			if (aud_en == 0) {
				goto label_ctrl_in_exit;
			}
			if (adts_bytes) {
				block.uiBSMemAddr += adts_bytes;
				block.uiBSVirAddr += adts_bytes;
				block.uiBSSize -= adts_bytes;
			}

			bsmux_ctrl_bs_enqueue(id, &block);
			bsmux_tsk_bs_in();
			bsmux_util_chk_frm_sync(id, p_data);
		}
		break;
	case BSMUX_TYPE_THUMB:
		{
			if (block.uiBSSize < bsmux_util_calc_reserved_size(id)) {
				bsmux_util_add_thumb(id, (void *)&block);
			} else {
				DBG_DUMP("THUMB too big !!0x%lx 0x%lx\r\n", block.uiBSMemAddr, block.uiBSSize);
			}
			goto label_ctrl_in_exit;
		}
		break;
	case BSMUX_TYPE_RAWEN:
		{
			DBG_DUMP("raw unsupported now\r\n");
			goto label_ctrl_in_exit;
		}
		break;
	case BSMUX_TYPE_GPSIN:
		{
			UINT32 gps_on = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_GPS_ON, &gps_on);
			if (gps_on) {
				bsmux_util_add_gps(id, (void *)&block);
			} else {
				status = E_PAR;
			}
			goto label_ctrl_in_exit;
		}
		break;
	case BSMUX_TYPE_MTAIN:
		{
			UINT32 meta_on = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_META_ON, &meta_on);
			if (meta_on) {
				/* todo: check sign is reg or not */
				bsmux_util_add_meta(id, (void *)&block);
			} else {
				status = E_PAR;
			}
			goto label_ctrl_in_exit;
		}
		break;
	default:
		DBG_DUMP("put data type %d\r\n", block.uiType);
		break;
	}

	// callback event: PUTBSDONE
	status = bsmux_cb_event(BSMUXER_CBEVENT_PUTBSDONE, p_data, id);
	if (status != E_OK) {
		DBG_ERR("[%d] cb event fail\r\n", id);
		status = E_SYS;
	}

label_ctrl_in_exit:
	return status;
}

ER bsmux_ctrl_extend(UINT32 id)
{
	UINT32 vSec, seamlessSec = 0, vfr = 0, vfn = 0;
	UINT32 unit = 0, max_num = 0, enable = 0;
	UINT32 extNum = 0, extSec = 0;
	UINT32 drop_vfn = 0;

	if (BSMUX_CTRL_IDX == id) {
		UINT32 idx;
		UINT32 tmpSec = 0;
		UINT32 cut_blk = 0;
		UINT32 pos_front = 0, pos_last = 0;
		UINT32 is_extend_next = 0;
		UINT32 ret_val = 0;

		// round 1: check
		for (idx = 0; idx < BSMUX_MAX_CTRL_NUM; idx++) {
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_UNIT, &unit);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_MAX_NUM, &max_num);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, &enable);

			if (!enable) { // not active
				continue;
			}

			bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, &extNum);
			if (extNum >= max_num) {
				DBG_ERR("BSM:[%d] Extend times full!!!\r\n", idx);
				return E_SYS;
			}

			bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_NOWSEC, &tmpSec);
			if (tmpSec == seamlessSec) {
				tmpSec = 0; //reset if cutfile
				is_extend_next = 1;
			}
			DBG_DUMP("[%d] NOWSEC=%d\r\n", idx, tmpSec);
			if (tmpSec >= (seamlessSec - 3)) { // busy, extend next
				tmpSec += unit;
				pos_last = ((tmpSec > pos_last) ? tmpSec : pos_last);
				extSec = ((pos_last > extSec) ? pos_last : extSec);
				DBG_DUMP("[%d] busy (tmpSec=%d pos_last=%d extSec=%d)\r\n", idx, tmpSec, pos_last, extSec);
			} else if (tmpSec <= 3) { // already cutfile
				tmpSec += unit;
				pos_front = ((tmpSec > pos_front) ? tmpSec : pos_front);
				extSec = ((extSec) ? extSec : pos_front); //keep extend sec
				DBG_DUMP("[%d] cutfile (tmpSec=%d pos_front=%d extSec=%d)\r\n", idx, tmpSec, pos_front, extSec);
			} else { // normal
				tmpSec += unit;
				extSec = ((tmpSec > extSec) ? tmpSec : extSec); //keep extend sec
				DBG_DUMP("[%d] normal (tmpSec=%d pos_last=%d extSec=%d)\r\n", idx, tmpSec, pos_last, extSec);
			}
		}

		if (pos_last && pos_front) {
			is_extend_next = 1;
			extSec = pos_front;
		}

		// round 2: set
		for (idx = 0; idx < BSMUX_MAX_CTRL_NUM; idx++) {
			bsmux_util_get_param(idx, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_UNIT, &unit);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_MAX_NUM, &max_num);
			bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, &enable);

			if (!enable) { // not active
				continue;
			}

			bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_NOWSEC, &tmpSec);
			if (is_extend_next) {
				if (tmpSec >= (seamlessSec - 3)) { // busy, extend next
					extNum = 1;
					bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, extNum);
					bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTNSEC, extSec);
					DBG_DUMP("[next][%d] extend next, NEW sec = %d !\r\n", idx, extSec);
					ret_val += (1<<idx);
				} else { // normal
					bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, &extNum);
					extNum += 1;
					bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, extNum);
					bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTSEC, extSec);
					DBG_DUMP("[next][%d] extend num = %d, NEW sec = %d !\r\n", idx, extNum, extSec);
				}
			} else { // normal
				bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, &extNum);
				extNum += 1;
				bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTNUM, extNum);
				bsmux_ctrl_bs_set_fileinfo(idx, BSMUX_FILEINFO_EXTSEC, extSec);
				DBG_DUMP("[normal][%d] extend num = %d, NEW sec = %d !\r\n", idx, extNum, extSec);
				bsmux_ctrl_bs_get_fileinfo(idx, BSMUX_FILEINFO_CUTBLK, &cut_blk);
				if (cut_blk) {
					DBG_DUMP("[normal][%d] extend cut blk num = %d!\r\n", idx, cut_blk);
					ret_val += (1<<idx);
				}
			}
		}

		if (ret_val > 0) {
			return ret_val;
		}

		return E_OK;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_UNIT, &unit);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_MAX_NUM, &max_num);
	bsmux_util_get_param(id, BSMUXER_PARAM_EXTINFO_ENABLE, &enable);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
	if (drop_vfn) {
		vfn += drop_vfn;
	}
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_EXTNUM, &extNum);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_EXTSEC, &extSec);

	if (!enable) {
		DBG_ERR("Extend disabled!!!\r\n");
		return E_OBJ;
	}

	if (extNum >= max_num) {
		DBG_ERR("Extend times full!!!\r\n");
		return E_SYS;
	}

	extNum += 1;
	vSec = vfn / vfr;
	extSec = vSec + unit;
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTNUM, extNum);
	bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTSEC, extSec);

	DBG_DUMP("extend num = %d, NEW sec = %d !\r\n", extNum, extSec);
	return E_OK;
}

ER bsmux_ctrl_pause(UINT32 id)
{
	if (BSMUX_CTRL_IDX == id) {
		DBG_DUMP("BSM: ctrl unsupported\r\n");
		return E_OK;
	}

	bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, 1);
	bsmux_tsk_trig_cutfile(id);
	return E_OK;
}

ER bsmux_ctrl_resume(UINT32 id)
{
	UINT32 emron = 0, rollsec = 0;
	UINT32 emrloop = 0, strgbuf = 0;

	if (BSMUX_CTRL_IDX == id) {
		DBG_DUMP("BSM: ctrl unsupported\r\n");
		return E_OK;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);

	if (emron) {
		//do rollback
		bsmux_ctrl_bs_rollback(id, rollsec, 0);
		//run
		bsmux_tsk_status_run(id);
		//chk
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, id);
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, 0);
		if (emrloop && strgbuf) {
			bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUT, 1);
			bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_HDR, 1);
		}
	} else {
		//do rollback
		bsmux_ctrl_bs_rollback(id, 1, 0); //get lastI
		//chk
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, id);
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, 0);
		//resume
		bsmux_tsk_status_resume(id);
	}
	return E_OK;
}

ER bsmux_ctrl_emergency(UINT32 id)
{
	ER result = E_OK;
	if (BSMUX_CTRL_IDX == id) {
		UINT32 emr_id, pause_id = 0;
		UINT32 emr_on = 0, now_sec = 0, seamless_sec = 0, rollback_sec = 0;
		UINT32 pos_front = 0, pos_last = 0, retry_cnt = 10;
		ER ret;

retry_check:
		// round 1: check
		for (emr_id = 0; emr_id < BSMUX_MAX_CTRL_NUM; emr_id++) {
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_PAUSE_ID, &pause_id);

			if (!emr_on) { // emr not active
				continue;
			}

			if (emr_id == pause_id) { // pause not active, not need to check
				continue;
			}

			bsmux_util_get_param(pause_id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamless_sec);
			bsmux_ctrl_bs_get_fileinfo(pause_id, BSMUX_FILEINFO_NOWSEC, &now_sec);
			if (now_sec == seamless_sec) {
				now_sec = 0; //reset if cutfile
				pos_front = 1;
			}
			DBG_IND("[%d] NOWSEC=%d\r\n", pause_id, now_sec);
			if (now_sec >= (seamless_sec - 3)) { // ready to cutfile
				pos_last = 1;
				DBG_IND("[%d] busy (now_sec=%d pos_last=%d)\r\n", pause_id, now_sec, pos_last);
			} else if (now_sec <= 3) { // already cutfile
				pos_front = 1;
				DBG_IND("[%d] cutfile (now_sec=%d pos_front=%d)\r\n", pause_id, now_sec, pos_front);
			} else { // normal
				DBG_IND("[%d] normal (now_sec=%d pos_last=%d)\r\n", pause_id, now_sec, pos_last);
			}
		}

		if (pos_last && pos_front) {
			DBG_IND("pos_last=%d pos_front=%d\r\n", pos_last, pos_front);
			vos_util_delay_us(300*1000);
			retry_cnt--;
			if (retry_cnt == 0) {
				DBG_DUMP("emergency busy. try to trig again.\r\n");
				result = E_TMOUT;
				goto exit;
			}
			DBG_DUMP("emergency busy.\r\n");
			goto retry_check;
		}

		// round 1-1: check rb sts
		for (emr_id = 0; emr_id < BSMUX_MAX_CTRL_NUM; emr_id++) {
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollback_sec);

			if (!emr_on) { // emr not active
				continue;
			}

			//check rollback. (rollback_sec+1: boundary case protection)
			ret = bsmux_ctrl_bs_rollback(emr_id, rollback_sec+1, 1);
			if (E_OK != ret) {
				DBG_ERR("[%d] rb sts %d failed\r\n", emr_id, rollback_sec);
			}

			if (ret) {
				result = ret;
				goto exit;
			}
		}

		// round 2: set pause
		for (emr_id = 0; emr_id < BSMUX_MAX_CTRL_NUM; emr_id++) {
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_PAUSE_ID, &pause_id);

			if (!emr_on) { // emr not active
				continue;
			}

			if (emr_id == pause_id) { // pause not active, not need to check
				continue;
			}

			bsmux_ctrl_bs_get_fileinfo(pause_id, BSMUX_FILEINFO_NOWSEC, &now_sec);
			DBG_DUMP("[%d] sec=%d\r\n", pause_id, now_sec);

			bsmux_util_set_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_ON, 1);
			bsmux_tsk_trig_cutfile(pause_id);
		}

		// round 3: set emr
		for (emr_id = 0; emr_id < BSMUX_MAX_CTRL_NUM; emr_id++) {
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
			bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollback_sec);

			if (!emr_on) { // emr not active
				continue;
			}

			bsmux_tsk_still(emr_id);

			//do rollback
			ret = bsmux_ctrl_bs_rollback(emr_id, rollback_sec, 0);
			if (E_OK != ret) {
				DBG_ERR("[%d] rollback %d failed\r\n", emr_id, rollback_sec);
				goto err_emr;
			}
			//run
			bsmux_tsk_status_run(emr_id);

err_emr:
			bsmux_tsk_awake(emr_id);
			if (ret) {
				result = ret;
			}
		}
	} else {
		//single path
		UINT32 emr_id = id, pause_id = 0;
		bsmux_util_get_param(emr_id, BSMUXER_PARAM_FILE_PAUSE_ID, &pause_id);
		result = bsmux_ctrl_trig_emr(emr_id, pause_id);
	}
exit:
	return result;
}

ER bsmux_ctrl_trig_emr(UINT32 id, UINT32 pause_id)
{
	UINT32 emron = 0, rollsec = 0;
	UINT32 emrloop = 0, strgbuf = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_ROLLBACKSEC, &rollsec);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);

	if (!emron) {
		DBG_DUMP("[%d] emr off\r\n", id);
		return E_OK;
	}

	if (emrloop == BSMUX_STEP_EMRLOOP) {
		DBG_DUMP("[%d] emr loop\r\n", id);
		return E_OK;
	}

	//do rollback
	if (emrloop && strgbuf) {
		DBG_IND("BSM:[%d][STRGBUF] no need to rollback\r\n", id);
	} else {

		// set still
		bsmux_tsk_still(id);

		if (E_OK != bsmux_ctrl_bs_rollback(id, rollsec, 0)) {
			DBG_ERR("[%d] rollback %d failed\r\n", id, rollsec);

			// set awake
			bsmux_tsk_awake(id);

			return E_SYS;
		}
	}

	//run
	bsmux_tsk_status_run(id);

	if (emrloop && strgbuf) {
		DBG_IND("BSM:[%d][STRGBUF] no need to awake\r\n", id);
	} else {
		// set awake
		bsmux_tsk_awake(id);
	}

	//chk pause_id
	if (pause_id != id) {
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, pause_id);
		bsmux_util_set_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_ON, 1);
		bsmux_tsk_trig_cutfile(pause_id);
	} else {
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, id);
		bsmux_util_set_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_ON, 0);
	}

	if (emrloop && strgbuf) {
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_CUT, 1);
		bsmux_util_set_param(id, BSMUXER_PARAM_STRGBUF_HDR, 1);
		DBG_IND("[%d][STRGBUF] CUT(1) HDR(1)\r\n", id);
	}

	return E_OK;
}

ER bsmux_ctrl_trig_cutnow(UINT32 id)
{
	UINT32 seamlessSec = 0, vfr = 0, vfn = 0;
	UINT32 sec = 0;

	if (BSMUX_CTRL_IDX == id) {
		DBG_DUMP("BSM: ctrl unsupported\r\n");
		return E_OK;
	}

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_SEAMLESSSEC, &seamlessSec);
	bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
	bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
	sec = vfn / vfr;
	if ((sec <= 3) || (sec >= (seamlessSec - 3))) {
		DBG_DUMP("BSM[%d]: sec %d NOCUT\r\n", sec, id);
		return E_OK;
	}

	bsmux_tsk_trig_cutfile(id);

	return E_OK;
}

