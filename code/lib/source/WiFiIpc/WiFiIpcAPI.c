#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "WiFiIpc/WiFiIpcAPI.h"
#include "WiFiIpcInt.h"
#include "WiFiIpcID.h"
#include "WiFiIpcMsg.h"
#include "WiFiIpcTsk.h"
#include "WiFiIpc/nvtwifi.h"
#include "kwrap/util.h"

#define min(a,b) \
	(a > b ? b : a)

static char                          mode[16];
static char                          *frequency_name[2]={"2.4G", "5G"};
static pthread_t                     adaptor_thread;
static volatile int                  report_enable;
static volatile int                  server_stop;
static volatile int                  ap_up;
static NVT_FREQUENCY_TYPE            frequency = NVT_WIFI_24G;
static int                           g_channel=0, g_is_opened=0;
static pthread_mutex_t               lock = PTHREAD_MUTEX_INITIALIZER;
static wifi_sta_status_cb_func_t     *NvtWifi_Sta_Status_CB = NULL;
static wifi_link_status_cb_func_t    *NvtWifi_Link_Status_CB = NULL;
static UINT32 g_u32StaModeIewvwntDelay = 1000; //unit ms


static ER get_mac(char *pMacAddr)
{
	unsigned int  imac[6] = { 0 };
	char str[1024] = { 0 };
	FILE *fPtr;

	system("ifconfig wlan0 | grep HWaddr > /var/run/wifiipc.mac");

	fPtr = fopen("/var/run/wifiipc.mac", "r");
	if (fPtr) {
		fgets(str, sizeof(str), fPtr);
		fclose(fPtr);
		if(strstr(str, "HWaddr "))
			sscanf(strstr(str, "HWaddr "), "HWaddr %02x:%02x:%02x:%02x:%02x:%02x",
					imac, imac+1, imac+2, imac+3, imac+4, imac+5);
	}

	system("rm /var/run/wifiipc.mac");

	if(pMacAddr){
		pMacAddr[0] = imac[0];
		pMacAddr[1] = imac[1];
		pMacAddr[2] = imac[2];
		pMacAddr[3] = imac[3];
		pMacAddr[4] = imac[4];
		pMacAddr[5] = imac[5];
	}
		
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_Open(WIFIIPC_OPEN *pOpen)
{
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_Close(void)
{
	return WIFIIPC_RET_OK;
}

static void* iwevent_monitor(void* arg)
{
	int  fd;
	char str[1024] = { 0 };
	unsigned char cmac[6];
	unsigned int  imac[6];
	int ret = 0, local_report_enable = 0, sta_connected = 0, local_ap_up = 0;
    int link2ap = 0;

	while(!server_stop)
	{
		pthread_mutex_lock(&lock);
		local_report_enable = report_enable;
		local_ap_up = ap_up; //ap_up is shared with station mode
		pthread_mutex_unlock(&lock);

		if(local_ap_up == 0){
			ret = system("ifconfig wlan0 > /var/run/wifiipc.ready");
			if(ret){
				printf("fail to execute ifconfig wlan0 and contine iwevent_monitor\n");
				sleep(1);
				//return NULL;
				continue;
			}
			sleep(1);
			
			fd = open("/var/run/wifiipc.ready", O_RDONLY, 0644);
			if (fd >= 0) {
				if(read(fd, str, sizeof(str)) > 0) {
					if(strstr(str, "UP")){
						if(local_ap_up == 0 && local_report_enable == 1 && NvtWifi_Sta_Status_CB){
							pthread_mutex_lock(&lock);
							ap_up = 1;
                            sta_connected = 0;
                            link2ap = 0;
							pthread_mutex_unlock(&lock);
                            if (strcmp(mode, "ap") == 0) {
							NvtWifi_Sta_Status_CB("wlan0", NULL,  NVT_WIFI_AP_READY);
                            } else if (strcmp(mode, "sta") == 0) {
                                NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_STA_READY);
                            } else {
                                NvtWifi_Sta_Status_CB("wlan0", NULL,  NVT_WIFI_AP_READY);
                            }
						}
					}
					memset(str, 0, sizeof(str));
				}
				memset(str, 0, sizeof(str));
				close(fd);
			}
			fd = -1;

			system("rm /var/run/wifiipc.ready");
		}else{
            if (strcmp(mode, "ap") == 0) {
    			system("hostapd_cli -p /var/run/hostapd -i wlan0 all_sta > /var/run/iwevent.output");
			sleep(1);
			
			fd = open("/var/run/iwevent.output", O_RDONLY,0644);
			if (fd >= 0) {
				if(read(fd, str, sizeof(str)) > 0) {
					if(strstr(str, "[ASSOC]")){
						sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", 
								imac, imac+1, imac+2, imac+3, imac+4, imac+5);
						cmac[0] = imac[0];cmac[1] = imac[1];cmac[2] = imac[2];cmac[3] = imac[3];cmac[4] = imac[4];cmac[5] = imac[5];

						if(sta_connected==0 && local_report_enable == 1 && NvtWifi_Sta_Status_CB){
							sta_connected = 1;
							NvtWifi_Sta_Status_CB("wlan0", (char*)cmac, NVT_WIFI_STA_STATUS_ASSOCIATED);
							NvtWifi_Sta_Status_CB("wlan0", (char*)cmac, NVT_WIFI_STA_STATUS_PORT_AUTHORIZED);
						}
					}

					memset(str, 0, sizeof(str));
				}else{
					if(sta_connected ==1 && local_report_enable == 1 && NvtWifi_Sta_Status_CB){
						sta_connected = 0;
						NvtWifi_Sta_Status_CB("wlan0", (char*)cmac, NVT_WIFI_STA_STATUS_DEAUTHENTICATED);
						NvtWifi_Sta_Status_CB("wlan0", (char*)cmac, NVT_WIFI_STA_STATUS_DISASSOCIATED);
					}
				}

				memset(str, 0, sizeof(str));
				close(fd);
			}
            } else if (strcmp(mode, "sta") == 0) {
    			system("wpa_cli -p /var/run/wpa_supplicant -i wlan0 status > /var/run/iwevent.output");
    			//sleep(1);
    			vos_util_delay_ms(g_u32StaModeIewvwntDelay);

    			fd = open("/var/run/iwevent.output", O_RDONLY,0644);
    			if (fd >= 0) {
    				if (read(fd, str, sizeof(str)) > 0) {
    					if (strstr(str, "mode=station")) {
    						if(link2ap==0 && local_report_enable == 1 && NvtWifi_Sta_Status_CB) {
    							link2ap = 1;
    							NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_LINK_STATUS_CONNECTED);
	}
    } else {
    						if(link2ap==1 && local_report_enable == 1 && NvtWifi_Sta_Status_CB){
    							link2ap = 0;
                    			NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_LINK_STATUS_DISCONNECTED);
                    			NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_LINK_STATUS_DISASSOCIATED);
					}
				}

				memset(str, 0, sizeof(str));
    				} else {
    					if (link2ap==1 && local_report_enable == 1 && NvtWifi_Sta_Status_CB) {
    						link2ap = 0;
                			NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_LINK_STATUS_DISCONNECTED);
                			NvtWifi_Link_Status_CB("wlan0", NVT_WIFI_LINK_STATUS_DISASSOCIATED);
			}
		}
    
    				memset(str, 0, sizeof(str));
    				close(fd);
		}
		}

            fd = -1;
			system("rm /var/run/iwevent.output");
	}
    }

	return NULL;
}

