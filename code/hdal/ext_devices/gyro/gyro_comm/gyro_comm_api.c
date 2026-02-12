#include "gyro_comm_ioctl.h"
#include "gyro_comm_platform.h"
#include "kwrap/sxcmd.h"
#include "kwrap/util.h"
#include "kwrap/error_no.h"
#include "comm/gyro_drv.h"

#define Delay_DelayMs(ms) vos_util_delay_ms(ms)
#define CFG_FPS_BASE 15

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/cmdsys.h"
#define simple_strtoul(param1, param2, param3) strtoul(param1, param2, param3)
#include "stdlib.h"
#else
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#endif

#define debug_msg 			vk_printk
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__                gyro_comm
#define __DBGLVL__                NVT_DBG_WRN
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/


/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
#define MAX_GYRO_OBJ_NUM 1
static PGYRO_OBJ p_gyro_reg_obj[MAX_GYRO_OBJ_NUM] = {0};

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_gyro_comm_init(void)
{
	nvt_dbg(IND, "\n");
	return 0;
}

int nvt_gyro_comm_uninit(void)
{
	nvt_dbg(IND, "\n");
	return 0;
}
PGYRO_OBJ nvt_gyro_comm_get_obj(UINT32 id)
{
    if (id >= MAX_GYRO_OBJ_NUM) {
		DBG_ERR("only support max %d gyro obj\r\n", MAX_GYRO_OBJ_NUM);
		return NULL;
	}
	return p_gyro_reg_obj[id];
}
extern INT32 isf_vdocap_reg_gyro_obj(UINT32 id, PGYRO_OBJ p_gyro_obj);

INT32 nvt_gyro_comm_set_obj(UINT32 id, PGYRO_OBJ p_gyro_obj)
{
    if (id >= MAX_GYRO_OBJ_NUM) {
		DBG_ERR("only support max %d gyro obj\r\n", MAX_GYRO_OBJ_NUM);
		return E_ID;
	}
	if ((p_gyro_reg_obj[id] == 0) && (p_gyro_obj != 0)) {
		p_gyro_reg_obj[id] = p_gyro_obj;
		DBG_DUMP("gyro_comm obj register ok!\r\n");
        isf_vdocap_reg_gyro_obj(id,p_gyro_obj);
		return 0;
	}
	if ((p_gyro_reg_obj[id] != 0) && (p_gyro_obj == 0)) {
		p_gyro_reg_obj[id] = 0;
		DBG_DUMP("gyro_comm obj un-register done!\r\n");
        isf_vdocap_reg_gyro_obj(id,0);
		return 0;
	}
    return E_SYS;
}
#if defined(__LINUX)
long nvt_gyro_comm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    static PGYRO_OBJ gy_obj=0;
	int __user *argp = (int __user *)arg;

	DBG_IND("cmd:%x gy_obj:%lx\r\n", cmd,(ULONG)gy_obj);

	switch (cmd) {
	case GYRO_IOC_OPEN:
    {
        gy_obj = nvt_gyro_comm_get_obj(GYRO_1);
		if(!gy_obj) {
            DBG_ERR("not reg gyro\r\n");
            return E_OBJ;
		}
        ret=gy_obj->open();
        if(ret!=0) {
            DBG_ERR("open %d\r\n",ret);
        }
	}
	break;
	case GYRO_IOC_CLOSE:
    {
		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}

        ret=gy_obj->close();
        if(ret!=0) {
            DBG_ERR("close %d\r\n",ret);
        }
        gy_obj=0;
	}
	break;
	case GYRO_IOC_MODE:
    {
        UINT32 value =0;
		ret = copy_from_user(&value, argp, sizeof(UINT32));
        if(ret!=0) {
            DBG_ERR("copy from\r\n");
		} else {
			if(!gy_obj) {
                DBG_ERR("not open\r\n");
                return E_OBJ;
			}
            ret=gy_obj->set_cfg(GYROCTL_CFG_MODE,&value);
            if(ret!=0) {
                DBG_ERR("mode %d\r\n",ret);
            }

		}
	}
	break;
	case GYRO_IOC_FREERUN:
    {
        GYRO_FREE_RUN_PARAM frParam = {0};

		ret = copy_from_user(&frParam, argp, sizeof(GYRO_FREE_RUN_PARAM));
        if(ret!=0) {
            DBG_ERR("copy_from_user\r\n");
		} else {
			if(!gy_obj) {
                DBG_ERR("not open\r\n");
                return E_OBJ;
			}
            ret=gy_obj->set_cfg(GYROCTL_CFG_FRUN_PARAM,&frParam);
            if(ret!=0) {
                DBG_ERR("frun %d\r\n",ret);
            }

		}
	}
	break;

	case GYRO_IOC_GET_SDATA:
    {
        GYRO_COMM_DATA gyro_data={0};
        UINT32 num=0;
		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}
        ret = gy_obj->get_data(&num,0,&gyro_data.gyro_x, &gyro_data.gyro_y, &gyro_data.gyro_z);
        if(ret!=0) {
            DBG_ERR("get_data %d\r\n",ret);
            return E_SYS;
        }
        ret = gy_obj->get_gsdata(&gyro_data.ags_x, &gyro_data.ags_y, &gyro_data.ags_z);
        if(ret!=0) {
            DBG_ERR("get_gsdata %d\r\n",ret);
            return E_SYS;
        }

        if(num==1) {
            ret = copy_to_user(argp, &gyro_data, sizeof(GYRO_COMM_DATA));
            if(ret!=0){
                DBG_ERR("copy_to_user\r\n");
            }
        } else {
            DBG_ERR("not single mode\r\n");
        }

	}
	break;

	case GYRO_IOC_SET_OFS:
    {
        GYRO_COMM_DATA gyro_data={0};

		ret = copy_from_user(&gyro_data, argp, sizeof(GYRO_COMM_DATA));
        if(ret!=0) {
            DBG_ERR("copy_from_user\r\n");
		} else {

    		if(!gy_obj) {
                DBG_ERR("not open\r\n");
                return E_OBJ;
    		}
            ret=gy_obj->set_cfg(GYROCTL_CFG_OFFSET,&gyro_data);
            if(ret!=0) {
                DBG_ERR("ofs %d\r\n",ret);
            }
		}
	}
	break;

    default:
        DBG_ERR("not sup %d\r\n",cmd);
        ret = E_NOSPT;
    break;


    }
	return ret;
}

