#!/bin/sh

TRY=0
HOSTAPD=`ps`
HOSTAPD=`echo $HOSTAPD | grep hostapd`
if [ -n "$HOSTAPD" ]; then
        echo "hostapd is running. Try to stop it"
	killall -9 hostapd
	while [ 1 ]
	do
		HOSTAPD=`ps`
		HOSTAPD=`echo $HOSTAPD | grep hostapd`
		if [ -n "$HOSTAPD" ]; then
			if [ $TRY -gt 5 ]; then
				echo "can't stop hostapd"
				exit 255
			else
				echo "TRY=$TRY"
				TRY=$((${TRY}+1))
	                        sleep 1
			fi
		else
			echo "hostapd is stopped"
			break
		fi
	done
fi

TRY=0
WPA=`ps`
WPA=`echo $WPA | grep wpa_supplicant`
if [ -n "$WPA" ]; then
        echo "wpa_supplicant is running. Try to stop it"
        killall -9 wpa_supplicant
        while [ 1 ]
        do
		WPA=`ps`
		WPA=`echo $WPA | grep wpa_supplicant`
                if [ -n "$WPA" ]; then
                        if [ $TRY -gt 5 ]; then
                                echo "can't stop wpa_supplicant"
                                exit 255
                        else
                                echo "TRY=$TRY"
                                TRY=$((${TRY}+1))
                                sleep 1
                        fi
                else
                        echo "wpa_supplicant is stopped"
                        break
                fi
        done
fi

killall -9 udhcpc

if [ "$1" == "ap" ]; then
    echo ctrl_interface=/var/run/hostapd >> /var/run/hostapd.conf
	echo beacon_int=100 >> /var/run/hostapd.conf
	echo driver=nl80211 >> /var/run/hostapd.conf	
	echo wme_enabled=1 >> /var/run/hostapd.conf
	echo ht_capab=[SHORT-GI-20][SHORT-GI-40][HT40+][HT40-] >> /var/run/hostapd.conf
	echo wpa_key_mgmt=WPA-PSK >> /var/run/hostapd.conf
	echo wpa_pairwise=CCMP >> /var/run/hostapd.conf
	echo max_num_sta=8 >> /var/run/hostapd.conf		
	echo wpa_group_rekey=86400 >> /var/run/hostapd.conf

	if [ "$2" == "0" ]; then
		ifconfig wlan0 up
		
		if [ "$3" == "2.4G" ]; then
			echo channel=6 >> /var/run/hostapd.conf
			echo hw_mode=g >> /var/run/hostapd.conf
			echo ieee80211n=1 >> /var/run/hostapd.conf			
		elif [ "$3" == "5G" ]; then
			echo 0x76 > /proc/net/rtl8821cs/wlan0/chan_plan
			echo channel=36 >> /var/run/hostapd.conf
			echo hw_mode=a >> /var/run/hostapd.conf
			echo ieee80211n=1 >> /var/run/hostapd.conf		
			echo ieee80211ac=1 >> /var/run/hostapd.conf
			echo vht_oper_chwidth=1>> /var/run/hostapd.conf
			echo vht_oper_centr_freq_seg0_idx=42 >> /var/run/hostapd.conf		
		fi
	else
		echo channel=$2 >> /var/run/hostapd.conf
	fi
	hostapd /var/run/hostapd.conf &
	TRY=0
	while [ 1 ]
	do
	        UP=`ifconfig wlan0 | grep UP`
	        echo "UP=$UP"
	
	        if [ -z "$UP" ]; then
	                echo "TRY=$TRY"
	                if [ $TRY -gt 5 ]; then
	                        hostapd stop
	                        exit 255
	                else
	                        TRY=$((${TRY}+1))
	                        sleep 1
	                fi
	        else
	                echo "UP ok"
	                ifconfig wlan0 192.168.1.254
			break
	        fi
	done

	udhcpd /etc/udhcpdw.conf
	TRY=0
	while [ 1 ]
	do
	        DHCP=`ps`
	        DHCP=`echo $DHCP | grep udhcpd`
	        echo "DHCP=$DHCP"
	
	        if [ -z "$DHCP" ]; then
	                echo "TRY=$TRY"
	                if [ $TRY -gt 5 ]; then
	                        echo "Can't run udhcpd"
	                        exit 255
	                else
	                        TRY=$((${TRY}+1))
	                        sleep 1
	                fi
	        else
	                echo "udhcpd ok"
	                exit 0
	        fi
	done
elif [ "$1" == "sta" ]; then
        wpa_supplicant -i wlan0 -c /var/run/wpa_supplicant.conf &
        TRY=0
        while [ 1 ]
        do
                UP=`ifconfig wlan0 | grep UP`
                echo "UP=$UP"

                if [ -z "$UP" ]; then
                        echo "TRY=$TRY"
                        if [ $TRY -gt 5 ]; then
                                wpa_supplicant stop
                                exit 255
                        else
                                TRY=$((${TRY}+1))
                                sleep 1
                        fi
                else
                        echo "UP ok"
                        ifconfig wlan0 192.168.1.1
                        break
                fi
        done

        udhcpc -i wlan0
        TRY=0
        while [ 1 ]
        do
                DHCP=`ps`
                DHCP=`echo $DHCP | grep udhcpc`
                echo "DHCP=$DHCP"

                if [ -z "$DHCP" ]; then
                        echo "TRY=$TRY"
                        if [ $TRY -gt 5 ]; then
                                echo "Can't run udhcpc"
                                exit 255
                        else
                                TRY=$((${TRY}+1))
                                sleep 1
                        fi
                else
                        echo "udhcpc ok"
                        exit 0
                fi
        done
fi

exit 255
