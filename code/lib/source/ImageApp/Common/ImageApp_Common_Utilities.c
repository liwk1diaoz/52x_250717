#include "ImageApp_Common_int.h"

/// ========== Cross file variables ==========
/// ========== Noncross file variables ==========

ER _LinkPortCntInc(UINT32 *pOrgCnt)
{
	vos_sem_wait(IACOMMON_SEMID_PORT_STATUS);
	if (*pOrgCnt & (PORT_INC_IND | PORT_DEC_IND)) {
		DBG_WRN("Count is already changed but not set yet!\r\n");
		vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
		return E_SYS;
	}
	*pOrgCnt += 1;
	*pOrgCnt |= PORT_INC_IND;
	vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
	return E_OK;
}

ER _LinkPortCntDec(UINT32 *pOrgCnt)
{
	vos_sem_wait(IACOMMON_SEMID_PORT_STATUS);
	if (*pOrgCnt & (PORT_INC_IND | PORT_DEC_IND)) {
		DBG_WRN("Count is already changed but not set yet!\r\n");
		vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
		return E_SYS;
	}
	if ((*pOrgCnt & ~(PORT_INC_IND | PORT_DEC_IND))== 0) {
		DBG_WRN("Count is already 0!\r\n");
		vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
		return E_SYS;
	}
	*pOrgCnt -= 1;
	*pOrgCnt |= PORT_DEC_IND;
	vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
	return E_OK;
}

ER _LinkUpdateStatus(MODULE_SET_STATE fp, HD_PATH_ID path_id, UINT32 *pTargetStatus, LINKUPDATESTATUS_CB fp_cb)
{
	HD_RESULT ret;

	vos_sem_wait(IACOMMON_SEMID_PORT_STATUS);
	if ((*pTargetStatus & (PORT_INC_IND | PORT_DEC_IND)) == (PORT_INC_IND | PORT_DEC_IND)) {
		DBG_WRN("Inc or Dec? Confused!\r\n");
		vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
		return E_SYS;
	}
	if (*pTargetStatus & PORT_INC_IND) {
		if ((*pTargetStatus & 0x0fffffff) == 1) {
			if (fp_cb) {
				fp_cb(path_id, CB_STATE_BEFORE_RUN);
			}
			if ((ret = fp(path_id, STATE_RUN)) != HD_OK) {
				DBG_ERR("_LinkUpdateStatus() inc fail=%d\r\n", ret);
				vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
				return E_SYS;
			}
			if (fp_cb) {
				fp_cb(path_id, CB_STATE_AFTER_RUN);
			}
		}
		*pTargetStatus &= ~PORT_INC_IND;
	} else if (*pTargetStatus & PORT_DEC_IND) {
		if ((*pTargetStatus & 0x0fffffff) == 0) {
			if (fp_cb) {
				fp_cb(path_id, CB_STATE_BEFORE_STOP);
			}
			if ((ret = fp(path_id, STATE_STOP)) != HD_OK) {
				DBG_ERR("_LinkUpdateStatus() dec fail=%d\r\n", ret);
				vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
				return E_SYS;
			}
			if (fp_cb) {
				fp_cb(path_id, CB_STATE_AFTER_STOP);
			}
		}
		*pTargetStatus &= ~PORT_DEC_IND;
	}
	vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
	return E_OK;
}

UINT32 _LinkCheckStatus(UINT32 status)
{
	UINT32 ret, act, sts;

	vos_sem_wait(IACOMMON_SEMID_PORT_STATUS);

	act = status & 0xf0000000;
	sts = status & 0x0fffffff;

	if (act) {
		if (sts == 1 && act == PORT_INC_IND) {
			ret = FALSE;
		} else if (sts == 0 && act == PORT_DEC_IND) {
			//ret = TRUE;
			ret = FALSE;        // treat this state as FALSE due to cannot push data at this transient
		} else {
			ret = sts ? TRUE : FALSE;
		}
	} else {
		ret = sts ? TRUE : FALSE;
	}
	vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
	return ret;
}

UINT32 _LinkCheckStatus2(UINT32 status)
{
	UINT32 ret, act, sts;

	vos_sem_wait(IACOMMON_SEMID_PORT_STATUS);

	act = status & 0xf0000000;
	sts = status & 0x0fffffff;

	if (act) {
		if (sts == 1 && act == PORT_INC_IND) {
			ret = STATE_PREPARE_TO_RUN;
		} else if (sts == 0 && act == PORT_DEC_IND) {
			ret = STATE_PREPARE_TO_STOP;
		} else {
			ret = sts ? STATE_RUN : STATE_STOP;
		}
	} else {
		ret = sts ? STATE_RUN : STATE_STOP;
	}
	vos_sem_sig(IACOMMON_SEMID_PORT_STATUS);
	return ret;
}

