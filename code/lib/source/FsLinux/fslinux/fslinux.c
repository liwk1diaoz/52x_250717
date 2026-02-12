#include <sys/ipc.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fslinux_cmd.h"
#include "fslinux_debug.h"
#include "fslinux_ipc.h"

/*
#define SEM_HANDLE pthread_mutex_t
#define SEM_CREATE(handle,init_cnt) pthread_mutex_init(&handle,NULL);if(init_cnt==0)pthread_mutex_lock(&handle)
#define SEM_SIGNAL(handle) pthread_mutex_unlock(&handle)
#define SEM_WAIT(handle) pthread_mutex_lock(&handle);
#define SEM_TRYWAIT(handle) pthread_mutex_trylock(&handle)
#define SEM_DESTROY(handle) pthread_mutex_destroy(&handle)
*/
#if FSLINUX_USE_IPC
#include <semaphore.h>
#define SEM_HANDLE sem_t
#define SEM_CREATE(handle,init_cnt) sem_init(&handle,0,init_cnt)
#define SEM_SIGNAL(handle) sem_post(&handle)
#define SEM_WAIT(handle) sem_wait(&handle);
#define SEM_TRYWAIT(handle) sem_trywait(&handle)
#define SEM_DESTROY(handle) sem_destroy(&handle)

//#define THREAD_DESTROY(thread_handle) if (thread_handle)pthread_cancel(thread_handle)
#define THREAD_RETURN() pthread_detach(pthread_self());pthread_exit(NULL);

static SEM_HANDLE g_mutex_main;

typedef struct {
    char Drive;
    SEM_HANDLE SemStart;
    SEM_HANDLE SemDone;
    pthread_t ThreadId;
    FSLINUX_IPCMSG IpcMsg;
} FSLINUX_THREAD_CTRL;

FSLINUX_THREAD_CTRL g_ThreadCtrl[FSLINUX_DRIVE_NUM] = {0};

static void DirectDoCmd(FSLINUX_IPCMSG *pMsgRcv)
{
	FSLINUX_CMDID CmdId;
	//struct timeval t1, t2;

	CmdId = pMsgRcv->CmdId;

	if (FSLINUX_CMDID_GETDISKINFO != CmdId) {
		DBG_ERR("Invalid CmdId %d\r\n", CmdId);
	}

	//gettimeofday(&t1,NULL);

	if (fslinux_cmd_DoCmd(pMsgRcv) < 0) {
        DBG_ERR("fslinux_DoCmdFunc failed\r\n");
		return;
    }

	//gettimeofday(&t2,NULL);
	//DBG_DUMP("t1 sec %d usec %d\n", t1.tv_sec, t1.tv_usec);
	//DBG_DUMP("t2 sec %d usec %d\n", t2.tv_sec, t2.tv_usec);

    //ack the same msg
    if (fslinux_ipc_AckCmd(pMsgRcv) < 0) {
        DBG_ERR("fslinux_ipc_AckCmd(%d) failed\r\n", CmdId);
    }
}

static void* ThreadFunc_DrvCmd(void* arg)
{
    FSLINUX_THREAD_CTRL *pThreadCtrl = (FSLINUX_THREAD_CTRL *)arg;

    DBG_IND("begin %c\r\n", pThreadCtrl->Drive);

    while(1)
    {
        SEM_WAIT(pThreadCtrl->SemStart);

        if(fslinux_cmd_DoCmd(&pThreadCtrl->IpcMsg) < 0)
        {
            DBG_ERR("fslinux_DoCmdFunc failed\r\n");
            break;
        }

        //Set SemDone before AckCmd to prevent IPC receiving a new command before we set SemDone
        SEM_SIGNAL(pThreadCtrl->SemDone);

        //ack the same msg
        if(fslinux_ipc_AckCmd(&pThreadCtrl->IpcMsg) < 0)
        {
            DBG_ERR("fslinux_ipc_AckCmd(%d) failed\r\n", pThreadCtrl->IpcMsg.CmdId);
            break;
        }

        if(FSLINUX_CMDID_FINISH == pThreadCtrl->IpcMsg.CmdId)
            break;
    }

    DBG_IND("end %c\r\n", pThreadCtrl->Drive);

    SEM_DESTROY(pThreadCtrl->SemStart);
    SEM_DESTROY(pThreadCtrl->SemDone);
    memset(pThreadCtrl, 0, sizeof(FSLINUX_THREAD_CTRL));

    THREAD_RETURN();
}

