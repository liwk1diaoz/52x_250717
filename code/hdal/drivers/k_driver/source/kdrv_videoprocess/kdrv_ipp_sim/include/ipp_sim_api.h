#ifndef __ipp_sim_api_h_
#define __ipp_sim_api_h_

#include "ipp_sim_drv.h"

int nvt_kdrv_ipp_show_stripe_settings(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv);
int nvt_kdrv_ipp_initialize_hw_and_param_buf(PIPP_SIM_INFO pime_info, unsigned char argc, char **pargv);
int nvt_kdrv_ipp_release_hw_and_param_buf(PIPP_SIM_INFO pime_info, unsigned char argc, char **pargv);
int nvt_kdrv_ipp_flow02_test(PIPP_SIM_INFO pime_info, unsigned char argc, char **pargv);
int nvt_kdrv_ipp_flow1_test(PIPP_SIM_INFO pime_info, unsigned char argc, char **pargv);
int nvt_kdrv_lca_test(PIPP_SIM_INFO pipp_info, unsigned char argc, char **pargv);

#endif
