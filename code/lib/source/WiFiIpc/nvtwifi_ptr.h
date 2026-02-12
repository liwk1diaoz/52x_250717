#ifndef _NVTWIFI_PTR_H_
#define _NVTWIFI_PTR_H_

#if 0 //debug
#define NVTWIFI_PRINT(fmtstr, args...) diag_printf("eCos: %s() " fmtstr, __func__, ##args)
#else //release
#define NVTWIFI_PRINT(...)
#endif

#define NVTWIFI_ERR(fmtstr, args...) diag_printf("eCoooooos: ERR %s() " fmtstr, __func__, ##args)

#endif //_NVTWIFI_DEBUG_H_

#define ipc_isCacheAddr(addr)       NvtIPC_IsCacheAddr(addr)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtIPC_GetNonCacheAddr(addr)

int Wifi_HAL_Test                      (void);
int Wifi_HAL_Init                      (int param);
int Wifi_HAL_Is_Interface_Up           (const char *pIntfName);
int Wifi_HAL_Interface_Up              (const char *pIntfName);
int Wifi_HAL_Interface_Down            (const char *pIntfName);
int Wifi_HAL_Interface_Config          (const char *pIntfName, char *addr, char *netmask);
int Wifi_HAL_Interface_Delete          (char *pIntfName);
int Wifi_HAL_Config                    (nvt_wifi_settings *pwifi);
int Wifi_HAL_Get_Mac                   (char *mac);
int Wifi_HAL_Set_Mac                   (const char *pIntfName, char *mac);
int Wifi_HAL_Run_Cmd                   (char *param1, char *param2, char *param3, char *param4);
int Wifi_HAL_RegisterStaCB             (char *pIntfName, void (*CB)(char *pIntfName, char *pMacAddr, int status));
int Wifi_HAL_RegisterLinkCB            (char *pIntfName, void (*CB)(char *pIntfName, int status));
int Wifi_HAL_RegisterWSCCB             (void (*CB)(int evt, int status));
int Wifi_HAL_RegisterWSCFlashCB        (int (*CB)(nvt_wsc_flash_param *param));
int Wifi_HAL_RegisterP2PCB             (char *pIntfName, void (*CB)(char *pIntfName,  int status));
int Wifi_HAL_RegisterScanCB            (char *pIntfName, void (*CB)(char *pIntfName, struct nvt_bss_desc *result, int status));
int Wifi_HAL_RegisterQueryCB           (char *pIntfName, void (*CB)(char *pIntfName, char *ssid, char *mode));
int Wifi_HAL_getWlSiteSurveyRequest    (char *pIntfName, int *pStatus);
int Wifi_HAL_getWlSiteSurveyResult     (char *pIntfName, NVT_SS_STATUS_T *pSSStatus, unsigned int size);
int Wifi_HAL_getWlP2PScanRequest       (char *pIntfName, int *pStatus);
int Wifi_HAL_getWlP2PScanResult        (char *pIntfName, NVT_SS_STATUS_T *pSSStatus, unsigned int size);
int Wifi_HAL_sendP2PProvisionCommInfo  (char *pIntfName, NVT_P2P_PROVISION_COMM_Tp pP2pProvision);
int Wifi_HAL_create_wscd               (void);
int Wifi_HAL_wsc_reinit                (void);
int Wifi_HAL_wsc_stop_service          (void);
int Wifi_HAL_wscd_status               (void);
int Wifi_HAL_generate_pin_code         (char *pinbuf);
int Wifi_HAL_ignore_down_up            (char *pIntfName, int val);
int Wifi_HAL_Wow_Clear                 (void);
int Wifi_HAL_Wow_Show_Status           (void);
int Wifi_HAL_Wow_SetIP                 (char *IP,char *Mac,char *AP);
int Wifi_HAL_Wow_Offload               (void);
int Wifi_HAL_Wow_Set_Pattern           (void);
int Wifi_HAL_Wow_Apply                 (void);
int Wifi_HAL_Wow_Keep_Alive_Pattern    (NVTWIFI_PARAM_KEEP_ALIVE *param);
int Wifi_HAL_Wow_Wakeup_Pattern        (NVTWIFI_PARAM_WAKEUP_PATTERN *param);
int Wifi_HAL_Wow_Apply2                (void);
int Wifi_HAL_Wow_Set_Auto_Recovery     (NVTWIFI_PARAM_SLEEP_RECOVERY *param);
int Wifi_HAL_Dhcp_Offload_Config       (NVTWIFI_PARAM_DHCP_OFFLOAD *param);
int Wifi_HAL_Dhcp_Offload_Enable       (int enable);
int Wifi_HAL_Set_Dtim                  (char *pIntfName, int unit, int duration);
int Wifi_HAL_Set_Beacon_Timeout        (char *pIntfName, int time);
int Wifi_HAL_Set_Gratuitous_Arp        (unsigned char smac[6], unsigned char sip[4], unsigned char dip[4], unsigned int  interval);
int Wifi_HAL_Set_Power_Manage_Mode     (char *pIntfName, int mode);
int Wifi_HAL_Set_Country               (const char *c);
int Wifi_HAL_Set_Keep_Resp_Detect      (unsigned char sip[4], int idx, int last, int interval);
int Wifi_HAL_Is_Associated             (char *pIntfName);
int Wifi_HAL_Set_Frequency             (NVT_FREQUENCY_TYPE f);

