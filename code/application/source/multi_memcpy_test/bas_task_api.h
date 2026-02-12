/******************************************************************************

   Copyright 2010-2020 Hikvision Co.,Ltd

   FileName: bas_task_api.h

   Description: 信号量使用公共文件

   Author:DSP-AI

   Date: 2018-08-06

   Modification History: <version> <time> <author> <desc>
   a)

    V1.0.0 2018-8-6 DSP_AI	  创建
******************************************************************************/
#ifndef _BAS_TASK_API_H_
#define _BAS_TASK_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/prctl.h>

/*最小线程栈大小*/
#define BAS_TASK_MIN_STACK_SIZE	(128 * 1024)
#define FAILURE -1
#define OK 0
/*线程*/
typedef pthread_t TSK_IDX;

/*最大创建任务数*/
#define BAS_TASK_MAX_CNT 1024
/*线程名最大长度*/
#define BAS_TASK_MAX_NAME_LEN 32
/*任务信息*/
typedef struct
{
    /*任务id*/
    unsigned int pid;
    /*任务参数*/
    unsigned int arg0;
    /*任务是否有效*/
    unsigned int alive;
    /*任务切换频率*/
    unsigned int switchFps;
    /*任务执行频率*/
    unsigned int workFps;
    /*任务CPU消耗*/
    unsigned int cpuLoad;
    /*任务优先级*/
    unsigned int priority;
    /*任务名称*/
    char name[BAS_TASK_MAX_NAME_LEN];
    /*任务堆栈大小*/
    unsigned int stackSize;
    /*保留字段*/
    unsigned int reserved[5];
} BAS_TASK_INFO, *PBAS_TASK_INFO;


/*任务task状态*/
typedef struct
{
    unsigned int taskCnt;
    BAS_TASK_INFO taskInfo[BAS_TASK_MAX_CNT];
} BAS_TASK_INFO_LIST, *PBAS_TASK_INFO_LIST;


/*******************************************************************************
   Function:             bas_task_creat
   Description:          任务创建,带有debug信息
   Input:                    pTaskIdx 线程ID
                          priority  优先级
                          stacksize 线程堆栈大小
                          pFunc 线程处理函数
                          param 线程处理参数
                          pName 线程名
   Output:               无
   Return:               0成功 其他失败
*******************************************************************************/
int bas_task_creat(TSK_IDX *pTaskIdx,
                   int priority,
                   int stacksize,
                   void *pFunc,
                   int param,
                   char *pName);

/*******************************************************************************
   Function:             bas_task_destroy
   Description:         任务销毁
   Input:                    pTaskIdx 线程ID
   Output:               无
   Return:               0成功 其他失败
*******************************************************************************/
int bas_task_destroy(TSK_IDX *pTaskIdx);

/*******************************************************************************
   Function:             bas_task_set_name
   Description:          增加内核线程统计
   Input:                  pName：线程名称
   Output:               无
   Return:               无
*******************************************************************************/
void bas_task_set_name(char *pName);

/*******************************************************************************
   Function:             bas_task_get_status_info
   Description:          获取运行状态信息
   Input:                pTaskInfo  任务信息
   Output:               无
   Return:                0成功其他失败
*******************************************************************************/
int bas_task_get_status_info(BAS_TASK_INFO_LIST *pTaskInfo);

/*******************************************************************************
   Function:             bas_task_get_status_idx
   Description:          获取任务id号
   Input:                  pTskIdx 任务ID
                             pTskName 任务名
                           tskParam 任务参数
                          bHaveParam 该任务是否有参数
   Output:               无
   Return:               0成功其他失败
*******************************************************************************/
int bas_task_get_status_idx(unsigned int *pTskIdx, char *pTskName, int tskParam, int bHaveParam);

#ifdef __cplusplus
}
#endif

#endif


