/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
SEM_HANDLE     BSMUX_SEMID_SAVEQ_CTL = 0;
SEM_HANDLE     BSMUX_SEMID_MKHDR_CTL = 0;
SEM_HANDLE     BSMUX_SEMID_STATUS_CTL = 0;
SEM_HANDLE     BSMUX_SEMID_ACTION_CTL = 0;
SEM_HANDLE     BSMUX_SEMID_BUFFER_CTL = 0;
BSMUX_CTRL_OBJ gBsMuxCtrlObj = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void bsmux_install_id(void)
{
	OS_CONFIG_SEMPHORE(BSMUX_SEMID_SAVEQ_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(BSMUX_SEMID_MKHDR_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(BSMUX_SEMID_STATUS_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(BSMUX_SEMID_ACTION_CTL, 0, 1, 1);
	OS_CONFIG_SEMPHORE(BSMUX_SEMID_BUFFER_CTL, 0, 1, 1);

	OS_CONFIG_FLAG(gBsMuxCtrlObj.FlagID);
	OS_CONFIG_SEMPHORE(gBsMuxCtrlObj.SemID, 0, 1, 1);
}

void bsmux_uninstall_id(void)
{
	vos_sem_destroy(BSMUX_SEMID_SAVEQ_CTL);
	vos_sem_destroy(BSMUX_SEMID_MKHDR_CTL);
	vos_sem_destroy(BSMUX_SEMID_STATUS_CTL);
	vos_sem_destroy(BSMUX_SEMID_ACTION_CTL);
	vos_sem_destroy(BSMUX_SEMID_BUFFER_CTL);

	vos_flag_destroy(gBsMuxCtrlObj.FlagID);
	vos_sem_destroy(gBsMuxCtrlObj.SemID);
}

PBSMUX_CTRL_OBJ bsmux_get_ctrl_obj(void)
{
	BSMUX_CTRL_OBJ *pCtrlObj;

	pCtrlObj = &gBsMuxCtrlObj;

	if (0 == pCtrlObj->SemID) {
		DBG_ERR("ID not installed\r\n");
		return NULL;
	}

	return pCtrlObj;
}

void bsmux_saveq_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(BSMUX_SEMID_SAVEQ_CTL);
}

void bsmux_saveq_unlock(void)
{
	vos_sem_sig(BSMUX_SEMID_SAVEQ_CTL);
}

void bsmux_mkhdr_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(BSMUX_SEMID_MKHDR_CTL);
}

void bsmux_mkhdr_unlock(void)
{
	vos_sem_sig(BSMUX_SEMID_MKHDR_CTL);
}

void bsmux_status_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(BSMUX_SEMID_STATUS_CTL);
}

void bsmux_status_unlock(void)
{
	vos_sem_sig(BSMUX_SEMID_STATUS_CTL);
}

void bsmux_action_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(BSMUX_SEMID_ACTION_CTL);
}

void bsmux_action_unlock(void)
{
	vos_sem_sig(BSMUX_SEMID_ACTION_CTL);
}

void bsmux_buffer_lock(void)
{
	// vos_sem_wait is identical to vos_sem_wait_interruptible
	vos_sem_wait(BSMUX_SEMID_BUFFER_CTL);
}

void bsmux_buffer_unlock(void)
{
	vos_sem_sig(BSMUX_SEMID_BUFFER_CTL);
}