#else

int nvt_gyro_comm_openf(char* file, int flag)
{
	return 1;
}

int nvt_gyro_comm_closef(int fd)
{
	return 0;
}

int nvt_gyro_comm_ioctl(int fd, unsigned int cmd, void *arg)
{
	int ret = 0;
    static PGYRO_OBJ gy_obj=0;

	DBG_IND("cmd:%x gy_obj:%lx\r\n", cmd,(ULONG)gy_obj);

	switch (cmd) {
	case GYRO_IOC_OPEN:
    {
        gy_obj = nvt_gyro_comm_get_obj(GYRO_1);
		if(!gy_obj) {
            DBG_ERR("not reg gyro\r\n");
            return E_OBJ;
		}
        ret=gy_obj->open();
        if(ret!=0) {
            DBG_ERR("open %d\r\n",ret);
        }
	}
	break;
	case GYRO_IOC_CLOSE:
    {
		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}

        ret=gy_obj->close();
        if(ret!=0) {
            DBG_ERR("close %d\r\n",ret);
        }
        gy_obj=0;
	}
	break;
	case GYRO_IOC_MODE:
    {
        UINT32 value =*(UINT32*)arg;
        DBG_IND("mode %d\r\n",value);

		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}
        ret=gy_obj->set_cfg(GYROCTL_CFG_MODE,&value);
        if(ret!=0) {
            DBG_ERR("mode %d\r\n",ret);
        }

	}
	break;
	case GYRO_IOC_FREERUN:
    {
        GYRO_FREE_RUN_PARAM *pfrParam = (GYRO_FREE_RUN_PARAM *)arg;
        if(pfrParam) {
            DBG_IND("uiDataNum %d \r\n",pfrParam->uiDataNum);
        } else {
            DBG_ERR("pfrParam NULL\r\n");
            return E_PAR;
        }
		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}
        ret=gy_obj->set_cfg(GYROCTL_CFG_FRUN_PARAM,pfrParam);
        if(ret!=0) {
            DBG_ERR("frun %d\r\n",ret);
        }


	}
	break;

	case GYRO_IOC_GET_SDATA:
    {
        GYRO_COMM_DATA *p_gyro_data=(GYRO_COMM_DATA *)arg;
        UINT32 num=0;
		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}
		if(!p_gyro_data) {
            DBG_ERR("p_gyro_data\r\n");
            return E_PAR;
		}
        ret = gy_obj->get_data(&num,0,&p_gyro_data->gyro_x, &p_gyro_data->gyro_y, &p_gyro_data->gyro_z);
        if(ret!=0) {
            DBG_ERR("get_data %d\r\n",ret);
            return E_SYS;
        }
        ret = gy_obj->get_gsdata(&p_gyro_data->ags_x, &p_gyro_data->ags_y, &p_gyro_data->ags_z);
        if(ret!=0) {
            DBG_ERR("get_gsdata %d\r\n",ret);
            return E_SYS;
        }

        DBG_IND("num%d \r\n",num);

        if(num!=1) {
            DBG_ERR("not single mode\r\n");
        }

	}
	break;

	case GYRO_IOC_SET_OFS:
    {
        GYRO_COMM_DATA *p_ofs_data=(GYRO_COMM_DATA *)arg;
        if(p_ofs_data) {
            DBG_IND("ofs %d,%d,%d\r\n",p_ofs_data->gyro_x,p_ofs_data->gyro_y,p_ofs_data->gyro_z);
        }else{
            DBG_ERR("p_ofs_data NULL\r\n");
            return E_PAR;
        }

		if(!gy_obj) {
            DBG_ERR("not open\r\n");
            return E_OBJ;
		}
        ret=gy_obj->set_cfg(GYROCTL_CFG_OFFSET,p_ofs_data);
        if(ret!=0) {
            DBG_ERR("ofs %d\r\n",ret);
        }
    }
	break;

    default:
        DBG_ERR("not sup %d\r\n",cmd);
        ret = E_NOSPT;
    break;


    }
	return ret;
}

