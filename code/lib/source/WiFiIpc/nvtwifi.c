#include <string.h>
#include <stdlib.h>

#include <nvtwifi.h>
#include "nvtwifi_int.h"

#if defined(_NETWORK_ON_CPU1_)

#include "nvtwifi_ptr.h"

int Wifi_HAL_Test (void)
{
#if 0
	if(!gCmdImplFp.Wifi_CmdId_Test)
		return NVTWIFI_RET_NO_FUNC;
	else
		return gCmdImplFp.Wifi_CmdId_Test();
#endif
    return NVTWIFI_RET_OK;
}

int Wifi_HAL_Init (int param)
{		
    if(!gCmdImplFp.Wifi_CmdId_Init)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%d)\r\n",param);
	
	return gCmdImplFp.Wifi_CmdId_Init(param);
}

int Wifi_HAL_Is_Interface_Up (const char *pIntfName)
{
	if(!gCmdImplFp.Wifi_CmdId_Is_Interface_Up)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_Is_Interface_Up((const char *)pIntfName);
}

int Wifi_HAL_Interface_Up(const char *pIntfName)
{
	if(!gCmdImplFp.Wifi_CmdId_Interface_Up)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_Interface_Up((const char *)pIntfName);
}

int Wifi_HAL_Interface_Down(const char *pIntfName)
{
	if(!gCmdImplFp.Wifi_CmdId_Interface_Down)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_Interface_Down((const char *)pIntfName);
}

int Wifi_HAL_Interface_Config(const char *pIntfName, char *addr, char *netmask)
{
	if(!gCmdImplFp.Wifi_CmdId_Interface_Config)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s, %s, %s)\r\n",pIntfName, addr, netmask);

    return gCmdImplFp.Wifi_CmdId_Interface_Config((const char *)pIntfName,addr,netmask);
}

int Wifi_HAL_Interface_Delete(char *pIntfName)
{
	if(!gCmdImplFp.Wifi_CmdId_Interface_Delete)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_Interface_Delete(pIntfName);
}

int Wifi_HAL_Config(nvt_wifi_settings *pwifi)
{
    NVTWIFI_PRINT("mode %s ,p2pmode %d security %s\r\n",pwifi->mode,pwifi->p2pmode,pwifi->security);
	
	if(!gCmdImplFp.Wifi_CmdId_Config)
		return NVTWIFI_RET_NO_FUNC;
	else
		return gCmdImplFp.Wifi_CmdId_Config(pwifi);
}

int Wifi_HAL_Get_Mac(char *mac)
{
    int  result =0;
	
	if(!gCmdImplFp.Wifi_CmdId_Get_Mac)
		return NVTWIFI_RET_NO_FUNC;

    result = gCmdImplFp.Wifi_CmdId_Get_Mac(mac);

    NVTWIFI_PRINT("wlan0 efuse mac A [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
    mac[0], mac[1], mac[2],
    mac[3], mac[4], mac[5]);

    return result;
}

int Wifi_HAL_Set_Mac(const char *pIntfName, char *mac)
{
	if(!gCmdImplFp.Wifi_CmdId_Set_Mac)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    NVTWIFI_PRINT("set mac [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
    mac[0], mac[1], mac[2],
    mac[3], mac[4], mac[5]);

    return gCmdImplFp.Wifi_CmdId_Set_Mac((const char *)pIntfName,mac);
}

int Wifi_HAL_Run_Cmd(char *param1, char *param2, char *param3, char *param4)
{
	if(!gCmdImplFp.Wifi_CmdId_Run_Cmd)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("%s %s %s %s %s\r\n", param1,param2,param3,param4);

    return gCmdImplFp.Wifi_CmdId_Run_Cmd(param1,param2,param3,param4);
}

int Wifi_HAL_RegisterStaCB(char *pIntfName, void (*CB)(char *pIntfName, char *pMacAddr, int status))
{
	if(!gCmdImplFp.Wifi_CmdId_RegisterStaCB)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    gCmdImplFp.Wifi_CmdId_RegisterStaCB((char *)pIntfName);

    NVTWIFI_PRINT("(%s)  end\r\n",pIntfName);

    return 0;
}

int Wifi_HAL_RegisterLinkCB(char *pIntfName, void (*CB)(char *pIntfName, int status))
{
	if(!gCmdImplFp.Wifi_CmdId_RegisterLinkCB)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_RegisterLinkCB((char *)pIntfName);
}

int Wifi_HAL_RegisterWSCCB(void (*CB)(int evt, int status))
{
	if(!gCmdImplFp.Wifi_CmdId_RegisterWSCCB)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT("NvtWifi_WSC_Event_CB\r\n");
    gCmdImplFp.Wifi_CmdId_RegisterWSCCB();
    return NVTWIFI_RET_OK ;
}

