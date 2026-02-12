/******************************************************************************

   Copyright 2010-2020 Hikvision Co.,Ltd

   FileName: bas_task.c

   Description:封装线程创建函数

   Author:DSP_AI

   Date:2018-8-7

   Modification History: <version> <time> <author> <desc>
   a)
   V1.0.0 2018-8-7 DSP_AI    创建
******************************************************************************/
#include "bas_task_api.h"
#include <stdarg.h>

static pthread_mutex_t muOnTask = PTHREAD_MUTEX_INITIALIZER;
static BAS_TASK_INFO_LIST *pTaskInfoList = NULL;

/*************************************************
   Function:		bas_task_get_priority_scope
   Description:	       获取线程优先级范围
   Input:	              pMinPriority 最小优先级输出
                            pMaxPriority 最大优先级输出
   Output:		无
   Return:		0 成功 其他失败
*************************************************/
static int bas_task_get_priority_scope(int *pMinPriority, int *pMaxPriority)
{
    if (pMinPriority != NULL)
    {
        (*pMinPriority) = sched_get_priority_min(SCHED_RR);
        if (*pMinPriority == -1)
        {
            return FAILURE;
        }
    }

    if (pMaxPriority != NULL)
    {
        (*pMaxPriority) = sched_get_priority_max(SCHED_RR);
        if (*pMaxPriority == -1)
        {
            return FAILURE;
        }
    }

    return OK;
}

/*************************************************
   Function:		bas_task_set_attr
   Description:	设置线程属性
   Input:	      pThreadAttr 最小优先级输出
                         priority 优先级
                         stackSize 线程 堆栈大小
   Output:		无
   Return:		0 成功 其他失败
*************************************************/
static int bas_task_set_attr(pthread_attr_t *pThreadAttr, int priority, unsigned int stackSize)
{
    int rval = 0;
    struct sched_param params;
    int maxPriority = 0, minPriority = 0;

    if (NULL == pThreadAttr)
    {
        printf("param error pThreadAttr:%p\n", pThreadAttr);
        return FAILURE;
    }

    rval = pthread_attr_init(pThreadAttr);
    if (rval != 0)
    {
        return FAILURE;
    }

    /* use the round robin scheduling algorithm */
    rval = pthread_attr_setschedpolicy(pThreadAttr, SCHED_RR);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    /* set the thread to be detached */
    rval = pthread_attr_setdetachstate(pThreadAttr, PTHREAD_CREATE_DETACHED);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    /* first get the scheduling parameter, then set the new priority */
    rval = pthread_attr_getschedparam(pThreadAttr, &params);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    rval = bas_task_get_priority_scope(&minPriority, &maxPriority);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    if (priority < minPriority)
    {
        priority = minPriority;
    }
    else if (priority > maxPriority)
    {
        priority = maxPriority;
    }

    params.sched_priority = priority;
    rval = pthread_attr_setschedparam(pThreadAttr, &params);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    /* when set stack size, we define a minmum value to avoid fail */
    if (stackSize < BAS_TASK_MIN_STACK_SIZE)
    {
        stackSize = BAS_TASK_MIN_STACK_SIZE;
    }

    rval = pthread_attr_setstacksize(pThreadAttr, stackSize);
    if (rval != 0)
    {
        pthread_attr_destroy(pThreadAttr);
        return FAILURE;
    }

    return OK;
}

/*************************************************
   Function:		bas_task_thread_creat
   Description:	线程创建
   Input:	      pThreadID 最小优先级输出
                priority 优先级
                stacksize 堆栈大小
                pFunc线程运行参数
                args 参数
   Output:		无
   Return:		0 成功 其他失败
*************************************************/
/*lint --e{579,530}*/
static int bas_task_thread_creat(
    pthread_t *pThreadID,
    int priority,
    unsigned int nStacksize,
    void *pFunc,
    char args, ...
    )
{
    int i = 0;
    void *arg[10] = {NULL};
    pthread_t tid, *raw = NULL;
    int rval = FAILURE;
    va_list ap;
    pthread_attr_t attr;

    if (pFunc == NULL || args > 10)
    {
        return FAILURE;
    }

    va_start(ap, args);
    for (i = 0; i < args; i++)
    {
        arg[i] = va_arg(ap, void *);
    }

    va_end(ap);

    if (pThreadID != NULL)
    {
        raw = pThreadID;
    }
    else
    {
        raw = &tid;
    }

    if (OK != bas_task_set_attr(&attr, priority, nStacksize))
    {
        return FAILURE;
    }

    if (args <= 1)
    {
        rval = pthread_create((pthread_t *)raw, &attr, pFunc, arg[0]);
        if (rval)
        {
            rval = FAILURE;
        }
        else
        {
            rval = OK;
        }
    }
    else
    {
        /*多个参数暂未实现*/
        return FAILURE;
    }

    pthread_attr_destroy(&attr);
    return rval;
}

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
                   char *pName)
{
    int err = 0;

#ifdef BAS_TASK_STATUS_DEBUG
    unsigned int cnt = 0;
#endif

    if ((pTaskIdx == NULL) || (pFunc == NULL))
    {
        printf("bas_task_creat param error\n");
        return FAILURE;
    }

#ifdef BAS_TASK_STATUS_DEBUG
    pthread_mutex_lock(&muOnTask);
    if (!pTaskInfoList)
    {
        pTaskInfoList = malloc(sizeof(BAS_TASK_INFO_LIST));
        if (!pTaskInfoList)
        {
            printf("pTaskInfoList malloc failed\n");
            pthread_mutex_unlock(&muOnTask);
            return FAILURE;
        }

        memset(pTaskInfoList, 0, sizeof(BAS_TASK_INFO_LIST));
    }

    cnt = pTaskInfoList->taskCnt;

    err = bas_task_thread_creat(pTaskIdx, priority, stacksize, pFunc, 1, param);

    if (cnt > BAS_TASK_MAX_CNT)
    {
        /* 超过记录个数直接返回 */
        printf("thread exceed max cnt\n");
        pthread_mutex_unlock(&muOnTask);
        return OK;
    }

    pTaskInfoList->taskInfo[cnt].arg0 = param;
    pTaskInfoList->taskInfo[cnt].alive = 1;
    pTaskInfoList->taskInfo[cnt].switchFps = 0;
    pTaskInfoList->taskInfo[cnt].workFps = 0;
    pTaskInfoList->taskInfo[cnt].stackSize = stacksize;
    pTaskInfoList->taskInfo[cnt].pid = *(unsigned int *)pTaskIdx;
    pTaskInfoList->taskInfo[cnt].priority = priority;
    if (pName)
    {
        strncpy(pTaskInfoList->taskInfo[cnt].name, pName, BAS_TASK_MAX_NAME_LEN);
    }

    pTaskInfoList->taskCnt++;
    pthread_mutex_unlock(&muOnTask);
#else
    err = bas_task_thread_creat(pTaskIdx, priority, stacksize, pFunc, 1, param);
#endif

    return err;
}

