#ifndef _USOCKETCLI_IPC_H_
#define _USOCKETCLI_IPC_H_

//=============================================================================
//define USOCKETCLI_SIM for simulation
//#define USOCKETCLI_SIM


//#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24 ))
#define USOCKETCLIIPC_BUF_CHK_SIZE    (4)
#define USOCKETCLIIPC_BUF_TAG         MAKEFOURCC('U','S','C','I')
#define USOCKCLIIPC_PARAM_BUF_SIZE      2048
#define USOCKCLIIPC_TRANSFER_BUF_SIZE   ((USOCKCLIIPC_PARAM_BUF_SIZE/2)-USOCKETCLIIPC_BUF_CHK_SIZE-4) // 4 is size of USOCKETCLI_TRANSFER_PARAM

//-------------------------------------------------------------------------
//USOCKETECOS API return value
/*
#define USOCKETCLI_RET_OK           0
#define USOCKETCLI_RET_OPENED       1
#define USOCKETCLI_RET_ERR          (-1)
#define USOCKETCLI_RET_NO_FUNC      (-2)
#define USOCKETCLI_RET_NOT_OPEN     (-3)
#define USOCKETCLI_RET_ERR_PAR      (-4)
*/

#define USOCKETCLI_TOKEN_PATH       "usockcliipc"
#define USOCKETCLI_MAX_PARAM                6
//-------------------------------------------------------------------------
//IPC command id and parameters
typedef enum{
    USOCKETCLI_CMDID_GET_VER_INFO  =   0x00000000,
    USOCKETCLI_CMDID_GET_BUILD_DATE,
    USOCKETCLI_CMDID_START,
    USOCKETCLI_CMDID_CONNECT,
    USOCKETCLI_CMDID_DISCONNECT,
    USOCKETCLI_CMDID_STOP,
    USOCKETCLI_CMDID_SEND,
    USOCKETCLI_CMDID_RCV,
    USOCKETCLI_CMDID_NOTIFY,
    USOCKETCLI_CMDID_SYSREQ_ACK,
    USOCKETCLI_CMDID_UNINIT,
}USOCKETCLI_CMDID;

typedef struct{
    USOCKETCLI_CMDID CmdId;
    int Arg;
}USOCKETCLI_MSG;

typedef struct{
    int param1;
    int param2;
    int param3;
}USOCKETCLI_PARAM_PARAM;

typedef struct{
    int obj;
    int size;
    char buf[USOCKCLIIPC_TRANSFER_BUF_SIZE];
}USOCKETCLI_TRANSFER_PARAM;

//-------------------------------------------------------------------------
//USOCKET definition

typedef struct _USOCKETCLI_OPEN_OBJ{
    unsigned int        RcvParamAddr;
    unsigned int        SndParamAddr;
}USOCKETCLI_OPEN_OBJ;


#ifndef __externC
#ifdef __cplusplus
# define __externC extern "C"
#else
# define __externC extern
#endif
#endif

__externC void USOCKETECOS_CmdLine(char *szCmd);

#endif //_USOCKETCLI_IPC_H_

