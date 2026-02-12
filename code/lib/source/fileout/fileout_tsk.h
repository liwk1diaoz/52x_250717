/**
    @brief 		Header file of fileout library.

    @file 		fileout_tsk.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _FILEOUT_TSK_H
#define _FILEOUT_TSK_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
extern void ISF_FileOut_Tsk_A(void);
extern void ISF_FileOut_Tsk_B(void);

INT32 fileout_tsk_init(void);
void fileout_tsk_start(UINT32 drive);
void fileout_tsk_destroy(UINT32 drive);
void fileout_tsk_destroy_multi(void);
INT32 fileout_tsk_trigger_ops(CHAR drive);
INT32 fileout_tsk_trigger_relque(CHAR drive);
INT32 fileout_tsk_sysinfo_init(CHAR drive);
INT32 fileout_tsk_sysinfo_uninit(CHAR drive);
#endif //_FILEOUT_TSK_H