int Wifi_HAL_RegisterWSCFlashCB(int (*CB)(nvt_wsc_flash_param *param))
{
	if(!gCmdImplFp.Wifi_CmdId_RegisterWSCFlashCB)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT("NvtWifi_CmdId_RegisterWSCFlashCB\r\n");
    gCmdImplFp.Wifi_CmdId_RegisterWSCFlashCB();
    return NVTWIFI_RET_OK ;
}

int Wifi_HAL_RegisterP2PCB(char *pIntfName, void (*CB)(char *pIntfName,  int status))
{
    if(!gCmdImplFp.Wifi_CmdId_RegisterP2PCB)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);
    return gCmdImplFp.Wifi_CmdId_RegisterP2PCB((char *)pIntfName);
}

int Wifi_HAL_RegisterQueryCB(char *pIntfName, void (*CB)(char *pIntfName, char *ssid, char *mode))
{
    if(!gCmdImplFp.Wifi_CmdId_RegisterQueryCB)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);
    return gCmdImplFp.Wifi_CmdId_RegisterQueryCB((char *)pIntfName);
}

int Wifi_HAL_getWlSiteSurveyRequest(char *pIntfName, int *pStatus)
{
	if(!gCmdImplFp.Wifi_CmdId_getWlSiteSurveyRequest)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_getWlSiteSurveyRequest(pIntfName,pStatus);
}

int Wifi_HAL_getWlSiteSurveyResult(char *pIntfName, NVT_SS_STATUS_T *pSSStatus, unsigned int size)
{
	if(!gCmdImplFp.Wifi_CmdId_getWlSiteSurveyResult)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);
	
    return gCmdImplFp.Wifi_CmdId_getWlSiteSurveyResult(pIntfName,pSSStatus,size);
}

int Wifi_HAL_getWlP2PScanRequest(char *pIntfName, int *pStatus)
{
	if(!gCmdImplFp.Wifi_CmdId_getWlP2PScanRequest)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_getWlP2PScanRequest(pIntfName,pStatus);
}

int Wifi_HAL_getWlP2PScanResult(char *pIntfName, NVT_SS_STATUS_T *pSSStatus, unsigned int size)
{
	if(!gCmdImplFp.Wifi_CmdId_getWlP2PScanResult)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_getWlP2PScanResult(pIntfName,pSSStatus);
}

int Wifi_HAL_sendP2PProvisionCommInfo(char *pIntfName, NVT_P2P_PROVISION_COMM_Tp pP2pProvision)
{
	if(!gCmdImplFp.Wifi_CmdId_sendP2PProvisionCommInfo)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s)\r\n",pIntfName);

    return gCmdImplFp.Wifi_CmdId_sendP2PProvisionCommInfo(pIntfName,pP2pProvision);
}

int Wifi_HAL_create_wscd(void)
{
	if(!gCmdImplFp.Wifi_CmdId_create_wscd)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT(" create_wscd \r\n");
    return gCmdImplFp.Wifi_CmdId_create_wscd();
}

int Wifi_HAL_wsc_reinit(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wsc_reinit)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT(" wsc_reinit \r\n");
    return gCmdImplFp.Wifi_CmdId_wsc_reinit();
}

int Wifi_HAL_wsc_stop_service(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wsc_stop_service)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT(" wsc_stop_service \r\n");

    return gCmdImplFp.Wifi_CmdId_wsc_stop_service();
}

int Wifi_HAL_wscd_status(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wscd_status)
		return NVTWIFI_RET_NO_FUNC;
		
    NVTWIFI_PRINT(" get_wscd_status \r\n");

    return gCmdImplFp.Wifi_CmdId_wscd_status() ;
}

int Wifi_HAL_generate_pin_code(char *pinbuf)
{
	if(!gCmdImplFp.Wifi_CmdId_generate_pin_code)
		return NVTWIFI_RET_NO_FUNC;
		
    gCmdImplFp.Wifi_CmdId_generate_pin_code(pinbuf);

    NVTWIFI_PRINT("(%s)\r\n",pinbuf);

    return NVTWIFI_RET_OK ;
}

int Wifi_HAL_ignore_down_up(char *pIntfName, int val)
{
	if(!gCmdImplFp.Wifi_CmdId_ignore_down_up)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s) %d\r\n",pIntfName,val);
    return gCmdImplFp.Wifi_CmdId_ignore_down_up(pIntfName,val);
}

int Wifi_HAL_Wow_Clear(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_clear)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_clear();
}

int Wifi_HAL_Wow_Show_Status(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_show_status)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_show_status();
}

