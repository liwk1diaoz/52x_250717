/**
    Source file for dal_md

    This file is the source file that implements the API for vendor_md.

    @file       vendor_md.c
    @ingroup    vendor_md

    Copyright   Novatek Microelectronics Corp. 2019.    All rights reserved.
*/

#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
//#include <sys/mman.h>
//#include "kwrap/error_no.h"
//#include "kwrap/type.h"
#include "vendor_md.h"
#include "../include/vendor_md/vendor_md_version.h"
#include "md_ioctl.h"
#include "hdal.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define NVTMPP_OPEN(...) 0
#define NVTMPP_IOCTL nvtmpp_ioctl
#define NVTMPP_CLOSE(...)

#define NVTMD_OPEN(...) 0
#define NVTMD_IOCTL nvt_md_ioctl
#define NVTMD_CLOSE(...)
#endif

#if defined(__LINUX)
#define NVTMPP_OPEN  open
#define NVTMPP_IOCTL ioctl
#define NVTMPP_CLOSE close

#define NVTMD_OPEN  open
#define NVTMD_IOCTL ioctl
#define NVTMD_CLOSE close
#endif

#define PROF                0
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    gettimeofday(&tstart, NULL);
	#define PROF_END(msg)   gettimeofday(&tend, NULL);  \
			printf("%s time (us): %lu\r\n", msg,         \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif

static BOOL md_api_init = FALSE;
static INT32 md_fd;
static INT32 nvtmpp_fd;

INT32 vendor_md_init(void)
{

	INT32 ret;
	VENDOR_MD_OPENCFG md_opencfg;
	md_fd = -1;
	nvtmpp_fd = -1;

	if (md_api_init == FALSE) {
		md_api_init = TRUE;
		// open md device
		md_fd = NVTMD_OPEN("/dev/kdrv_md0", O_RDWR);
		if (md_fd < 0) {
			printf("[VENDOR_MD] Open kdrv_md fail!\n");
			return HD_ERR_NG;
		}

		// open nvtmpp device
		nvtmpp_fd = NVTMPP_OPEN("/dev/nvtmpp", O_RDWR|O_SYNC);
		if (nvtmpp_fd < 0) {
			printf("open /dev/nvtmpp error\r\n");
			return HD_ERR_SYS;
		}

		md_opencfg.clock_sel = 240;
		ret = NVTMD_IOCTL(md_fd, MD_IOC_OPENCFG, &md_opencfg);
		if (ret < 0) {
			printf("[VENDOR_MD] md opencfg fail! \n");
			return ret;
		}

		ret = NVTMD_IOCTL(md_fd, MD_IOC_OPEN, NULL);
		if (ret < 0) {
			printf("[VENDOR_MD] md open fail! \n");
			return ret;
		}

		return 0;
	} else {
		printf("[VENDOR_MD] md module is already initialised. \n");
		return -1;
	}
	return 0;
}

INT32 vendor_md_uninit(void)
{

	INT32 ret = 0;
	if (md_api_init == TRUE) {
		md_api_init = FALSE;
		ret = NVTMD_IOCTL(md_fd, MD_IOC_CLOSE, NULL);
		if (ret < 0) {
			printf("[VENDOR_MD] md close fail! \n");
			return ret;
		}

		if (md_fd != -1) {
			NVTMD_CLOSE(md_fd);
		}

        if (nvtmpp_fd != -1) {
            NVTMPP_CLOSE(nvtmpp_fd);
        }
		return 0;
	} else {
		printf("[VENDOR_MD] md module is already uninitialised. \n");
		return -1;
	}

	return 0;
}

INT32 vendor_md_set(VENDOR_MD_PARAM_ID param_id, void *p_param)
{

	int ret = 0;
	if (md_api_init == FALSE) {
		printf("[VENDOR_MD] md should be init before set param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_MD_PARAM_ALL:{
		VENDOR_MD_PARAM* md_param = (VENDOR_MD_PARAM *)p_param;
#if VENDOR_MD_IOREMAP_IN_KERNEL
#else
		NVTMPP_IOC_VB_PA_TO_VA_S nvtmpp_msg = {0};
#endif

		UINT32 in0_addr_bak, in1_addr_bak, in2_addr_bak, in3_addr_bak, in4_addr_bak, in5_addr_bak;
		UINT32 out0_addr_bak, out1_addr_bak, out2_addr_bak, out3_addr_bak;
		in0_addr_bak = md_param->InInfo.uiInAddr0;
		in1_addr_bak = md_param->InInfo.uiInAddr1;
		in2_addr_bak = md_param->InInfo.uiInAddr2;
		in3_addr_bak = md_param->InInfo.uiInAddr3;
		in4_addr_bak = md_param->InInfo.uiInAddr4;
		in5_addr_bak = md_param->InInfo.uiInAddr5;
		out0_addr_bak = md_param->OutInfo.uiOutAddr0;
		out1_addr_bak = md_param->OutInfo.uiOutAddr1;
		out2_addr_bak = md_param->OutInfo.uiOutAddr2;
		out3_addr_bak = md_param->OutInfo.uiOutAddr3;

		//convert addr params for kernel space.
/*
		md_param->InInfo.uiInAddr0 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr0);
		md_param->InInfo.uiInAddr1 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr1);
		md_param->InInfo.uiInAddr2 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr2);
		md_param->InInfo.uiInAddr3 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr3);
		md_param->InInfo.uiInAddr4 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr4);
		md_param->InInfo.uiInAddr5 = nvtmpp_sys_pa2va(md_param->InInfo.uiInAddr5);
		md_param->OutInfo.uiOutAddr0 = nvtmpp_sys_pa2va(md_param->OutInfo.uiOutAddr0);
		md_param->OutInfo.uiOutAddr1 = nvtmpp_sys_pa2va(md_param->OutInfo.uiOutAddr1);
		md_param->OutInfo.uiOutAddr2 = nvtmpp_sys_pa2va(md_param->OutInfo.uiOutAddr2);
		md_param->OutInfo.uiOutAddr3 = nvtmpp_sys_pa2va(md_param->OutInfo.uiOutAddr3);
*/
#if VENDOR_MD_IOREMAP_IN_KERNEL
#else
		nvtmpp_msg.pa = md_param->InInfo.uiInAddr0;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr0 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->InInfo.uiInAddr1;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr1 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->InInfo.uiInAddr2;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr2 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->InInfo.uiInAddr3;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr3 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->InInfo.uiInAddr4;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr4 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->InInfo.uiInAddr5;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->InInfo.uiInAddr5 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->OutInfo.uiOutAddr0;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->OutInfo.uiOutAddr0 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->OutInfo.uiOutAddr1;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->OutInfo.uiOutAddr1 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->OutInfo.uiOutAddr2;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->OutInfo.uiOutAddr2 = nvtmpp_msg.va;

		nvtmpp_msg.pa = md_param->OutInfo.uiOutAddr3;
		ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		md_param->OutInfo.uiOutAddr3 = nvtmpp_msg.va;
#endif

		ret = NVTMD_IOCTL(md_fd, MD_IOC_SET_PARAM, (VENDOR_MD_PARAM *)md_param);
		if (ret < 0) {
		  printf("[VENDOR_MD] md set VENDOR_MD_PARAM fail! \n");
		  return ret;
		  }
		//recover addr params for user space.
		md_param->InInfo.uiInAddr0 = in0_addr_bak;
		md_param->InInfo.uiInAddr1 = in1_addr_bak;
		md_param->InInfo.uiInAddr2 = in2_addr_bak;
		md_param->InInfo.uiInAddr3 = in3_addr_bak;
		md_param->InInfo.uiInAddr4 = in4_addr_bak;
		md_param->InInfo.uiInAddr5 = in5_addr_bak;
		md_param->OutInfo.uiOutAddr0 = out0_addr_bak;
		md_param->OutInfo.uiOutAddr1 = out1_addr_bak;
		md_param->OutInfo.uiOutAddr2 = out2_addr_bak;
		md_param->OutInfo.uiOutAddr3 = out3_addr_bak;
		break;
	}
	default:
		printf("[VENDOR_MD] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_md_get(VENDOR_MD_PARAM_ID param_id, void *p_param)
{

	int ret = 0;
	if (md_api_init == FALSE) {
		printf("[VENDOR_MD] ai should be init before get param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_MD_PARAM_ALL:
		ret = NVTMD_IOCTL(md_fd, MD_IOC_GET_PARAM, (VENDOR_MD_PARAM *)p_param);
		if (ret < 0) {
			printf("[VENDOR_MD] md get VENDOR_MD_PARAM fail! \n");
			return ret;
		}
		break;
	case VENDOR_MD_PARAM_GET_REG:
		ret = NVTMD_IOCTL(md_fd, MD_IOC_GET_REG, (VENDOR_MD_REG_DATA *)p_param);
		if (ret < 0) {
			printf("[VENDOR_MD] md get REG fail! \n");
			return ret;
		}
		break;	
	default:
		printf("[VENDOR_MD] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_md_trigger(VENDOR_MD_TRIGGER_PARAM *p_param)
{
	int ret = 0;

	if (md_api_init == FALSE) {
		printf("[VENDOR_MD] ai should be init before trigger\n");
		return -1;
	}

	PROF_START();
	ret = NVTMD_IOCTL(md_fd, MD_IOC_TRIGGER, p_param);
	PROF_END("vendor_md_drv");
	if (ret < 0) {
		printf("[VENDOR_MD] md trigger fail! \n");
		return ret;
	}

	return ret;
}

INT32 vendor_md_get_vendor_version(CHAR* vendor_version)
{
    INT32 ret = 0;
    snprintf(vendor_version, 20, VENDOR_MD_VERSION);
    return ret;
}

INT32 vendor_md_get_kdrv_version(CHAR* kdrv_version)
{
	INT32 ret = 0;

	ret = NVTMD_IOCTL(md_fd, MD_IOC_GET_VER, kdrv_version);
	if (ret < 0) {
		printf("[VENDOR_MD] md open fail! \n");
	}	

	return ret;
}


