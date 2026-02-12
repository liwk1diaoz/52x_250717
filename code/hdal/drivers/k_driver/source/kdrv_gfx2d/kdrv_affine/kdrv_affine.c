#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "affine_platform.h"
#include "kdrv_gfx2d/kdrv_affine.h"
//#include "affine_reg.h"
#include "affine_dbg.h"

/*!
 * @fn INT32 kdrv_affine_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_GFX2D_AFFINE: affine engine
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_open(UINT32 chip, UINT32 engine)
{
	ER ret;

	switch (engine) {
	case KDRV_GFX2D_AFFINE:
		ret = affine_open(AFFINE_ID_1);
		break;
	default:
		DBG_ERR("%s: invalid engine 0x%x\r\n", __func__, (unsigned int)engine);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_affine_open);

/*!
 * @fn INT32 kdrv_affine_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_GFX2D_AFFINE: affine engine
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_close(UINT32 chip, UINT32 engine)
{
	ER ret;

	switch (engine) {
	case KDRV_GFX2D_AFFINE:
		ret = affine_close(AFFINE_ID_1);
		break;
	default:
		DBG_ERR("%s: invalid engine 0x%x\r\n", __func__, (unsigned int)engine);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_affine_close);

/*!
 * @fn INT32 kdrv_affine_trigger(INT32 handler,
 *				 KDRV_AFFINE_TRIGGER_PARAM *p_param,
 *                               KDRV_CALLBACK_FUNC *p_cb_func,
 *                               VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id                    the id of hardware
 * @param p_param               the parameter for trigger
 * @param p_cb_func             the callback function
 * @param p_user_data           the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_trigger(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
                                                KDRV_CALLBACK_FUNC *p_cb_func,
                                                VOID *p_user_data)
{
	ER ret;

	switch (KDRV_DEV_ID_ENGINE(id)) {
	case KDRV_GFX2D_AFFINE:
				ret = affine_enqueue(AFFINE_ID_1, p_param, p_cb_func);
		break;
	default:
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	return ret;
}
EXPORT_SYMBOL(kdrv_affine_trigger);

/*!
 * @fn INT32 kdrv_grph_get(UINT32 handler, KDRV_AFFINE_PARAM_ID id,
 *				VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_get(UINT32 id, KDRV_AFFINE_PARAM_ID param_id,
			VOID *p_param)
{
	ER ret;
	UINT32* p_freq = p_param;

	if (param_id != AFFINE_PARAM_FREQ) {
		DBG_ERR("%s: param_id %d not support\r\n", __func__, param_id);
		return -1;
	}

	if (p_param == NULL) {
		DBG_ERR("%s: p_param NULL\r\n", __func__);
		return -1;
	}

	if (KDRV_DEV_ID_ENGINE(id) == KDRV_GFX2D_AFFINE) {
		ret = affine_getconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_FREQ, p_freq);
	} else {
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_affine_get);

/*!
 * @fn INT32 kdrv_affine_set(UINT32 handler, KDRV_AFFINE_PARAM_ID id,
 *				VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_affine_set(UINT32 id, KDRV_AFFINE_PARAM_ID param_id,
			VOID *p_param)
{
	ER ret;
	UINT32 context = 0;

//	if (param_id != AFFINE_PARAM_FREQ) {
//		DBG_ERR("%s: param_id %d not support\r\n", __func__, param_id);
//		return -1;
//	}

	if (p_param == NULL) {
		DBG_ERR("%s: p_param NULL\r\n", __func__);
		return -1;
	}

	context = *((UINT32*)p_param);

	if (KDRV_DEV_ID_ENGINE(id) == KDRV_GFX2D_AFFINE) {
		switch (param_id) {
		case AFFINE_PARAM_FREQ:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_FREQ, context);
			break;
		case AFFINE_PARAM_COEFF_A:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_A, context);
			break;
		case AFFINE_PARAM_COEFF_B:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_B, context);
			break;
		case AFFINE_PARAM_COEFF_C:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_C, context);
			break;
		case AFFINE_PARAM_COEFF_D:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_D, context);
			break;
		case AFFINE_PARAM_COEFF_E:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_E, context);
			break;
		case AFFINE_PARAM_COEFF_F:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_COEFF_F, context);
			break;
		case AFFINE_PARAM_SUBBLK1:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_SUBBLK1, context);
			break;
		case AFFINE_PARAM_SUBBLK2:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_SUBBLK2, context);
			break;
		case AFFINE_PARAM_DIRECT_COEFF:
			ret = affine_setconfig(AFFINE_ID_1, AFFINE_CONFIG_ID_DIRECT_COEFF, context);
			break;

		default:
			DBG_ERR("%s: invalid param id 0x%x\r\n", __func__, (unsigned int)context);
			return -1;
		}
	} else {
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_affine_set);
