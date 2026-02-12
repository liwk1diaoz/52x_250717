#if defined(__LINUX)
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#elif defined(__FREERTOS)
#endif
#include "ctl_ipp_drv.h"
#include "ctl_ipp_dbg.h"
#include "kdf_ipp_int.h"
#include "kdf_ipp_id_int.h"
#include "ctl_ipp_id_int.h"
#include "ctl_ipp_int.h"
#include "ctl_ipp_isp_int.h"
#include "ipp_event_id_int.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define CTL_IPP_NEW_INIT_FLOW (DISABLE)
#else
#define CTL_IPP_NEW_INIT_FLOW (ENABLE)
#endif

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ctl_ipp_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{

	return 0;
}

int nvt_ctl_ipp_drv_init(MODULE_INFO *pmodule_info)
{
	#if (CTL_IPP_NEW_INIT_FLOW == DISABLE)
	CTL_IPP_CTX_BUF_CFG ctx_buf;
	UINT32 buf_size, buf_addr;
	INT32 rt;

	ctx_buf.n = 16;
	buf_size = ctl_ipp_query(ctx_buf);
	buf_addr = (UINT32)ctl_ipp_util_os_malloc_wrap(buf_size);
	rt = ctl_ipp_init(ctx_buf, buf_addr, buf_size);
	#endif
	ctl_ipp_isp_drv_init();

	return 0;
}

int nvt_ctl_ipp_drv_remove(MODULE_INFO *pmodule_info)
{
	#if (CTL_IPP_NEW_INIT_FLOW == DISABLE)
	ctl_ipp_uninit();
	#endif
	ctl_ipp_isp_drv_uninit();

	return 0;
}

int nvt_ctl_ipp_drv_suspend(MODULE_INFO *pmodule_info)
{
	/* Add suspend operation here*/

	return 0;
}

int nvt_ctl_ipp_drv_resume(MODULE_INFO *pmodule_info)
{
	/* Add resume operation here*/

	return 0;
}