typedef struct _NVTWIFI_CMDID_IMPL
{
    int               (*Wifi_CmdId_Test)(void);
    int               (*Wifi_CmdId_Init)(int param);
    int               (*Wifi_CmdId_Is_Interface_Up)(const char *pIntfName);
    int               (*Wifi_CmdId_Interface_Up)(const char *pIntfName);
    int               (*Wifi_CmdId_Interface_Down)(const char *pIntfName);
    int               (*Wifi_CmdId_Interface_Config)(const char *pIntfName, char *addr, char *netmask);
    int               (*Wifi_CmdId_Interface_Delete)(char *pIntfName);
    int               (*Wifi_CmdId_Config)(nvt_wifi_settings *pwifi);
    int               (*Wifi_CmdId_Get_Mac)(char *mac);
    int               (*Wifi_CmdId_Set_Mac)(const char *pIntfName, char *mac);
    int               (*Wifi_CmdId_Run_Cmd)(char *param1, char *param2, char *param3, char *param4);
    int               (*Wifi_CmdId_RegisterStaCB)(char *pIntfName);
    int               (*Wifi_CmdId_RegisterLinkCB)(char *pIntfName);
    int               (*Wifi_CmdId_RegisterWSCCB)(void);
    int               (*Wifi_CmdId_RegisterWSCFlashCB)(void);
    int               (*Wifi_CmdId_RegisterP2PCB)(char *pIntfName);
    int               (*Wifi_CmdId_RegisterQueryCB)(char *pIntfName);
    int               (*Wifi_CmdId_getWlSiteSurveyRequest)(char *pIntfName, int *pStatus);
    int               (*Wifi_CmdId_getWlSiteSurveyResult)(char *pIntfName, NVT_SS_STATUS_T *pSSStatus, unsigned int size);
    int               (*Wifi_CmdId_getWlP2PScanRequest)(char *pIntfName, int *pStatus);
    int               (*Wifi_CmdId_getWlP2PScanResult)(char *pIntfName, NVT_SS_STATUS_T *pSSStatus);
    int               (*Wifi_CmdId_sendP2PProvisionCommInfo)(char *pIntfName, NVT_P2P_PROVISION_COMM_Tp pP2pProvision);
    int               (*Wifi_CmdId_create_wscd)(void);
    int               (*Wifi_CmdId_wsc_reinit)(void);
    int               (*Wifi_CmdId_wsc_stop_service)(void);
    int               (*Wifi_CmdId_wscd_status)(void);
    int               (*Wifi_CmdId_generate_pin_code)(char *pinbuf);
    int               (*Wifi_CmdId_ignore_down_up)(char *pIntfName, int val);
    int               (*Wifi_CmdId_wow_clear)(void);
    int               (*Wifi_CmdId_wow_show_status)(void);
    int               (*Wifi_CmdId_wow_set_ip)(char *IP,char *Mac,char *AP);
    int               (*Wifi_CmdId_wow_offload)(void);
    int               (*Wifi_CmdId_wow_set_pattern)(void);
    int               (*Wifi_CmdId_wow_apply)(void);
	int               (*Wifi_CmdId_wow_keep_alive_pattern)(NVTWIFI_PARAM_KEEP_ALIVE *param);
    int               (*Wifi_CmdId_wow_wakeup_pattern)(NVTWIFI_PARAM_WAKEUP_PATTERN *param);
    int               (*Wifi_CmdId_wow_apply2)(void);
    int               (*Wifi_CmdId_wow_set_auto_recovery)(NVTWIFI_PARAM_SLEEP_RECOVERY *param);
    int               (*Wifi_CmdId_dhcp_offload_config)(NVTWIFI_PARAM_DHCP_OFFLOAD *param);
    int               (*Wifi_CmdId_dhcp_offload_enable)(int enable);
    int               (*Wifi_CmdId_set_dtim)(char *pIntfName, int unit, int duration);
    int               (*Wifi_CmdId_set_beacon_timeout)(char *pIntfName, int time);
    int               (*Wifi_CmdId_set_gratuitous_arp)(unsigned char smac[6], unsigned char sip[4], unsigned char dip[4], unsigned int  interval);
    int               (*Wifi_CmdId_set_power_manage_mode)(char *pIntfName, int mode);
    int               (*Wifi_CmdId_set_country)(const char *c);
    int               (*Wifi_CmdId_set_keep_resp_detect)(unsigned char sip[4], int idx, int last, int interval);
    int               (*Wifi_CmdId_is_associated)(char *pIntfName);
    int               (*Wifi_CmdId_set_frequency)(NVT_FREQUENCY_TYPE f);
}NVTWIFI_CMDID_IMPL;

extern const NVTWIFI_CMDID_IMPL gCmdImplFp;

void Wifi_P2P_CB(char *pIntfName,  int status);
int  Wifi_WSC_Flash_CB(nvt_wsc_flash_param *flash_param);
void Wifi_WSC_Event_CB(int evt, int status);
void Wifi_Link_Status_CB(char *pIntfName,int status);
void Wifi_Sta_Status_CB(char *pIntfName, char *pMacAddr, int status);
void Wifi_Query_CB(char *pIntfName, char *ssid, char *mode);

extern void UINet_StationStatus_CB(char *pIntfName, char *pMacAddr, int status);
extern void UINet_Link2APStatus_CB(char *pIntfName, int status);
extern void UINet_WSCEvent_CB(int evt, int status);
extern int UINet_WSCFlashParam_CB(nvt_wsc_flash_param *param);
extern void UINet_P2PEvent_CB(char *pIntfName,  int status);
extern void UINet_Query_CB(char *pIntfName, char *ssid, char *mode);

