/**
	@brief Source file of vendor media videoout.

	@file vendor_videoout.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__LINUX)
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "kdrv_videoout/nvt_fb.h"
#else
#include "kdrv_videoout/kdrv_vdoout.h"
static KDRV_DEV_ENGINE disp_eng_id_tbl[2] ={KDRV_VDOOUT_ENGINE0,KDRV_VDOOUT_ENGINE0};
static UINT32 get_kdrv_fmt_coef(UINT32 fmt);
#endif
#include "kflow_videoout/isf_vdoout.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "vendor_videoout.h"
#include "videosprite/videosprite.h"


#define DEV_BASE        ISF_UNIT_VDOOUT
#define DEV_COUNT       ISF_MAX_VDOOUT
#define HD_DEV_BASE     HD_DAL_VIDEOOUT_BASE
#define HD_DEV_MAX      HD_DAL_VIDEOOUT_MAX
#define IN_BASE         ISF_IN_BASE
#define IN_COUNT        1

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if(id < DEV_COUNT) { \
				(dev_id) = DEV_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_IN_ID(in_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((in_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((in_id) >= HD_IN_BASE && (in_id) <= HD_IN_MAX) { \
			UINT32 id = (in_id) - HD_IN_BASE; \
			if(id < IN_COUNT) { \
				(in_id) = IN_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define DBG_ERR(fmt, args...) 	printf("%s: " fmt, __func__, ##args)
#define DBG_DUMP				printf

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define ISF_OPEN     isf_flow_open
#define ISF_IOCTL    isf_flow_ioctl
#define ISF_CLOSE    isf_flow_close
#endif
#if defined(__LINUX)
#define ISF_OPEN     open
#define ISF_IOCTL    ioctl
#define ISF_CLOSE    close
#endif

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern int _hd_common_get_fd(void);

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
int misc_fd = -1;
#if defined(__LINUX)
#else
//for rtos change ping-pong buffer
//only get fb format/w/h in first init, uninit would clear
//for ARGB8565 OSD buffer is 2 plane (Alpha+RGB)
UINT32 bInited = 0;
UINT32 fmt=0,w=0,h=0;
#endif
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if defined(__LINUX)
static INT32 _hd_videoout_get_nvtfb_lyr(HD_FB_ID fb_id)
{
	switch(fb_id){
	case HD_FB0:
		return NVT_LAYER_OSD_1;
	case HD_FB2:
		return NVT_LAYER_VIDEO_2;

	default:
		DBG_ERR("not sup %d\r\n",fb_id);
		return -1;
	}
}
#else
static INT32 _hd_videoout_get_nvtfb_lyr(HD_FB_ID fb_id)
{
	switch(fb_id){
	case HD_FB0:
		return VDDO_DISPLAYER_OSD1;
	case HD_FB2:
		return VDDO_DISPLAYER_VDO2;

	default:
		DBG_ERR("not sup %d\r\n",fb_id);
		return -1;
	}
}
#endif

static HD_RESULT vendor_osg_set_incontinuous_mask(HD_PATH_ID path_id, VENDOR_VIDEOOUT_INCONTINUOUS_MASK *p_param)
{
	OSG_INCONTINUOUS_MASK attr = { 0 };

	if(!p_param){
		DBG_ERR("vendor_osg_set_incontinuous_mask() : p_param is NULL\n");
		return HD_ERR_NULL_PTR;
	}

	attr.type         = OSG_MASK_TYPE_INCONTINUOUS;
	attr.color        = p_param->color;
	attr.x            = p_param->position.x;
	attr.y            = p_param->position.y;
	attr.h_line_len   = p_param->h_line_len;
	attr.h_hole_len   = p_param->h_hole_len;
	attr.v_line_len   = p_param->v_line_len;
	attr.v_hole_len   = p_param->v_hole_len;
	attr.h_thickness  = p_param->h_thickness;
	attr.v_thickness  = p_param->v_thickness;

	return hd_videoout_set(path_id, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);
}

HD_RESULT vendor_videoout_set(UINT32 id, VENDOR_VIDEOOUT_ITEM item, VOID *p_param)
{
	switch (item) {
		case VENDOR_VIDEOOUT_ITEM_INCONTINUOUS_MASK :
			return vendor_osg_set_incontinuous_mask(id, p_param);
        case VENDOR_VIDEOOUT_ITEM_FB_INIT:
            {
				VENDOR_FB_INIT* hd_fb = 0;
                INT32 result =HD_OK;

                if(id > VENDOR_VIDEOOUT_ID1) {
                    return HD_ERR_PARAM;
                }

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}

                hd_fb = (VENDOR_FB_INIT*)p_param;

                #if defined(__LINUX)
            	struct nvt_fb_layer_para fb_init;

                ///////  open fb_misc fd ///////
                if(misc_fd==-1){
                	misc_fd = open("/dev/fb_misc", O_RDWR);
                	if (!misc_fd)	{
                		DBG_ERR("Cannot open fb_misc device.\n");
                		return HD_ERR_DRV;
                	}

                    #if 0
                	///////  init fb_misc ///////
                	result=ioctl(misc_fd, IOCTL_FB_INIT, 0);
                    if (result) {
                        result = ioctl(misc_fd, IOCTL_FB_DEINIT, 0);
                        if(result==0) {
                            result=ioctl(misc_fd, IOCTL_FB_INIT, 0);
                        } else {
                            DBG_ERR("FB_INIT fail %d\r\n",result);
                            return HD_ERR_FAIL;
                        }
                    }
                    #endif
                }

                ///////  init layer buffer ///////
            	fb_init.fb_va_addr = 0;
            	fb_init.fb_pa_addr = hd_fb->pa_addr;
            	fb_init.buffer_len = hd_fb->buf_len;
            	fb_init.ide_id = id;
            	fb_init.layer_id = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);

           		DBG_DUMP("fb_id(%d) buf(0x%x) size(0x%x)\n",hd_fb->fb_id,hd_fb->pa_addr,hd_fb->buf_len);

                result= ioctl(misc_fd, IOCTL_FB_LAYER_INIT, &fb_init);
                if ((result)&&(__errno_location()!=NULL)&&(errno==EALREADY)) { //for coverity 136955:NULL_RETURNS
                    result=ioctl(misc_fd, IOCTL_FB_LAYER_DEINIT, &fb_init);
                    if(result==0) {
                        result=ioctl(misc_fd, IOCTL_FB_LAYER_INIT, &fb_init);
                    } else {
                        DBG_ERR("FB_INIT fail %d\r\n",result);
                        return HD_ERR_FAIL;
                    }
            	}
                #else
	            KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
	            KDRV_DEV_ENGINE disp_eng = disp_eng_id_tbl[id];
	            UINT32 handler = KDRV_DEV_ID(0, disp_eng, 0);
                if(bInited ==0)
                {

        			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);
        			result=kdrv_vddo_get(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
        			if(result!=0)
        				return result;
        			result = kdrv_vddo_trigger(handler, NULL);
        			if(result!=0)
        				return result;

        			fmt = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format;
                    w = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width;
                    h = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height;
        			//printf("id %d fmt %d %d %d\r\n",hd_fb->fb_id,fmt,w,h);

        		}
                if (fmt ==0){
                    printf("err set fmt first!\r\n");
                    return -1;
                }

            	memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
            	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = hd_fb->pa_addr;
                if(fmt == VDDO_DISPBUFFORMAT_ARGB8565) {
                	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = ARGB8565_A_ADDR(hd_fb->pa_addr,w,h);
                } else {
                    kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = 0;
                }
           	    kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);
            	result = kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
            	if(result !=0){
                    printf("err %d %x %d\r\n",(int)result,disp_eng,(int)id);
            	}
            	else
            		result=kdrv_vddo_trigger(handler, NULL);

                //DBG_DUMP("freertos fmt %x %x %x\n",fmt,kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y,kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr);
                if(result==0)
                    bInited = 1;

                #endif

                return result;

            }
            break;
            case VENDOR_VIDEOOUT_ITEM_FB_UNINIT:
            {

                if(id > VENDOR_VIDEOOUT_ID1) {
                    return HD_ERR_PARAM;
                }

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}

                #if defined(__LINUX)
				VENDOR_FB_UNINIT* hd_fb = (VENDOR_FB_UNINIT*)p_param;
            	struct nvt_fb_layer_para fb_init;

                ///////  deinit layer buffer ///////
            	fb_init.fb_va_addr = 0;
            	fb_init.fb_pa_addr = 0;
            	fb_init.buffer_len = 0;
            	fb_init.ide_id = id;
            	fb_init.layer_id = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);

           		DBG_DUMP("fb_id(%d) \n",hd_fb->fb_id);
                if(misc_fd>=0) {
                    if (ioctl(misc_fd, IOCTL_FB_LAYER_DEINIT, &fb_init)) {
                	    DBG_ERR("IOCTL_FB_LAYER_DEINIT error\n");
                    	return HD_ERR_DRV;
                	}
                    #if 0
                    ///////  deinit fb_misc ///////
                    if (ioctl(misc_fd, IOCTL_FB_DEINIT,0)) {
                        DBG_ERR("IOCTL_FB_DEINIT error\n");
                    }
                    #endif
                    ///////  close fb_misc fd ///////
                	close(misc_fd);
                    misc_fd =-1;
            	}
                #else
                //DBG_DUMP("freertos VENDOR_VIDEOOUT_ITEM_FB_UNINIT\r\n");
                bInited = 0;
                fmt =0;
                w =0;
                h =0;
                #endif
                return HD_OK;

            }
            break;
            case VENDOR_VIDEOOUT_ITEM_ENTER_SLEEP:
            case VENDOR_VIDEOOUT_ITEM_EXIT_SLEEP:
            {
            	int isf_fd = 0;
                int sleep_on=0;
                HD_DAL self_id = DEV_BASE + id;
            	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
            	int r;
                HD_RESULT rv = 0;

    			isf_fd = _hd_common_get_fd();

    			if (isf_fd <= 0) {
    				return HD_ERR_UNINIT;
    			}

                if(id <= VENDOR_VIDEOOUT_ID1) {
                    cmd.dest = ISF_PORT(self_id, ISF_CTRL);
                } else {
                    HD_IO ctrl_id = HD_GET_CTRL(id);

                    if(ctrl_id == HD_CTRL) {
                        HD_DAL self_id = HD_GET_DEV(id);
                        _HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

			            cmd.dest = ISF_PORT(self_id, ISF_CTRL);
                    } else {
                        return HD_ERR_PARAM;
                    }
                }

                //printf("item %d ,cmd.dest 0x%x\r\n",item,cmd.dest);

                if(item==VENDOR_VIDEOOUT_ITEM_ENTER_SLEEP) {
                    sleep_on =1;
                } else {
                    sleep_on =0;
                }
				cmd.param = VDOOUT_PARAM_SLEEP;
				cmd.value = sleep_on;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//DBG_DUMP("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);

    			if (r == 0) {
    				switch (cmd.rv) {
    				case ISF_OK:
    					rv = HD_OK;
    					break;
    				default:
    					rv = HD_ERR_SYS;
    					break;
    				}
    			} else {
    				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
    					rv= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
    				} else {
    				    DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    				}
    			}
                return rv;
    		}
            break;
            case VENDOR_VIDEOOUT_ITEM_FUNC_CONFIG:
            {
            	int isf_fd = 0;
            	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
            	int r;
                HD_RESULT rv = 0;
                HD_IO in_id = HD_GET_IN(id);
                HD_DAL self_id = HD_GET_DEV(id);
				VENDOR_VIDEOOUT_FUNC_CONFIG* vnd_cfg = 0;
				VDOOUT_VND_CFG vout_cfg={0};

    			isf_fd = _hd_common_get_fd();

    			if (isf_fd <= 0) {
    				return HD_ERR_UNINIT;
    			}

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

                vnd_cfg = (VENDOR_VIDEOOUT_FUNC_CONFIG*)p_param;
                vout_cfg.in_func = vnd_cfg->in_func;
                _HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

                cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDOOUT_PARAM_VND_CFG;
				cmd.value = (UINT32)&vout_cfg;
				cmd.size = sizeof(VDOOUT_VND_CFG);
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//DBG_DUMP("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);

    			if (r == 0) {
    				switch (cmd.rv) {
    				case ISF_OK:
    					rv = HD_OK;
    					break;
    				default:
    					rv = HD_ERR_SYS;
    					break;
    				}
    			} else {
    				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
    					rv= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
    				} else {
    				    DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    				}
    			}
                return rv;
    		}
            break;
            case VENDOR_VIDEOOUT_ITEM_PARAM_IN:
            {
            	int isf_fd = 0;
            	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
            	int r;
                HD_RESULT rv = 0;
                HD_IO in_id = HD_GET_IN(id);
                HD_DAL self_id = HD_GET_DEV(id);
				VENDOR_VIDEOOUT_IN* vnd_param_in = 0;
                VDOOUT_VND_IN vout_in = {0};

    			isf_fd = _hd_common_get_fd();

    			if (isf_fd <= 0) {
    				return HD_ERR_UNINIT;
    			}

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}

				if(!(in_id >= HD_IN_BASE && in_id <= HD_IN_MAX)) {return HD_ERR_IO;}
				_HD_CONVERT_IN_ID(in_id , rv);	if(rv != HD_OK) {	return rv;}

                vnd_param_in = (VENDOR_VIDEOOUT_IN*)p_param;
                vout_in.queue_depth = vnd_param_in->queue_depth;
                _HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

                cmd.dest = ISF_PORT(self_id, in_id);
				cmd.param = VDOOUT_PARAM_VND_IN;
				cmd.value = (UINT32)&vout_in;
				cmd.size = sizeof(VDOOUT_VND_CFG);
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				DBG_DUMP("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);

    			if (r == 0) {
    				switch (cmd.rv) {
    				case ISF_OK:
    					rv = HD_OK;
    					break;
    				default:
    					rv = HD_ERR_SYS;
    					break;
    				}
    			} else {
    				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
    					rv= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
    				} else {
    				    DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    				}
    			}
                return rv;
    		}
            case VENDOR_VIDEOOUT_ITEM_DEV_CONFIG:
            {
                DBG_ERR("please modify nvt-xxx-display.dtsi\r\n");
                return HD_ERR_NOT_SUPPORT;
                #if 0
            	int isf_fd = 0;
            	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
            	int r;
                HD_RESULT rv = 0;
                HD_IO ctrl_id = HD_GET_CTRL(id);
                HD_DAL self_id = HD_GET_DEV(id);
				VENDOR_VIDEOOUT_DEV_CONFIG* vnd_cfg = 0;
				VDOOUT_IF_CFG vout_if_cfg={0};

    			isf_fd = _hd_common_get_fd();

    			if (isf_fd <= 0) {
    				return HD_ERR_UNINIT;
    			}

                if(ctrl_id != HD_CTRL) {
                    return HD_ERR_PARAM ;
                }

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}
                vnd_cfg = (VENDOR_VIDEOOUT_DEV_CONFIG*)p_param;

                if(sizeof(VDOOUT_IF_CFG)==sizeof(VENDOR_VIDEOOUT_IF_CFG))
				    memcpy((void *)&vout_if_cfg,(void *)&vnd_cfg->if_cfg,sizeof(VDOOUT_IF_CFG));
				else
					return HD_ERR_USER; //lib header not sync

                _HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

                cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOOUT_PARAM_VND_IF_CFG;
				cmd.value = (UINT32)&vout_if_cfg;
				cmd.size = sizeof(VDOOUT_IF_CFG);
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//DBG_DUMP("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);

    			if (r == 0) {
    				switch (cmd.rv) {
    				case ISF_OK:
    					rv = HD_OK;
    					break;
    				default:
    					rv = HD_ERR_SYS;
    					break;
    				}
    			} else {
    				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
    					rv= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
    				} else {
    				    DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    				}
    			}
                return rv;
                #endif
    		}
            break;
            case VENDOR_VIDEOOUT_ITEM_MERGE_WND:
            {
            	int isf_fd = 0;
            	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
            	int r;
                HD_RESULT rv = 0;
                HD_IO ctrl_id = HD_GET_CTRL(id);
                HD_DAL self_id = HD_GET_DEV(id);
                UINT32 flow_status = *(UINT32 *)p_param;

    			isf_fd = _hd_common_get_fd();

    			if (isf_fd <= 0) {
    				return HD_ERR_UNINIT;
    			}

                if(ctrl_id != HD_CTRL) {
                    return HD_ERR_PARAM ;
                }

                _HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}

                cmd.dest = ISF_PORT(self_id, ISF_CTRL);
				cmd.param = VDOOUT_PARAM_VND_MERGE_WND;
				cmd.value = flow_status;
				cmd.size = 0;
				r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
				//DBG_DUMP("===>1:r=%d,rv=%08x\r\n", r, cmd.rv);

    			if (r == 0) {
    				switch (cmd.rv) {
    				case ISF_OK:
    					rv = HD_OK;
    					break;
    				default:
    					rv = HD_ERR_SYS;
    					break;
    				}
    			} else {
    				if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
    					rv= cmd.rv; // ISF_ERR is exactly the same with HD_ERR
    				} else {
    				    DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
    					rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
    				}
    			}
                return rv;
            }
            break;
		default :
			DBG_ERR("unsupported item = %d\n", item);
			return HD_ERR_NOT_SUPPORT;
	};
}

HD_RESULT vendor_videoout_get(UINT32 id, VENDOR_VIDEOOUT_ITEM item, VOID *p_param)
{
	switch (item) {
#if defined(__FREERTOS)
        case VENDOR_VIDEOOUT_ITEM_FB_INIT:
            {
				VENDOR_FB_INIT* hd_fb = 0;
                INT32 result =HD_OK;

                if(id > VENDOR_VIDEOOUT_ID1) {
                    return HD_ERR_PARAM;
                }

                if(!p_param){
            		return HD_ERR_NULL_PTR;
            	}
                hd_fb = (VENDOR_FB_INIT*)p_param;

	            KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
	            KDRV_DEV_ENGINE disp_eng = disp_eng_id_tbl[id];
	            UINT32 handler = KDRV_DEV_ID(0, disp_eng, 0);
                UINT32 fmt=0,w=0,h=0;
                {

        			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);
        			result=kdrv_vddo_get(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
        			if(result!=0)
        				return result;
        			result = kdrv_vddo_trigger(handler, NULL);
        			if(result!=0)
        				return result;

        			fmt = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format;
                    w = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width;
                    h = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height;
        			//printf("id %d fmt %d %d %d\r\n",hd_fb->fb_id,fmt,w,h);

        		}
                if (fmt ==0){
                    printf("err set fmt first!\r\n");
                    return -1;
                }



            	memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
           	    kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = _hd_videoout_get_nvtfb_lyr(hd_fb->fb_id);
            	result = kdrv_vddo_get(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
            	if(result !=0){
                    printf("err %d %x %d\r\n",(int)result,disp_eng,(int)id);
            	}
            	else{
            		result=kdrv_vddo_trigger(handler, NULL);
            	}
                if(result == HD_OK) {
                    hd_fb->pa_addr = kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y;
                    hd_fb->buf_len = get_kdrv_fmt_coef(fmt)*w*h;

                }
                //DBG_DUMP("freertos VENDOR_VIDEOOUT_ITEM_FB_INIT %x %x\n",hd_fb->pa_addr,hd_fb->buf_len);
                return result;

            }
            break;
#endif
		default :
			DBG_ERR("unsupported item = %d\n", item);
			return HD_ERR_NOT_SUPPORT;
	};
}

#if defined(__FREERTOS)
static UINT32 get_kdrv_fmt_coef(UINT32 fmt)
{
	switch(fmt) {
		case VDDO_DISPBUFFORMAT_ARGB1555:
			return 2;
		case VDDO_DISPBUFFORMAT_ARGB8888:
			return 4;
		case VDDO_DISPBUFFORMAT_ARGB4444:
			return 2;
		case VDDO_DISPBUFFORMAT_ARGB8565:
			return 3; //ARGB
		case VDDO_DISPBUFFORMAT_YUV420PACK:
			return 1;
		case VDDO_DISPBUFFORMAT_YUV422PACK:
			return 1;
        case VDDO_DISPBUFFORMAT_PAL8:
			return 1;
		default:
			DBG_ERR("not supp %d\r\n",fmt);
			return 1;
	}
}
#endif