#endif

//===============



void frun_gyro_cb(void)
{
    #if 1  //in isr
    UINT32  puiNum = 0;
    INT32 AgyroX[32];
    INT32 AgyroY[32];
    INT32 AgyroZ[32];
    INT32 AgsX[32];
    INT32 AgsY[32];
    INT32 AgsZ[32];

    PGYRO_OBJ gy_obj = nvt_gyro_comm_get_obj(GYRO_1);

    gy_obj->get_data(&puiNum,0,AgyroX, AgyroY, AgyroZ);

    DBG_DUMP("%d, X[1]=%5d, Y[1]=%5d, Z[1]=%5d\r\n",puiNum,AgyroX[1],AgyroY[1],AgyroZ[1]);
    gy_obj->get_gsdata(AgsX, AgsY, AgsZ);

    DBG_DUMP("gX[1]=%5d, gY[1]=%5d, gZ[1]=%5d\r\n",AgsX[1],AgsY[1],AgsZ[1]);
    #endif
}
BOOL gyro_comm_srun(UINT32 count)
{
    PGYRO_OBJ gy_obj = nvt_gyro_comm_get_obj(GYRO_1);
    INT32 result=0;
    UINT32 value=0;
    UINT32 num = 0;
    INT32   tmpX,tmpY,tmpZ;
    INT32   accX,accY,accZ;
    UINT32 loop =0;
	DBG_ERR("gyro test %d\r\n",count);

    if(gy_obj==NULL) {
        return FALSE;
    }

    result=gy_obj->open();
    if(result!=0) {
        return result;
    }
    value= GYRO_OP_MODE_SINGLE_ACCESS;
    result=gy_obj->set_cfg(GYROCTL_CFG_MODE,&value);
    if(result!=0){
        return result;
    }

    for(loop=0;loop<count;loop++) {
        gy_obj->get_data(&num,0,&tmpX, &tmpY, &tmpZ);
        DBG_DUMP("%d tmpX=%5d, tmpY=%5d, tmpZ=%5d\r\n",num,tmpX,tmpY,tmpZ);
        gy_obj->get_gsdata(&accX, &accY, &accZ);
        DBG_DUMP("%d accX=%5d, accY=%5d, accZ=%5d\r\n",num,accX,accY,accZ);
        Delay_DelayMs(1);
    }

    gy_obj->close();
	return TRUE;
}
BOOL gyro_comm_frun(UINT32 item)
{
    PGYRO_OBJ gy_obj = nvt_gyro_comm_get_obj(GYRO_1);
    INT32 result=0;
    UINT32 value=0;
    UINT32 frameRate=30,dataNum=32,triggeridx = 0;
    UINT32 src=GYRO_SIE_1;

	DBG_DUMP("gyro free item %d \r\n",item);

    if(gy_obj==NULL) {
        return FALSE;
    }

    if(item == 5) {
        DBG_DUMP("gyro close \r\n");
        gy_obj->close();
        return TRUE;
    }

    result=gy_obj->open();
    if(result!=0) {
        return result;
    }
    if(item==3) {
        DBG_DUMP("set src %d \r\n",src);
        result=gy_obj->set_cfg(GYROCTL_CFG_SIE_SRC,(void *)&src);
        if(result!=0) {
            return result;
        }
    }
    if(item==2) {
        DBG_DUMP("set cb \r\n");
        dataNum =8;
        result=gy_obj->set_cfg(GYROCTL_CFG_SIE_CB,(void *)frun_gyro_cb);
        if(result!=0) {
            return result;
        }
    }

    {
        GYRO_FREE_RUN_PARAM frParam = {0};

        frParam.uiPeriodUs = 1000000/frameRate;
        frParam.uiDataNum = dataNum;
        frParam.uiTriggerIdx = triggeridx;
        if(item==3) {
            frParam.triggerMode = GYRO_FREERUN_SIE_SYNC;
        }else {
            frParam.triggerMode = GYRO_FREERUN_FWTRIGGER;
        }
        result=gy_obj->set_cfg(GYROCTL_CFG_FRUN_PARAM,&frParam);
        if(result!=0){
            return result;
        }
    }

    value= GYRO_OP_MODE_FREE_RUN;
    result=gy_obj->set_cfg(GYROCTL_CFG_MODE,&value);
    if(result!=0){
        return result;
    }


	return TRUE;
}

