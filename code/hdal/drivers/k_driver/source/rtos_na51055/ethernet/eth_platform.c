#include "eth_platform.h"

#if defined __UITRON || defined __ECOS

#elif defined(__FREERTOS)
//
// Internal Definitions
//
THREAD_HANDLE uiEthernetTaskID = 0;
THREAD_HANDLE eth_phy_check_task_id = 0;
static ID uiEthFlagID = 0;

static vk_spinlock_t v_eth_spin_locks;
static struct netif *local_netif = NULL;

unsigned int rtos_eth_debug_level = NVT_DBG_ERR;
#else


#endif
void	nvt_enable_irq(int number);
void	nvt_disable_irq(int number);


ER eth_platform_flg_clear(ID id, FLGPTN flg)
{
	return clr_flg(uiEthFlagID, flg);
}

ER eth_platform_flg_set(ID id, FLGPTN flg)
{
	 return iset_flg(uiEthFlagID, flg);
}

FLGPTN eth_platform_flg_wait(ID id, FLGPTN flg)
{
	FLGPTN              ui_flag = 0;

	wai_flg(&ui_flag, uiEthFlagID, flg, TWF_ORW | TWF_CLR);

	return ui_flag;
}

UINT32 eth_platform_spin_lock(void)
{
#if defined __UITRON || defined __ECOS
	loc_cpu();
#elif defined(__FREERTOS)
	unsigned long eth_spin_flags = 0;
	vk_spin_lock_irqsave(&v_eth_spin_locks, eth_spin_flags);
	return eth_spin_flags;
#else
	return 0;
#endif
}

void eth_platform_spin_unlock(UINT32 flags)
{
#if defined __UITRON || defined __ECOS
	unl_cpu();
#elif defined(__FREERTOS)
	vk_spin_unlock_irqrestore(&v_eth_spin_locks, flags);
#else
	;
#endif
}

void eth_platform_delay_ms(UINT32 ms)
{
	vos_task_delay_ms(ms);
}

void eth_platform_delay_us(UINT32 us)
{
	delay_us(us);
}

void eth_platform_sta_tsk(ID id)
{
	THREAD_RESUME(uiEthernetTaskID);
}

void eth_platform_sta_phy_chk_tsk(ID id)
{
	THREAD_RESUME(eth_phy_check_task_id);
}

void eth_platform_ter_tsk(ID id)
{
	THREAD_DESTROY(uiEthernetTaskID);
}

void eth_platform_ter_phy_chk_tsk(ID id)
{
	THREAD_DESTROY(eth_phy_check_task_id);
}

void eth_platform_int_enable(ID id)
{
	nvt_enable_irq(INT_ID_ETHERNET);
}

void eth_platform_int_disable(ID id)
{
	nvt_disable_irq(INT_ID_ETHERNET);
}
/*
void abort(void)
{
	DBG_ERR("(halt)\r\n");

#if 1
	//coverity[no_escape]
	while (1) {
		eth_platform_delay_ms(500);
	}
#else
	// Debug usage
	debug_msg("wait uart ESC to continue\r\n");
	loc_cpu();
	//coverity[no_escape]
	while (1) {
		CHAR KeyEsc;
		uart_chkChar(&KeyEsc);
		if (KeyEsc == 27) {
			break;
		}
	}
	unl_cpu();
#endif

}
*/
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
static int is_create = 0;
void eth_set_internal_phy(void)
{
	//pinmux_set_config(PIN_FUNC_ETH, PIN_ETH_CFG_INTERANL | PIN_ETH_CFG_LED1);
}

void eth_set_local_netif(struct netif *netif)
{
	local_netif = netif;
}

irqreturn_t eth_platform_isr(int irq, void *devid)
{
	eth_isr();
	return IRQ_HANDLED;
}
void eth_platform_create_resource(void)
{
	if (!is_create) {


		OS_CONFIG_FLAG(uiEthFlagID);
		//THREAD_CREATE(uiEthernetTaskID, eth_task, local_netif, "eth_task");
		uiEthernetTaskID = vos_task_create(eth_task,  local_netif, "eth_task",   1,     (8*1024));
		THREAD_CREATE(eth_phy_check_task_id, nvtimeth_phy_thread, NULL, "eth_phy_check_tsk");

		vk_spin_lock_init(&v_eth_spin_locks);

		request_irq(INT_ID_ETHERNET, eth_platform_isr, IRQF_TRIGGER_HIGH, "ethernet", 0);

		is_create = 1;
	}
}
eth_callback callback = {
	eth_open,
	eth_send,
	eth_waitRcv,
	eth_setConfig
};
void eth_lwip_init(void)
{
	struct vos_mem_cma_info_t eth_buf;

	eth_set_callback(&callback);

	if (0 != vos_mem_init_cma_info(&eth_buf, VOS_MEM_CMA_TYPE_CACHE, 200*1024)) {
		DBG_ERR("init cma failed\r\n");
		return ;
	}

	if (NULL == vos_mem_alloc_from_cma(&eth_buf)) {
		DBG_ERR("alloc cma failed\r\n");
		return ;
	}
	eth_setConfig(ETH_CONFIG_ID_SET_MEM_REGION, eth_buf.paddr);
}
#else

#endif
void eth_platform_release_resource(void)
{
	rel_flg(uiEthFlagID);
}
#endif