ER WiFiIpc_init_wlan0_netdev(int pri)
{
	int ret;
	
	if(g_is_opened)
		return WIFIIPC_RET_OK;

	memset(mode, 0, sizeof(mode));
	pthread_mutex_lock(&lock);
	report_enable = 0;
	ap_up = 0;
	pthread_mutex_unlock(&lock);
	server_stop = 0;
	
	ret = pthread_create(&adaptor_thread, NULL , iwevent_monitor, NULL);
	if(ret){
		DBG_ERR("fail to create iwevent_monitor thread\r\n");
		return WIFIIPC_RET_ERR;
	}
		
	ret = system("/usr/share/wifiscripts/init.sh");
	if(ret){
		DBG_ERR("init.sh fail = %d\r\n", ret);
		server_stop = 1;
		pthread_join(adaptor_thread, NULL);
		return WIFIIPC_RET_ERR;
	}
		
	g_is_opened = 1;
	
	return ret;
}

ER WiFiIpc_interface_up(const char *pIntfName)
{
	int  ret;
	char cmd[64];
  
	pthread_mutex_lock(&lock);
	
	report_enable = 1;

	sprintf(cmd, "/usr/share/wifiscripts/up.sh %s %d %s", mode, g_channel, frequency_name[frequency]);
	ret = system(cmd);
	
	pthread_mutex_unlock(&lock);

	return ret;
}

ER WiFiIpc_interface_down(const char *pIntfName)
{
	int ret;
	char cmd[64];
        
	pthread_mutex_lock(&lock);
	
	report_enable = 0;
	ap_up = 0;
	sprintf(cmd, "/usr/share/wifiscripts/down.sh %s", mode);
	ret = system(cmd);
		
	pthread_mutex_unlock(&lock);

	return ret;
}