/*******************************************************************************
   Function:             bas_task_destroy
   Description:         任务销毁
   Input:                    pTaskIdx 线程ID
   Output:               无
   Return:               0成功 其他失败
*******************************************************************************/
int bas_task_destroy(TSK_IDX *pTaskIdx)
{
    int err = 0;
    unsigned int idx = 0;

    if (pTaskIdx == NULL)
    {
        printf("bas_task_destroy param error\n");
        return FAILURE;
    }

    pthread_mutex_lock(&muOnTask);

    for (idx = 0; idx < pTaskInfoList->taskCnt; idx++)
    {
        if (*(unsigned int *)pTaskIdx == pTaskInfoList->taskInfo[idx].pid)
        {
            /*将数组后面的往前移*/
            if (idx < (pTaskInfoList->taskCnt - 1))
            {
                memcpy(&pTaskInfoList->taskInfo[idx], &pTaskInfoList->taskInfo[idx + 1],
                       sizeof(BAS_TASK_INFO) * (pTaskInfoList->taskCnt - idx - 1));
            }

            pTaskInfoList->taskCnt--;
        }
    }

    pthread_mutex_unlock(&muOnTask);

    pthread_detach(*pTaskIdx);

    return err;
}

/*******************************************************************************
   Function:             bas_task_set_name
   Description:          增加内核线程统计
   Input:                  pName：线程名称
   Output:               无
   Return:               无
*******************************************************************************/
void bas_task_set_name(char *pName)
{
    if (pName)
    {
        prctl(PR_SET_NAME, pName);
    }

    return;
}

/*******************************************************************************
   Function:             bas_task_status_update
   Description:          线程运行状态更新
   Input:                无
   Output:               无
   Return:               无
*******************************************************************************/
void bas_task_status_update(void)
{

}

/*******************************************************************************
   Function:             bas_task_commit_job
   Description:          统计任务运行
   Input:                tskIdx 任务号
                           cnt 计数
   Output:               无
   Return:               无
*******************************************************************************/
void bas_task_commit_job(unsigned int tskIdx, unsigned int cnt)
{

}

/*******************************************************************************
   Function:             bas_task_get_status_info
   Description:          获取运行状态信息
   Input:                pTaskInfo  任务信息
   Output:               无
   Return:                0成功其他失败
*******************************************************************************/
int bas_task_get_status_info(BAS_TASK_INFO_LIST *pTaskInfo)
{
    if (!pTaskInfo)
    {
        printf("bas_task_get_status_info param error");
        return FAILURE;
    }

    if (!pTaskInfoList)
    {
        printf("pTaskInfoList NULL\n");
        return FAILURE;
    }

    pthread_mutex_lock(&muOnTask);
    memcpy(pTaskInfo, pTaskInfoList, sizeof(BAS_TASK_INFO_LIST));
    pthread_mutex_unlock(&muOnTask);

    return OK;
}

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
int bas_task_get_status_idx(unsigned int *pTskIdx, char *pTskName, int tskParam, int bHaveParam)
{
    unsigned int idx = 0;
    unsigned int cnt = 0;
    int ret = FAILURE;

    if ((NULL == pTskIdx) || (NULL == pTskName) || (NULL == pTaskInfoList))
    {
        return FAILURE;
    }

    pthread_mutex_lock(&muOnTask);
    cnt = pTaskInfoList->taskCnt;

    for (idx = 0; idx < cnt; idx++)
    {
        if (!strcmp((char *)pTaskInfoList->taskInfo[idx].name, pTskName))
        {
            if (bHaveParam)
            {
                if (tskParam == pTaskInfoList->taskInfo[idx].arg0)
                {
                    pTaskInfoList->taskInfo[idx].pid = pthread_self();
                    ret = OK;
                    *pTskIdx = idx;
                }
            }
            else
            {
                pTaskInfoList->taskInfo[idx].pid = pthread_self();
                ret = OK;
                *pTskIdx = idx;
            }
        }
    }

    pthread_mutex_unlock(&muOnTask);

    return ret;
}