static void* ThreadFunc_RcvMsg(void* arg)
{
    FSLINUX_IPCMSG MsgRcv = {0};
    FSLINUX_THREAD_CTRL *pThreadCtrl = NULL;

    DBG_IND("begin\r\n");

    while(1)
    {
        if(fslinux_ipc_WaitCmd(&MsgRcv) < 0)
        {
            DBG_ERR("fslinux_ipc_WaitCmd failed\r\n");
            break;
        }

        if(MsgRcv.CmdId == FSLINUX_CMDID_IPC_DOWN)
        {
            //if we got IPC down here, it assumes that all DrvCmd threads are terminated by finish cmd
            //so we directly ack this cmd and finish this rcv thread
            if(fslinux_ipc_AckCmd(&MsgRcv) < 0)
                DBG_WRN("fslinux_ipc_AckCmd(%d) failed\r\n", MsgRcv.CmdId);

            break; // terminate this thread
        }

		if (MsgRcv.CmdId == FSLINUX_CMDID_GETDISKINFO) {
			DirectDoCmd(&MsgRcv);
			continue;
		}

        pThreadCtrl = &g_ThreadCtrl[MsgRcv.Drive - FSLINUX_DRIVE_NAME_FIRST];
        if(0 == pThreadCtrl->ThreadId)
        {//create a thread for the drive
            pThreadCtrl->Drive = MsgRcv.Drive;
            SEM_CREATE(pThreadCtrl->SemStart, 0);
            SEM_CREATE(pThreadCtrl->SemDone, 1);

            DBG_IND("Create thread %c\r\n", pThreadCtrl->Drive);
            if(0 != pthread_create(&pThreadCtrl->ThreadId, NULL, ThreadFunc_DrvCmd, (void*)pThreadCtrl))
            {
                DBG_ERR("pthread_create failed\r\n");
                break;
            }
        }

        //We should always get SemDone here, because uITRON FsLinux should send one cmd per drive at a time
        if(0 != SEM_TRYWAIT(pThreadCtrl->SemDone))
        {
            DBG_ERR("The previous command(%d) not done\r\n", pThreadCtrl->IpcMsg.CmdId);
            break;
        }

        //copy the received message and trigger the owner thread to process
        memcpy(&pThreadCtrl->IpcMsg, &MsgRcv, sizeof(MsgRcv));
        SEM_SIGNAL(pThreadCtrl->SemStart);
    }

    DBG_IND("end\r\n");

    SEM_SIGNAL(g_mutex_main);
    THREAD_RETURN();
}

void fslinux_SigInt(int sig)
{
    fslinux_ipc_Uninit();
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

void fslinux_SigKill(int sig)
{
    fslinux_ipc_Uninit();
    signal(SIGKILL, SIG_DFL);
    kill(getpid(), SIGKILL);
}

int main(int argc, char *argv[])
{
    FSLINUX_IPCMSG AckMsg = {0};
    pthread_t ThreadId_RcvMsg;
	int ret_thread;

    if(argc < 2)
    {
        DBG_ERR("No interface ver\r\n");
        goto LABEL_EXIT;
    }

    if(0 != strcmp(argv[1], FSLINUX_INTERFACE_VER))
    {
        DBG_ERR("u:%s L:%s not matched\r\n", argv[1], FSLINUX_INTERFACE_VER);
        goto LABEL_EXIT;
    }

    if(fslinux_cmd_Init() < 0)
    {
        DBG_ERR("fslinux_cmd_Init failed\r\n");
        goto LABEL_EXIT;
    }

    if(fslinux_ipc_Init() < 0)
    {
        DBG_ERR("fslinux_ipc_Init failed\r\n");
        goto LABEL_EXIT;
    }

    //register SIGINT & SIGKILL after getting fslinux_IPC_Init
    signal(SIGINT, fslinux_SigInt);
    signal(SIGKILL, fslinux_SigKill);

    //create the service thread
	ret_thread = pthread_create(&ThreadId_RcvMsg, NULL, ThreadFunc_RcvMsg, 0);
    if(0 != ret_thread)
    {
        DBG_ERR("pthread_create failed, err %d\r\n", ret_thread);
        goto LABEL_EXIT;
    }

    //ack the syscall request command
    AckMsg.CmdId = FSLINUX_CMDID_ACKPID;
    AckMsg.Drive = '*';
    AckMsg.phy_ArgAddr = getpid();
    DBG_IND("pid = %d\r\n", AckMsg.phy_ArgAddr);
    if(fslinux_ipc_AckCmd(&AckMsg) < 0)
    {
        DBG_ERR("fslinux_ipc_AckCmd failed\r\n");
        goto LABEL_EXIT;
    }

    SEM_CREATE(g_mutex_main,0);
    SEM_WAIT(g_mutex_main);
    SEM_DESTROY(g_mutex_main);

LABEL_EXIT:
    fslinux_ipc_Uninit();
    return 0;
}
#endif
