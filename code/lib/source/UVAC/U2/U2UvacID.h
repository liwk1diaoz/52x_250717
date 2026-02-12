#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "UVAC.h"

#ifndef _U2UVACID_H
#define _U2UVACID_H

#define PRI_UVACVIDEO           10
#define PRI_UVACISOIN           (PRI_UVACVIDEO + 2)
#define PRI_UVACISOOUT          4
#define STKSIZE_UVACVIDEO           8192 //8192  //4096
#define STKSIZE_UVACISOIN_VID       4096  //2048
#define STKSIZE_UVACISOIN_AUD       2048
#define STKSIZE_UVACISOOUT          4096  //2048

extern THREAD_HANDLE U2UVACVIDEOTSK_ID; ///< task id
extern THREAD_HANDLE U2UVACVIDEOTSK_ID2; ///< task id
//extern THREAD_HANDLE U2UVACISOINTSK_ID; ///< task id
extern THREAD_HANDLE U2UVAC_TX_VDO1_ID; ///< task id
extern THREAD_HANDLE U2UVAC_TX_VDO2_ID; ///< task id
extern THREAD_HANDLE U2UVAC_TX_AUD1_ID; ///< task id
extern THREAD_HANDLE U2UVAC_TX_AUD2_ID; ///< task id
extern THREAD_HANDLE U2UVAC_UAC_RX_ID;
extern ID FLG_ID_U2UVAC; ///< flag id
extern ID FLG_ID_U2UVAC_UAC; ///< flag id
extern ID FLG_ID_U2UVAC_FRM; ///< flag id
extern ID FLG_ID_U2UVAC_UAC_RX;
extern ID SEMID_U2UVC_QUEUE; ///< semaphore id
extern ID SEMID_U2UVC_READ_CDC; ///< semaphore id
extern ID SEMID_U2UVC_WRITE_CDC; ///< semaphore id
extern ID SEMID_U2UVC_READ2_CDC; ///< semaphore id
extern ID SEMID_U2UVC_WRITE2_CDC; ///< semaphore id
extern ID SEMID_U2UVC_WRITE_HID;
extern ID SEMID_U2UVC_READ_HID;
extern ID SEMID_U2UVC_UAC_QUEUE;
extern ID FLG_ID_U2SIDC; ///< flag id
extern ID FLG_ID_U2SIDCINTR; ///< flag id
#if 0//gU2UvacMtpEnabled
extern UINT32 _SECTION(".kercfg_data") U2UVACSIDCTSK_ID; ///< task id
extern UINT32 _SECTION(".kercfg_data") U2UVACSIDCINTRTSK_ID; ///< task id
extern UINT32 _SECTION(".kercfg_data") SEMID_U2UVC_OBJ_SIDC; ///< semaphore id
extern UINT32 _SECTION(".kercfg_data") SEMID_U2UVC_DIR_SIDC; ///< semaphore id
#endif

#endif //_UVACID_H
