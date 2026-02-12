/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ctl_vpe_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     XL Wei
    @date       2020/05/12
*/
#include <linux/module.h>
#include "kflow_videoprocess/ctl_vpe.h"
#include "kflow_videoprocess/ctl_vpe_isp.h"
#include "ctl_vpe_int.h"

#if CTL_VPE_MODULE_ENABLE
EXPORT_SYMBOL(ctl_vpe_open);
EXPORT_SYMBOL(ctl_vpe_close);
EXPORT_SYMBOL(ctl_vpe_ioctl);
EXPORT_SYMBOL(ctl_vpe_set);
EXPORT_SYMBOL(ctl_vpe_get);
EXPORT_SYMBOL(ctl_vpe_query);
EXPORT_SYMBOL(ctl_vpe_init);
EXPORT_SYMBOL(ctl_vpe_uninit);

EXPORT_SYMBOL(ctl_vpe_isp_evt_fp_reg);
EXPORT_SYMBOL(ctl_vpe_isp_evt_fp_unreg);
EXPORT_SYMBOL(ctl_vpe_isp_set);
EXPORT_SYMBOL(ctl_vpe_isp_get);
EXPORT_SYMBOL(ctl_vpe_dump_all);
#endif
