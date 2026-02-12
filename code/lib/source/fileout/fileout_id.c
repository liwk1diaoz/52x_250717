/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
ID FILEOUT_FLGID_PORT_CTL = 0;
SEM_HANDLE FILEOUT_SEMID_QUEUE_CTL = 0;
SEM_HANDLE FILEOUT_SEMID_PATH_CTL = 0;
SEM_HANDLE FILEOUT_SEMID_OPS_CTL = 0;

FILEOUT_CTRL_OBJ gFileOutCtrlObj[FILEOUT_DRV_NUM] = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void fileout_install_id(void)
{
	OS_CONFIG_FLAG(FILEOUT_FLGID_PORT_CTL);
	OS_CONFIG_SEMPHORE(FILEOUT_SEMID_QUEUE_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(FILEOUT_SEMID_PATH_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(FILEOUT_SEMID_OPS_CTL, 0, 1, 1);

	//OS_CONFIG_TASK(gFileOutCtrlObj[0].TaskID, PRI_ISF_FILEOUT,  STKSIZE_ISF_FILEOUT,  ISF_FileOut_Tsk_A);
	OS_CONFIG_FLAG(gFileOutCtrlObj[0].FlagID);
	OS_CONFIG_SEMPHORE(gFileOutCtrlObj[0].SemID, 0, 1, 1);

	//OS_CONFIG_TASK(gFileOutCtrlObj[1].TaskID, PRI_ISF_FILEOUT,  STKSIZE_ISF_FILEOUT,  ISF_FileOut_Tsk_B);
	OS_CONFIG_FLAG(gFileOutCtrlObj[1].FlagID);
	OS_CONFIG_SEMPHORE(gFileOutCtrlObj[1].SemID, 0, 1, 1);
}

void fileout_uninstall_id(void)
{
	vos_flag_destroy(FILEOUT_FLGID_PORT_CTL);
	vos_sem_destroy(FILEOUT_SEMID_QUEUE_CTL);
	vos_sem_destroy(FILEOUT_SEMID_PATH_CTL);
	vos_sem_destroy(FILEOUT_SEMID_OPS_CTL);

	vos_flag_destroy(gFileOutCtrlObj[0].FlagID);
	vos_sem_destroy(gFileOutCtrlObj[0].SemID);

	vos_flag_destroy(gFileOutCtrlObj[1].FlagID);
	vos_sem_destroy(gFileOutCtrlObj[1].SemID);
}

PFILEOUT_CTRL_OBJ fileout_get_ctrl_obj(CHAR drive)
{
	FILEOUT_CTRL_OBJ *pCtrlObj;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	pCtrlObj = &gFileOutCtrlObj[drive - FILEOUT_DRV_NAME_FIRST];

	if (0 == pCtrlObj->SemID) {
		DBG_ERR("ID not installed\r\n");
		return NULL;
	}

	return pCtrlObj;
}

void fileout_queue_lock(UINT32 queue_idx)
{
	FLGPTN ret_flgptn;
	FLGPTN wait_flgptn;

	wait_flgptn = FLGPTN_BIT(queue_idx);

	// vos_flag_wait is identical to vos_flag_wait_interruptible
	vos_flag_wait(&ret_flgptn, FILEOUT_FLGID_PORT_CTL, wait_flgptn, TWF_ORW | TWF_CLR);
}

void fileout_queue_unlock(UINT32 queue_idx)
{
	FLGPTN flgptn;

	flgptn = FLGPTN_BIT(queue_idx);

	if (vos_flag_chk(FILEOUT_FLGID_PORT_CTL, flgptn)) {
		DBG_WRN("already unlocked, queue_idx %d\r\n", (int)queue_idx);
	}

	vos_flag_set(FILEOUT_FLGID_PORT_CTL, flgptn);
}
STATIC_ASSERT(ISF_FILEOUT_IN_NUM <= 32); //for FLGPTN_BIT

void fileout_ops_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(FILEOUT_SEMID_OPS_CTL);
}

void fileout_ops_unlock(void)
{
	vos_sem_sig(FILEOUT_SEMID_OPS_CTL);
}

void fileout_qhandle_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(FILEOUT_SEMID_QUEUE_CTL);
}

void fileout_qhandle_unlock(void)
{
	vos_sem_sig(FILEOUT_SEMID_QUEUE_CTL);
}

void fileout_path_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(FILEOUT_SEMID_PATH_CTL);
}

void fileout_path_unlock(void)
{
	vos_sem_sig(FILEOUT_SEMID_PATH_CTL);
}