static BOOL _gyro_comm_test(unsigned char argc, char **pargv)
{
    UINT32 gryo_cmd =0;
	unsigned char idx = 1;
    UINT32 count=0;

	if (argc > idx) {
		sscanf(pargv[idx++], "%d", (int *)&gryo_cmd);
        if(gryo_cmd==1) {
            if(argc > idx) {
                sscanf(pargv[idx++], "%d", (int *)&count);
                 gyro_comm_srun(count);
            } else {
                gyro_comm_srun(50);
            }
        }else {
            gyro_comm_frun(gryo_cmd);
        }
	}

	return 1;
}

static SXCMD_BEGIN(gyro_comm_cmd, "gyro_comm")
SXCMD_ITEM("test", 	_gyro_comm_test,"gryo test")
SXCMD_END()

int gyro_comm_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(gyro_comm_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "gyro_comm");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", gyro_comm_cmd[loop].p_name, gyro_comm_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int gyro_comm_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(gyro_comm, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(gyro_comm_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	//here, we follow standard c main() function, argv[0] should be a "program"
	//argv[0] = "vprc"
	//argv[1] = "(cmd)"
	if (strncmp(argv[1], "?", 2) == 0) {
		gyro_comm_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], gyro_comm_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = gyro_comm_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		gyro_comm_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}


#if defined(__LINUX)
EXPORT_SYMBOL(nvt_gyro_comm_set_obj);
#endif
