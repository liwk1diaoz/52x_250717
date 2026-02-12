#ifndef _FILEDB_ID_H
#define _FILEDB_ID_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include "Type.h"
#else
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/type.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define FILEDB_NUM                     3

/*-----------------------------------------------------------------------------*/
/* Extern Variables                                                            */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
extern UINT32 _SECTION(".kercfg_data") SEMID_FILEDB_COMM;
extern UINT32 _SECTION(".kercfg_data") SEMID_FILEDB[FILEDB_NUM];
extern UINT32 _SECTION(".kercfg_data") FLG_ID_FILEDB;
#else
extern SEM_HANDLE _SECTION(".kercfg_data") SEMID_FILEDB_COMM;
extern SEM_HANDLE _SECTION(".kercfg_data") SEMID_FILEDB[FILEDB_NUM];
extern ID _SECTION(".kercfg_data") FLG_ID_FILEDB;
#endif

#endif //_FILEDB_ID_H