ER WiFiIpc_interface_config(const char *pIntfName, char *addr, char *netmask)
{
	char cmd[512];
	int  ret;
	
	pthread_mutex_lock(&lock);
	sprintf(cmd, "/usr/share/wifiscripts/ip_config.sh %s %s %s", pIntfName, addr, netmask);
	ret = system(cmd);
	pthread_mutex_unlock(&lock);
	
	return ret;
}

ER WiFiIpc_delete_interface_addr(const char *pIntfName)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_get_wlan0_efuse_mac(char *pMacAddr)
{
	int ret;
	
	pthread_mutex_lock(&lock);
	
	ret = get_mac(pMacAddr);
	if(ret)
		DBG_ERR("fail to get wifi mac address\n");
	
	pthread_mutex_unlock(&lock);
		
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_set_mac_address(const char *pIntfName, char *pMacAddr)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_RunSystemCmd(char *filepath, ...)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_register_wifi_sta_status_cb(const char *pIntfName, wifi_sta_status_cb_func_t *pFunc)
{
	pthread_mutex_lock(&lock);
	NvtWifi_Sta_Status_CB = pFunc;
	report_enable = 1;
	pthread_mutex_unlock(&lock);
	
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_register_wifi_link_status_cb(const char *pIntfName, wifi_link_status_cb_func_t *pFunc)
{
	pthread_mutex_lock(&lock);
	NvtWifi_Link_Status_CB = pFunc;
	report_enable = 1;
	pthread_mutex_unlock(&lock);
	
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_register_p2p_event_indicate_cb(const char *pIntfName, p2p_event_indicate_cb_func_t *pFunc)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_register_wsc_event_cb(wsc_event_cb_func_t *pFunc)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_register_wsc_flash_param_cb(wsc_flash_param_cb_func_t *pFunc)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_Config(nvt_wifi_settings *pwifi)
{
	int fd;
	char str[128] = { 0 };
	char cmd[512] = { 0 };
	char *file_path = NULL;
	char mac[6] = {0, 0, 0, 0, 0, 0};
	//https://jira.novatek.com.tw/browse/IVOT_N12086_CO-315
	char Tstr[2] = { 0 };
    char u_str[128] = { 0 };
	
	pthread_mutex_lock(&lock);

	if (strcmp(pwifi->mode, "ap") == 0){

        strncat(cmd, "interface=wlan0\n", 511);
        g_channel = pwifi->channel;
#if 0
        if(frequency == NVT_WIFI_5G){
            strncat(cmd, "ieee80211ac=1\nhw_mode=a\n\n", 511);
        } else {
            strncat(cmd, "ieee80211n=1\nhw_mode=g\n", 511);
        }
#endif
        if(pwifi->auto_ssid_plus_mac){
            if(get_mac(mac)){
                DBG_ERR("fail to get wifi mac address\n");
                pthread_mutex_unlock(&lock);
                return NVTWIFI_RET_NO_FUNC;
            }

            if(pwifi->auto_ssid_plus_mac > NVT_WIFI_AP_MAC_AUTO){
                sprintf(u_str, "ssid=%s", pwifi->ssid);
                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC0){
                    sprintf(Tstr, "%02x", mac[0]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }
                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC1){
                    sprintf(Tstr, "%02x", mac[1]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }
                
                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC2){
                    sprintf(Tstr, "%02x", mac[2]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }

                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC3){
                    sprintf(Tstr, "%02x", mac[3]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }

                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC4){
                    sprintf(Tstr, "%02x", mac[4]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }
                
                if(pwifi->auto_ssid_plus_mac & NVT_WIFI_AP_MAC5){
                    sprintf(Tstr, "%02x", mac[5]);
                    strncat(u_str, Tstr, strlen(Tstr));
                }
                strncat(u_str, "\n", strlen("\n"));
                if(strlen(u_str) > NVT_WSC_MAX_SSID_LEN){
                    sprintf(str, "ssid=%s\n", pwifi->ssid);
                }else{
                    sprintf(str, "%s", u_str);
                }
            }else{
                sprintf(str, "ssid=%s%02x%02x%02x%02x%02x%02x\n", pwifi->ssid,
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
        }else
            sprintf(str, "ssid=%s\n", pwifi->ssid);

        strncat(cmd, str, 511);
        file_path = "/var/run/hostapd.conf";
		
		if (strcmp(pwifi->security, "none") == 0){}
		else if (strcmp(pwifi->security, "wpa-psk-tkip") == 0){}
		else if (strcmp(pwifi->security, "wpa2-psk-aes") == 0){
	                strncat(cmd, "wpa=2\nrsn_pairwise=CCMP\n", 511);
			sprintf(str, "wpa_passphrase=%s\n", pwifi->passphrase);
			strncat(cmd, str, 511);
		}
	}
	else if (strcmp(pwifi->mode, "sta") == 0){
        strncat(cmd, "ctrl_interface=/var/run/wpa_supplicant\n", 511);
		strncat(cmd, "network={\n", 511);
		sprintf(str, "\tssid=\"%s\"\n", pwifi->ssid);
		strncat(cmd, str, 511);

		if (strcmp(pwifi->security, "none") == 0){}
		else if (strcmp(pwifi->security, "wpa-psk-tkip") == 0){}
		else if (strcmp(pwifi->security, "wpa2-psk-aes") == 0){
                        strncat(cmd, "\tproto=RSN\n\tkey_mgmt=WPA-PSK\n\tpairwise=CCMP TKIP\n\tgroup=CCMP TKIP\n", 511);
                        sprintf(str, "\tpsk=\"%s\"\n", pwifi->passphrase);
                        strncat(cmd, str, 511);
		}

		strncat(cmd, "}", 511);
		file_path = "/var/run/wpa_supplicant.conf";
	}
	else
	{
		DBG_ERR("unknown wifi mode: %s\n", pwifi->mode);
		pthread_mutex_unlock(&lock);
		return NVTWIFI_RET_NO_FUNC;
	}

	sprintf(str, "rm %s", file_path);
	system(str);

	fd = open(file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if(fd < 0){
		DBG_ERR("fail to open %s\n", file_path);
		pthread_mutex_unlock(&lock);
		return NVTWIFI_RET_ERR;
	}

	if(write(fd,cmd,strlen(cmd))<0){
		close(fd);
		pthread_mutex_unlock(&lock);
		return NVTWIFI_RET_ERR;
	}

	close(fd);

	strncpy(mode, pwifi->mode, sizeof(mode) - 1);
	
	pthread_mutex_unlock(&lock);
	
	return WIFIIPC_RET_OK;
}

ER WiFiIpc_getWlSiteSurveyRequest(const char *pIntfName, int *pStatus)
{
	pthread_mutex_lock(&lock);
	system("rm /var/run/iwlist.result");
	*pStatus = system("iwlist wlan0 scanning > /var/run/iwlist.result");
	pthread_mutex_unlock(&lock);
	
	return WIFIIPC_RET_OK;
}


ER WiFiIpc_getWlSiteSurveyResult(const char *pIntfName, NVT_SS_STATUS_Tp pStatus, UINT32 size)
{
	FILE *fPtr;
	char str[4096] = { 0 };
	int num = -1;
	unsigned int  imac[6];
	int channel;
	unsigned char protocol[32];
	char *encrypt = NULL;
	char *temp;
	
	pthread_mutex_lock(&lock);

	memset(pStatus, 0, sizeof(NVT_SS_STATUS_T));

	fPtr = fopen("/var/run/iwlist.result", "r");
	if (!fPtr) {
		DBG_ERR("fopen(/var/run/iwlist.result, r) fail\r\n");
		return NVTWIFI_RET_ERR;
	}
	while (fgets(str, sizeof(str), fPtr) != NULL) {
		if(num == NVT_MAX_BSS_DESC){
			DBG_ERR("AP number out-number default(%d)\r\n", NVT_MAX_BSS_DESC);
			break;
		}else{
			if(!strncmp(str, "          Cell ", strlen("          Cell "))){
				num++;
				temp = strstr(str, " - Address: ");
				if(temp){
					sscanf(temp, " - Address: %02x:%02x:%02x:%02x:%02x:%02x", imac, imac+1, imac+2, imac+3, imac+4, imac+5);
					pStatus->bssdb[num].bssid[0] = imac[0];
					pStatus->bssdb[num].bssid[1] = imac[1];
					pStatus->bssdb[num].bssid[2] = imac[2];
					pStatus->bssdb[num].bssid[3] = imac[3];
					pStatus->bssdb[num].bssid[4] = imac[4];
					pStatus->bssdb[num].bssid[5] = imac[5];
					pStatus->bssdb[num].capability = !nvt_cPrivacy;
					encrypt = NULL;
				}
			} else if(!strncmp(str, "                    ESSID:", strlen("                    ESSID:")) && (num >= 0)){
				temp = strstr(str, "ESSID:");
				if(temp){
		                	sscanf(temp, "ESSID:\"%s", pStatus->bssdb[num].ssid);
					pStatus->bssdb[num].ssid[strlen((const char*)pStatus->bssdb[num].ssid) - 1] = 0;
	                                pStatus->bssdb[num].ssidlen = strlen((const char*)pStatus->bssdb[num].ssid);
				}
                        }
                        else if(!strncmp(str, "                    Protocol:", strlen("                    Protocol:")) && (num >= 0)){
				temp = strstr(str, "802.11");
				if(temp){
	                                sscanf(temp, "802.11%s", protocol);
					pStatus->bssdb[num].network = 0;
					if(strstr((const char*)protocol, "ac"))
						pStatus->bssdb[num].network |= NVT_WIRELESS_11AC;
					else if(strstr((const char*)protocol, "a"))
						pStatus->bssdb[num].network |= NVT_WIRELESS_11A;
					if(strstr((const char*)protocol, "b"))
						pStatus->bssdb[num].network |= NVT_WIRELESS_11B;
					if(strstr((const char*)protocol, "g"))
						pStatus->bssdb[num].network |= NVT_WIRELESS_11G;
					if(strstr((const char*)protocol, "n"))
						pStatus->bssdb[num].network |= NVT_WIRELESS_11N;
				}
			} else if(!strncmp(str, "                    Frequency:", strlen("                    Frequency:")) && (num >= 0)){
				temp = strstr(str, "Channel");
				if(temp){
					sscanf(temp, "Channel %d)", &channel);
					pStatus->bssdb[num].channel = channel;
				}
			} else if(!strncmp(str, "                    IE: IEEE 802.11i", strlen("                    IE: IEEE 802.11i")) && (num >= 0)){
				pStatus->bssdb[num].capability = nvt_cPrivacy;
				if(strstr(str, "WPA2"))
					encrypt = "WPA2";
				else if(strstr(str, "WPA"))
					encrypt = "WPA";
				else if(strstr(str, "WEP")){
					encrypt = "WEP";
					pStatus->bssdb[num].t_stamp[0] = 0;
				} else{
					pStatus->bssdb[num].capability = !nvt_cPrivacy;
					encrypt = NULL;
				}
			} else if(strstr(str, "Group Cipher : ") && (num >= 0)){
				if(strstr(str, "TKIP")){
					if(encrypt && !strcmp(encrypt, "WPA2"))
						pStatus->bssdb[num].t_stamp[0] = (0x01 << 24);
					else if(encrypt && !strcmp(encrypt, "WPA"))
						pStatus->bssdb[num].t_stamp[0] = (0x01 << 8);
				} else if(strstr(str, "CCMP")){
					if(encrypt && !strcmp(encrypt, "WPA2"))
						pStatus->bssdb[num].t_stamp[0] = (0x04 << 24);
					else if(encrypt && !strcmp(encrypt, "WPA"))
						pStatus->bssdb[num].t_stamp[0] = (0x04 << 8);
				}
			}
		}
	}
	fclose(fPtr);

	pStatus->number = (num + 1);
	
	pthread_mutex_unlock(&lock);

	return WIFIIPC_RET_OK;
}

ER WiFiIpc_getWlP2PScanRequest(const char *pIntfName, int *pStatus)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_getWlP2PScanResult(const char *pIntfName, NVT_SS_STATUS_Tp pStatus, UINT32 size)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_sendP2PProvisionCommInfo(const char *pIntfName, NVT_P2P_PROVISION_COMM_Tp pP2PProvisionComm)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_create_wscd(void)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_wsc_reinit(void)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_wsc_stop_service(void)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_generate_pin_code(char *pinbuf)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_wscd_is_start(void)
{
	return WIFIIPC_RET_ERR;
}

ER WiFiIpc_interface_remove(const char *pIntfName)
{
	DBG_ERR("Not support");
	return NVTWIFI_RET_NO_FUNC;
}

ER WiFiIpc_set_frequency(NVT_FREQUENCY_TYPE f)
{
    frequency = f;

    return NVTWIFI_RET_OK;
}

ER WiFiIpc_set_iwevent_delay(UINT32 WifiMode,UINT32 delay)
{
    switch (WifiMode) {
    case WIFIIPC_STA_MODE_IWEVENT_DELAY:
        DBG_DUMP("WIFIIPC_STA_MODE_IWEVENT_DELAY: delay %d ms\r\n", delay);
        if (delay < 50) {
            DBG_DUMP("Lower than 50ms. Set to 50ms\r\n");
            g_u32StaModeIewvwntDelay = 50;
        } else {
            g_u32StaModeIewvwntDelay = delay;
        }
        break;

    default:
        DBG_ERR("Not wifi mode for choice: %d\r\n", WifiMode);
        break;
}

    return NVTWIFI_RET_OK;
}