int Wifi_HAL_Wow_SetIP(char *IP,char *Mac,char *AP)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_set_ip)
		return NVTWIFI_RET_NO_FUNC;

    NVTWIFI_PRINT("(%s) (%s) (%s)\r\n",IP, MAC, AP);
    return gCmdImplFp.Wifi_CmdId_wow_set_ip(IP, Mac, AP);
}

int Wifi_HAL_Wow_Offload(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_offload)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_offload();
}

int Wifi_HAL_Wow_Set_Pattern(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_set_pattern)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_set_pattern();
}

int Wifi_HAL_Wow_Apply(void)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_apply)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_apply();
}

int Wifi_HAL_Wow_Keep_Alive_Pattern(NVTWIFI_PARAM_KEEP_ALIVE *param)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_keep_alive_pattern)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_keep_alive_pattern(param);
}

int Wifi_HAL_Wow_Wakeup_Pattern(NVTWIFI_PARAM_WAKEUP_PATTERN *param)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_wakeup_pattern)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_wakeup_pattern(param);
}

int Wifi_HAL_Wow_Apply2()
{
	if(!gCmdImplFp.Wifi_CmdId_wow_apply2)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_apply2();
}

int Wifi_HAL_Wow_Set_Auto_Recovery(NVTWIFI_PARAM_SLEEP_RECOVERY *param)
{
	if(!gCmdImplFp.Wifi_CmdId_wow_set_auto_recovery)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_wow_set_auto_recovery(param);
}

int Wifi_HAL_Dhcp_Offload_Config (NVTWIFI_PARAM_DHCP_OFFLOAD *param)
{
	if(!gCmdImplFp.Wifi_CmdId_dhcp_offload_config)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_dhcp_offload_config(param);
}

int Wifi_HAL_Dhcp_Offload_Enable (int enable)
{
	if(!gCmdImplFp.Wifi_CmdId_dhcp_offload_enable)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_dhcp_offload_enable(enable);
}

int Wifi_HAL_Set_Dtim (char *pIntfName, int unit, int duration)
{
	if(!gCmdImplFp.Wifi_CmdId_set_dtim)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_dtim(pIntfName, unit, duration);
}

int Wifi_HAL_Set_Beacon_Timeout (char *pIntfName, int time)
{
	if(!gCmdImplFp.Wifi_CmdId_set_beacon_timeout)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_beacon_timeout(pIntfName, time);
}

int Wifi_HAL_Set_Gratuitous_Arp(unsigned char smac[6], unsigned char sip[4], unsigned char dip[4], unsigned int  interval)
{
	if(!gCmdImplFp.Wifi_CmdId_set_gratuitous_arp)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_gratuitous_arp(smac, sip, dip, interval);
}

int Wifi_HAL_Set_Power_Manage_Mode (char *pIntfName, int mode)
{
	if(!gCmdImplFp.Wifi_CmdId_set_power_manage_mode)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_power_manage_mode(pIntfName, mode);
}

int Wifi_HAL_Set_Country (const char *c)
{
	if(!gCmdImplFp.Wifi_CmdId_set_country)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_country(c);
}

int Wifi_HAL_Set_Keep_Resp_Detect (unsigned char sip[4], int idx, int last, int interval)
{
	if(!gCmdImplFp.Wifi_CmdId_set_keep_resp_detect)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_keep_resp_detect(sip, idx, last, interval);
}

int Wifi_HAL_Is_Associated (char *pIntfName)
{
	if(!gCmdImplFp.Wifi_CmdId_is_associated)
		return NVTWIFI_RET_NO_FUNC;
	
    	return gCmdImplFp.Wifi_CmdId_is_associated(pIntfName);
}

void Wifi_Sta_Status_CB(char *pIntfName, char *pMacAddr, int status)
{
    UINet_StationStatus_CB(pIntfName, pMacAddr, status);

}

void Wifi_Link_Status_CB(char *pIntfName,int status)
{
    UINet_Link2APStatus_CB(pIntfName, status);

}

void Wifi_WSC_Event_CB(int evt, int status)
{
    UINet_WSCEvent_CB(evt, status);

}

int Wifi_WSC_Flash_CB(nvt_wsc_flash_param *flash_param)
{
    UINet_WSCFlashParam_CB(flash_param);
	return 0;
}

void Wifi_P2P_CB(char *pIntfName,  int status)
{
    UINet_P2PEvent_CB(pIntfName, status);

}

void Wifi_Query_CB(char *pIntfName, char *ssid, char *mode)
{
	UINet_Query_CB(pIntfName, ssid, mode);
}

int Wifi_HAL_Set_Frequency (NVT_FREQUENCY_TYPE f)
{
	if(!gCmdImplFp.Wifi_CmdId_set_frequency)
		return NVTWIFI_RET_NO_FUNC;

    return gCmdImplFp.Wifi_CmdId_set_frequency(f);
}

#endif
