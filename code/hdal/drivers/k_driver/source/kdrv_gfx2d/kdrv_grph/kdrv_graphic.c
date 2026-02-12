#include "kwrap/type.h"
#include "grph_platform.h"
#include "kdrv_gfx2d/kdrv_grph.h"
#include "graphic_dbg.h"

#include <kwrap/error_no.h>

#if defined(__FREERTOS)
#define ARRAY_SIZE(X) (sizeof((X)) / sizeof((X[0])))
#define EXPORT_SYMBOL(x)
#endif


/*!
 * @fn INT32 kdrv_grph_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_GFX2D_GRPH0: graphic engine 0
 *                  - @b KDRV_GFX2D_GRPH1: graphic engine 1
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_open(UINT32 chip, UINT32 engine)
{
	ER ret;

	switch (engine) {
	case KDRV_GFX2D_GRPH0:
		ret = graph_open(GRPH_ID_1);
		break;
	case KDRV_GFX2D_GRPH1:
		ret = graph_open(GRPH_ID_2);
		break;
	default:
		DBG_ERR("%s: invalid engine 0x%x\r\n", __func__, (unsigned int)engine);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_grph_open);

/*!
 * @fn INT32 kdrv_grph_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_GFX2D_GRPH0: graphic engine 0
 *                  - @b KDRV_GFX2D_GRPH1: graphic engine 1
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_close(UINT32 chip, UINT32 engine)
{
	ER ret;

	switch (engine) {
	case KDRV_GFX2D_GRPH0:
		ret = graph_close(GRPH_ID_1);
		break;
	case KDRV_GFX2D_GRPH1:
		ret = graph_close(GRPH_ID_2);
		break;
	default:
		DBG_ERR("%s: invalid engine 0x%x\r\n", __func__, (unsigned int)engine);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_grph_close);

/*!
 * @fn INT32 kdrv_grph_trigger(INT32 handler, KDRV_GRPH_TRIGGER_PARAM *p_param,
                                 KDRV_CALLBACK_FUNC *p_cb_func,
                                 VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id                    the id of hardware
 * @param p_param               the parameter for trigger
 * @param p_cb_func             the callback function
 * @param p_user_data           the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_trigger(UINT32 id, KDRV_GRPH_TRIGGER_PARAM *p_param,
                                                KDRV_CALLBACK_FUNC *p_cb_func,
                                                VOID *p_user_data)
{
	ER ret;

	switch (KDRV_DEV_ID_ENGINE(id)) {
	case KDRV_GFX2D_GRPH0:
		ret = graph_enqueue(GRPH_ID_1, p_param, p_cb_func);
//		ret = graph_trigger(GRPH_ID_1, p_request, p_cb_func);
		break;
	case KDRV_GFX2D_GRPH1:
		ret = graph_enqueue(GRPH_ID_2, p_param, p_cb_func);
//		ret = graph_trigger(GRPH_ID_2, p_request, p_cb_func);
		break;
	default:
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	return ret;
}
EXPORT_SYMBOL(kdrv_grph_trigger);

/*!
 * @fn INT32 kdrv_grph_get(UINT32 handler, KDRV_GRPH_PARAM_ID id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_get(UINT32 id, KDRV_GRPH_PARAM_ID param_id, VOID *p_param)
{
	ER ret;
	UINT32* p_freq = p_param;

	if (param_id != KDRV_GRPH_PARAM_FREQ) {
		DBG_ERR("%s: param_id %d not support\r\n", __func__, param_id);
		return -1;
	}

	if (p_param == NULL) {
		DBG_ERR("%s: p_param NULL\r\n", __func__);
		return -1;
	}

	switch (KDRV_DEV_ID_ENGINE(id)) {
	case KDRV_GFX2D_GRPH0:
		ret = graph_get_config(GRPH_ID_1, GRPH_CONFIG_ID_FREQ, p_freq);
		break;
	case KDRV_GFX2D_GRPH1:
		ret = graph_get_config(GRPH_ID_2, GRPH_CONFIG_ID_FREQ, p_freq);
		break;
	default:
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_grph_get);

/*!
 * @fn INT32 kdrv_grph_set(UINT32 handler, KDRV_GRPH_PARAM_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_set(UINT32 id, KDRV_GRPH_PARAM_ID param_id, VOID *p_param)
{
	ER ret;
	UINT32 freq = 0;

	if (param_id != KDRV_GRPH_PARAM_FREQ) {
		DBG_ERR("%s: param_id %d not support\r\n", __func__, param_id);
		return -1;
	}

	if (p_param == NULL) {
		DBG_ERR("%s: p_param NULL\r\n", __func__);
		return -1;
	}

	freq = *((UINT32*)p_param);

	switch (KDRV_DEV_ID_ENGINE(id)) {
	case KDRV_GFX2D_GRPH0:
		ret = graph_set_config(GRPH_ID_1, GRPH_CONFIG_ID_FREQ, freq);
		break;
	case KDRV_GFX2D_GRPH1:
		ret = graph_set_config(GRPH_ID_2, GRPH_CONFIG_ID_FREQ, freq);
		break;
	default:
		DBG_ERR("%s: invalid id 0x%x\r\n", __func__, (unsigned int)id);
		return -1;
	}

	if (ret != E_OK) return -1;

	return 0;
}
EXPORT_SYMBOL(kdrv_grph_set);
