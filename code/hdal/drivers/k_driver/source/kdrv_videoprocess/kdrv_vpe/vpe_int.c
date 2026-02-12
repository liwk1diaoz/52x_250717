
#include "vpe_platform.h"
#include "vpe_int.h"

static VPE_USED_STATUS vpe_eng_lock_status  = VPE_ENGINE_UNLOCKED;


volatile NT98528_VPE_REGISTER_STRUCT *vpeg = NULL;


BOOL fw_vpe_power_saving_en = TRUE;


UINT32 vpe_hw_read(UINT32 reg)
{
	UINT32 read_val;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		// do nothing...
		read_val = 0;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		read_val = VPE_GETREG(reg);
	} else {
	    read_val = 0;

        DBG_ERR("VPE: get chip id %d out of range\r\n", nvt_get_chip_id());
    }

	return read_val;
}

INT32 vpe_hw_write(UINT32 reg, UINT32 val)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		// do nothing...
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		VPE_SETREG(reg, val);
	} else {
        DBG_ERR("VPE: get chip id %d out of range\r\n", nvt_get_chip_id());
    }

	return 0;
}

int vpe_hw_init(void)
{
	return 0;

}

int vpe_hw_exit(void)
{

	return 0;
}


ER vpe_lock(VOID)
{
	ER er_return = E_OK;

	er_return = vpe_platform_sem_wait();
	if (er_return != E_OK) {
		return er_return;
	}

	vpe_eng_lock_status = VPE_ENGINE_LOCKED;  // engine is opened

	return er_return;
}
//------------------------------------------------------------------------------


ER vpe_unlock(VOID)
{
	ER er_return;

	vpe_eng_lock_status = VPE_ENGINE_UNLOCKED;  // engine is closed

	er_return = vpe_platform_sem_signal();
	if (er_return != E_OK) {
		return er_return;
	}

	return er_return;
}
//------------------------------------------------------------------------------


ER vpe_attach(VOID)
{
	vpe_platform_enable_clk();

	return E_OK;
}
//------------------------------------------------------------------------------


ER vpe_detach(VOID)
{
	vpe_platform_disable_clk();

	return E_OK;
}


//----------------------------------------------------------


ER vpe_reset(VOID)
{
	vpeg->reg_0.bit.vpe_dma_sw_rst = 1;
	while(1) {
		if (vpeg->reg_260.bit.dma0_shut_down_done == 1) {
			break;
		}
	}

	vpeg->reg_0.bit.vpe_sw_rst = 1;
	while(1) {
		if (vpeg->reg_260.bit.hw_idle == 1) {
			break;
		}
	}
	vpeg->reg_0.bit.vpe_sw_rst = 0;


	vpe_platform_enable_sram_shutdown();
	vpe_detach();
	vpe_platform_unprepare_clk();

	/*clk_prepare_enable(ime_clk[0]);*/
	vpe_platform_prepare_clk();
	vpe_attach();
	vpe_platform_disable_sram_shutdown();

	vpeg->reg_256.word = 0xffffffff; // vpe interrupt status
	vpeg->reg_257.word = 0x00000000; // vpe interrupt enable

	vpe_platform_flg_clear(FLGPTN_VPE_FRAMEEND);
	vpe_platform_flg_clear(FLGPTN_VPE_LLERR);
	vpe_platform_flg_clear(FLGPTN_VPE_LLEND);
	vpe_platform_flg_clear(FLGPTN_VPE_RES0_NFDONE);
	vpe_platform_flg_clear(FLGPTN_VPE_RES1_NFDONE);
	vpe_platform_flg_clear(FLGPTN_VPE_RES2_NFDONE);
	vpe_platform_flg_clear(FLGPTN_VPE_RES3_NFDONE);

	return E_OK;
}
//-------------------------------------------------------------------------------



