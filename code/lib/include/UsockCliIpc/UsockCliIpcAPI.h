/**
    User socket client export variables and function prototypes.

    UsockCliIpc is simple socket client API.
    Through this API user can send and receive network data.

    @file       UsockCliIpcAPI.h
    @ingroup    mIUsockCliIpcAPI

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/
#ifndef _USOCKCLIIPCAPI_H
#define _USOCKCLIIPCAPI_H

#ifdef __ECOS
#define USE_IPC                  0
#elif defined (__FREERTOS)
#define USE_IPC                  0
#elif defined (__LINUX_USER__)
#define USE_IPC                  0
#else
#define USE_IPC                  1
#endif
#include "kwrap/type.h"
/**
    @name Usocket client error status.
*/
//@{
#define USOCKETCLI_RET_OK           0          // funcion success
#define USOCKETCLI_RET_OPENED       1          // is opend
#define USOCKETCLI_RET_ERR          (-1)       // function fail
#define USOCKETCLI_RET_NO_FUNC      (-2)       // no command function
#define USOCKETCLI_RET_NOT_OPEN     (-3)       // not open ipc
#define USOCKETCLI_RET_ERR_PAR      (-4)       // error parameter
//@}

typedef struct
{
    UINT32                  sharedMemAddr;  // shared memory address
    UINT32                  sharedMemSize;  // shared memory size
} USOCKCLIIPC_OPEN;
#if (!USE_IPC)
#ifdef __ECOS //use thirdParty include file
#include <cyg/usocket_cli/usocket_cli.h>
#else //use tmp usocket.h file for compile
#include "usocket_cli.h"
#endif
#else


/**
    Usocket client receive prototype.
    User can register receive callback in UsockCliIpc_Connect().
    It would be a callback function so it would use usocket client task.

    @param[out] addr    receive buffer address.
    @param[out] size    receive data size.

    @return user can return result back.
*/
typedef int usocket_cli_recv(char* addr, int size);

/**
    Usocket client notify prototype.
    User can register notify callback in UsockCliIpc_Connect().

    @param[out] status    client connet status.
    @param[out] parm      connet parameter.

    @return void.

*/
typedef void usocket_cli_notify(int status,int parm);

#define USOCKETCLI_IP_LEN          (16)

/**
    Usocket install structure.
*/
typedef struct
{
    usocket_cli_notify         *notify;        // notify the status
    usocket_cli_recv           *recv;
    char                       svrIP[USOCKETCLI_IP_LEN];   // server IP addr
    int                        portNum;        // socket port number
    int                        sockbufSize;    // socket buffer size
    int                        timeout;
    int                        connect_socket;
} usocket_cli_obj;

/**
    @name Usocket client notify status.
*/
//@{
#define CYG_USOCKETCLI_STATUS_CLIENT_CONNECT           0   // client connect.
#define CYG_USOCKETCLI_STATUS_CLIENT_REQUEST           1   // client has request comimg in
#define CYG_USOCKETCLI_STATUS_CLIENT_DISCONNECT        2   // client disconnect.
//@}

#endif
/**
    Install UsockCliIpc flag and semaphore id.

    @return void.
*/
extern void UsockCliIpc_InstallID(void) _SECTION(".kercfg_text");

/**
    Open user socket client IPC task.
    This API would open IPC of user socket.

    @param[in]      pOpen  user should assign none-cacha buffer for IPC

    @return
        - @b E_OK if success, others fail
*/
extern ER UsockCliIpc_Open(USOCKCLIIPC_OPEN *pOpen);

/**
    Close user socket client IPC task.
    This API would close IPC of user socket client .
    It should run after UsockCliIpc_Stop();

    @return
        - @b E_OK if success, others fail
*/
extern ER   UsockCliIpc_Close(void);

/**
    UsockCliIpc Start.
    It would start thread to wait receive data

    @return
        - @b E_OK if success, others fail
*/
extern ER    UsockCliIpc_Start(void);

/**
    UsockCliIpc Connect.
    This is user socket client initialization function.
    It would connect to socket server
    @param[out] pObj       init socket client parameter.

    @return
        - @b E_OK if success, others fail
*/
extern INT32 UsockCliIpc_Connect(usocket_cli_obj*  pObj);

/**
    User socket client send data.

    @param[in] handle      client socket handle.

    @param[out] addr       Send buffer address.
                           Buffer address must be 16 bytes alignment.
                           Buffer size also need 16 byte alignment;even data is not alignment.
    @param[in,out] size    Send data size,and would return real send data size.
                           Size address must be 16 bytes alignment.

    @return
        - @b (0)  send success.
        - @b (-1) send fail.

    @return
        - @b E_OK if success, others fail
*/
extern ER   UsockCliIpc_Send(INT32 handle,char* addr, int* size);

/**
    UsockCliIpc Disconnect.

    It would disconnect to socket server
    @param[in,out] handle       client socket handle.

    @return
        - @b E_OK if success, others fail
*/
extern ER   UsockCliIpc_Disconnect(INT32 *handle);

/**
    User socket client Uninit.
    This API would close user socket.

    @return
        - @b E_OK if success, others fail
*/
extern ER   UsockCliIpc_Stop(void);

#endif /* _USOCKCLIIPCAPI_H  */
